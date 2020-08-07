#include "../json_parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

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
read_json_file(char filename[])
{
  FILE *json_file=fopen(filename, "rb");
  assert(json_file);

  long filesize=fetch_filesize(json_file);
  char *buffer=read_file(json_file, filesize);

  fclose(json_file);

  return buffer;
}

/* create new json item and return it's address */
static CJSON_item_t*
new_item(CJSON_item_t *item)
{
  ++item->n; //update object's properties count
  //update memory space for property's list
  item->properties = realloc(item->properties, item->n*sizeof(CJSON_item_t*));
  assert(item->properties);
  //allocate memory space for new property (which is a nested item)
  item->properties[item->n-1] = calloc(1,sizeof(CJSON_item_t));
  assert(item->properties[item->n-1]);
  //get parent address of the new property
  item->properties[item->n-1]->parent = item;
  //return new property address
  return item->properties[item->n-1];
}

/* wraps json item and returns its parent's address (if exists) */
static CJSON_item_t*
wrap_item(CJSON_item_t *item, char *buffer){
  return item->parent;
}

static CJSON_data*
store_keyname(CJSON_t *cjson, CJSON_data *keyname)
{
  ++cjson->keylist.n;
  cjson->keylist.list = realloc(cjson->keylist.list,cjson->keylist.n*sizeof(char*));
  assert(cjson->keylist.list);

  int i=cjson->keylist.n-1;
  while ((i > 0) && (strcmp(keyname, cjson->keylist.list[i-1]) < 0)){
    cjson->keylist.list[i] = cjson->keylist.list[i-1];
    --i;
  } cjson->keylist.list[i] = keyname;

  return cjson->keylist.list[i];
}

static CJSON_data*
search_keyname(CJSON_t *cjson, CJSON_data *keyname)
{
  int top=cjson->keylist.n-1;
  int low=0;
  int mid;

  int cmp;
  while (low <= top){
    mid = ((ulong)low + (ulong)top) >> 1;
    cmp=strcmp(keyname, cjson->keylist.list[mid]);
    if (cmp == 0){
      free(keyname);
      return cjson->keylist.list[mid];
    }
    if (cmp < 0)
      top = mid-1;
    else
      low = mid+1;
  }

  return store_keyname(cjson, keyname);
}

/* get numerical key for array type
    json data, in string format */
static CJSON_data*
get_array_key(CJSON_t *cjson, CJSON_item_t *item)
{
  const int len=25;
  //will be free'd inside search_keyname if necessary
  CJSON_data *keyname=malloc(len*sizeof(CJSON_data));
  assert(keyname);
  snprintf(keyname,len,"%ld",item->n);

  return search_keyname(cjson, keyname);
}

static CJSON_data*
get_json_string(char **ptr_buffer)
{
  char *start=*ptr_buffer;
  char *end=start;

  while (*end != DOUBLE_QUOTES){
    if (*end++ == '\\'){ //skips \" char
      ++end;
    }
  }

  CJSON_data *data=strndup(start, end-start);
  assert(data);

  *ptr_buffer = end+1;

  return data;
}

static double
get_json_number(char **ptr_buffer)
{
  char *start=*ptr_buffer-1;
  char *end=start;

  if (*end == '-'){
    ++end;
  }
  while (isdigit(*end)){
    ++end;
  }
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

  CJSON_data *data=strndup(start, end-start);
  assert(data);

  double number;
  sscanf(data,"%lf",&number);

  free(data);

  *ptr_buffer = end;

  return number;
}

/* get and return data from appointed datatype */
static CJSON_value_t
datatype_token(CJSON_types_t datatype, char **ptr_buffer)
{
  char *temp_buffer=*ptr_buffer;

  CJSON_value_t value={0};
  switch (datatype){
    case JsonString:
      value.string = get_json_string(ptr_buffer);
      break;
    case JsonNumber:
      value.number = get_json_number(ptr_buffer);
      break;
    case JsonBoolean:
      if (*(temp_buffer-1) == 't'){
        temp_buffer += strlen("true")-2;
        value.boolean = 1;
      } else {
        temp_buffer += strlen("false")-2;
        value.boolean = 0;
      }
      *ptr_buffer = temp_buffer+1;
      break;
    case JsonNull:
      temp_buffer += strlen("null")-2;
      *ptr_buffer = temp_buffer+1;
      break;
    default:
      fprintf(stderr,"ERROR: invalid datatype %ld\n", datatype);
      exit(1);
      break;
  }

  return value;
}

/* create property from appointed JSON datatype
    and return the item containing it */
static CJSON_item_t*
new_property(CJSON_item_t *item, CJSON_data *key, CJSON_types_t datatype, char **ptr_buffer)
{
  char *temp_buffer=*ptr_buffer;

  item = new_item(item);
  item->key = key;
  item->datatype = datatype;
  item->value = datatype_token(datatype, &temp_buffer);
  item = wrap_item(item, temp_buffer);

  *ptr_buffer = temp_buffer; //save buffer changes

  return item;
}

/* create nested object/array and return
    the nested object/array address.

   note that it only retrieves the value
    start, but it doesn't know the value's
    length until wrap_item() function has 
    been called.
  nested objects/array move
    through json argument much like a binary tree */
static CJSON_item_t*
new_nested_object(CJSON_item_t *item, CJSON_data *key, CJSON_types_t datatype, char **ptr_buffer)
{
  item = new_item(item);
  item->key = key;
  item->datatype = datatype;

  return item;
}

/* get bitmask bits and json datatype by
    evaluating buffer's current position */
