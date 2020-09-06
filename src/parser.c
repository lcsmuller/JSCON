#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "hashtable.h"
#include "parser.h"
#include "public.h"
#include "macros.h"

struct utils_s {
  char *buffer;
  char tmp_key[KEY_LENGTH]; //holds keys found between calls
  json_hasht_st *last_accessed_hashtable; //holds last hashtable accessed
};

/* create and branch json item to current's and return it's address */
static json_item_st*
json_item_branch_create(json_item_st *item)
{
  ++item->num_branch;
  item->branch = realloc(item->branch, item->num_branch * sizeof *item);
  assert(NULL != item->branch);

  item->branch[item->num_branch-1] = calloc(1, sizeof *item);
  assert(NULL != item->branch[item->num_branch-1]);

  item->branch[item->num_branch-1]->parent = item;

  return item->branch[item->num_branch-1];
}

/* Destroy current item and all of its nested object/arrays */
void
json_item_destroy(json_item_st *item)
{
  for (size_t i=0; i < item->num_branch; ++i){
    json_item_destroy(item->branch[i]);
  }

  switch (json_item_get_type(item)){
  case JSON_STRING:
    free(item->string);
    item->string = NULL;
    break;
  case JSON_OBJECT:
  case JSON_ARRAY:
    json_hashtable_destroy(item->hashtable);
    /* FALLTHROUGH */
  default:
    free(item->branch);
    item->branch = NULL;
    break;
  }

  free(item);
  item = NULL;
}

/* fetch string type json and return
  allocated string */
static json_string_kt
json_string_set(char **p_buffer)
{
  char *start = *p_buffer;
  assert('\"' == *start); //makes sure a string is given

  char *end = ++start;
  while (('\0' != *end) && ('\"' != *end)){
    if ('\\' == *end++){ //skips escaped characters
      ++end;
    }
  }
  assert('\"' == *end); //makes sure end of string exists

  *p_buffer = end + 1; //skips double quotes buffer position

  json_string_kt set_str = strndup(start, end - start);
  assert(NULL != set_str);

  return set_str;
}

/* fetch string type json and parse into static string */
static void
json_string_set_static(char **p_buffer, char set_str[], const int kStr_length)
{
  char *start = *p_buffer;
  assert('\"' == *start); //makes sure a string is given

  char *end = ++start;
  while (('\0' != *end) && ('\"' != *end)){
    if ('\\' == *end++){ //skips escaped characters
      ++end;
    }
  }
  assert('\"' == *end); //makes sure end of string exists

  *p_buffer = end+1; //skips double quotes buffer position
  
  /* if actual length is lesser than desired length,
    use the actual length instead */
  int length;
  if (kStr_length > end - start){
    strncpy(set_str, start, end - start);
    length = end-start;
  } else {
    strncpy(set_str, start, kStr_length);
    length = kStr_length;
  }
  set_str[length] = 0;
}

/* fetch number json type by parsing string,
  return value converted to double type */
static json_number_kt
json_number_set(char **p_buffer)
{
  char *start = *p_buffer;
  char *end = start;

  if ('-' == *end){
    ++end; //skips minus sign
  }

  assert(isdigit(*end)); //interrupt if char isn't digit

  while (isdigit(*++end))
    continue; //skips while char is digit

  if ('.' == *end){
    while (isdigit(*++end))
      continue;
  }

  //if exponent found skips its signal and numbers
  if (('e' == *end) || ('E' == *end)){
    ++end;
    if (('+' == *end) || ('-' == *end)){ 
      ++end;
    }
    assert(isdigit(*end));
    while (isdigit(*++end))
      continue;
  }

  char get_numstr[MAX_DIGITS] = {0};
  strncpy(get_numstr, start, end-start);

  json_number_kt set_number;
  sscanf(get_numstr,"%lf",&set_number); //@todo: replace sscanf?

  *p_buffer = end;

  return set_number;
}

/* get and return value from given json type */
static json_item_st*
json_item_set_value(json_type_et get_type, json_item_st *item, struct utils_s *utils)
{
  item->type = get_type;

  switch (item->type){
  case JSON_STRING:
      item->string = json_string_set(&utils->buffer);
      break;
  case JSON_NUMBER:
      item->number = json_number_set(&utils->buffer);
      break;
  case JSON_BOOLEAN:
      if ('t' == *utils->buffer){
        utils->buffer += 4; //skips length of "true"
        item->boolean = 1;
        break;
      }
      utils->buffer += 5; //skips length of "false"
      item->boolean = 0;
      break;
  case JSON_NULL:
      utils->buffer += 4; //skips length of "null"
      break;
  case JSON_ARRAY:
  case JSON_OBJECT:
      item->hashtable = json_hashtable_init();

      json_hashtable_link_r(item, &utils->last_accessed_hashtable);
      ++utils->buffer;
      break;
  default:
      fprintf(stderr,"ERROR: invalid datatype %ld\n", item->type);
      exit(EXIT_FAILURE);
  }

  return item;
}

