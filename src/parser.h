#ifndef JSONC_PARSER_H
#define JSONC_PARSER_H

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

/* All of the possible json datatypes */
typedef enum {
  JSON_UNDEFINED        = 0,
  JSON_NULL             = 1 << 0,
  JSON_BOOLEAN          = 1 << 1,
  JSON_NUMBER_INTEGER   = 1 << 2,
  JSON_NUMBER_DOUBLE    = 1 << 3,
  JSON_NUMBER           = JSON_NUMBER_INTEGER | JSON_NUMBER_DOUBLE,
  JSON_STRING           = 1 << 4,
  JSON_OBJECT           = 1 << 5,
  JSON_ARRAY            = 1 << 6,
  JSON_ALL              = UINT32_MAX,
} json_type_et;

typedef char* json_string_kt;
typedef double json_double_kt;
typedef int64_t json_integer_kt;
typedef bool json_boolean_kt;
struct json_hasht_s; //forward declaration, type is defined at hashtable.h

/* members should not be accessed directly, they are only
    mean't to be used internally by the lib, or accessed through
    public.h functions, access directly at your own risk. 
    
    key: item's json key (NULL if root)

    parent: object or array that its part of (NULL if root)

    num_branch: amount of enumerable properties (0 if not object or array type)

    type: item's json datatype (check enum json_type_e for flags)

    union {string, d_number, i_number, boolean, hashtable}:
      string,d_number,i_number,boolean: item literal value, denoted by
        its type. 
      hashtable: if item type is object or array, it will have a
        hashtable for easily sorting through its branches by keys

    last_accessed_branch: simulate stack trace by storing last accessed
      branch value, this is used for movement functions that require state 
      to be preserved between calls, while also adhering to recursivity
      rules. (check public.c json_next() for example)
*/
typedef struct json_item_s {
  json_string_kt key;

  struct json_item_s *parent;
  struct json_item_s **branch;
  size_t num_branch;

  json_type_et type;
  union {
    json_string_kt string;
    json_double_kt d_number;
    json_integer_kt i_number;
    json_boolean_kt boolean;
    struct json_hasht_s *hashtable;
  };

  size_t last_accessed_branch;

} json_item_st;

/* parse buffer and returns a json item */
json_item_st* json_parse(char *buffer);

/* same, but with a user created function that can manipulate
  the parsing contents */
//@todo: replace this with a write callback modifier, make a default write callback
json_item_st* json_parse_reviver(char *buffer, void (*fn)(json_item_st*));

/* clean up json item and global allocated keys */
void json_destroy(json_item_st *item);

#endif
