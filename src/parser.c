#include "../JSON.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

/* create new json item and return it's address */
static json_item_st*
json_item_property_create(json_item_st *item)
{
  ++item->num_branch; //update object's property count
  //update memory space for property's list
  item->branch = realloc(item->branch, item->num_branch*sizeof(json_item_st*));
  assert(item->branch);
  //allocate memory space for new property (nested item)
  item->branch[item->num_branch-1] = calloc(1,sizeof(json_item_st));
  assert(item->branch[item->num_branch-1]);
  //get parent address of the new property
  item->branch[item->num_branch-1]->parent = item;
  //return new property address
  return item->branch[item->num_branch-1];
}

/* Destroy current item and all of its nested object/arrays */
static void
json_item_destroy(json_item_st *item)
{
  for (size_t i=0; i < item->num_branch; ++i){
    json_item_destroy(item->branch[i]);
  }
  free(item->branch);

  if (item->type == JSON_STRING){
    free(item->string);
  }
  free(item);
}

json_st*
json_create()
{
  json_st *new_json = calloc(1,sizeof(json_st));
  assert(new_json);

  new_json->root = calloc(1,sizeof(json_item_st));
  assert(new_json->root);

  return new_json;
}

/* Destroy json struct */
void
json_destroy(json_st *json)
{
  json_item_destroy(json->root);
  
  while (json->num_ptr_key){
    free(*json->list_ptr_key[--json->num_ptr_key]);
    free(json->list_ptr_key[json->num_ptr_key]);
  }
  free(json->list_ptr_key);
  free(json);
}

static json_string_kt*
json_string_cache_key(json_st *json, const json_string_kt cache_entry)
{
  ++json->num_ptr_key;

  json->list_ptr_key = realloc(json->list_ptr_key, json->num_ptr_key*sizeof(char**));
  assert(json->list_ptr_key);

  int i = json->num_ptr_key-1;
  while ((i > 0) && (strcmp(cache_entry, *json->list_ptr_key[i-1]) < 0)){
    json->list_ptr_key[i] = json->list_ptr_key[i-1];
    --i;
  }
  //this extra space will be necessary for
  // doing replace operations
  json_string_kt *ptr_new_key = malloc(sizeof(json_string_kt));
  assert(ptr_new_key);
  *ptr_new_key = strndup(cache_entry,strlen(cache_entry));
  assert(*ptr_new_key);

  json->list_ptr_key[i] = ptr_new_key;
  return json->list_ptr_key[i];
}

static json_string_kt*
json_string_get_key(json_st *json, const json_string_kt cache_entry)
{
  int found_index = json_search_key(json, cache_entry);
  if (found_index != -1)
    return json->list_ptr_key[found_index];
  // if key not found, create it and save in cache
  return json_string_cache_key(json, cache_entry);
}

/* get numerical key for array type
    json data, in string format */
static json_string_kt* //fix: get asprintf to work
json_string_set_array_key(json_st *json, json_item_st *item)
{
  char cache_entry[MAX_DIGITS];
  snprintf(cache_entry,MAX_DIGITS-1,"%ld",item->num_branch);

  return json_string_get_key(json, cache_entry);
}

static json_string_kt
json_string_set(char **ptr_buffer)
{
  char *start = *ptr_buffer;
  assert(*start == '\"');

  char *end = ++start;
  while ((*end) && (*end != '\"')){
    if (*end++ == '\\'){ //skips \" char
      ++end;
    }
  }
  assert(*end == '\"');
  *ptr_buffer = end+1; //store position after double quotes

  json_string_kt set_str = strndup(start, end-start);
  assert(set_str);

  return set_str;
}

static void
json_string_set_static(char **ptr_buffer, json_string_kt set_str, const int n)
{
  char *start = *ptr_buffer;
  assert(*start == '\"');

  char *end = ++start;
  while ((*end) && (*end != '\"')){
    if (*end++ == '\\'){ //skips \" char
      ++end;
    }
  }
  assert(*end == '\"');
  *ptr_buffer = end+1; //store position after double quotes
  
  if (n < end-start)
    strncpy(set_str, start, n);
  else
    strncpy(set_str, start, end-start);
}

static json_number_kt
json_number_set(char **ptr_buffer)
{
  char *start = *ptr_buffer;
  char *end = start;

  if (*end == '-'){
    ++end;
  }

  assert(isdigit(*end));
  while (isdigit(*++end))
    continue;

  if (*end == '.'){
    while (isdigit(*++end))
      continue;
  }
  //check for exponent
  if ((*end == 'e') || (*end == 'E')){
    ++end;
    if ((*end == '+') || (*end == '-')){
      ++end;
    }
    assert(isdigit(*end));
    while (isdigit(*++end))
      continue;
  }

  assert(end-start <= MAX_DIGITS);
  char get_numstr[MAX_DIGITS] = {0};
  strncpy(get_numstr, start, end-start);

  json_number_kt set_number;
  sscanf(get_numstr,"%lf",&set_number);

  *ptr_buffer = end;

  return set_number;
}

