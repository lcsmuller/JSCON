/*
 * Copyright (c) 2020 Lucas Müller
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include <libjscon.h>

#include "jscon-common.h"
#include "debug.h"


struct jscon_utils_s {
  char *buffer;
  char *key; //holds key ptr to be received by item
  jscon_htwrap_st *last_accessed_htwrap; //holds last htwrap accessed
  jscon_callbacks_ft* parser_cb; //parser callback
};

/* function pointers used while building json items, 
  jscon_create_value_ft points to functions prefixed by "_jscon_value_set_"
  jscon_create_item_ft points to functions prefixed by "jscon_decode" */
typedef void (jscon_create_value_ft)(jscon_item_st *item, struct jscon_utils_s *utils);
typedef jscon_item_st* (jscon_create_item_ft)(jscon_item_st*, struct jscon_utils_s*, jscon_create_value_ft*);

/* create a new branch to current jscon object item, and return
  the new branch address */
static jscon_item_st*
_jscon_new_branch(jscon_item_st *item)
{
  ++item->comp->num_branch;

  item->comp->branch[jscon_size(item)-1] = calloc(1, sizeof(jscon_item_st));
  DEBUG_ASSERT(NULL != item->comp->branch[jscon_size(item)-1], "Out of memory");

  item->comp->branch[jscon_size(item)-1]->parent = item;

  return item->comp->branch[jscon_size(item)-1];
}

static void
_jscon_destroy_composite(jscon_item_st *item)
{
  Jscon_htwrap_destroy(item->comp->htwrap);

  free(item->comp->branch);
  item->comp->branch = NULL;

  free(item->comp);
  item->comp = NULL;
}

static void
_jscon_destroy_preorder(jscon_item_st *item)
{
  switch (jscon_get_type(item)){
  case JSCON_OBJECT:
  case JSCON_ARRAY:
      for (size_t i=0; i < jscon_size(item); ++i){
        _jscon_destroy_preorder(item->comp->branch[i]);
      }
      _jscon_destroy_composite(item);
      break;
  case JSCON_STRING:
      free(item->string);
      item->string = NULL;
      break;
  default:
      break;
  }

  free(item->key);
  item->key = NULL;

  free(item);
  item = NULL;
}

/* destroy current item and all of its nested object/arrays */
void
jscon_destroy(jscon_item_st *item){
  _jscon_destroy_preorder(jscon_get_root(item));
}

static char*
_jscon_utils_decode_string(struct jscon_utils_s *utils)
{
  char *start = utils->buffer;
  DEBUG_ASSERT('\"' == *start, "Not a string"); //makes sure a string is given

  char *end = ++start;
  while (('\0' != *end) && ('\"' != *end)){
    if ('\\' == *end++){ //skips escaped characters
      ++end;
    }
  }
  DEBUG_ASSERT('\"' == *end, "Not a string"); //makes sure end of string exists

  utils->buffer = end + 1; //skips double quotes buffer position

  char *set_str = strndup(start, end - start);
  DEBUG_ASSERT(NULL != set_str, "Out of memory");

  return set_str;
}

/* fetch string type jscon and return allocated string */
static void
_jscon_value_set_string(jscon_item_st *item, struct jscon_utils_s *utils)
{
  item->type = JSCON_STRING;
  item->string = _jscon_utils_decode_string(utils);
}

