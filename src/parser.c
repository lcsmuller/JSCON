#include "../json_parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

/* numerical key for arrays will be generated and
    stored here */
struct global_numlist {
  char **list;
  long n; 
} numlist;

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
  item->properties = realloc(item->properties, sizeof(CJSON_item_t*)*item->n);
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

/* get numerical key for array type
    json data, in string format */
static CJSON_data_t
array_datatype(CJSON_item_t *item)
{
  CJSON_data_t key={0};
  //check if global number list already contains
  //current object property's amount value
  if (numlist.n <= item->n){
    char number[25]; //hold number in string format
    //assign property's amount value to number
    snprintf(number,24,"%ld",item->n);
    //update global list size to include new number
    numlist.list = realloc(numlist.list,(item->n+1)*sizeof(struct global_numlist));
    assert(numlist.list);
    //push new number to it and update list count
    numlist.list[item->n] = strdup(number);
    assert(numlist.list[item->n]);
    numlist.n = item->n+1;
  }
  //updates key with global list's numerical string
  key.start = numlist.list[item->n];
  key.length = strlen(key.start);

  return key;
}

static CJSON_data_t
get_json_string(char **ptr_buffer)
{
  CJSON_data_t data={0};
  char *temp_buffer=*ptr_buffer;

  data.start = temp_buffer;
  while (*temp_buffer != DOUBLE_QUOTES){
    if (*temp_buffer++ == '\\'){ //skips \" char
      ++temp_buffer;
    }
  }
  data.length = temp_buffer - data.start;

  *ptr_buffer = temp_buffer+1;

  return data;
}

static CJSON_data_t
get_json_number(char **ptr_buffer)
{
  CJSON_data_t data={0};
  char *temp_buffer=*ptr_buffer;

  data.start = temp_buffer-1;
  if (*temp_buffer == '-')
    ++temp_buffer;
  while (isdigit(*temp_buffer))
    ++temp_buffer;
  if (*temp_buffer == '.'){
    while (isdigit(*++temp_buffer))
      continue;
  }
  //check for exponent
  if ((*temp_buffer == 'e') || (*temp_buffer == 'E')){
    ++temp_buffer;
    if ((*temp_buffer == '+') || (*temp_buffer == '-'))
      ++temp_buffer;
    while (isdigit(*temp_buffer))
      ++temp_buffer;
  }
  --temp_buffer; //return from non numerical char
  data.length = temp_buffer+1 - data.start;

  *ptr_buffer = temp_buffer+1;

  return data;
}

/* get and return data from appointed datatype */
static CJSON_value_t
datatype_token(CJSON_types_t datatype, char **ptr_buffer)
{
  CJSON_data_t data={0};
  CJSON_value_t value={0};

  char *temp_buffer=*ptr_buffer;
  char *temp_str;

  switch (datatype){
    case JsonString:
      data = get_json_string(ptr_buffer);
      value.string = strndup(data.start,data.length);
      break;
    case JsonNumber:
      data = get_json_number(ptr_buffer);
      temp_str = strndup(data.start,data.length);
      sscanf(temp_str,"%lf",&value.number);
      free(temp_str);
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
new_property(CJSON_item_t *item, CJSON_data_t key, CJSON_types_t datatype, char **ptr_buffer)
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
new_nested_object(CJSON_item_t *item, CJSON_data_t key, CJSON_types_t datatype, char **ptr_buffer)
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
apply_json_token(CJSON_item_t *item, CJSON_data_t *key, CJSON_types_t datatype, bitmask_t *mask, char **ptr_buffer)
{
  if (BITMASK_EQUALITY(*mask,FoundKey) || BITMASK_EQUALITY(*mask,FoundAssign)
      || BITMASK_EQUALITY(*mask,0))
    return item; //early exit if mask is 0 or set with only FoundKey or FoundAssign

  char *temp_buffer=*ptr_buffer;

  if (*mask & FoundProperty){
    //similar to updating current's node attributes in a binary tree
    item = new_property(item, *key, datatype, &temp_buffer);
    BITMASK_CLEAR(*mask,FoundProperty);
  }
  else if (*mask & (FoundObject|FoundArray)){
    //similar to creating a new node in a binary tree
    // and then returning its address
    item = new_nested_object(item, *key, datatype, &temp_buffer);
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

CJSON_item_t*
parse_json(char *buffer)
{
  CJSON_item_t *first_item=calloc(1,sizeof(CJSON_item_t));
  assert(first_item);
  CJSON_item_t *item=first_item;

  CJSON_data_t key={0};

  bitmask_t mask=0;
  CJSON_types_t datatype=0;
  while (*buffer){ //while not null terminator char
    //get tokens(datatype, mask) with current buffer's position evaluation 
    eval_json_token(&datatype, &mask, buffer);
    ++buffer;
    //deal if special key fetching case for when item is an array
    if (item->datatype == JsonArray)
      key = array_datatype(item);
    //else check if bitmask demands a key's string to be fetched
    else if (BITMASK_EQUALITY(mask,FoundString|FoundKey)){
      key = get_json_string(&buffer);
      BITMASK_CLEAR(mask,mask);
      continue;
    }
    //perform actions indicated by bitmask, applying fetched tokens
    item = apply_json_token(item, &key, datatype, &mask, &buffer);
  }

  return first_item;
}

static void
apply_reviver(CJSON_item_t *item, void (*reviver)(CJSON_item_t*)){
  (*reviver)(item);
  for (size_t i=0 ; i < item->n ; ++i){
    apply_reviver(item->properties[i], reviver);
  }
}

CJSON_item_t*
parse_json_reviver(char *buffer, void (*reviver)(CJSON_item_t*)){
  CJSON_item_t *item = parse_json(buffer);
  if (reviver != NULL){
    apply_reviver(item, reviver);
  }

  return item;
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

/* destroy global number list and json item */
void
destroy_json(CJSON_item_t *item)
{
  if (numlist.n){
    while (numlist.n >= 0){
      free(numlist.list[numlist.n]);
      --numlist.n;
    } free(numlist.list);
  }

  destroy_json_item(item);
}
