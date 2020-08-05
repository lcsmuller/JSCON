#include "../json_parser.h"

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

/* bits to be assigned to a mask so that an action
    related to that bit can be performed */
typedef enum {
  FoundString   = 1 << 0, //will be checked for key or value
  FoundObject   = 1 << 1,
  FoundArray    = 1 << 2,
  FoundProperty = 1 << 3 | FoundString, 
  FoundKey      = 1 << 4,
  FoundAssign   = 1 << 5,
  FoundWrapper  = 1 << 6,
} bitmask_t;

/* A for bitmask variable, B for bit to be performed action */
#define BITMASK_SET(A,B) ((A) |= (B))
#define BITMASK_CLEAR(A,B) ((A) &= ~(B))
#define BITMASK_TOGGLE(A,B) ((A) ^= (B))
#define BITMASK_EQUALITY(A,B) ((A) == (B)) ? (A) : (0)


/* All of the possible JSON datatypes */
typedef enum {
  JsonNull      = 1 << 0,
  JsonTrue      = 1 << 1,
  JsonFalse     = 1 << 2,
  JsonNumber    = 1 << 3,
  JsonString    = 1 << 4,
  JsonObject    = 1 << 5,
  JsonArray     = 1 << 6,
  JsonAll       = ULONG_MAX,
} CJSON_types_t;


#define OPEN_SQUARE_BRACKET '['
#define CLOSE_SQUARE_BRACKET ']'
#define OPEN_BRACKET '{'
#define CLOSE_BRACKET '}'
#define DOUBLE_QUOTES '\"'
#define COLON ':'
#define COMMA ','


/* hold json data (key/value) as it is 
    when parsed, in its string format */
typedef struct CJSON_data {
  char *start; //points to start of data
  size_t length; //amt of chars contained in this data 
} CJSON_data_t;

/* hold json object/array related configuration
    in the case that it's neither, then it won't
    be used */
typedef struct CJSON_object {
  struct CJSON_item *parent; //point to parent if exists
  struct CJSON_item **properties; //and all of its properties
  size_t n; //amount of enumerable properties
} CJSON_object_t;

/* mainframe struct that holds every configuration
    necessary for when parsing a json argument */
typedef struct CJSON_item {
  CJSON_types_t datatype; //item's json datatype
  CJSON_data_t key; //key in string format
  CJSON_data_t val; //value in string format
  CJSON_object_t obj; //object datatype will have properties
} CJSON_item_t;

/* read appointed file's filesize in long format,
    reads file contents up to filesize and returns
    a buffer with the fetched content */
char*
read_json_file(char filename[]);
/* parse json arguments and returns a CJSON_item_t
    variable with the extracted configurations */
CJSON_item_t*
parse_json(char *buffer);
CJSON_item_t*
parse_json_reviver(char *buffer, void (*fn)(CJSON_item_t*));
/* destroy CJSON_item_t variable, and all of its
    nested objects/arrays */
void
destroy_json(CJSON_item_t *item);