static double
_jscon_utils_decode_double(struct jscon_utils_s *utils)
{
  char *start = utils->buffer;
  char *end = start;

  /* 1st STEP: check for a minus sign and skip it */
  if ('-' == *end){
    ++end; //skips minus sign
  }

  /* 2nd STEP: skips until a non digit char found */
  DEBUG_ASSERT(isdigit(*end), "Not a number"); //interrupt if char isn't digit
  while (isdigit(*++end)){
    continue; //skips while char is digit
  }

  /* 3rd STEP: if non-digit char is not a comma then it must be
      an integer*/
  if ('.' == *end){
    while (isdigit(*++end)){
      continue;
    }
  }

  /* 4th STEP: if exponent found skips its tokens */
  if (('e' == *end) || ('E' == *end)){
    ++end;
    if (('+' == *end) || ('-' == *end)){ 
      ++end;
    }
    DEBUG_ASSERT(isdigit(*end), "Not a number");
    while (isdigit(*++end)){
      continue;
    }
  }

  /* 5th STEP: convert string to double and return its value */
  char numerical_string[MAX_DIGITS];
  strscpy(numerical_string, start, end-start + 1);

  double set_double;
  sscanf(numerical_string,"%lf",&set_double);

  utils->buffer = end; //skips entire length of number

  return set_double;
}

/* fetch number jscon type by parsing string,
  find out whether its a integer or double and assign*/
static void
_jscon_value_set_number(jscon_item_st *item, struct jscon_utils_s *utils)
{
  double set_double = _jscon_utils_decode_double(utils);
  if (DOUBLE_IS_INTEGER(set_double)){
    item->type = JSCON_INTEGER;
    item->i_number = (long long)set_double;
  } else {
    item->type = JSCON_DOUBLE;
    item->d_number = set_double;
  }
}

static bool
_jscon_utils_decode_boolean(struct jscon_utils_s *utils)
{
  if ('t' == *utils->buffer){
    utils->buffer += 4; //skips length of "true"
    return true;
  }
  utils->buffer += 5; //skips length of "false"
  return false;
}

static void
_jscon_value_set_boolean(jscon_item_st *item, struct jscon_utils_s *utils)
{
  item->type = JSCON_BOOLEAN;
  item->boolean = _jscon_utils_decode_boolean(utils);
}

static void
_jscon_utils_decode_null(struct jscon_utils_s *utils){
  utils->buffer += 4; //skips length of "null"
}

static void
_jscon_value_set_null(jscon_item_st *item, struct jscon_utils_s *utils)
{
  item->type = JSCON_NULL;
  _jscon_utils_decode_null(utils);
}

static jscon_composite_st*
_jscon_utils_decode_composite(struct jscon_utils_s *utils, size_t n_branch){
  jscon_composite_st *new_comp = calloc(1, sizeof *new_comp);
  DEBUG_ASSERT(NULL != new_comp, "Out of memory");

  new_comp->htwrap = Jscon_htwrap_init();

  new_comp->branch = malloc((1+n_branch) * sizeof(jscon_item_st*));
  DEBUG_ASSERT(NULL != new_comp->branch, "Out of memory");

  ++utils->buffer; //skips composite's '{' or '[' delim

  return new_comp;
}

inline static size_t
_jscon_count_property(char *buffer)
{
  /* skips the item and all of its nests, special care is taken for any
      inner string is found, as it might contain a delim character that
      if not treated as a string will incorrectly trigger 
      depth action*/
  size_t depth = 0;
  size_t num_branch = 0;
  do {
    switch (*buffer){
    case ':':
        num_branch += (depth == 1);
        break;
    case '{':
        ++depth;
        break;
    case '}':
        --depth;
        break;
    case '\"':
        /* loops until null terminator or end of string are found */
        do {
          /* skips escaped characters */
          if ('\\' == *buffer++){
            ++buffer;
          }
        } while ('\0' != *buffer && '\"' != *buffer);
        DEBUG_ASSERT('\"' == *buffer, "Not a string");
        break;
    }

    ++buffer; //skips whatever char

    if (0 == depth) return num_branch; //entire item has been skipped, return

  } while ('\0' != *buffer);

  DEBUG_ERR("Bad formatting");
  abort();
}

static void
_jscon_value_set_object(jscon_item_st *item, struct jscon_utils_s *utils)
{
  item->type = JSCON_OBJECT;

  item->comp = _jscon_utils_decode_composite(utils, _jscon_count_property(utils->buffer));
  Jscon_htwrap_link_r(item, &utils->last_accessed_htwrap);
}