/* Create nested object and return the nested object address. 
  This is used for arrays and objects type json */
static json_item_st*
json_item_set_incomplete(json_item_st *item, json_type_et get_type, struct utils_s *utils)
{
  item = json_item_branch_create(item);
  item = json_item_set_value(get_type, item, utils);
  item->key = strdup(utils->tmp_key);
  assert(NULL != item->key);

  return item;
}

/* Wrap array or object type json, which means
  all of its properties have been created */
static json_item_st*
json_item_wrap(json_item_st *item){
  return json_item_get_parent(item);
}

/* Create a property. The "complete" means all
  of its value is created at once, as opposite
  of array or object type json */
static json_item_st*
json_item_set_complete(json_item_st *item, json_type_et get_type, struct utils_s *utils)
{
  item = json_item_set_incomplete(item, get_type, utils);
  return json_item_wrap(item);
}

/* this will be active if the current item is of array type json,
  whatever item is created here will be this array's property.
  if a ']' token is found then the array is wrapped up */
static json_item_st*
json_item_build_array(json_item_st *item, struct utils_s *utils)
{
  json_item_st* (*item_setter)(json_item_st*, json_type_et, struct utils_s *utils);
  json_type_et tmp_type;

  switch (*utils->buffer){
  case ']':/*ARRAY WRAPPER DETECTED*/
      ++utils->buffer;
      json_hashtable_build(item);
      return json_item_wrap(item);
  case '{':/*OBJECT DETECTED*/
      tmp_type = JSON_OBJECT;
      item_setter = &json_item_set_incomplete;
      break;
  case '[':/*ARRAY DETECTED*/
      tmp_type = JSON_ARRAY;
      item_setter = &json_item_set_incomplete;
      break;
  case '\"':/*STRING DETECTED*/
      tmp_type = JSON_STRING;
      item_setter = &json_item_set_complete;
      break;
  case 't':/*CHECK FOR*/
  case 'f':/* BOOLEAN */
      if (!STRNEQ(utils->buffer,"true",4) && !STRNEQ(utils->buffer,"false",5)){
        goto error;
      }
      tmp_type = JSON_BOOLEAN;
      item_setter = &json_item_set_complete;
      break;
  case 'n':/*CHECK FOR NULL*/
      if (!STRNEQ(utils->buffer,"null",4)){
        goto error;
      }
      tmp_type = JSON_NULL;
      item_setter = &json_item_set_complete;
      break;
  case ',': /*NEXT ELEMENT TOKEN*/
      ++utils->buffer;
      return item;
  default:
      /*IGNORE CONTROL CHARACTER*/
      if (isspace(*utils->buffer) || iscntrl(*utils->buffer)){
        ++utils->buffer;
        return item;
      }
      /*CHECK FOR NUMBER*/
      if (!isdigit(*utils->buffer) && ('-' != *utils->buffer)){
        goto error;
      }
      tmp_type = JSON_NUMBER;
      item_setter = &json_item_set_complete;
      break;
  }

  //creates numerical key for the array element
  snprintf(utils->tmp_key, MAX_DIGITS-1, "%ld", item->num_branch);

  return (*item_setter)(item, tmp_type, utils);


  error:
    fprintf(stderr,"ERROR: invalid json token %c\n", *utils->buffer);
    exit(EXIT_FAILURE);
}

/* this will be active if the current item is of object type json,
  whatever item is created here will be this object's property.
  if a '}' token is found then the object is wrapped up */
