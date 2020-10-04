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

#ifndef JSCON_PUBLIC_H
#define JSCON_PUBLIC_H

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

#include "macros.h"
#include "hashtable.h"

/* All of the possible jscon datatypes */
typedef enum {
  /* DATATYPE FLAGS */
  JSCON_UNDEFINED        = 0,
  JSCON_NULL             = 1 << 0,
  JSCON_BOOLEAN          = 1 << 1,
  JSCON_INTEGER          = 1 << 2,
  JSCON_DOUBLE           = 1 << 3,
  JSCON_STRING           = 1 << 4,
  JSCON_OBJECT           = 1 << 5,
  JSCON_ARRAY            = 1 << 6,
  /* SUPERSET FLAGS */
  JSCON_NUMBER           = JSCON_INTEGER | JSCON_DOUBLE,
  JSCON_ANY              = USHRT_MAX,
} jscon_type_et;

/* JSCON PRIMITIVE TYPES */
typedef char jscon_char_kt;
typedef double jscon_double_kt;
typedef long long jscon_integer_kt;
typedef _Bool jscon_boolean_kt;

/* JSCON COMPOSITE TYPES
  if jscon_item type is of composite type (object or array) it will
  include a jscon_composite_st struct with the following attributes:
      branch: for sorting through object's properties/array elements

      num_branch: amount of enumerable properties/elements contained

      last_accessed_branch: simulate stack trace by storing the last accessed
        branch address. this is used for movement functions that require state
        to be preserved between calls, while also adhering to tree traversal
        rules. (check public.c jscon_iter_next() for example)

      htwrap: hashtable special wrapper for easily sorting through
        branches by keys, and skipping primitives (check hashtable.h and
        hashtable.c for more info, and public.c jscon_iter_composite_r() to see
        it in action)*/

typedef struct {
  struct jscon_item_s **branch;
  size_t num_branch;

  size_t last_accessed_branch;

  struct jscon_htwrap_s htwrap;
} jscon_composite_st;

/* these attributes should not be accessed directly, they are only
    meant to be used internally by the lib, or accessed through
    public.h functions.
    
    key: item's jscon key (NULL if root)

    parent: object or array that its part of (NULL if root)

    type: item's jscon datatype (check enum jscon_type_e for flags)

    union {string, d_number, i_number, boolean, comp}:
      string,d_number,i_number,boolean: item literal value, denoted by
        its type. 
      comp: if item type is object or array, it will contain a
        jscon_composite_st struct datatype. */
typedef struct jscon_item_s {
  char *key;

  struct jscon_item_s *parent;

  jscon_type_et type;
  union {
    jscon_char_kt *string;
    jscon_double_kt d_number;
    jscon_integer_kt i_number;
    jscon_boolean_kt boolean;
    jscon_composite_st *comp;
  };
} jscon_item_st;

/* linked list used for linking items to be assigned to a
    object or array via jscon_object() or jscon_array() */
struct jscon_node_s {
  jscon_item_st *item;
  struct jscon_node_s *next;
  struct jscon_node_s *prev;
};

typedef struct jscon_list_s {
  struct jscon_node_s *first;
  struct jscon_node_s *last;
  size_t num_node;
} jscon_list_st;

//used for setting callbacks
typedef jscon_item_st* (jscon_callbacks_ft)(jscon_item_st*);

/* JSCON INIT */
jscon_item_st *jscon_null(const char *kKey);
jscon_item_st *jscon_boolean(jscon_boolean_kt boolean, const char *kKey);
jscon_item_st *jscon_integer(jscon_integer_kt i_number, const char *kKey);
jscon_item_st *jscon_double(jscon_double_kt d_number, const char *kKey);
jscon_item_st *jscon_number(jscon_double_kt d_number, const char *kKey);
jscon_item_st *jscon_string(jscon_char_kt *string, const char *kKey);

jscon_list_st *jscon_list_init();
jscon_item_st *jscon_object(jscon_list_st *list, const char *kKey);
jscon_item_st *jscon_array(jscon_list_st *list, const char *kKey);

/* JSCON DESTRUCTORS */
/* clean up jscon item and global allocated keys */
void jscon_destroy(jscon_item_st *item);
void jscon_list_destroy(jscon_list_st *list);

/* JSCON DECODING */
/* parse buffer and returns a jscon item */
jscon_item_st* jscon_parse(char *buffer);
jscon_callbacks_ft* jscon_parser_callback(jscon_callbacks_ft *new_cb);
/* only parse json values from given parameters */
void jscon_scanf(char *buffer, char *format, ...);
 
/* JSCON ENCODING */
char* jscon_stringify(jscon_item_st *root, jscon_type_et type);

/* JSCON LIST MANIPULATION */
void jscon_list_append(jscon_list_st *list, jscon_item_st *item);

/* JSCON UTILITIES */
size_t jscon_size(const jscon_item_st* kItem);
jscon_item_st* jscon_dettach(jscon_item_st *item);
jscon_item_st* jscon_iter_composite_r(jscon_item_st *item, jscon_item_st **p_current_item);
jscon_item_st* jscon_iter_next(jscon_item_st* item);
jscon_item_st* jscon_clone(jscon_item_st *item);
jscon_char_kt* jscon_typeof(const jscon_item_st* kItem);
jscon_char_kt* jscon_strdup(const jscon_item_st* kItem);
jscon_char_kt* jscon_strcpy(char *dest, const jscon_item_st* kItem);
int jscon_typecmp(const jscon_item_st* kItem, const jscon_type_et kType);
int jscon_keycmp(const jscon_item_st* kItem, const char *kKey);
int jscon_doublecmp(const jscon_item_st* kItem, const jscon_double_kt kDouble);
int jscon_intcmp(const jscon_item_st* kItem, const jscon_integer_kt kInteger);

/* JSCON GETTERS */
size_t jscon_get_depth(jscon_item_st *item);
jscon_item_st* jscon_get_root(jscon_item_st* item);
jscon_item_st* jscon_get_branch(jscon_item_st *item, const char *kKey);
jscon_item_st* jscon_get_sibling(const jscon_item_st* kOrigin, const size_t kRelative_index);
jscon_item_st* jscon_get_parent(const jscon_item_st* kItem);
jscon_item_st* jscon_get_byindex(const jscon_item_st* kItem, const size_t kIndex);
size_t jscon_get_index(const jscon_item_st* kItem, const char *kKey);
jscon_type_et jscon_get_type(const jscon_item_st* kItem);
jscon_char_kt* jscon_get_key(const jscon_item_st* kItem);
jscon_boolean_kt jscon_get_boolean(const jscon_item_st* kItem);
jscon_char_kt* jscon_get_string(const jscon_item_st* kItem);
jscon_double_kt jscon_get_double(const jscon_item_st* kItem);
jscon_integer_kt jscon_get_integer(const jscon_item_st* kItem);

#endif