/* get and return data from appointed datatype */
static json_item_st*
json_item_set_value(json_type_et get_type, json_item_st *item, char **ptr_buffer)
{
  item->type = get_type;

  switch (item->type){
    case JSON_STRING:
      item->string = json_string_set(ptr_buffer);
      break;
    case JSON_NUMBER:
      item->number = json_number_set(ptr_buffer);
      break;
    case JSON_BOOLEAN:
      if (**ptr_buffer == 't'){
        *ptr_buffer += 4; //length of "true"
        item->boolean = 1;
        break;
      }
      *ptr_buffer += 5; //length of "false"
      item->boolean = 0;
      break;
    case JSON_NULL:
      *ptr_buffer += 4; //length of "null"
      break;
    case JSON_ARRAY:
    case JSON_OBJECT:
      ++*ptr_buffer;
      break;
    default:
      fprintf(stderr,"ERROR: invalid datatype %ld\n", item->type);
      exit(EXIT_FAILURE);
  }

  return item;
}

/* create nested object and return
    the nested object address. */
static json_item_st*
json_item_set_incomplete(json_item_st *item, json_string_kt *get_ptr_key, json_type_et get_type, char **ptr_buffer)
{
  item = json_item_property_create(item);
  item->ptr_key = get_ptr_key;
  item = json_item_set_value(get_type, item, ptr_buffer);

  return item;
}

static json_item_st*
json_item_wrap(json_item_st *item){
  return item->parent; //wraps property in item (completes it)
}

/* create property from appointed JSON datatype
    and return the item containing it */
static json_item_st*
json_item_set_complete(json_item_st *item, json_string_kt *get_ptr_key, json_type_et get_type, char **ptr_buffer)
{
  item = json_item_set_incomplete(item, get_ptr_key, get_type, ptr_buffer);
  return json_item_wrap(item);
}

static json_item_st*
json_item_build_array(json_st *json, json_item_st *item, char **ptr_buffer)
{
  json_item_st* (*item_setter)(json_item_st*,json_string_kt*,json_type_et,char**);
  json_type_et set_type;

  switch (**ptr_buffer){
    case ']':/*ARRAY WRAPPER DETECTED*/
      ++*ptr_buffer;
      return json_item_wrap(item);
    case '{':/*OBJECT DETECTED*/
      set_type = JSON_OBJECT;
      item_setter = &json_item_set_incomplete;
      break;
    case '[':/*ARRAY DETECTED*/
      set_type = JSON_ARRAY;
      item_setter = &json_item_set_incomplete;
      break;
    case '\"':/*STRING DETECTED*/
      set_type = JSON_STRING;
      item_setter = &json_item_set_complete;
      break;
    case 't':/*CHECK FOR*/
    case 'f':/* BOOLEAN */
      if (strncmp(*ptr_buffer,"true",4) 
          && strncmp(*ptr_buffer,"false",5)){
        goto error;
      }
      set_type = JSON_BOOLEAN;
      item_setter = &json_item_set_complete;
      break;
    case 'n':/*CHECK FOR NULL*/
      if (strncmp(*ptr_buffer,"null",4)){
        goto error;
      }
      set_type = JSON_NULL;
      item_setter = &json_item_set_complete;
      break;
    case ',': /*ignore comma*/
      ++*ptr_buffer;
      return item;
    default:
      /*IGNORE CONTROL CHARACTER*/
      if (isspace(**ptr_buffer)
          || iscntrl(**ptr_buffer)){
        ++*ptr_buffer;
        return item;
      }
      /*CHECK FOR NUMBER*/
      if (!isdigit(**ptr_buffer)
          && (**ptr_buffer != '-')){
        goto error;
      }
      set_type = JSON_NUMBER;
      item_setter = &json_item_set_complete;
      break;
  }

  return (*item_setter)(item,json_string_set_array_key(json,item),set_type,ptr_buffer);

  error:
    fprintf(stderr,"ERROR: invalid json token %c\n", **ptr_buffer);
    exit(EXIT_FAILURE);
}

