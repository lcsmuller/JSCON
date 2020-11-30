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
  jscon_composite_t *last_accessed_comp; //holds last composite accessed
  jscon_cb *parse_cb; //parser callback
};

/* function pointers used while building json items, 
  jscon_create_value points to functions prefixed by "_jscon_value_set_"
  jscon_create_item points to functions prefixed by "jscon_decode" */
typedef void (jscon_create_value)(jscon_item_t *item, struct jscon_utils_s *utils);
typedef jscon_item_t* (jscon_create_item)(jscon_item_t*, struct jscon_utils_s*, jscon_create_value*);

static jscon_item_t*
_jscon_item_init()
{
  jscon_item_t *new_item = calloc(1, sizeof *new_item);
  DEBUG_ASSERT(NULL != new_item, "Out of memory");

  return new_item;
}

/* create a new branch to current jscon object item, and return
  the new branch address */
static jscon_item_t*
_jscon_branch_init(jscon_item_t *item)
{
  ++item->comp->num_branch;

  item->comp->branch[item->comp->num_branch-1] = _jscon_item_init();

  item->comp->branch[item->comp->num_branch-1]->parent = item;

  return item->comp->branch[item->comp->num_branch-1];
}

static void
_jscon_composite_destroy(jscon_item_t *item)
{
  hashtable_destroy(item->comp->hashtable);

  free(item->comp->branch);
  item->comp->branch = NULL;

  free(item->comp);
  item->comp = NULL;
}

