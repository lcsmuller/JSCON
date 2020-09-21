#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "libjsonc.h"

struct utils_s {
  char *buffer;
  char set_key[KEY_LENGTH]; //holds key
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
jsonc_set_key(struct utils_s *utils)
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
    strncpy(utils->set_key, start, end - start);
    length = end-start;
  } else {
    strncpy(utils->set_key, start, KEY_LENGTH-1);
    length = KEY_LENGTH-1;
  }
  utils->set_key[length] = 0;
}

static jsonc_char_kt*
utils_buffer_skip_string(struct utils_s *utils)
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

  return set_str;
}

/* fetch string type jsonc and return
  allocated string */
static void
jsonc_set_value_string(jsonc_item_st *item, struct utils_s *utils)
{

  item->string = utils_buffer_skip_string(utils);
  item->type = JSONC_STRING;
}

static jsonc_double_kt
utils_buffer_skip_double(struct utils_s *utils)
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

  utils->buffer = end; //skips entire length of number

  return set_double;
}

/* fetch number jsonc type by parsing string,
  find out whether its a integer or double and assign*/
static void
jsonc_set_value_number(jsonc_item_st *item, struct utils_s *utils)
{
  double set_double = utils_buffer_skip_double(utils);
  if (DOUBLE_IS_INTEGER(set_double)){
    item->i_number = (jsonc_integer_kt)set_double;
    item->type = JSONC_NUMBER_INTEGER;
  } else {
    item->d_number = set_double;
    item->type = JSONC_NUMBER_DOUBLE;
  }
}

static jsonc_boolean_kt
utils_buffer_skip_boolean(struct utils_s *utils)
{
  if ('t' == *utils->buffer){
    utils->buffer += 4; //skips length of "true"
    return true;
  }
  utils->buffer += 5; //skips length of "false"
  return false;
}

static void
jsonc_set_value_boolean(jsonc_item_st *item, struct utils_s *utils)
{
  item->boolean = utils_buffer_skip_boolean(utils);
  item->type = JSONC_BOOLEAN;
}

static void
utils_buffer_skip_null(struct utils_s *utils){
  utils->buffer += 4; //skips length of "null"
}

static void
jsonc_set_value_null(jsonc_item_st *item, struct utils_s *utils)
{
  utils_buffer_skip_null(utils);
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
  item->key = strdup(utils->set_key);
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
  snprintf(utils->set_key, MAX_DIGITS-1, "%ld", item->num_branch);

  return (*item_setter)(item, utils, value_setter);


  error:
    fprintf(stderr,"ERROR: invalid json token %c\n", *utils->buffer);
    exit(EXIT_FAILURE);
}