inline static size_t
_jscon_count_element(char *buffer)
{
  /* skips the item and all of its nests, special care is taken for any
      inner string is found, as it might contain a delim character that
      if not treated as a string will incorrectly trigger 
      depth action*/
  size_t depth = 0;
  size_t num_branch = 0;
  do {
    switch (*buffer){
    case ',':
        num_branch += (depth == 1);
        break;
    case '[':
        ++depth;
        break;
    case ']':
        --depth;
        break;
    case '\"':
        /* loops until null terminator or end of string are found */
        do {
          /* skips escaped characters */
          if ('\\' == *buffer++){
            ++buffer;
          }
        } while ('\0' != *buffer && '\"' != *buffer);
        DEBUG_ASSERT('\"' == *buffer, "Not a String");
        break;
    }

    ++buffer; //skips whatever char

    if (0 == depth) return num_branch; //entire item has been skipped, return

  } while ('\0' != *buffer);

  DEBUG_ERR("Bad formatting");
  abort();
}

static void
_jscon_value_set_array(jscon_item_st *item, struct jscon_utils_s *utils)
{
  item->type = JSCON_ARRAY;

  item->comp = _jscon_utils_decode_composite(utils, _jscon_count_element(utils->buffer));
  Jscon_htwrap_link_r(item, &utils->last_accessed_htwrap);
}

/* create nested composite type (object/array) and return 
    the address. */
static jscon_item_st*
_jscon_new_composite(jscon_item_st *item, struct jscon_utils_s *utils, jscon_create_value_ft *value_setter)
{
  item = _jscon_new_branch(item);
  item->key = utils->key;
  utils->key = NULL;

  (*value_setter)(item, utils);
  item = (utils->parser_cb)(item);

  return item;
}

/* wrap array or object type jscon, which means
    all of its branches have been created */
static jscon_item_st*
_jscon_wrap_composite(jscon_item_st *item, struct jscon_utils_s *utils)
{
  ++utils->buffer; //skips '}' or ']'
  Jscon_htwrap_build(item);
  return jscon_get_parent(item);
}

/* create a primitive data type branch. */
static jscon_item_st*
_jscon_append_primitive(jscon_item_st *item, struct jscon_utils_s *utils, jscon_create_value_ft *value_setter)
{
  item = _jscon_new_branch(item);
  item->key = utils->key;
  utils->key = NULL;

  (*value_setter)(item, utils);
  item = (utils->parser_cb)(item);

  return jscon_get_parent(item);
}

/* this routine is called when setting a branch of a composite type
    (object and array) item. */
static jscon_item_st*
_jscon_build_branch(jscon_item_st *item, struct jscon_utils_s *utils)
{
  jscon_create_item_ft *item_setter;
  jscon_create_value_ft *value_setter;

  switch (*utils->buffer){
  case '{':/*OBJECT DETECTED*/
      //DEBUG_PUTS("NEW OBJECT\n");
      item_setter = &_jscon_new_composite;
      value_setter = &_jscon_value_set_object;
      break;
  case '[':/*ARRAY DETECTED*/
      //DEBUG_PUTS("NEW ARRAY\n");
      item_setter = &_jscon_new_composite;
      value_setter = &_jscon_value_set_array;
      break;
  case '\"':/*STRING DETECTED*/
      //DEBUG_PUTS("NEW STRING\n");
      item_setter = &_jscon_append_primitive;
      value_setter = &_jscon_value_set_string;
      break;
  case 't':/*CHECK FOR*/
  case 'f':/* BOOLEAN */
      if (!STRNEQ(utils->buffer,"true",4) && !STRNEQ(utils->buffer,"false",5)){
        goto token_error;
      }
      //DEBUG_PUTS("NEW BOOL\n");
      item_setter = &_jscon_append_primitive;
      value_setter = &_jscon_value_set_boolean;
      break;
  case 'n':/*CHECK FOR NULL*/
      if (!STRNEQ(utils->buffer,"null",4)){
        goto token_error; 
      }
      //DEBUG_PUTS("NEW NULL\n");
      item_setter = &_jscon_append_primitive;
      value_setter = &_jscon_value_set_null;
      break;
  default:
      /*CHECK FOR NUMBER*/
      if (!isdigit(*utils->buffer) && ('-' != *utils->buffer)){
        goto token_error;
      }
      //DEBUG_PUTS("NEW NUMBER\n");
      item_setter = &_jscon_append_primitive;
      value_setter = &_jscon_value_set_number;
      break;
  }

  return (*item_setter)(item, utils, value_setter);


token_error:
  DEBUG_ERR("Invalid JSON Token: %c", *utils->buffer);
  abort();
}

