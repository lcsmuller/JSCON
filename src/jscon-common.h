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

#ifndef JSCON_COMMON_H_
#define JSCON_COMMON_H_

//#include "libjscon.h" << implicit
#include "hashtable.h"
#include "strscpy.h"

#define JSCON_VERSION "0.0"

#define MAX_DIGITS 17

#define STRLT(s,t) (strcmp(s,t) < 0)
#define STREQ(s,t) (0 == strcmp(s,t))
#define STRNEQ(s,t,n) (0 == strncmp(s,t,n))

#define IN_RANGE(n,lo,hi) (((n) > (lo)) && ((n) < (hi)))

#define DOUBLE_IS_INTEGER(d) \
  ((d) <= LLONG_MIN || (d) >= LLONG_MAX || (d) == (long long)(d))

//@todo add escaped characters
#define ALLOWED_JSON_CHAR(c) (isspace(c) || isalnum(c) || '_' == (c) || '-' == (c))
#define CONSUME_BLANK_CHARS(str) for( ; (isspace(*str) || iscntrl(*str)) ; ++str)

#define IS_COMPOSITE(i) ((i) && jscon_typecmp(i, JSCON_OBJECT|JSCON_ARRAY))
#define IS_EMPTY_COMPOSITE(i) (IS_COMPOSITE(i) && 0 == jscon_size(i))
#define IS_PRIMITIVE(i) ((i) && !jscon_typecmp(i, JSCON_OBJECT|JSCON_ARRAY))
#define IS_PROPERTY(i) (jscon_typecmp(i->parent, JSCON_OBJECT))
#define IS_ELEMENT(i) (jscon_typecmp(i->parent, JSCON_ARRAY))
#define IS_LEAF(i) (IS_PRIMITIVE(i) || IS_EMPTY_COMPOSITE(i))
#define IS_ROOT(i) (NULL == i->parent)

typedef struct jscon_htwrap_s {
  struct hashtable_s *hashtable;

  struct jscon_item_s *root; //points to root item (object or array)
  struct jscon_htwrap_s *next; //points to next composite item's htwrap
  struct jscon_htwrap_s *prev; //points to prev composite item's htwrap
} jscon_htwrap_st;

jscon_htwrap_st* Jscon_htwrap_init();
void Jscon_htwrap_destroy(jscon_htwrap_st *htwrap);
void Jscon_htwrap_link_r(struct jscon_item_s *item, jscon_htwrap_st **last_accessed_htwrap);
void Jscon_htwrap_build(struct jscon_item_s *item);
struct jscon_item_s* Jscon_htwrap_get(const char *key, struct jscon_item_s *item);
struct jscon_item_s* Jscon_htwrap_set(const char *key, struct jscon_item_s *item);

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

  struct jscon_htwrap_s *htwrap;
} jscon_composite_st;

/*
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

  enum jscon_type type;
  union {
    char *string;
    double d_number;
    long long i_number;
    bool boolean;
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

#endif
