#ifndef JSONC_PARSER_H
#define JSONC_PARSER_H

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

/* All of the possible json datatypes */
typedef enum {
  JSON_UNDEFINED = 0,
  JSON_NULL      = 1 << 0,
  JSON_BOOLEAN   = 1 << 1,
  JSON_NUMBER    = 1 << 2,
  JSON_STRING    = 1 << 3,
  JSON_OBJECT    = 1 << 4,
  JSON_ARRAY     = 1 << 5,
  JSON_ALL       = ULONG_MAX,
} json_type_et;

typedef char* json_string_kt;
typedef double json_number_kt;
typedef short json_boolean_kt;

struct json_hasht_s; //forward declaration

/* mainframe struct that holds every configuration
    necessary for when parsing a json argument */
typedef struct json_item_s {
  json_string_kt key; //item's json key
  json_type_et type; //item's json datatype
  union { //literal value
    json_string_kt string;
    json_number_kt number;
    json_boolean_kt boolean;
    struct json_hasht_s *hashtable;
  };

  struct json_item_s *parent; //pointer to parent (null if root)
  struct json_item_s **branch; //pointer to properties
  size_t num_branch; //amount of enumerable properties
  int last_accessed_branch; //last accessed property from this item
} json_item_st;

/* parse buffer and returns a json item */
json_item_st* json_item_parse(char *buffer);
/* same, but with a user created function that can manipulate
  the parsing contents */
json_item_st* json_item_parse_reviver(char *buffer, void (*fn)(json_item_st*));
/* clean up json item and global allocated keys */
void json_item_destroy(json_item_st *item);

#endif