/* this will be active if the current item is of array type jscon,
  whatever item is created here will be this array's property.
  if a ']' token is found then the array is wrapped up */
static jscon_item_st*
_jscon_build_array(jscon_item_st *item, struct jscon_utils_s *utils)
{
  CONSUME_BLANK_CHARS(utils->buffer);
  //DEBUG_PUTS("arr{%s}: \'%c\' ", item->key, *utils->buffer);
  switch (*utils->buffer){
  case ']':/*ARRAY WRAPPER DETECTED*/
      //DEBUG_PUTS("WRAP {%s}\n", item->key);
      return _jscon_wrap_composite(item, utils);
  case ',': /*NEXT ELEMENT TOKEN*/
      //DEBUG_PUTS("NEXT ELEMENT\n");
      ++utils->buffer; //skips ','
      CONSUME_BLANK_CHARS(utils->buffer);

      return item;
  default:
   {
      //creates numerical key for the array element
      char numerical_key[MAX_DIGITS];
      snprintf(numerical_key, MAX_DIGITS-1, "%ld", jscon_size(item));

      DEBUG_ASSERT(NULL == utils->key, "utils->key wasn't freed");
      utils->key = strdup(numerical_key);
      DEBUG_ASSERT(NULL != utils->key, "Out of memory");

      //DEBUG_PUTS("ARRTOKEN: \'%c\' ", *utils->buffer);
      //DEBUG_PUTS("CREATE {%s} ", utils->key);

      return _jscon_build_branch(item, utils);
   }
  }

  //token error checking done inside _jscon_build_branch
}

/* this will be active if the current item is of object type jscon,
  whatever item is created here will be this object's property.
  if a '}' token is found then the object is wrapped up */
static jscon_item_st*
_jscon_build_object(jscon_item_st *item, struct jscon_utils_s *utils)
{
  CONSUME_BLANK_CHARS(utils->buffer);
  //DEBUG_PUTS("obj{%s}: \'%c\' ", item->key, *utils->buffer);
  switch (*utils->buffer){
  case '}':/*OBJECT WRAPPER DETECTED*/
      //DEBUG_PUTS("WRAP {%s}\n", item->key);
      return _jscon_wrap_composite(item, utils);
  case '\"':/*KEY STRING DETECTED*/
      DEBUG_ASSERT(NULL == utils->key, "utils->key wasn't freed");
      utils->key = _jscon_utils_decode_string(utils);
      DEBUG_ASSERT(':' == *utils->buffer, "Missing ':' token after key"); //check for key's assign token 
      ++utils->buffer; //skips ':'
      CONSUME_BLANK_CHARS(utils->buffer);
      //DEBUG_PUTS("OBJTOKEN: \'%c\' ", *utils->buffer);
      //DEBUG_PUTS("CREATE {%s} ", utils->key);
      return _jscon_build_branch(item, utils);
  case ',': /*NEXT PROPERTY TOKEN*/
      //DEBUG_PUTS("NEXT PROPERTY\n");
      ++utils->buffer; //skips ','
      CONSUME_BLANK_CHARS(utils->buffer);

      return item;
  default:
      /*SKIPS IF CONTROL CHARACTER*/
      if (!(isspace(*utils->buffer) || iscntrl(*utils->buffer))){
        DEBUG_ERR("Invalid JSON Token: %c", *utils->buffer);
      }
      return item;
  }
}