static json_item_st*
json_item_build_object(json_item_st *item, struct utils_s *utils)
{
  json_item_st* (*item_setter)(json_item_st*, json_type_et, struct utils_s *utils);
  json_type_et tmp_type;

  switch (*utils->buffer){
  case '}':/*OBJECT WRAPPER DETECTED*/
      ++utils->buffer;
      json_hashtable_build(item);
      return json_item_wrap(item);
  case '\"':/*KEY STRING DETECTED*/
      json_string_set_static(&utils->buffer, utils->tmp_key, KEY_LENGTH);
      return item;
  case ':':/*VALUE DETECTED*/
      do { //skips space and control characters before next switch
        ++utils->buffer;
      } while (isspace(*utils->buffer) || iscntrl(*utils->buffer));

      switch (*utils->buffer){ //fix: move to function
      case '{':/*OBJECT DETECTED*/
          tmp_type = JSON_OBJECT;
          item_setter = &json_item_set_incomplete;
          break;
      case '[':/*ARRAY DETECTED*/
          tmp_type = JSON_ARRAY;
          item_setter = &json_item_set_incomplete;
          break;
      case '\"':/*STRING DETECTED*/
          tmp_type = JSON_STRING;
          item_setter = &json_item_set_complete;
          break;
      case 't':/*CHECK FOR*/
      case 'f':/* BOOLEAN */
          if (!STRNEQ(utils->buffer,"true",4) && !STRNEQ(utils->buffer,"false",5)){
            goto error;
          }
          tmp_type = JSON_BOOLEAN;
          item_setter = &json_item_set_complete;
          break;
      case 'n':/*CHECK FOR NULL*/
          if (!STRNEQ(utils->buffer,"null",4)){
            goto error; 
          }
          tmp_type = JSON_NULL;
          item_setter = &json_item_set_complete;
          break;
      default:
          /*CHECK FOR NUMBER*/
          if (!isdigit(*utils->buffer) && ('-' != *utils->buffer)){
            goto error;
          }
          tmp_type = JSON_NUMBER;
          item_setter = &json_item_set_complete;
          break;
      }
      return (*item_setter)(item, tmp_type, utils);
  case ',': //ignore comma
      ++utils->buffer;
      return item;
  default:
      /*IGNORE CONTROL CHARACTER*/
      if (isspace(*utils->buffer) || iscntrl(*utils->buffer)){
        ++utils->buffer;
        return item;
      }
      goto error;
  }


  error:
    fprintf(stderr,"ERROR: invalid json token %c\n", *utils->buffer);
    exit(EXIT_FAILURE);
}

/* this call will only be used once, at the first iteration,
  it also allows the creation of a json that's not part of an
  array or object. ex: json_item_parse("10") */
static json_item_st*
json_item_build_entity(json_item_st *item, struct utils_s *utils)
{
  json_type_et tmp_type;

  switch (*utils->buffer){
  case '{':/*OBJECT DETECTED*/
      tmp_type = JSON_OBJECT;
      break;
  case '[':/*ARRAY DETECTED*/
      tmp_type = JSON_ARRAY;
      break;
  case '\"':/*STRING DETECTED*/
      tmp_type = JSON_STRING;
      break;
  case 't':/*CHECK FOR*/
  case 'f':/* BOOLEAN */
      if (!STRNEQ(utils->buffer,"true",4) && !STRNEQ(utils->buffer,"false",5)){
        goto error;
      }
      tmp_type = JSON_BOOLEAN;
      break;
  case 'n':/*CHECK FOR NULL*/
      if (!STRNEQ(utils->buffer,"null",4)){
        goto error;
      }
      tmp_type = JSON_NULL;
      break;
  default:
      /*IGNORE CONTROL CHARACTER*/
      if (isspace(*utils->buffer) || iscntrl(*utils->buffer)){
        ++utils->buffer;
        return item;
      }
      /*CHECK FOR NUMBER*/
      if (!isdigit(*utils->buffer) && ('-' != *utils->buffer)){
        goto error;
      }
      tmp_type = JSON_NUMBER;
      break;
  }

  return json_item_set_value(tmp_type, item, utils);


  error:
    fprintf(stderr,"ERROR: invalid json token %c\n", *utils->buffer);
    exit(EXIT_FAILURE);
}

/* build json item by evaluating buffer's current position token */
static json_item_st*
json_item_build(json_item_st *item, struct utils_s *utils)
{
  switch(item->type){
  case JSON_OBJECT:
      return json_item_build_object(item, utils);
  case JSON_ARRAY:
      return json_item_build_array(item, utils);
  case JSON_UNDEFINED://this should be true only at the first call
      return json_item_build_entity(item, utils);
  default: //nothing else to build, check buffer for potential error
      if (isspace(*utils->buffer) || iscntrl(*utils->buffer)){
        ++utils->buffer; //moves if cntrl character found ('\n','\b',..)
        return item;
      }
      goto error;
  }

  error:
    fprintf(stderr,"ERROR: invalid json token %c\n",*utils->buffer);
    exit(EXIT_FAILURE);
}

/* parse contents from buffer into a json item object
  and return its root */
json_item_st*
json_item_parse(char *buffer)
{
  json_item_st *root = calloc(1, sizeof *root);
  assert(NULL != root);

  struct utils_s utils = {
    .buffer = buffer,
  };

  json_item_st *item = root;
  //build while item and buffer aren't nulled
  while ((NULL != item) && ('\0' != *utils.buffer)){
    item = json_item_build(item, &utils);
  }

  return root;
}

/* apply user defined function to contents being parsed */
static void
apply_reviver(json_item_st *item, void (*reviver)(json_item_st*))
{
  (*reviver)(item);
  for (size_t i=0; i < item->num_branch; ++i){
    apply_reviver(item->branch[i], reviver);
  }
}

/* like original parse, but with an option to modify json
  content during parsing */
json_item_st*
json_item_parse_reviver(char *buffer, void (*reviver)(json_item_st*))
{
  json_item_st *root = json_item_parse(buffer);

  if (NULL != reviver){
    apply_reviver(root, reviver);
  }

  return root;
}
