#ifndef JSONC_PARSER_H
#define JSONC_PARSER_H

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

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
  JSONC_ALL              = UINT32_MAX,
} jsonc_type_et;

typedef char* jsonc_string_kt;
typedef double jsonc_double_kt;
typedef int64_t jsonc_integer_kt;
typedef bool jsonc_boolean_kt;
struct jsonc_hasht_s; //forward declaration, type is defined at hashtable.h

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
  jsonc_string_kt key;

  struct jsonc_item_s *parent;
  struct jsonc_item_s **branch;
  size_t num_branch;

  jsonc_type_et type;
  union {
    jsonc_string_kt string;
    jsonc_double_kt d_number;
    jsonc_integer_kt i_number;
    jsonc_boolean_kt boolean;
    struct jsonc_hasht_s *hashtable;
  };

  size_t last_accessed_branch;

} jsonc_item_st;

/* parse buffer and returns a jsonc item */
jsonc_item_st* jsonc_parse(char *buffer);

/* same, but with a user created function that can manipulate
  the parsing contents */
//@todo: replace this with a write callback modifier, make a default write callback
jsonc_item_st* jsonc_parse_reviver(char *buffer, void (*fn)(jsonc_item_st*));

/* clean up jsonc item and global allocated keys */
void jsonc_destroy(jsonc_item_st *item);

#endif
