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
  char *json_text=malloc(filesize+1);
  assert(json_text);
  //read file into json_text
  fread(json_text,sizeof(char),filesize,ptr_file);
  json_text[filesize] = '\0';

  return json_text;
}

/* returns json_text containing file content */
char*
get_json_text(char filename[])
{
  FILE *file=fopen(filename, "rb");
  assert(file);

  long filesize=fetch_filesize(file);
  char *json_text=read_file(file, filesize);

  fclose(file);

  return json_text;
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
CjsonString_set(char **ptr_json_text)
{
  char *start=*ptr_json_text;
  char *end=start;

  while (*end != '\"'){
    if (*end++ == '\\'){ //skips \" char
      ++end;
    }
  }

  CjsonString *data=strndup(start, end-start);

  *ptr_json_text = end+1;

  return data;
}

static CjsonNumber
CjsonNumber_set(char **ptr_json_text)
{
  char *start=*ptr_json_text-1;
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

  *ptr_json_text = end;

  return number;
}

/* get and return data from appointed datatype */
static CjsonValue
CjsonValue_set(CjsonDType new_dtype, char **ptr_json_text)
{
  CjsonValue new_value={0};
  switch (new_dtype){
    case String:
      new_value.string = CjsonString_set(ptr_json_text);
      break;
    case Number:
      new_value.number = CjsonNumber_set(ptr_json_text);
      break;
    case Boolean:
      if ((*ptr_json_text)[-1] == 't'){
        *ptr_json_text += 3; //length of "true"-1
        new_value.boolean = 1;
        break;
      }
      *ptr_json_text += 4; //length of "false"-1
      new_value.boolean = 0;
      break;
    case Null:
      *ptr_json_text += 3; //length of "null"-1
      break;
    default:
      fprintf(stderr,"ERROR: invalid datatype %ld\n", new_dtype);
      exit(EXIT_FAILURE);
      break;
  }

  return new_value;
}

/* create property from appointed JSON datatype
    and return the item containing it */
static CjsonItem*
CjsonItem_set_wrap(CjsonItem *item, CjsonString *new_key, CjsonDType new_dtype, char **ptr_json_text)
{
  item = CjsonItem_create(item);
  item->key = new_key;
  item->dtype = new_dtype;
  item->value = CjsonValue_set(new_dtype, ptr_json_text);

  item = item->parent;

  return item;
}

/* create nested object/array and return
    the nested object/array address. */
static CjsonItem*
CjsonItem_set(CjsonItem *item, CjsonString *new_key, CjsonDType new_dtype, char **ptr_json_text)
{
  item = CjsonItem_create(item);
  item->key = new_key;
  item->dtype = new_dtype;

  return item;
}

/* get bitmask bits and json datatype by
    evaluating json_text's current position */
static void
CjsonItem_eval(CjsonDType *new_dtype, CjsonDetectDType *bitmask, char *json_text)
{
  switch (*json_text){
    case ',':/*KEY DETECTED*/
      BITMASK_SET(*bitmask,FoundKey);
      break;
    case ':':/*KEY TO VALUE ASSIGNMENT DETECTED*/
      BITMASK_SET(*bitmask,FoundAssign);
      break;
    case '\"':/*STRING DETECTED*/
      BITMASK_SET(*bitmask,FoundString);
      *new_dtype = String;
      break;
    case '{':/*OBJECT DETECTED*/
      BITMASK_SET(*bitmask,FoundObject|FoundKey);
      *new_dtype = Object;
      break;
    case '[':/*ARRAY DETECTED*/
      BITMASK_SET(*bitmask,FoundArray);
      *new_dtype = Array;
      break;
    case '}': /*WRAPPER*/
    case ']':/*DETECTED*/
      BITMASK_SET(*bitmask,FoundWrapper);
      break;
    case 't': /* CHECK FOR */
    case 'f':/*TRUE or FALSE*/
      if (strncmp(json_text,"true",strlen("true"))
          && strncmp(json_text,"false",strlen("false")))
        break; //break if not true or false
      BITMASK_SET(*bitmask, FoundProperty);
      *new_dtype = Boolean;
      break;
    case 'n':/*CHECK FOR NULL*/
      if (strncmp(json_text,"null",strlen("null")))
        break; //break if not null
      BITMASK_SET(*bitmask, FoundProperty);
      *new_dtype = Null;
      break;
    default:/*CHECK FOR NUMBER*/
      if ((!isdigit(*json_text)) && (*json_text != '-'))
        break; //break if first char is not digit or minus sign
      BITMASK_SET(*bitmask, FoundProperty);
      *new_dtype = Number;
      break;
  }
}

/* perform actions appointed by bitmask and
    return newly updated or retrieved item */
static CjsonItem*
CjsonItem_apply(CjsonItem *item, CjsonString *key, CjsonDType dtype, CjsonDetectDType *bitmask, char **ptr_json_text)
{
  if (BITMASK_EQUALITY(*bitmask,FoundKey) || BITMASK_EQUALITY(*bitmask,FoundAssign)
      || BITMASK_EQUALITY(*bitmask,0))
    return item; //early exit if bitmask is 0 or set with only FoundKey or FoundAssign

  if (*bitmask & FoundProperty){
    //create and wrap item
    item = CjsonItem_set_wrap(item, key, dtype, ptr_json_text);
    BITMASK_CLEAR(*bitmask,FoundProperty);
  }
  else if (*bitmask & (FoundObject|FoundArray)){
    //create and return new nested item address
    item = CjsonItem_set(item, key, dtype, ptr_json_text);
    BITMASK_CLEAR(*bitmask,(FoundObject|FoundArray));
  }
  else if (*bitmask & FoundWrapper){
    item = item->parent; //wraps object/array
    BITMASK_CLEAR(*bitmask,FoundWrapper);
  }

  if (*bitmask & FoundAssign)
    BITMASK_CLEAR(*bitmask,FoundAssign);

  return item;
}

Cjson*
Cjson_parse(char *json_text)
{
  Cjson *cjson=Cjson_create();

  CjsonItem *item=cjson->item;
  CjsonDetectDType bitmask=0;
  CjsonString *new_key=NULL;
  CjsonDType new_dtype=0;

  while (*json_text){ //while not null terminator char
    //get tokens(datatype, bitmask) with current json_text's position evaluation 
    CjsonItem_eval(&new_dtype, &bitmask, json_text);
    ++json_text;
    //deal if special key fetching case for when item is an array
    if (item->dtype == Array)
      new_key = CjsonString_set_array_key(cjson,item);
    //else check if bitmask demands a key's string to be fetched
    else if (BITMASK_EQUALITY(bitmask,FoundString|FoundKey)){
      CjsonString *cache_entry=NULL;
      cache_entry = CjsonString_set(&json_text);
      new_key = CjsonString_get_key(cjson,cache_entry);
      BITMASK_CLEAR(bitmask,bitmask);
    }
    //set item by performing actions indicated by bitmask
    item = CjsonItem_apply(item, new_key, new_dtype, &bitmask, &json_text);
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
Cjson_parse_reviver(char *json_text, void (*reviver)(CjsonItem*))
{
  Cjson *cjson = Cjson_parse(json_text);

  if (reviver)
    apply_reviver(cjson->item, reviver);

  return cjson;
}

