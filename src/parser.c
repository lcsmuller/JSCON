#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "libjsonc.h"

struct utils_s {
  char *buffer;
  char hold_key[KEY_LENGTH]; //holds keys found between calls
  jsonc_hasht_st *last_accessed_hashtable; //holds last hashtable accessed
  jsonc_callbacks_ft* parser_cb; //parser callback
};

/* function pointers used while building json items, 
  jsonc_create_value_ft points to functions prefixed by "jsonc_set_value_"
  jsonc_create_item_ft points to functions prefixed by "jsonc_set_item_" */
typedef void (jsonc_create_value_ft)(jsonc_item_st *item, struct utils_s *utils);
typedef jsonc_item_st* (jsonc_create_item_ft)(jsonc_item_st*, struct utils_s*, jsonc_create_value_ft*);

/* create a new branch to current jsonc item, and return the new branch address */
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

/* destroy current item and all of its nested object/arrays */
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

/* fetch string type jsonc and parse into static string */
static void
jsonc_hold_key(struct utils_s *utils)
{
  char *start = utils->buffer;

  char *end = ++start;
  while (('\0' != *end) && ('\"' != *end)){
    if ('\\' == *end++){ //skips escaped characters
      ++end;
    }
  }
  assert('\"' == *end); //makes sure end of string exists

  utils->buffer = end+1; //skips double quotes buffer position
  
  /* if actual length is lesser than desired length,
    use the actual length instead */
  int length;
  if (KEY_LENGTH > end - start){
    strncpy(utils->hold_key, start, end - start);
    length = end-start;
  } else {
    strncpy(utils->hold_key, start, KEY_LENGTH-1);
    length = KEY_LENGTH-1;
  }
  utils->hold_key[length] = 0;
}

/* fetch string type jsonc and return
  allocated string */
static void
jsonc_set_value_string(jsonc_item_st *item, struct utils_s *utils)
{
  char *start = utils->buffer;
  assert('\"' == *start); //makes sure a string is given

  char *end = ++start;
  while (('\0' != *end) && ('\"' != *end)){
    if ('\\' == *end++){ //skips escaped characters
      ++end;
    }
  }
  assert('\"' == *end); //makes sure end of string exists

  utils->buffer = end + 1; //skips double quotes buffer position

  jsonc_char_kt* set_str = strndup(start, end - start);
  assert(NULL != set_str);

  item->string = set_str;

  item->type = JSONC_STRING;
}

/* fetch number jsonc type by parsing string,
  find out whether its a integer or double and assign*/
static void
jsonc_set_value_number(jsonc_item_st *item, struct utils_s *utils)
{
  char *start = utils->buffer;
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
    item->d_number = set_double;
    item->type = JSONC_NUMBER_DOUBLE;
  } else {
    item->i_number = (int64_t)set_double;
    item->type = JSONC_NUMBER_INTEGER;
  }

  utils->buffer = end; //skips entire length of number
}

static void
jsonc_set_value_boolean(jsonc_item_st *item, struct utils_s *utils)
{
  if ('t' == *utils->buffer){
    item->boolean = true;
    utils->buffer += 4; //skips length of "true"
    return;
  }
  item->boolean = false;
  utils->buffer += 5; //skips length of "false"

  item->type = JSONC_BOOLEAN;
}

static void
jsonc_set_value_null(jsonc_item_st *item, struct utils_s *utils)
{
  utils->buffer += 4; //skips length of "null"

  item->type = JSONC_NULL;
}

//executed inside jsonc_set_value_object and jsonc_set_value_array routines
inline static void
jsonc_set_hashtable(jsonc_item_st *item, struct utils_s *utils)
{
  item->hashtable = jsonc_hashtable_init();
  jsonc_hashtable_link_r(item, &utils->last_accessed_hashtable);
}

static void
jsonc_set_value_object(jsonc_item_st *item, struct utils_s *utils)
{
  jsonc_set_hashtable(item, utils);
  ++utils->buffer; //skips '{' token
  item->type = JSONC_OBJECT;
}