static jsonc_item_st*
jsonc_set_object_property(jsonc_item_st *item, struct utils_s *utils)
{
  jsonc_set_key(utils);
  assert(':' == *utils->buffer);

  /* skips ':' and consecutive space and any control characters */
  do {
    ++utils->buffer;
  } while (isspace(*utils->buffer) || iscntrl(*utils->buffer));

  jsonc_create_item_ft *item_setter;
  jsonc_create_value_ft *value_setter;

  switch (*utils->buffer){
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
  switch (*utils->buffer){
  case '}':/*OBJECT WRAPPER DETECTED*/
      return jsonc_wrap_incomplete_item(item, utils);
  case '\"':/*KEY STRING DETECTED*/
      return jsonc_set_object_property(item, utils);
  case ',': /*NEXT PROPERTY TOKEN*/
      ++utils->buffer; //skips ','
      return item;
  default:
      /*SKIPS IF CONTROL CHARACTER*/
      if (!(isspace(*utils->buffer) || iscntrl(*utils->buffer))){
        goto error;
      }
      ++utils->buffer;
      return item;
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

static void
jsonc_map_assign(struct utils_s *utils, va_list ap)
{
  switch (*utils->buffer){
  /* TODO: fix wrong object being selected (first child) */
  case '{':/*OBJECT DETECTED*/
  case '[':/*ARRAY DETECTED*/
      *va_arg(ap, jsonc_item_st**) = jsonc_parse(utils->buffer);
      break;
  case '\"':/*STRING DETECTED*/
      *va_arg(ap, jsonc_char_kt**) = utils_buffer_skip_string(utils);
      break;
  case 't':/*CHECK FOR*/
  case 'f':/* BOOLEAN */
      if (!STRNEQ(utils->buffer,"true",4) && !STRNEQ(utils->buffer,"false",5)){
        goto error;
      }
      *va_arg(ap, jsonc_boolean_kt*) = utils_buffer_skip_boolean(utils);
      break;
  case 'n':/*CHECK FOR NULL*/
      if (!STRNEQ(utils->buffer,"null",4)){
        goto error; 
      }
      utils_buffer_skip_null(utils);
      break;
  default:
      /*CHECK FOR NUMBER*/
      if (!isdigit(*utils->buffer) && ('-' != *utils->buffer)){
        goto error;
      }
      
      {
        double set_double = utils_buffer_skip_double(utils);
        if (DOUBLE_IS_INTEGER(set_double)){
          *va_arg(ap, jsonc_integer_kt*) = (jsonc_integer_kt)set_double;
        } else {
          *va_arg(ap, jsonc_double_kt*) = set_double;
        }
      }
      break;
  }

  return;


  error:
    fprintf(stderr,"ERROR: invalid json token %c\n",*utils->buffer);
    exit(EXIT_FAILURE);
}

static void
jsonc_map_skip(struct utils_s *utils)
{
  /* TODO: fix cases repeating themselves */
  int num_nest = 1;
  switch (*utils->buffer){
  case '{':/*OBJECT DETECTED*/
      do {
        if (0 == num_nest){
          break;
        }

        switch(*utils->buffer){
        case '{':
            ++num_nest;
            ++utils->buffer;
            break;
        case '}':
            --num_nest;
            ++utils->buffer;
            break;
        case '\"':
            do {
              if ('\\' == *utils->buffer++){
                ++utils->buffer;
              }
            } while ('\0' != *utils->buffer && '\"' != *utils->buffer);
            assert('\"' == *utils->buffer);
            ++utils->buffer; //skips last comma
            break;
        default:
            ++utils->buffer;
            break;
        }
      } while ('\0' != *utils->buffer);
      assert('\0' != *utils->buffer);
      break;
  case '[':/*ARRAY DETECTED*/
      do {
        if (0 == num_nest){
          break;
        }

        switch(*utils->buffer){
        case '[':
            ++num_nest;
            ++utils->buffer;
            break;
        case ']':
            --num_nest;
            ++utils->buffer;
            break;
        case '\"':
            do {
              if ('\\' == *utils->buffer++){
                ++utils->buffer;
              }
            } while ('\0' != *utils->buffer && '\"' != *utils->buffer);
            assert('\"' == *utils->buffer);
            ++utils->buffer; //skips last comma
            break;
        default:
            ++utils->buffer;
            break;
        }
      } while ('\0' != *utils->buffer);
      assert('\0' != *utils->buffer);
      break;
  case '\"':/*STRING DETECTED*/
      do {
        if ('\\' == *utils->buffer++){
          ++utils->buffer;
        }
      } while ('\0' != *utils->buffer && '\"' != *utils->buffer);
      assert('\"' == *utils->buffer);
      ++utils->buffer; //skips last comma
      break;
  default:
      while ('\0' != *utils->buffer && ',' != *utils->buffer){
        ++utils->buffer;
      }
      break;
  }
}

void
jsonc_map(char *buffer, char *arg_keys, ...)
{
  //skip any space and control characters at start of buffer
  while (isspace(*buffer) || iscntrl(*buffer)){
    ++buffer;
  }
  assert('{' == *buffer); //must be a json object

  struct utils_s utils = {
    .buffer = buffer,
  };

  va_list ap;
  va_start(ap, arg_keys);

  /* TODO: check for wrong inputs, ex : ",,,,A,B,C,,,D,"
      check for individual words, instead of amount of commas*/
  size_t num_keys = 1;
  for (int i=0; '\0' != arg_keys[i]; ++i){
   if (',' == arg_keys[i]){
      ++num_keys;
   }
  }

  char keys_array[num_keys][KEY_LENGTH];

  /* split individual keys from arg_keys and make
      each a key_array element */
  char c;
  size_t key_index = 0;
  size_t char_index = 0;
  do {
    if (',' == (c = *arg_keys++)){
      keys_array[key_index][char_index] = '\0';
      char_index = 0;
      ++key_index;
      assert(key_index <= num_keys);
    } else {
      keys_array[key_index][char_index] = c;
      ++char_index;
      assert(char_index <= KEY_LENGTH);
    }
  } while ('\0' != c);


  size_t last_key_index = 0;
  while ('\0' != *utils.buffer){
    switch (*utils.buffer){
    case '\"':
        jsonc_set_key(&utils);
        assert(':' == *utils.buffer);
        /* skips ':' and consecutive space and any control characters */
        do {
          ++utils.buffer;
        } while (isspace(*utils.buffer) || iscntrl(*utils.buffer));

        /* check wether key found is wanted or not */
        /* TODO: turn this into a hashtable implementation */
        if (STREQ(keys_array[last_key_index], utils.set_key)){
          jsonc_map_assign(&utils, ap);
          ++last_key_index;
        } else {
          jsonc_map_skip(&utils);
        }

        fprintf(stderr, "%s\n", utils.set_key);
        break;
    default:
        ++utils.buffer; //moves if cntrl character found ('\n','\b',..)
        break;
    }
  }

  va_end(ap);
}
