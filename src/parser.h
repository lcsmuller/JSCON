#include "../JSON.h"

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

/* All of the possible JSON datatypes */
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

/* mainframe struct that holds every configuration
    necessary for when parsing a json argument */
typedef struct json_item_s {
  struct json_item_s *parent; //pointer to parent (null if root)
  struct json_item_s **branch; //pointer to properties
  size_t num_branch; //amount of enumerable properties
  int last_accessed_branch; //last accessed property from this item

  json_string_kt *ptr_key; //pointer to string of key

  json_type_et type; //item's json datatype
  union { //literal value
    json_string_kt string;
    json_number_kt number;
    json_boolean_kt boolean;
  };
} json_item_st;

typedef struct {
  json_item_st *root; //points to root json item
  json_string_kt **list_ptr_key; //stores pointer to keys created
  size_t num_ptr_key; //amt of pointer keys stored
} json_st;

/* parse json arguments and returns a Json object
    with the extracted information */
json_st* json_parse(char *buffer);
json_st* json_parse_reviver(char *buffer, void (*fn)(json_item_st*));

json_st* json_create();
void json_destroy(json_st *json);
