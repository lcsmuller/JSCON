#include "../CJSON.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

Cjson*
Cjson_create()
{
  Cjson *new_cjson=calloc(1,sizeof(Cjson));
  assert(new_cjson);

  new_cjson->item=calloc(1,sizeof(CjsonItem));
  assert(new_cjson->item);

  return new_cjson;
}

/* destroy current item and all of its nested object/arrays */
static void
CjsonItem_destroy(CjsonItem *item)
{
  for (size_t i=0 ; i < item->n ; ++i)
    CjsonItem_destroy(item->property[i]);
  free(item->property);

  if (item->dtype == String)
    free(item->value.string);
  free(item);
}

/* destroy cjson struct */
void
Cjson_destroy(Cjson *cjson)
{
  CjsonItem_destroy(cjson->item);
  
  if (cjson->list_size){
    do {
      free(cjson->keylist[--cjson->list_size]);
    } while (cjson->list_size);
    free(cjson->keylist);
  } free(cjson);
}

/* returns file size in long format */
static long
fetch_filesize(FILE *ptr_file)
{
  fseek(ptr_file, 0, SEEK_END);
  long filesize=ftell(ptr_file);
  assert(filesize > 0);
  fseek(ptr_file, 0, SEEK_SET);

  return filesize;
}

/* returns file content */
static char*
read_file(FILE* ptr_file, long filesize)
{
  char *buffer=malloc(filesize+1);
  assert(buffer);
  //read file into buffer
  fread(buffer,sizeof(char),filesize,ptr_file);
  buffer[filesize] = '\0';

  return buffer;
}

/* returns buffer containing file content */
char*
get_buffer(char filename[])
{
  FILE *file=fopen(filename, "rb");
  assert(file);

  long filesize=fetch_filesize(file);
  char *buffer=read_file(file, filesize);

  fclose(file);

  return buffer;
}

/* create new json item and return it's address */
static CjsonItem*
CjsonItem_create(CjsonItem *item)
{
  ++item->n; //update object's property count
  //update memory space for property's list
  item->property = realloc(item->property, item->n*sizeof(CjsonItem*));
  assert(item->property);
  //allocate memory space for new property (which is a nested item)
  item->property[item->n-1] = calloc(1,sizeof(CjsonItem));
  assert(item->property[item->n-1]);
  //get parent address of the new property
  item->property[item->n-1]->parent = item;
  //return new property address
  return item->property[item->n-1];
}

static CjsonString*
CjsonString_cache_key(Cjson *cjson, CjsonString *cache_entry)
{
  ++cjson->list_size;
  cjson->keylist = realloc(cjson->keylist,cjson->list_size*sizeof(char*));
  assert(cjson->keylist);

  int i=cjson->list_size-1;
  while ((i > 0) && (strcmp(cache_entry, cjson->keylist[i-1]) < 0)){
    cjson->keylist[i] = cjson->keylist[i-1];
    --i;
  } cjson->keylist[i] = cache_entry;

  return cjson->keylist[i];
}

static CjsonString*
CjsonString_get_key(Cjson *cjson, char *cache_entry)
{
  int top=cjson->list_size-1;
  int low=0;
  int mid;

  int cmp;
  while (low <= top){
    mid = ((ulong)low + (ulong)top) >> 1;
    cmp=strcmp(cache_entry, cjson->keylist[mid]);
    if (cmp == 0){
      free(cache_entry);
      return cjson->keylist[mid];
    }
    if (cmp < 0)
      top = mid-1;
    else
      low = mid+1;
  }

  return CjsonString_cache_key(cjson, cache_entry);
}

/* get numerical key for array type
    json data, in string format */
static CjsonString*
CjsonString_set_array_key(Cjson *cjson, CjsonItem *item)
{
  const int len=25;
  //will be free'd inside CjsonString_get_key if necessary
  CjsonString *cache_entry=malloc(len);
  assert(cache_entry);

  snprintf(cache_entry,len,"%ld",item->n);

  return CjsonString_get_key(cjson, cache_entry);
}

static CjsonString*
CjsonString_set(char **ptr_buffer)
{
  char *start=*ptr_buffer;
  char *end=start;

  while (*end != '\"'){
    if (*end++ == '\\'){ //skips \" char
      ++end;
    }
  }

  CjsonString *data=strndup(start, end-start);

  *ptr_buffer = end+1;

  return data;
}

static CjsonNumber
CjsonNumber_set(char **ptr_buffer)
{
  char *start=*ptr_buffer-1;
  char *end=start;

  if (*end == '-')
    ++end;

  while (isdigit(*end))
    ++end;

  if (*end == '.'){
    while (isdigit(*++end))
      continue;
  }
  //check for exponent
  if ((*end == 'e') || (*end == 'E')){
    ++end;
    if ((*end == '+') || (*end == '-'))
      ++end;
    while (isdigit(*end))
      ++end;
  }

  CjsonString *data=strndup(start, end-start);
  assert(data);

  CjsonNumber number;
  sscanf(data,"%lf",&number);

  free(data);

  *ptr_buffer = end;

  return number;
}