/* this call will only be used once, at the first iteration,
  it also allows the creation of a jscon that's not part of an
  array or object. ex: jscon_item_parse("10") */
static jscon_item_st*
_jscon_build_entity(jscon_item_st *item, struct jscon_utils_s *utils)
{
  //DEBUG_PUTS("ent{%s}: \'%c\' ", item->key, *utils->buffer);
  switch (*utils->buffer){
  case '{':/*OBJECT DETECTED*/
      //DEBUG_PUTS("SET OBJECT\n");
      _jscon_value_set_object(item, utils);
      break;
  case '[':/*ARRAY DETECTED*/
      //DEBUG_PUTS("SET ARRAY\n");
      _jscon_value_set_array(item, utils);
      break;
  case '\"':/*STRING DETECTED*/
      //DEBUG_PUTS("SET STRING\n");
      _jscon_value_set_string(item, utils);
      break;
  case 't':/*CHECK FOR*/
  case 'f':/* BOOLEAN */
      if (!STRNEQ(utils->buffer,"true",4) && !STRNEQ(utils->buffer,"false",5)){
        goto token_error;
      }
      //DEBUG_PUTS("SET BOOL\n");
      _jscon_value_set_boolean(item, utils);
      break;
  case 'n':/*CHECK FOR NULL*/
      if (!STRNEQ(utils->buffer,"null",4)){
        goto token_error;
      }
      //DEBUG_PUTS("SET NULL\n");
      _jscon_value_set_null(item, utils);
      break;
  default:
      CONSUME_BLANK_CHARS(utils->buffer);
      /*CHECK FOR NUMBER*/
      if (!isdigit(*utils->buffer) && ('-' != *utils->buffer)){
        goto token_error;
      }
      //DEBUG_PUTS("SET NUMBER\n");
      _jscon_value_set_number(item, utils);
      break;
  }

  return item;


token_error:
  DEBUG_ERR("Invalid JSON Token: %c", *utils->buffer);
  abort(); 
}

static inline jscon_item_st*
_jscon_default_callback(jscon_item_st *item){
  return item;
}

jscon_callbacks_ft*
jscon_parser_callback(jscon_callbacks_ft *new_cb)
{
  jscon_callbacks_ft *parser_cb = &_jscon_default_callback;

  if (NULL != new_cb){
    parser_cb = new_cb;
  }

  return parser_cb;
}

/* parse contents from buffer into a jscon item object
  and return its root */
jscon_item_st*
jscon_parse(char *buffer)
{
  jscon_item_st *root = calloc(1, sizeof *root);
  if (NULL == root) return NULL;

  struct jscon_utils_s utils = {
    .buffer = buffer,
    .parser_cb = jscon_parser_callback(NULL),
  };
  
  //build while item and buffer aren't nulled
  jscon_item_st *item = root;
  while ((NULL != item) && ('\0' != *utils.buffer)){
    switch(item->type){
    case JSCON_OBJECT:
        item = _jscon_build_object(item, &utils);
        break;
    case JSCON_ARRAY:
        item = _jscon_build_array(item, &utils);
        break;
    case JSCON_UNDEFINED://this should be true only at the first call
        item = _jscon_build_entity(item, &utils);

        /* primitives can't have branches, skip the rest  */
        if (IS_PRIMITIVE(item)) return item;

        break;
    default: //nothing else to build, check buffer for potential error
        if (!(isspace(*utils.buffer) || iscntrl(*utils.buffer))){
          DEBUG_ERR("Invalid JSON Token: %c", *utils.buffer);
        }
        ++utils.buffer; //moves if cntrl character found ('\n','\b',..)
        //DEBUG_PUTS("UNDEFINED \'%c\'\n", *utils.buffer);
        break;
    }
  }

  return root;
}

/* struct that will be stored at jscon_scanf dictionary */
/* @todo make this more explicit */
struct chunk_s {
  void *value;
  char specifier[5];
};

