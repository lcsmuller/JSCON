#include "../CJSON.h"

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

#define FOUND_KEY 1

/* A for bitmask variable, B for bit to be performed action */
#define BITMASK_SET(A,B) ((A) |= (B))
#define BITMASK_CLEAR(A,B) ((A) &= ~(B))
#define BITMASK_TOGGLE(A,B) ((A) ^= (B))

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
    a buffer with the fetched content */
char* get_buffer(char filename[]);

/* parse json arguments and returns a CjsonItem
    variable with the extracted configurations */
Cjson* Cjson_parse(char *buffer);
Cjson* Cjson_parse_reviver(char *buffer, void (*fn)(CjsonItem*));

Cjson* Cjson_create();
void Cjson_destroy(Cjson *cjson);
