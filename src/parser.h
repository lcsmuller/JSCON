#include "../json_parser.h"

#include <stdlib.h>
#include <stdio.h>

enum trigger_mask {
  Found_Null    = 1 << 0,
  Found_True    = 1 << 1,
  Found_False   = 1 << 2,
  Found_Boolean = Found_True|Found_False,
  Found_Number  = 1 << 3,
  Found_String  = 1 << 4,
  Found_Object  = 1 << 5,
  Found_Array   = 1 << 6,
  Found_Key     = 1 << 7,
  Found_Wrapper = 1 << 8,
};

enum json_datatype {
  JSON_Null     = 1 << 0,
  JSON_True     = 1 << 1,
  JSON_False    = 1 << 2,
  JSON_Boolean  = JSON_True|JSON_False,
  JSON_Number   = 1 << 3,
  JSON_String   = 1 << 4,
  JSON_Object   = 1 << 5,
  JSON_Array    = 1 << 6,
  JSON_Key      = 1 << 7,
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

typedef struct json_data {
  char *start, *end;
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
void print_json_item(json_item *item, unsigned long datatype, FILE *stream);
void destroy_json_item(json_item *item);
