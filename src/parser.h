#include "../CJSON.h"

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
  JsonBoolean   = 1 << 1,
  JsonNumber    = 1 << 2,
  JsonString    = 1 << 3,
  JsonObject    = 1 << 4,
  JsonArray     = 1 << 5,
  JsonAll       = ULONG_MAX,
} CJSON_types_t;


#define OPEN_SQUARE_BRACKET '['
#define CLOSE_SQUARE_BRACKET ']'
#define OPEN_BRACKET '{'
#define CLOSE_BRACKET '}'
#define DOUBLE_QUOTES '\"'
#define COLON ':'
#define COMMA ','

typedef char CJSON_data;

typedef struct CJSON_value {
  union {
    short boolean;
    double number;
    CJSON_data* string;
  };
} CJSON_value_t;

/* mainframe struct that holds every configuration
    necessary for when parsing a json argument */
typedef struct CJSON_item {
  CJSON_types_t dtype; //item's json datatype

  struct CJSON_item *parent; //point to parent if exists
  struct CJSON_item **properties; //and all of its properties
  size_t n; //amount of enumerable properties

  CJSON_data *key; //key in string format
  CJSON_value_t value; //literal value
} CJSON_item_t;

typedef struct CJSON {
  CJSON_item_t *item;

  long memsize;
  char **keylist;
  long n; //key amt
} CJSON_t;

/* read appointed file's filesize in long format,
    reads file contents up to filesize and returns
    a buffer with the fetched content */
char*
read_json_file(char filename[]);
/* parse json arguments and returns a CJSON_item_t
    variable with the extracted configurations */
CJSON_t*
parse_json(char *buffer);
CJSON_t*
parse_json_reviver(char *buffer, void (*fn)(CJSON_item_t*));
/* destroy CJSON_item_t variable, and all of its
    nested objects/arrays */
void
destroy_json(CJSON_t *cjson);