inline static void
_jscon_scanf_skip_string(struct jscon_utils_s *utils)
{
  /* loops until null terminator or end of string are found */
  do {
    /* skips escaped characters */
    if ('\\' == *utils->buffer++){
      ++utils->buffer;
    }
  } while ('\0' != *utils->buffer && '\"' != *utils->buffer);
  DEBUG_ASSERT('\"' == *utils->buffer, "Not a String");
  ++utils->buffer; //skip double quotes
}

inline static void
_jscon_scanf_skip_composite(int ldelim, int rdelim, struct jscon_utils_s *utils)
{
  /* skips the item and all of its nests, special care is taken for any
      inner string is found, as it might contain a delim character that
      if not treated as a string will incorrectly trigger 
      depth action*/
  size_t depth = 0;
  do {
    if (ldelim == *utils->buffer){
      ++depth;
      ++utils->buffer; //skips ldelim char
    } else if (rdelim == *utils->buffer) {
      --depth;
      ++utils->buffer; //skips rdelim char
    } else if ('\"' == *utils->buffer) { //treat string separetely
      _jscon_scanf_skip_string(utils);
    } else {
      ++utils->buffer; //skips whatever char
    }

    if (0 == depth) return; //entire item has been skipped, return

  } while ('\0' != *utils->buffer);
}

static void
_jscon_scanf_skip(struct jscon_utils_s *utils)
{
  switch (*utils->buffer){
  case '{':/*OBJECT DETECTED*/
      _jscon_scanf_skip_composite('{', '}', utils);
      return;
  case '[':/*ARRAY DETECTED*/
      _jscon_scanf_skip_composite('[', ']', utils);
      return;
  case '\"':/*STRING DETECTED*/
      _jscon_scanf_skip_string(utils);
      return;
  default:
      //consume characters while not end of string or not new key
      while ('\0' != *utils->buffer && ',' != *utils->buffer){
        ++utils->buffer;
      }
      return;
  }
}

static char*
_jscon_scanf_format_info(char *specifier, size_t *p_tmp)
{
  size_t *n_bytes;
  size_t discard; //throw values here if p_tmp is NULL
  if (NULL != p_tmp){
    n_bytes = p_tmp;
  } else {
    n_bytes = &discard;
  }

  if (STREQ(specifier, "js")){
    *n_bytes = sizeof(char*);
    return "char*";
  }
  if (STREQ(specifier, "jd")){
    *n_bytes = sizeof(long long);
    return "long long*";
  }
  if (STREQ(specifier, "jf")){
    *n_bytes = sizeof(double);
    return "double*";
  }
  if (STREQ(specifier, "jb")){
    *n_bytes = sizeof(bool);
    return "bool*";
  }
  if (STREQ(specifier, "ji")){
    /* this will never get validated at _jscon_scanf_apply(),
        but it doesn't matter, a JSCON_NULL item is created either way */
    *n_bytes = sizeof(jscon_item_st*);
    return "jscon_item_st**";
  }

  *n_bytes = 0;
  return "NaN";
}