static void
_jscon_destroy_preorder(jscon_item_t *item)
{
  switch (item->type){
  case JSCON_OBJECT:
  case JSCON_ARRAY:
        for (size_t i=0; i < item->comp->num_branch; ++i){
          _jscon_destroy_preorder(item->comp->branch[i]);
        }
        _jscon_composite_destroy(item);
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
jscon_destroy(jscon_item_t *item){
  _jscon_destroy_preorder(jscon_get_root(item));
}

/* fetch string type jscon and return allocated string */
static void
_jscon_value_set_string(jscon_item_t *item, struct jscon_utils_s *utils)
{
  item->type = JSCON_STRING;
  item->string = Jscon_decode_string(&utils->buffer);
}

/* fetch number jscon type by parsing string,
  find out whether its a integer or double and assign*/
static void
_jscon_value_set_number(jscon_item_t *item, struct jscon_utils_s *utils)
{
  double set_double = Jscon_decode_double(&utils->buffer);
  if (DOUBLE_IS_INTEGER(set_double)){
    item->type = JSCON_INTEGER;
    item->i_number = (long long)set_double;
  } else {
    item->type = JSCON_DOUBLE;
    item->d_number = set_double;
  }
}

static void
_jscon_value_set_boolean(jscon_item_t *item, struct jscon_utils_s *utils)
{
  item->type = JSCON_BOOLEAN;
  item->boolean = Jscon_decode_boolean(&utils->buffer);
}

static void
_jscon_value_set_null(jscon_item_t *item, struct jscon_utils_s *utils)
{
  item->type = JSCON_NULL;
  Jscon_decode_null(&utils->buffer);
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
_jscon_value_set_object(jscon_item_t *item, struct jscon_utils_s *utils)
{
  item->type = JSCON_OBJECT;

  item->comp = Jscon_decode_composite(&utils->buffer, _jscon_count_property(utils->buffer));
  Jscon_composite_link_r(item, &utils->last_accessed_comp);
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
_jscon_value_set_array(jscon_item_t *item, struct jscon_utils_s *utils)
{
  item->type = JSCON_ARRAY;

  item->comp = Jscon_decode_composite(&utils->buffer, _jscon_count_element(utils->buffer));
  Jscon_composite_link_r(item, &utils->last_accessed_comp);
}

/* create nested composite type (object/array) and return 
    the address. */
static jscon_item_t*
_jscon_composite_init(jscon_item_t *item, struct jscon_utils_s *utils, jscon_create_value *value_setter)
{
  item = _jscon_branch_init(item);
  item->key = utils->key;
  utils->key = NULL;

  (*value_setter)(item, utils);
  item = (utils->parse_cb)(item);

  return item;
}

/* wrap array or object type jscon, which means
    all of its branches have been created */
static jscon_item_t*
_jscon_wrap_composite(jscon_item_t *item, struct jscon_utils_s *utils)
{
  ++utils->buffer; //skips '}' or ']'
  Jscon_composite_build(item);
  return item->parent;
}

/* create a primitive data type branch. */
static jscon_item_t*
_jscon_append_primitive(jscon_item_t *item, struct jscon_utils_s *utils, jscon_create_value *value_setter)
{
  item = _jscon_branch_init(item);
  item->key = utils->key;
  utils->key = NULL;

  (*value_setter)(item, utils);
  item = (utils->parse_cb)(item);

  return item->parent;
}

/* this routine is called when setting a branch of a composite type
    (object and array) item. */
static jscon_item_t*
_jscon_branch_build(jscon_item_t *item, struct jscon_utils_s *utils)
{
  jscon_create_item *item_setter;
  jscon_create_value *value_setter;

  switch (*utils->buffer){
  case '{':/*OBJECT DETECTED*/
        item_setter = &_jscon_composite_init;
        value_setter = &_jscon_value_set_object;
        break;
  case '[':/*ARRAY DETECTED*/
        item_setter = &_jscon_composite_init;
        value_setter = &_jscon_value_set_array;
        break;
  case '\"':/*STRING DETECTED*/
        item_setter = &_jscon_append_primitive;
        value_setter = &_jscon_value_set_string;
        break;
  case 't':/*CHECK FOR*/
  case 'f':/* BOOLEAN */
        if (!STRNEQ(utils->buffer,"true",4) && !STRNEQ(utils->buffer,"false",5)){
          goto token_error;
        }
        item_setter = &_jscon_append_primitive;
        value_setter = &_jscon_value_set_boolean;
        break;
  case 'n':/*CHECK FOR NULL*/
        if (!STRNEQ(utils->buffer,"null",4)){
          goto token_error; 
        }
        item_setter = &_jscon_append_primitive;
        value_setter = &_jscon_value_set_null;
        break;
  default:
        /*CHECK FOR NUMBER*/
        if (!isdigit(*utils->buffer) && ('-' != *utils->buffer)){
          goto token_error;
        }
        item_setter = &_jscon_append_primitive;
        value_setter = &_jscon_value_set_number;
        break;
  }

  return (*item_setter)(item, utils, value_setter);


token_error:
  DEBUG_ERR("Invalid '%c' token", *utils->buffer);
  abort();
}

/* this will be active if the current item is of array type jscon,
  whatever item is created here will be this array's property.
  if a ']' token is found then the array is wrapped up */
static jscon_item_t*
_jscon_array_build(jscon_item_t *item, struct jscon_utils_s *utils)
{
  CONSUME_BLANK_CHARS(utils->buffer);
  switch (*utils->buffer){
  case ']':/*ARRAY WRAPPER DETECTED*/
        return _jscon_wrap_composite(item, utils);
  case ',': /*NEXT ELEMENT TOKEN*/
        ++utils->buffer; //skips ','
        CONSUME_BLANK_CHARS(utils->buffer);
  /* fall through */
  default:
   {
        /* creates numerical key for the array element */
        char numerical_key[MAX_DIGITS];
        snprintf(numerical_key, MAX_DIGITS-1, "%ld", item->comp->num_branch);

        DEBUG_ASSERT(NULL == utils->key, "utils->key wasn't freed");
        utils->key = strdup(numerical_key);
        DEBUG_ASSERT(NULL != utils->key, "Out of memory");

        return _jscon_branch_build(item, utils);
   }
  }

  //token error checking done inside _jscon_branch_build
}

/* this will be active if the current item is of object type jscon,
  whatever item is created here will be this object's property.
  if a '}' token is found then the object is wrapped up */
static jscon_item_t*
_jscon_object_build(jscon_item_t *item, struct jscon_utils_s *utils)
{
  CONSUME_BLANK_CHARS(utils->buffer);
  switch (*utils->buffer){
  case '}':/*OBJECT WRAPPER DETECTED*/
        return _jscon_wrap_composite(item, utils);
  case ',': /*NEXT PROPERTY TOKEN*/
        ++utils->buffer; //skips ','
        CONSUME_BLANK_CHARS(utils->buffer);
  /* fall through */
  case '\"':/*KEY STRING DETECTED*/
        DEBUG_ASSERT(NULL == utils->key, "utils->key wasn't freed");
        utils->key = Jscon_decode_string(&utils->buffer);
        DEBUG_ASSERT(':' == *utils->buffer, "Missing ':' token after key"); //check for key's assign token 
        ++utils->buffer; //skips ':'
        CONSUME_BLANK_CHARS(utils->buffer);
        return _jscon_branch_build(item, utils);
  default:
        if (!IS_BLANK_CHAR(*utils->buffer)){
          DEBUG_ERR("Invalid '%c' token", *utils->buffer);
        }
        CONSUME_BLANK_CHARS(utils->buffer);
        return item;
  }
}

/* this call will only be used once, at the first iteration,
  it also allows the creation of a jscon that's not part of an
  array or object. ex: jscon_item_parse("10") */
static jscon_item_t*
_jscon_entity_build(jscon_item_t *item, struct jscon_utils_s *utils)
{
  switch (*utils->buffer){
  case '{':/*OBJECT DETECTED*/
        _jscon_value_set_object(item, utils);
        break;
  case '[':/*ARRAY DETECTED*/
        _jscon_value_set_array(item, utils);
        break;
  case '\"':/*STRING DETECTED*/
      _jscon_value_set_string(item, utils);
      break;
  case 't':/*CHECK FOR*/
  case 'f':/* BOOLEAN */
        if (!STRNEQ(utils->buffer,"true",4) && !STRNEQ(utils->buffer,"false",5)){
          goto token_error;
        }
        _jscon_value_set_boolean(item, utils);
        break;
  case 'n':/*CHECK FOR NULL*/
        if (!STRNEQ(utils->buffer,"null",4)){
          goto token_error;
        }
        _jscon_value_set_null(item, utils);
        break;
  default:/*CHECK FOR NUMBER*/
        CONSUME_BLANK_CHARS(utils->buffer);
        if (!isdigit(*utils->buffer) && ('-' != *utils->buffer)){
          goto token_error;
        }
        _jscon_value_set_number(item, utils);
        break;
  }

  return item;


token_error:
  DEBUG_ERR("Invalid '%c' token", *utils->buffer);
  abort(); 
}

static inline jscon_item_t*
_jscon_default_callback(jscon_item_t *item){
  return item;
}

jscon_cb*
jscon_parse_cb(jscon_cb *new_cb)
{
  jscon_cb *parse_cb = &_jscon_default_callback;

  if (NULL != new_cb){
    parse_cb = new_cb;
  }

  return parse_cb;
}

/* parse contents from buffer into a jscon item object
  and return its root */
jscon_item_t*
jscon_parse(char *buffer)
{
  jscon_item_t *root = calloc(1, sizeof *root);
  if (NULL == root) return NULL;

  struct jscon_utils_s utils = {
    .buffer = buffer,
    .parse_cb = jscon_parse_cb(NULL),
  };
  
  //build while item and buffer aren't nulled
  jscon_item_t *item = root;
  while ((NULL != item) && ('\0' != *utils.buffer)){
    switch(item->type){
    case JSCON_OBJECT:
          item = _jscon_object_build(item, &utils);
          break;
    case JSCON_ARRAY:
          item = _jscon_array_build(item, &utils);
          break;
    case JSCON_UNDEFINED:
          /* this should be true only at the first iteration */
          item = _jscon_entity_build(item, &utils);
          if (IS_PRIMITIVE(item)) return item;

          break;
    default:
          DEBUG_ERR("Unknown item->type found\n\t"
                    "Code: %d", item->type);
    }
  }

  return root;
}
