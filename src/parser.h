#include "../json_parser.h"

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

typedef unsigned long ulong;

enum trigger_mask {
  FoundNull    = 1 << 0,
  FoundTrue    = 1 << 1,
  FoundFalse   = 1 << 2,
  FoundBoolean = FoundTrue|FoundFalse,
  FoundNumber  = 1 << 3,
  FoundString  = 1 << 4,
  FoundObject  = 1 << 5,
  FoundArray   = 1 << 6,
  FoundKey     = 1 << 7,
  FoundWrapper = 1 << 8,
};

enum json_datatype {
  JsonNull     = 1 << 0,
  JsonTrue     = 1 << 1,
  JsonFalse    = 1 << 2,
  JsonNumber   = 1 << 3,
  JsonString   = 1 << 4,
  JsonObject   = 1 << 5,
  JsonArray    = 1 << 6,
  JsonKey      = 1 << 7,
  JsonAll      = ULONG_MAX,
};

#define OPEN_SQUARE_BRACKET '['
#define OPEN_BRACKET '{'
#define CLOSE_SQUARE_BRACKET ']'
#define CLOSE_BRACKET '}'

#define DOUBLE_QUOTES '\"'
#define COLON ':'
#define COMMA ','


#define BITMASK_SET(A,B) ((A) |= (B))
#define BITMASK_CLEAR(A,B) ((A) &= ~(B))
#define BITMASK_TOGGLE(A,B) ((A) ^= (B))
#define BITMASK_EQUALITY(A,B) ((A) == (B)) ? (1) : (0)

typedef struct json_data {
  char *start;
  size_t length; //amt of chars between left&right quotes
} json_data;

typedef struct json_object {
  struct json_item *parent; //point to parent if exists
  struct json_item **properties; //and all of its properties
  size_t n; //amount of enumerable properties
} json_object;

typedef struct json_item {
  enum json_datatype datatype; //variable/property datatype
  json_data key; //variable/property name
  json_data val; //variable/property contents

  json_object obj; //if of object datatype will have properties

} json_item;

char *read_json_file(char file[]);

json_item *parse_json(char *json_file);
void print_json_item(json_item *item, ulong datatype, FILE *stream);
void destroy_json_item(json_item *item);
