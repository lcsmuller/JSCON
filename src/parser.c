#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "libjsonc.h"

struct utils_s {
  char *buffer;
  char tmp_key[KEY_LENGTH]; //holds keys found between calls
  jsonc_hasht_st *last_accessed_hashtable; //holds last hashtable accessed
  jsonc_callbacks_ft* parser_cb; //parser callback
};

typedef jsonc_item_st* (jsonc_item_set_ft)(jsonc_item_st*, jsonc_type_et, struct utils_s *utils);

/* create and branch jsonc item to current's and return it's address */
static jsonc_item_st*
jsonc_branch_create(jsonc_item_st *item)
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
jsonc_destroy(jsonc_item_st *item)
{
  for (size_t i=0; i < item->num_branch; ++i){
    jsonc_destroy(item->branch[i]);
  }

  switch (jsonc_get_type(item)){
  case JSONC_STRING:
    free(item->string);
    item->string = NULL;
    break;
  case JSONC_OBJECT:
  case JSONC_ARRAY:
    jsonc_hashtable_destroy(item->hashtable);
    free(item->branch);
    item->branch = NULL;
    break;
  default:
    break;
  }

  free(item);
  item = NULL;
}

/* fetch string type jsonc and return
  allocated string */
static jsonc_char_kt*
jsonc_string_set(char **p_buffer)
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

  jsonc_char_kt* set_str = strndup(start, end - start);
  assert(NULL != set_str);

  return set_str;
}

/* fetch string type jsonc and parse into static string */
static void
jsonc_string_set_static(char **p_buffer, char set_str[], const int kStr_length)
{
  char *start = *p_buffer;

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

/* fetch number jsonc type by parsing string,
  find out whether its a integer or double and assign*/
static void
jsonc_number_set(jsonc_item_st *item, char **p_buffer)
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

  jsonc_double_kt set_double;
  sscanf(get_numstr,"%lf",&set_double); //TODO: replace sscanf?

  if (!DOUBLE_IS_INTEGER(set_double)){
    item->type = JSONC_NUMBER_DOUBLE;
    item->d_number = set_double;
  } else {
    item->type = JSONC_NUMBER_INTEGER;
    item->i_number = (int64_t)set_double;
  }

  *p_buffer = end;
}

/* get and return value from given jsonc type */
static jsonc_item_st*
jsonc_set_value(jsonc_type_et get_type, jsonc_item_st *item, struct utils_s *utils)
{
  item->type = get_type;

  switch (item->type){
  case JSONC_STRING:
      item->string = jsonc_string_set(&utils->buffer);
      break;
  case JSONC_NUMBER: //check for double or integer
      jsonc_number_set(item, &utils->buffer);
      break;
  case JSONC_BOOLEAN:
      if ('t' == *utils->buffer){
        item->boolean = true;
        utils->buffer += 4; //skips length of "true"
        break;
      }
      item->boolean = false;
      utils->buffer += 5; //skips length of "false"
      break;
  case JSONC_NULL:
      utils->buffer += 4; //skips length of "null"
      break;
  case JSONC_ARRAY:
  case JSONC_OBJECT:
      item->hashtable = jsonc_hashtable_init();
      jsonc_hashtable_link_r(item, &utils->last_accessed_hashtable);
      ++utils->buffer;
      break;
  default:
      fprintf(stderr,"ERROR: invalid datatype %d\n", item->type);
      exit(EXIT_FAILURE);
  }

  return (utils->parser_cb)(item);
}

/* Create nested object and return the nested object address. 
  This is used for arrays and objects type jsonc */
static jsonc_item_st*
jsonc_set_incomplete(jsonc_item_st *item, jsonc_type_et get_type, struct utils_s *utils)
{
  item = jsonc_branch_create(item);
  item->key = strdup(utils->tmp_key);
  assert(NULL != item->key);

  item = jsonc_set_value(get_type, item, utils);

  return item;
}

/* Wrap array or object type jsonc, which means
  all of its properties have been created */
static jsonc_item_st*
jsonc_wrap(jsonc_item_st *item){
  return jsonc_get_parent(item);
}

/* Create a property. The "complete" means all
  of its value is created at once, as opposite
  of array or object type jsonc */
static jsonc_item_st*
jsonc_set_complete(jsonc_item_st *item, jsonc_type_et get_type, struct utils_s *utils)
{
  item = jsonc_set_incomplete(item, get_type, utils);
  return jsonc_wrap(item);
}

/* this will be active if the current item is of array type jsonc,
  whatever item is created here will be this array's property.
  if a ']' token is found then the array is wrapped up */
static jsonc_item_st*
jsonc_build_array(jsonc_item_st *item, struct utils_s *utils)
{
  jsonc_item_set_ft *item_setter;
  jsonc_type_et tmp_type;

  switch (*utils->buffer){
  case ']':/*ARRAY WRAPPER DETECTED*/
      ++utils->buffer;
      jsonc_hashtable_build(item);
      return jsonc_wrap(item);
  case '{':/*OBJECT DETECTED*/
      tmp_type = JSONC_OBJECT;
      item_setter = &jsonc_set_incomplete;
      break;
  case '[':/*ARRAY DETECTED*/
      tmp_type = JSONC_ARRAY;
      item_setter = &jsonc_set_incomplete;
      break;
  case '\"':/*STRING DETECTED*/
      tmp_type = JSONC_STRING;
      item_setter = &jsonc_set_complete;
      break;
  case 't':/*CHECK FOR*/
  case 'f':/* BOOLEAN */
      if (!STRNEQ(utils->buffer,"true",4) && !STRNEQ(utils->buffer,"false",5)){
        goto error;
      }
      tmp_type = JSONC_BOOLEAN;
      item_setter = &jsonc_set_complete;
      break;
  case 'n':/*CHECK FOR NULL*/
      if (!STRNEQ(utils->buffer,"null",4)){
        goto error;
      }
      tmp_type = JSONC_NULL;
      item_setter = &jsonc_set_complete;
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
      tmp_type = JSONC_NUMBER;
      item_setter = &jsonc_set_complete;
      break;
  }

  //creates numerical key for the array element
  snprintf(utils->tmp_key, MAX_DIGITS-1, "%ld", item->num_branch);

  return (*item_setter)(item, tmp_type, utils);


  error:
    fprintf(stderr,"ERROR: invalid json token %c\n", *utils->buffer);
    exit(EXIT_FAILURE);
}