static void
_jscon_scanf_apply(struct jscon_utils_s *utils, struct chunk_s *chunk)
{
  char *specifier = chunk->specifier;
  void *value = chunk->value;
    

  /* if specifier is item, simply call jscon_parse at current buffer token */
  if (STREQ(specifier, "ji")){
    jscon_item_st **item = value;
    *item = jscon_parse(utils->buffer);
    _jscon_scanf_skip(utils); //skip characters parsed by jscon_parse

    /* get key, but keep in mind that this item is an "entity". The key will
        be ignored when serializing with jscon_stringify(); */
    (*item)->key = utils->key;
    utils->key = NULL;
    return;
  }

  /* specifier must be a primitive */
  char err_typeis[50];
  switch (*utils->buffer){
  case '\"':/*STRING DETECTED*/
   {
      if (!STREQ(specifier, "js")){
        char reason[] = "char* or jscon_item_st**";
        strscpy(err_typeis, reason, strlen(reason));
        goto type_error;
      }
      
      char *string = _jscon_utils_decode_string(utils);
      strscpy(value, string, strlen(string));
      free(string);
      return;
   }
  case 't':/*CHECK FOR*/
  case 'f':/* BOOLEAN */
   {
      if (!STRNEQ(utils->buffer,"true",4) && !STRNEQ(utils->buffer,"false",5)){
        goto token_error;
      }
      if (!STREQ(specifier, "jb")){
        char reason[] = "bool* or jscon_item_st**";
        strscpy(err_typeis, reason, strlen(reason));
        goto type_error;
      }

      bool *boolean = value;
      *boolean = _jscon_utils_decode_boolean(utils);
      return;
   }
  case 'n':/*CHECK FOR NULL*/
   {
      if (!STRNEQ(utils->buffer,"null",4)){
        goto token_error; 
      }

      _jscon_utils_decode_null(utils);

      /* null conversion */
      size_t n_bytes; //get amount of bytes that should be set to 0
      _jscon_scanf_format_info(specifier, &n_bytes);
      memset(value, 0, n_bytes);
      return;
   }
  case '{':/*OBJECT DETECTED*/
  case '[':/*ARRAY DETECTED*/
   {
      char reason[] = "jscon_item_st**";
      strscpy(err_typeis, reason, strlen(reason));
      goto type_error;
   }
  default:
   { /*CHECK FOR NUMBER*/
      if (!isdigit(*utils->buffer) && ('-' != *utils->buffer)){
        goto token_error;
      }
      
      double tmp = _jscon_utils_decode_double(utils);
      if (DOUBLE_IS_INTEGER(tmp)){
        if (!STREQ(specifier, "jd")){
          char reason[] = "long long* or jscon_item_st**";
          strscpy(err_typeis, reason, strlen(reason));
          goto type_error;
        }
        long long *number_i = value;
        *number_i = (long long)tmp;
      } else {
        if (!STREQ(specifier, "jf")){
          char reason[] = "double* or jscon_item_st**";
          strscpy(err_typeis, reason, strlen(reason));
          goto type_error;
        }
        double *number_d = value; 
        *number_d = tmp;
      }
      return;
   }
  }


type_error:
  DEBUG_ERR("Expected specifier %s but specifier is %s( found: \"%s\" )\n", err_typeis, _jscon_scanf_format_info(specifier, NULL), specifier);

token_error:
  DEBUG_ERR("Invalid JSON Token: %c", *utils->buffer);
}

/* count amount of keys and check for formatting errors */
static size_t
_jscon_scanf_format_analyze(char *format)
{
  /* count each "word" composed of allowed key characters */
  size_t num_keys = 0;
  char c;
  while (true) //run until end of string found
  {
    //consume any space or control characters sequence
    CONSUME_BLANK_CHARS(format);
    c = *format;

    /*1st STEP: check for key '#' prefix existence */
    if ('#' != c){
      DEBUG_ASSERT('#' == c, "Missing format '#' key prefix"); //make sure key prefix is there
    }
    c = *++format;

    /* @todo check for escaped characters */
    /*2nd STEP: check if key matches a criteria for being a key */
    if (!ALLOWED_JSON_CHAR(c)){
      DEBUG_ERR("Key char not allowed: '%c'", c);
    }

    ++num_keys; //found key, increment num_keys
    do { //consume remaining key characters
      c = *++format;
    } while (ALLOWED_JSON_CHAR(c));

    /*3rd STEP: check if key's type is specified */
    if ('%' != c){
      DEBUG_ASSERT('%' == c, "Missing format '%' type specifier prefix");
    }

    /*@todo error check for available unknown characters */
    do { /* consume type formatting characters */
      c = *++format;
    } while (isalpha(c));

    if ('\0' == c) return num_keys; //return if found end of string

    ++format; //start next key
  }
}

