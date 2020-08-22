#include "../JSON.h"

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

/* All of the possible JSON datatypes */
typedef enum {
  Undefined = 0,
  Null      = 1 << 0,
  Boolean   = 1 << 1,
  Number    = 1 << 2,
  String    = 1 << 3,
  Object    = 1 << 4,
  Array     = 1 << 5,
  All       = ULONG_MAX,
} JsonDType;

typedef char JsonString;
typedef double JsonNumber;
typedef short JsonBool;

/* mainframe struct that holds every configuration
    necessary for when parsing a json argument */
typedef struct JsonItem {
  struct JsonItem *parent; //pointer to parent (null if root)
  struct JsonItem **property; //pointer to properties
  size_t n_property; //amount of enumerable properties

  JsonString *key; //key in string format

  JsonDType dtype; //item's json datatype
  union { //literal value
    JsonString* string;
    JsonNumber number;
    JsonBool boolean;
  };
} JsonItem;

/* used for simulating recursive movement, especially useful for
    a tree like structure with nests (aka JSON) */
typedef struct {
  short depth;
  short *ptr;
  short *trace;
} Stack;

typedef struct {
  JsonItem *root; //always points to root json item
  JsonItem *item_ptr; //used for json item movement

  char **keylist; //stores keys found amongst json items
  size_t n_keylist; //amt of keys

  Stack stack; //simulate recursive movement
} Json;

/* parse json arguments and returns a JsonItem
    variable with the extracted configurations */
Json* Json_Parse(char *buffer);
Json* Json_ParseReviver(char *buffer, void (*fn)(JsonItem*));

Json* Json_Create();
void Json_Destroy(Json *json);
