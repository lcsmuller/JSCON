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

/* main json struct */
typedef struct json_item_s {
  json_string_kt key; //item's json key

  struct json_item_s *parent; //pointer to parent (null if root)
  struct json_item_s **branch; //pointer to properties
  size_t num_branch; //amount of enumerable properties
  size_t last_accessed_branch; //last accessed property from this item

  json_type_et type; //item's json datatype
  union { //literal values
    json_string_kt string;
    json_double_kt d_number;
    json_integer_kt i_number;
    json_boolean_kt boolean;
    struct json_hasht_s *hashtable; //used for sorting through object/array branches
  };

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