static void
_jscon_scanf_format_decode(char *format, dictionary_st *dictionary, va_list ap)
{
  DEBUG_ASSERT('\0' != *format, "Empty String"); //can't be empty string

  const size_t STR_LEN = 256;
  char str[STR_LEN];

  struct chunk_s *chunk;

  /* split individual keys from specifiers and set them
      to a hashtable */
  char c;
  size_t i = 0; //str char index
  while (true) //run until end of string found
  {
    CONSUME_BLANK_CHARS(format);
    c = *format;

    DEBUG_ASSERT('#' == c, "Missing format '#' key prefix"); //make sure key prefix is there
    c = *++format; //skip '#'
    
    /* 1st STEP: build key specific characters */
    while ('%' != c){
      str[i] = c;
      ++i;
      DEBUG_ASSERT(i <= STR_LEN, "Overflow buffer");
      
      c = *++format;
    }
    str[i] = '\0'; //key is formed
    ++i;
    DEBUG_ASSERT(i <= STR_LEN, "Overflow buffer");

    /* 2nd STEP: key is formed, proceed to fetch the specifier */
    //skips '%' char and fetch successive specifier characters
    do { //isolate specifier characters
      c = *++format;
      str[i] = c;
      ++i;
      DEBUG_ASSERT(i <= STR_LEN, "Overflow buffer");
    } while(isalpha(c));

    str[i-1] = '\0';

    chunk = calloc(1, sizeof *chunk);
    DEBUG_ASSERT(NULL != chunk, "Out of memory");

    strscpy(chunk->specifier, &str[strlen(str)+1], 4); //get specifier string
    chunk->value = va_arg(ap, void*);

    if ( STREQ("NaN", _jscon_scanf_format_info(chunk->specifier, NULL)) ){
      DEBUG_ERR("Unknown type specifier %%%s", chunk->specifier);
    }

    dictionary_set(dictionary, str, chunk, &free);

    if ('\0' == c) return; //end of string, return

    i = 0; //resets i to start next key from scratch
    ++format; //start of next key
  }
}

/* works like sscanf, will parse stuff only for the keys specified to the format string parameter. the variables assigned to ... must be in
the correct order, and type, as the requested keys.

  every key found that doesn't match any of the requested keys will be
  ignored along with all its contents. */
void
jscon_scanf(char *buffer, char *format, ...)
{
  DEBUG_ASSERT(NULL != buffer, "Missing JSON text");

  CONSUME_BLANK_CHARS(buffer);

  if ('{' != *buffer){
    DEBUG_ERR("Item type must be JSCON_OBJECT");
  }

  struct jscon_utils_s utils = {
    .buffer = buffer,
  };

  va_list ap;
  va_start(ap, format);

  size_t num_key = _jscon_scanf_format_analyze(format);

  /* key/value dictionary fetched from format */
  dictionary_st *dictionary = dictionary_init();
  dictionary_build(dictionary, num_key);

  //return keys for freeing later
  _jscon_scanf_format_decode(format, dictionary, ap);
  DEBUG_ASSERT(num_key == dictionary->len, "Number of keys encountered is different than allocated");

  while ('\0' != *utils.buffer)
  {
    if ('\"' == *utils.buffer){
      DEBUG_ASSERT(NULL == utils.key, "utils.key wasn't freed");
      utils.key = _jscon_utils_decode_string(&utils);
      DEBUG_ASSERT(':' == *utils.buffer, "Missing ':' token after key"); //check for key's assign token 

      ++utils.buffer; //consume ':'
      CONSUME_BLANK_CHARS(utils.buffer);

      /* check whether key found is specified */
      struct chunk_s *chunk = dictionary_get(dictionary, utils.key);
      if (NULL != chunk){
        _jscon_scanf_apply(&utils, chunk);
      } else {
        _jscon_scanf_skip(&utils);
      }

      if (NULL != utils.key){
        free(utils.key);
        utils.key = NULL;
      }
    } else {
      ++utils.buffer;
    }
  }

  dictionary_destroy(dictionary);

  va_end(ap);
}