static json_item_st*
json_item_build_object(json_st *json, json_item_st *item, json_string_kt **ptr_key, char **ptr_buffer)
{
  json_item_st* (*item_setter)(json_item_st*,json_string_kt*,json_type_et,char**);
  json_type_et set_type;
  char set_key[KEY_LENGTH] = {0};

  switch (**ptr_buffer){
    case '}':/*OBJECT WRAPPER DETECTED*/
      ++*ptr_buffer;
      return json_item_wrap(item);
    case '\"':/*KEY STRING DETECTED*/
      json_string_set_static(ptr_buffer,set_key,KEY_LENGTH);
      *ptr_key = json_string_get_key(json,set_key);
      return item;
    case ':':/*VALUE DETECTED*/
        do { //skips space and control characters before next switch
          ++*ptr_buffer;
        } while (isspace(**ptr_buffer) || iscntrl(**ptr_buffer));

        switch (**ptr_buffer){ //fix: move to function
          case '{':/*OBJECT DETECTED*/
            set_type = JSON_OBJECT;
            item_setter = &json_item_set_incomplete;
            break;
          case '[':/*ARRAY DETECTED*/
            set_type = JSON_ARRAY;
            item_setter = &json_item_set_incomplete;
            break;
          case '\"':/*STRING DETECTED*/
            set_type = JSON_STRING;
            item_setter = &json_item_set_complete;
            break;
          case 't':/*CHECK FOR*/
          case 'f':/* BOOLEAN */
            if (strncmp(*ptr_buffer,"true",4)
                && strncmp(*ptr_buffer,"false",5)){
              goto error;
            }
            set_type = JSON_BOOLEAN;
            item_setter = &json_item_set_complete;
            break;
          case 'n':/*CHECK FOR NULL*/
            if (strncmp(*ptr_buffer,"null",4)){
              goto error; 
            }
            set_type = JSON_NULL;
            item_setter = &json_item_set_complete;
            break;
          default:
            /*CHECK FOR NUMBER*/
            if (!isdigit(**ptr_buffer)
                && (**ptr_buffer != '-')){
              goto error;
            }
            set_type = JSON_NUMBER;
            item_setter = &json_item_set_complete;
            break;
        }
      return (*item_setter)(item,*ptr_key,set_type,ptr_buffer);
    case ',': //ignore comma
      ++*ptr_buffer;
      return item;
    default:
      /*IGNORE CONTROL CHARACTER*/
      if (isspace(**ptr_buffer) || iscntrl(**ptr_buffer)){
        ++*ptr_buffer;
        return item;
      }
      goto error;
  }

  error:
    fprintf(stderr,"ERROR: invalid json token %c\n", **ptr_buffer);
    exit(EXIT_FAILURE);
}

static json_item_st*
json_item_build_entity(json_item_st *item, char **ptr_buffer)
{
  json_type_et set_type;

  switch (**ptr_buffer){
    case '{':/*OBJECT DETECTED*/
      set_type = JSON_OBJECT;
      break;
    case '[':/*ARRAY DETECTED*/
      set_type = JSON_ARRAY;
      break;
    case '\"':/*STRING DETECTED*/
      set_type = JSON_STRING;
      break;
    case 't':/*CHECK FOR*/
    case 'f':/* BOOLEAN */
      if (strncmp(*ptr_buffer,"true",4)
          && strncmp(*ptr_buffer,"false",5)){
        goto error;
      }
      set_type = JSON_BOOLEAN;
      break;
    case 'n':/*CHECK FOR NULL*/
      if (strncmp(*ptr_buffer,"null",4)){
        goto error;
      }
      set_type = JSON_NULL;
      break;
    default:
      /*IGNORE CONTROL CHARACTER*/
      if (isspace(**ptr_buffer)
          || iscntrl(**ptr_buffer)){
        ++*ptr_buffer;
        return item;
      }
      /*CHECK FOR NUMBER*/
      if (!isdigit(**ptr_buffer)
          && (**ptr_buffer != '-')){
        goto error;
      }
      set_type = JSON_NUMBER;
      break;
  }

  return json_item_set_value(set_type, item, ptr_buffer);

  error:
    fprintf(stderr,"ERROR: invalid json token %c\n",**ptr_buffer);
    exit(EXIT_FAILURE);
}

/* get json  by evaluating buffer's current position */
static json_item_st*
json_item_build(json_st *json, json_item_st *item, json_string_kt **ptr_key, char **ptr_buffer)
{
  switch(item->type){
    case JSON_OBJECT:
      return json_item_build_object(json,item,ptr_key,ptr_buffer);
    case JSON_ARRAY:
      return json_item_build_array(json,item,ptr_buffer);
    case JSON_UNDEFINED://this should be true only at the first call
      return json_item_build_entity(item,ptr_buffer);
    default: //nothing else to build, check buffer for potential error
      if (isspace(**ptr_buffer) || iscntrl(**ptr_buffer)){
        ++*ptr_buffer; //moves if cntrl character found ('\n','\b',..)
        return item;
      }
      goto error;
  }

  error:
    fprintf(stderr,"ERROR: invalid json token %c\n",**ptr_buffer);
    exit(EXIT_FAILURE);
}

json_st*
json_parse(char *buffer)
{
  json_st *json = json_create();

  json_string_kt *ptr_set_key;
  json_item_st *item = json->root;
  //build while item and buffer aren't nulled
  while (item && *buffer){
    item = json_item_build(json,item,&ptr_set_key,&buffer);
  }

  return json;
}

static void
apply_reviver(json_item_st *item, void (*reviver)(json_item_st*))
{
  (*reviver)(item);
  for (size_t i=0; i < item->num_branch; ++i){
    apply_reviver(item->branch[i], reviver);
  }
}

json_st*
json_parse_reviver(char *buffer, void (*reviver)(json_item_st*))
{
  json_st *json = json_parse(buffer);

  if (reviver){
    apply_reviver(json->root, reviver);
  }

  return json;
}