static void
eval_json_token(CJSON_types_t *datatype, bitmask_t *mask, char *buffer)
{
  switch (*buffer){
    case COMMA:/*NEW PROPERTY DETECTED*/
      BITMASK_SET(*mask,FoundKey);
      break;
    case COLON:/*KEY DETECTED*/
      BITMASK_SET(*mask,FoundAssign);
      break;
    case DOUBLE_QUOTES:/*STRING DETECTED*/
      BITMASK_SET(*mask,FoundString);
      *datatype = JsonString;
      break;
    case OPEN_BRACKET:/*OBJECT DETECTED*/
      BITMASK_SET(*mask,FoundObject|FoundKey);
      *datatype = JsonObject;
      break;
    case OPEN_SQUARE_BRACKET:/*ARRAY DETECTED*/
      BITMASK_SET(*mask,FoundArray);
      *datatype = JsonArray;
      break;
    case CLOSE_BRACKET:       /*WRAPPER*/
    case CLOSE_SQUARE_BRACKET:/*DETECTED*/
      BITMASK_SET(*mask,FoundWrapper);
      break;
    case 't':  /* CHECK FOR */
    case 'f': /*TRUE or FALSE*/
      if (strncmp(buffer,"true",strlen("true"))
          && strncmp(buffer,"false",strlen("false")))
        break; //break if not true or false
      BITMASK_SET(*mask, FoundProperty);
      *datatype = JsonBoolean;
      break;
    case 'n':/*CHECK FOR NULL*/
      if (strncmp(buffer,"null",strlen("null")))
        break; //break if not null
      BITMASK_SET(*mask, FoundProperty);
      *datatype = JsonNull;
      break;
    default:/*CHECK FOR NUMBER*/
      if ((!isdigit(*buffer)) && (*buffer != '-'))
        break; //break if first char is not digit or minus sign
      BITMASK_SET(*mask, FoundProperty);
      *datatype = JsonNumber;
      break;
  }
}

/* perform actions appointed by bitmask and
    return newly updated or retrieved item */
static CJSON_item_t*
apply_json_token(CJSON_item_t *item, CJSON_data *key, CJSON_types_t datatype, bitmask_t *mask, char **ptr_buffer)
{
  if (BITMASK_EQUALITY(*mask,FoundKey) || BITMASK_EQUALITY(*mask,FoundAssign)
      || BITMASK_EQUALITY(*mask,0))
    return item; //early exit if mask is 0 or set with only FoundKey or FoundAssign

  char *temp_buffer=*ptr_buffer;

  if (*mask & FoundProperty){
    //similar to updating current's node attributes in a binary tree
    item = new_property(item, key, datatype, &temp_buffer);
    BITMASK_CLEAR(*mask,FoundProperty);
  }
  else if (*mask & (FoundObject|FoundArray)){
    //similar to creating a new node in a binary tree
    // and then returning its address
    item = new_nested_object(item, key, datatype, &temp_buffer);
    BITMASK_CLEAR(*mask,(FoundObject|FoundArray));
  }
  else if (*mask & FoundWrapper){
    //similar to retrieving the node's parent in a binary tree
    item = wrap_item(item, temp_buffer);
    BITMASK_CLEAR(*mask,FoundWrapper);
  }

  if (*mask & FoundAssign)
    BITMASK_CLEAR(*mask,FoundAssign);

  *ptr_buffer = temp_buffer;

  return item;
}

CJSON_t*
parse_json(char *buffer)
{
  CJSON_t *cjson=calloc(1,sizeof(CJSON_t));
  assert(cjson);
  cjson->item=calloc(1,sizeof(CJSON_item_t));
  assert(cjson->item);

  CJSON_item_t *item=cjson->item;

  cjson->memsize = strlen(buffer);

  bitmask_t mask=0;
  CJSON_data *key=NULL;
  CJSON_types_t datatype=0;
  while (*buffer){ //while not null terminator char
    //get tokens(datatype, mask) with current buffer's position evaluation 
    eval_json_token(&datatype, &mask, buffer);
    ++buffer;
    //deal if special key fetching case for when item is an array
    if (item->datatype == JsonArray)
      key = get_array_key(cjson,item);
    //else check if bitmask demands a key's string to be fetched
    else if (BITMASK_EQUALITY(mask,FoundString|FoundKey)){
      CJSON_data *keyname=NULL;
      keyname = get_json_string(&buffer);
      key = search_keyname(cjson,keyname);
      BITMASK_CLEAR(mask,mask);
      continue;
    }
    //perform actions indicated by bitmask, applying fetched tokens
    item = apply_json_token(item, key, datatype, &mask, &buffer);
  }

  return cjson;
}

static void
apply_reviver(CJSON_item_t *item, void (*reviver)(CJSON_item_t*))
{
  (*reviver)(item);
  for (size_t i=0 ; i < item->n ; ++i){
    apply_reviver(item->properties[i], reviver);
  }
}

CJSON_t*
parse_json_reviver(char *buffer, void (*reviver)(CJSON_item_t*))
{
  CJSON_t *cjson = parse_json(buffer);
  if (reviver != NULL){
    apply_reviver(cjson->item, reviver);
  }

  return cjson;
}

/* destroy current item and all of its nested object/arrays */
static void
destroy_json_item(CJSON_item_t *item)
{
  for (size_t i=0 ; i < item->n ; ++i){
    destroy_json_item(item->properties[i]);
  } free(item->properties);

  if (item->datatype == JsonString)
    free(item->value.string);
  free(item);
}

/* destroy cjson struct */
void
destroy_json(CJSON_t *cjson)
{
  destroy_json_item(cjson->item);
  
  if (cjson->keylist.n){
    while (--cjson->keylist.n >= 0){
      free(cjson->keylist.list[cjson->keylist.n]);
    } free(cjson->keylist.list);
  } free(cjson);
}
