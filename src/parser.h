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
} CjsonDetectDType;

/* A for bitmask variable, B for bit to be performed action */
#define BITMASK_SET(A,B) ((A) |= (B))
#define BITMASK_CLEAR(A,B) ((A) &= ~(B))
#define BITMASK_TOGGLE(A,B) ((A) ^= (B))
#define BITMASK_EQUALITY(A,B) ((A) == (B))?(A):(0)


/* All of the possible JSON datatypes */
typedef enum {
  Null      = 1 << 0,
  Boolean   = 1 << 1,
  Number    = 1 << 2,
  String    = 1 << 3,
  Object    = 1 << 4,
  Array     = 1 << 5,
  All       = ULONG_MAX,
} CjsonDType;

typedef char CjsonString;
typedef double CjsonNumber;
typedef short CjsonBool;

typedef struct {
  union {
    CjsonString* string;
    CjsonNumber number;
    CjsonBool boolean;
  };
} CjsonValue;

/* mainframe struct that holds every configuration
    necessary for when parsing a json argument */
typedef struct CjsonItem {
  CjsonDType dtype; //item's json datatype

  struct CjsonItem *parent; //point to parent if exists
  struct CjsonItem **property; //and all of its properties
  size_t n; //amount of enumerable properties

  CjsonString *key; //key in string format
  CjsonValue value; //literal value
} CjsonItem;

typedef struct {
  CjsonItem *item;

  char **keylist;
  size_t list_size;
} Cjson;

/* read appointed file's filesize in long format,
    reads file contents up to filesize and returns
    a json_text with the fetched content */
char* get_json_text(char filename[]);

/* parse json arguments and returns a CjsonItem
    variable with the extracted configurations */
Cjson* Cjson_parse(char *json_text);
Cjson* Cjson_parse_reviver(char *json_text, void (*fn)(CjsonItem*));

Cjson* Cjson_create();
void Cjson_destroy(Cjson *cjson);