static void
jsonc_set_value_array(jsonc_item_st *item, struct utils_s *utils)
{
  jsonc_set_hashtable(item, utils);
  ++utils->buffer; //skips '[' token
  item->type = JSONC_ARRAY;
}

/* Create nested object and return the nested object address. 
  This is used for arrays and objects type jsonc */
static jsonc_item_st*
jsonc_set_incomplete_item(jsonc_item_st *item, struct utils_s *utils, jsonc_create_value_ft *value_setter)
{
  item = jsonc_branch_create(item);
  item->key = strdup(utils->hold_key);
  assert(NULL != item->key);

  (*value_setter)(item, utils);

  return (utils->parser_cb)(item);
}

/* Wrap array or object type jsonc, which means
  all of its properties have been created */
static jsonc_item_st*
jsonc_wrap_incomplete_item(jsonc_item_st *item, struct utils_s *utils)
{
  ++utils->buffer; //skips '}' or ']'
  jsonc_hashtable_build(item);
  return jsonc_get_parent(item);
}

/* Create a property. The "complete" means all
  of its value is created at once, as opposite
  of array or object type jsonc */
static jsonc_item_st*
jsonc_set_complete_item(jsonc_item_st *item, struct utils_s *utils, jsonc_create_value_ft *value_setter)
{
  item = jsonc_set_incomplete_item(item, utils, value_setter);
  return jsonc_get_parent(item);
}

/* this will be active if the current item is of array type jsonc,
  whatever item is created here will be this array's property.
  if a ']' token is found then the array is wrapped up */