/* get and return data from appointed datatype */
static CjsonValue
CjsonValue_set(CjsonDType set_dtype, char **ptr_buffer)
{
  CjsonValue set_value={0};
  switch (set_dtype){
    case String:
      set_value.string = CjsonString_set(ptr_buffer);
      break;
    case Number:
      set_value.number = CjsonNumber_set(ptr_buffer);
      break;
    case Boolean:
      if ((*ptr_buffer)[-1] == 't'){
        *ptr_buffer += 3; //length of "true"-1
        set_value.boolean = 1;
        break;
      }
      *ptr_buffer += 4; //length of "false"-1
      set_value.boolean = 0;
      break;
    case Null:
      *ptr_buffer += 3; //length of "null"-1
      break;
    default:
      fprintf(stderr,"ERROR: invalid datatype %ld\n", set_dtype);
      exit(EXIT_FAILURE);
      break;
  }

  return set_value;
}

/* create property from appointed JSON datatype
    and return the item containing it */
static CjsonItem*
CjsonItem_set_wrap(CjsonItem *item, CjsonString *get_key, CjsonDType get_dtype, char **ptr_buffer)
{
  item = CjsonItem_create(item);
  item->key = get_key;
  item->dtype = get_dtype;
  item->value = CjsonValue_set(item->dtype, ptr_buffer);

  item = item->parent; //wraps item

  return item;
}

/* create nested object/array and return
    the nested object/array address. */
static CjsonItem*
CjsonItem_set(CjsonItem *item, CjsonString *get_key, CjsonDType get_dtype, char **ptr_buffer)
{
  item = CjsonItem_create(item);
  item->key = get_key;
  item->dtype = get_dtype;

  return item;
}

/* get bitmask bits and json datatype by
    evaluating buffer's current position */
static CjsonItem*
CjsonItem_build(Cjson *cjson, CjsonItem *item, short *bitmask, CjsonString **ptr_set_key, char **ptr_buffer)
{
  CjsonItem* (*setter)(CjsonItem *item, CjsonString *set_key, CjsonDType set_dtype, char **ptr_buffer) = NULL;

  CjsonDType set_dtype=0;

  switch (*(*ptr_buffer)++){
    case ',':/*KEY DETECTED*/
      BITMASK_SET(*bitmask,FOUND_KEY);
      return item;
    case '\"':/*STRING DETECTED*/
      if ((item->dtype != Array) && (*bitmask & FOUND_KEY)){
        /* is a key string */
        *ptr_set_key = CjsonString_get_key(cjson,CjsonString_set(ptr_buffer));
        BITMASK_CLEAR(*bitmask,FOUND_KEY);
        return item;
      }
      /* is normal string value */
      setter = &CjsonItem_set_wrap;
      set_dtype = String;
      break;
    case '{':/*OBJECT DETECTED*/
      setter = &CjsonItem_set;
      set_dtype = Object;
      BITMASK_SET(*bitmask,FOUND_KEY);
      break;
    case '[':/*ARRAY DETECTED*/
      setter = &CjsonItem_set;
      set_dtype = Array;
      break;
    case '}': /*WRAPPER*/
    case ']':/*DETECTED*/
      //WRAPS OBJECT or ARRAY, ALL OF ITS MEMBERS HAVE BEEN FOUND
      return item->parent;
    case 't': /* CHECK FOR */
    case 'f':/*TRUE or FALSE*/
      if (strncmp(*ptr_buffer-1,"true",4) && strncmp(*ptr_buffer-1,"false",5))
        return item;
      setter = &CjsonItem_set_wrap;
      set_dtype = Boolean;
      break;
    case 'n':/*CHECK FOR NULL*/
      if (strncmp(*ptr_buffer-1,"null",4))
        return item;
      setter = &CjsonItem_set_wrap;
      set_dtype = Null;
      break;
    default:/*CHECK FOR NUMBER*/
      if ((!isdigit((*ptr_buffer)[-1])) && ((*ptr_buffer)[-1] != '-'))
        return item;
      setter = &CjsonItem_set_wrap;
      set_dtype = Number;
      break;
  }

  /* if hasn't reach a return thus far, it means the json item
    is ready to be created (all of its composing information
    have been found) */

  if (item->dtype == Array) //creates key for array element
    *ptr_set_key = CjsonString_set_array_key(cjson,item);

  item = (*setter)(item, *ptr_set_key, set_dtype, ptr_buffer);
  *ptr_set_key=NULL;

  return item;
}

Cjson*
Cjson_parse(char *buffer)
{
  Cjson *cjson=Cjson_create();

  CjsonItem *item=cjson->item;
  CjsonString *set_key=NULL;
  short bitmask=0;
  while (*buffer){ //while not null terminator char
    item = CjsonItem_build(cjson, item, &bitmask, &set_key, &buffer);
  }

  return cjson;
}

static void
apply_reviver(CjsonItem *item, void (*reviver)(CjsonItem*))
{
  (*reviver)(item);
  for (size_t i=0 ; i < item->n ; ++i)
    apply_reviver(item->property[i], reviver);
}

Cjson*
Cjson_parse_reviver(char *buffer, void (*reviver)(CjsonItem*))
{
  Cjson *cjson = Cjson_parse(buffer);

  if (reviver)
    apply_reviver(cjson->item, reviver);

  return cjson;
}

