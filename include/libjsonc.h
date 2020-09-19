#ifndef JSONC_PUBLIC_H
#define JSONC_PUBLIC_H

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

#include "macros.h"
#include "hashtable.h"

/* All of the possible jsonc datatypes */
typedef enum {
  /* DATATYPE FLAGS */
  JSONC_UNDEFINED        = 0,
  JSONC_NULL             = 1 << 0,
  JSONC_BOOLEAN          = 1 << 1,
  JSONC_NUMBER_INTEGER   = 1 << 2,
  JSONC_NUMBER_DOUBLE    = 1 << 3,
  JSONC_STRING           = 1 << 4,
  JSONC_OBJECT           = 1 << 5,
  JSONC_ARRAY            = 1 << 6,
  /* SUPERSET FLAGS */
  JSONC_NUMBER           = JSONC_NUMBER_INTEGER | JSONC_NUMBER_DOUBLE,
  JSONC_ALL              = USHRT_MAX,
} jsonc_type_et;

typedef char jsonc_char_kt;
typedef double jsonc_double_kt;
typedef long long jsonc_integer_kt;
typedef _Bool jsonc_boolean_kt;

/* members should not be accessed directly, they are only
    mean't to be used internally by the lib, or accessed through
    public.h functions, access directly at your own risk. 
    
    key: item's jsonc key (NULL if root)

    parent: object or array that its part of (NULL if root)

    num_branch: amount of enumerable properties (0 if not object or array type)

    type: item's jsonc datatype (check enum jsonc_type_e for flags)

    union {string, d_number, i_number, boolean, hashtable}:
      string,d_number,i_number,boolean: item literal value, denoted by
        its type. 
      hashtable: if item type is object or array, it will have a
        hashtable for easily sorting through its branches by keys

    last_accessed_branch: simulate stack trace by storing last accessed
      branch value, this is used for movement functions that require state 
      to be preserved between calls, while also adhering to recursivity
      rules. (check public.c jsonc_next() for example)
*/
typedef struct jsonc_item_s {
  char *key;

  struct jsonc_item_s *parent;
  struct jsonc_item_s **branch;
  size_t num_branch;

  jsonc_type_et type;
  union {
    jsonc_char_kt *string;
    jsonc_double_kt d_number;
    jsonc_integer_kt i_number;
    jsonc_boolean_kt boolean;
    struct jsonc_hasht_s *hashtable;
  };

  size_t last_accessed_branch;

} jsonc_item_st;

//used for setting callbacks
typedef jsonc_item_st* (jsonc_callbacks_ft)(jsonc_item_st*);

/* JSON PARSER */
/* parse buffer and returns a jsonc item */
jsonc_item_st* jsonc_parse(char *buffer);
jsonc_callbacks_ft* jsonc_parser_callback(jsonc_callbacks_ft *new_cb);
/* clean up jsonc item and global allocated keys */
void jsonc_destroy(jsonc_item_st *item);

/* JSON STRINGIFY */
char* jsonc_stringify(jsonc_item_st *root, jsonc_type_et type);

/* JSONC UTILITIES */
jsonc_item_st* jsonc_next_object_r(jsonc_item_st *item, jsonc_item_st **p_current_item);
jsonc_item_st* jsonc_next(jsonc_item_st* item);
jsonc_item_st* jsonc_clone(jsonc_item_st *item);
jsonc_char_kt* jsonc_typeof(const jsonc_item_st* kItem);
jsonc_char_kt* jsonc_strdup(const jsonc_item_st* kItem);
jsonc_char_kt* jsonc_strncpy(char *dest, const jsonc_item_st* kItem, size_t n);
int jsonc_typecmp(const jsonc_item_st* kItem, const jsonc_type_et kType);
int jsonc_keycmp(const jsonc_item_st* kItem, const char *kKey);
int jsonc_doublecmp(const jsonc_item_st* kItem, const jsonc_double_kt kDouble);
int jsonc_intcmp(const jsonc_item_st* kItem, const jsonc_integer_kt kInteger);
void jsonc_double_tostr(const jsonc_double_kt kDouble, jsonc_char_kt* p_str, const int kDigits);

/* JSONC GETTERS */
jsonc_item_st* jsonc_get_root(jsonc_item_st* item);
jsonc_item_st* jsonc_get_branch(jsonc_item_st *item, const char *kKey);
jsonc_item_st* jsonc_get_sibling(const jsonc_item_st* kOrigin, const size_t kRelative_index);
jsonc_item_st* jsonc_get_parent(const jsonc_item_st* kItem);
jsonc_item_st* jsonc_get_byindex(const jsonc_item_st* kItem, const size_t kIndex);
size_t jsonc_get_num_branch(const jsonc_item_st* kItem);
jsonc_type_et jsonc_get_type(const jsonc_item_st* kItem);
jsonc_char_kt* jsonc_get_key(const jsonc_item_st* kItem);
jsonc_boolean_kt jsonc_get_boolean(const jsonc_item_st* kItem);
jsonc_char_kt* jsonc_get_string(const jsonc_item_st* kItem);
jsonc_double_kt jsonc_get_double(const jsonc_item_st* kItem);
jsonc_integer_kt jsonc_get_integer(const jsonc_item_st* kItem);

#endif
