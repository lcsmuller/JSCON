#include "../json_parser.h"

#include <stdlib.h>
#include <stdio.h>

enum trigger_mask {
  Found_Null    = 0,
  Found_Boolean = 1 << 0,
  Found_Number  = 1 << 1,
  Found_String  = 1 << 2,
  Found_Object  = 1 << 3,
  Found_Array   = 1 << 4,
  Found_Key     = 1 << 5,
  Found_Wrapper = 1 << 6,
};

enum json_datatype {
  JSON_Null     = 0,
  JSON_Boolean  = 1 << 0,
  JSON_Number   = 1 << 1,
  JSON_String   = 1 << 2,
  JSON_Object   = 1 << 3,
  JSON_Array    = 1 << 4,
  JSON_Key      = 1 << 5,
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
  struct json_parse *parent; //point to parent if exists
  struct json_parse **properties; //and all of its properties
  size_t n; //amount of enumerable properties
} json_object;

typedef struct json_parse {
  enum json_datatype datatype; //variable/property datatype
  json_data key; //variable/property name
  json_data val; //variable/property contents

  json_object obj; //if of object datatype will have properties

} json_parse;

char *read_json_file(char file[]);

json_parse *parse_json(char *json_file);
void print_json_parse(json_parse *parse, enum json_datatype datatype, FILE *stream);
void destroy_json_parse(json_parse *parse);