/* this will be active if the current item is of object type jsonc,
  whatever item is created here will be this object's property.
  if a '}' token is found then the object is wrapped up */
static jsonc_item_st*
jsonc_build_object(jsonc_item_st *item, struct utils_s *utils)
{
  jsonc_item_set_ft *item_setter;
  jsonc_type_et tmp_type;

  switch (*utils->buffer){
  case '}':/*OBJECT WRAPPER DETECTED*/
      ++utils->buffer;
      jsonc_hashtable_build(item);
      return jsonc_wrap(item);
  case '\"':/*KEY STRING DETECTED*/
      jsonc_string_set_static(&utils->buffer, utils->tmp_key, KEY_LENGTH);
      return item;
  case ':':/*VALUE DETECTED*/
      do { //skips space and control characters before next switch
        ++utils->buffer;
      } while (isspace(*utils->buffer) || iscntrl(*utils->buffer));

      switch (*utils->buffer){ //fix: move to function
      case '{':/*OBJECT DETECTED*/
          tmp_type = JSONC_OBJECT;
          item_setter = &jsonc_set_incomplete;
          break;
      case '[':/*ARRAY DETECTED*/
          tmp_type = JSONC_ARRAY;
          item_setter = &jsonc_set_incomplete;
          break;
      case '\"':/*STRING DETECTED*/
          tmp_type = JSONC_STRING;
          item_setter = &jsonc_set_complete;
          break;
      case 't':/*CHECK FOR*/
      case 'f':/* BOOLEAN */
          if (!STRNEQ(utils->buffer,"true",4) && !STRNEQ(utils->buffer,"false",5)){
            goto error;
          }
          tmp_type = JSONC_BOOLEAN;
          item_setter = &jsonc_set_complete;
          break;
      case 'n':/*CHECK FOR NULL*/
          if (!STRNEQ(utils->buffer,"null",4)){
            goto error; 
          }
          tmp_type = JSONC_NULL;
          item_setter = &jsonc_set_complete;
          break;
      default:
          /*CHECK FOR NUMBER*/
          if (!isdigit(*utils->buffer) && ('-' != *utils->buffer)){
            goto error;
          }
          tmp_type = JSONC_NUMBER;
          item_setter = &jsonc_set_complete;
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
  it also allows the creation of a jsonc that's not part of an
  array or object. ex: jsonc_item_parse("10") */
static jsonc_item_st*
jsonc_build_entity(jsonc_item_st *item, struct utils_s *utils)
{
  jsonc_type_et tmp_type;

  switch (*utils->buffer){
  case '{':/*OBJECT DETECTED*/
      tmp_type = JSONC_OBJECT;
      break;
  case '[':/*ARRAY DETECTED*/
      tmp_type = JSONC_ARRAY;
      break;
  case '\"':/*STRING DETECTED*/
      tmp_type = JSONC_STRING;
      break;
  case 't':/*CHECK FOR*/
  case 'f':/* BOOLEAN */
      if (!STRNEQ(utils->buffer,"true",4) && !STRNEQ(utils->buffer,"false",5)){
        goto error;
      }
      tmp_type = JSONC_BOOLEAN;
      break;
  case 'n':/*CHECK FOR NULL*/
      if (!STRNEQ(utils->buffer,"null",4)){
        goto error;
      }
      tmp_type = JSONC_NULL;
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
      tmp_type = JSONC_NUMBER;
      break;
  }

  return jsonc_set_value(tmp_type, item, utils);


  error:
    fprintf(stderr,"ERROR: invalid json token %c\n", *utils->buffer);
    exit(EXIT_FAILURE);
}

/* build jsonc item by evaluating buffer's current position token */
static jsonc_item_st*
jsonc_build(jsonc_item_st *item, struct utils_s *utils)
{
  switch(item->type){
  case JSONC_OBJECT:
      return jsonc_build_object(item, utils);
  case JSONC_ARRAY:
      return jsonc_build_array(item, utils);
  case JSONC_UNDEFINED://this should be true only at the first call
      return jsonc_build_entity(item, utils);
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

static inline jsonc_item_st*
jsonc_default_callback(jsonc_item_st *item){
  return item;
}

jsonc_callbacks_ft*
jsonc_parser_callback(jsonc_callbacks_ft *new_cb)
{
  jsonc_callbacks_ft *parser_cb = &jsonc_default_callback;

  if (NULL != new_cb){
    parser_cb = new_cb;
  }

  return parser_cb;
}

/* parse contents from buffer into a jsonc item object
  and return its root */
jsonc_item_st*
jsonc_parse(char *buffer)
{
  jsonc_item_st *root = calloc(1, sizeof *root);
  assert(NULL != root);

  struct utils_s utils = {
    .buffer = buffer,
    .parser_cb = jsonc_parser_callback(NULL),
  };

  jsonc_item_st *item = root;
  //build while item and buffer aren't nulled
  while ((NULL != item) && ('\0' != *utils.buffer)){
    item = jsonc_build(item, &utils);
  }

  return root;
}