static jsonc_item_st*
jsonc_build_array(jsonc_item_st *item, struct utils_s *utils)
{
  jsonc_create_item_ft *item_setter;
  jsonc_create_value_ft *value_setter;

  switch (*utils->buffer){
  case ']':/*ARRAY WRAPPER DETECTED*/
      return jsonc_wrap_incomplete_item(item, utils);
  case '{':/*OBJECT DETECTED*/
      item_setter = &jsonc_set_incomplete_item;
      value_setter = &jsonc_set_value_object;
      break;
  case '[':/*ARRAY DETECTED*/
      item_setter = &jsonc_set_incomplete_item;
      value_setter = &jsonc_set_value_array;
      break;
  case '\"':/*STRING DETECTED*/
      item_setter = &jsonc_set_complete_item;
      value_setter = &jsonc_set_value_string;
      break;
  case 't':/*CHECK FOR*/
  case 'f':/* BOOLEAN */
      if (!STRNEQ(utils->buffer,"true",4) && !STRNEQ(utils->buffer,"false",5)){
        goto error;
      }
      item_setter = &jsonc_set_complete_item;
      value_setter = &jsonc_set_value_boolean;
      break;
  case 'n':/*CHECK FOR NULL*/
      if (!STRNEQ(utils->buffer,"null",4)){
        goto error;
      }
      item_setter = &jsonc_set_complete_item;
      value_setter = &jsonc_set_value_null;
      break;
  case ',': /*NEXT ELEMENT TOKEN*/
      ++utils->buffer; //skips ','
      return item;
  default:
      /*SKIPS IF CONTROL CHARACTER*/
      if (isspace(*utils->buffer) || iscntrl(*utils->buffer)){
        ++utils->buffer;
        return item;
      }
      /*CHECK FOR NUMBER*/
      if (!isdigit(*utils->buffer) && ('-' != *utils->buffer)){
        goto error;
      }
      item_setter = &jsonc_set_complete_item;
      value_setter = &jsonc_set_value_number;
      break;
  }

  //creates numerical key for the array element
  snprintf(utils->hold_key, MAX_DIGITS-1, "%ld", item->num_branch);

  return (*item_setter)(item, utils, value_setter);


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
  jsonc_create_item_ft *item_setter;
  jsonc_create_value_ft *value_setter;

  switch (*utils->buffer){
  case '}':/*OBJECT WRAPPER DETECTED*/
      return jsonc_wrap_incomplete_item(item, utils);
  case '\"':/*KEY STRING DETECTED*/
      jsonc_hold_key(utils);
      return item;
  case ',': /*NEXT PROPERTY TOKEN*/
      ++utils->buffer; //skips ','
      return item;
  case ':':/*VALUE DETECTED*/
      do {
        ++utils->buffer; //skips ':' and consecutive space and control characters (if any)
      } while (isspace(*utils->buffer) || iscntrl(*utils->buffer));

      switch (*utils->buffer){ //TODO: nested switch are weird
      case '{':/*OBJECT DETECTED*/
          item_setter = &jsonc_set_incomplete_item;
          value_setter = &jsonc_set_value_object;
          break;
      case '[':/*ARRAY DETECTED*/
          item_setter = &jsonc_set_incomplete_item;
          value_setter = &jsonc_set_value_array;
          break;
      case '\"':/*STRING DETECTED*/
          item_setter = &jsonc_set_complete_item;
          value_setter = &jsonc_set_value_string;
          break;
      case 't':/*CHECK FOR*/
      case 'f':/* BOOLEAN */
          if (!STRNEQ(utils->buffer,"true",4) && !STRNEQ(utils->buffer,"false",5)){
            goto error;
          }
          item_setter = &jsonc_set_complete_item;
          value_setter = &jsonc_set_value_boolean;
          break;
      case 'n':/*CHECK FOR NULL*/
          if (!STRNEQ(utils->buffer,"null",4)){
            goto error; 
          }
          item_setter = &jsonc_set_complete_item;
          value_setter = &jsonc_set_value_null;
          break;
      default:
          /*CHECK FOR NUMBER*/
          if (!isdigit(*utils->buffer) && ('-' != *utils->buffer)){
            goto error;
          }
          item_setter = &jsonc_set_complete_item;
          value_setter = &jsonc_set_value_number;
          break;
      }

      return (*item_setter)(item, utils, value_setter);

  default:
      /*SKIPS IF CONTROL CHARACTER*/
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
  switch (*utils->buffer){
  case '{':/*OBJECT DETECTED*/
      jsonc_set_value_object(item, utils);
      break;
  case '[':/*ARRAY DETECTED*/
      jsonc_set_value_array(item, utils);
      break;
  case '\"':/*STRING DETECTED*/
      jsonc_set_value_string(item, utils);
      break;
  case 't':/*CHECK FOR*/
  case 'f':/* BOOLEAN */
      if (!STRNEQ(utils->buffer,"true",4) && !STRNEQ(utils->buffer,"false",5)){
        goto error;
      }
      jsonc_set_value_boolean(item, utils);
      break;
  case 'n':/*CHECK FOR NULL*/
      if (!STRNEQ(utils->buffer,"null",4)){
        goto error;
      }
      jsonc_set_value_null(item, utils);
      break;
  default:
      /*SKIPS IF CONTROL CHARACTER*/
      if (isspace(*utils->buffer) || iscntrl(*utils->buffer)){
        ++utils->buffer;
        break;
      }
      /*CHECK FOR NUMBER*/
      if (!isdigit(*utils->buffer) && ('-' != *utils->buffer)){
        goto error;
      }
      jsonc_set_value_number(item, utils);
      break;
  }

  return item;


  error:
    fprintf(stderr,"ERROR: invalid json token %c\n", *utils->buffer);
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

  //build while item and buffer aren't nulled
  jsonc_item_st *item = root;
  while ((NULL != item) && ('\0' != *utils.buffer)){
    switch(item->type){
    case JSONC_OBJECT:
        item = jsonc_build_object(item, &utils);
        break;
    case JSONC_ARRAY:
        item = jsonc_build_array(item, &utils);
        break;
    case JSONC_UNDEFINED://this should be true only at the first call
        item = jsonc_build_entity(item, &utils);
        break;
    default: //nothing else to build, check buffer for potential error
        if (!(isspace(*utils.buffer) || iscntrl(*utils.buffer))){
          goto error;
        }
        ++utils.buffer; //moves if cntrl character found ('\n','\b',..)
        break;
    }
  }

  return root;


  error:
    fprintf(stderr,"ERROR: invalid json token %c\n",*utils.buffer);
    exit(EXIT_FAILURE);
}
