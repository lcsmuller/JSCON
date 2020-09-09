#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "public.h"
#include "parser.h"
#include "macros.h"


struct utils_s {
  char *buffer_base; //buffer's base (first position)
  size_t buffer_offset; //length of buffer
  /*a setter method, will be either utils_buffer_method_count or
     utils_buffer_method_update*/
  void (*method)(char get_char, struct utils_s* utils);
};

/* calculates buffer length */ 
static void
utils_buffer_method_count(char get_char, struct utils_s *utils){
  ++utils->buffer_offset;
}

/* fills pre-allocated buffer (by utils_buffer_method_count's length) 
  with json items converted to string */ 
static void
utils_buffer_method_update(char get_char, struct utils_s *utils)
{
  utils->buffer_base[utils->buffer_offset] = get_char;
  ++utils->buffer_offset;
}

/* fills buffer at utils->buffer_offset position with provided string */
static void
utils_buffer_fill_string(json_string_kt string, struct utils_s *utils)
{
  while ('\0' != *string){
    (*utils->method)(*string,utils);
    ++string;
  }
}

/* converts double to string and concatenate it to buffer */
static void
utils_buffer_fill_double(json_double_kt d_number, struct utils_s *utils)
{
  char get_strnum[MAX_DIGITS];
  json_double_tostr(d_number, get_strnum, MAX_DIGITS);

  utils_buffer_fill_string(get_strnum,utils); //store value in utils
}

/* converts int to string and concatenate it to buffer */
static void
utils_buffer_fill_integer(json_integer_kt i_number, struct utils_s *utils)
{
  char get_strnum[MAX_DIGITS];
  snprintf(get_strnum, MAX_DIGITS-1, "%ld", i_number);

  utils_buffer_fill_string(get_strnum,utils); //store value in utils
}

/* stringify json items by recursively going through its branches

   I could try to make this iterative by using json_next(), but there
    wouldn't be much to gain from it, since i still would have to 
    manage my own stack, which would lead to a more confusing and
    hardly more efficient code */
static void
json_recursive_print(json_item_st *item, json_type_et type, struct utils_s *utils)
{
  /* stringify json item only if its of the same given type */
  if (!json_typecmp(item, type|JSON_OBJECT|JSON_ARRAY))
    return;

  /* prints only object type json members key, array could be printed
    aswell, but that wouldn't conform to standard*/
  if ((NULL != item->key) && json_typecmp(item->parent, JSON_OBJECT)){
    (*utils->method)('\"', utils);
    utils_buffer_fill_string(item->key, utils);
    (*utils->method)('\"', utils);
    (*utils->method)(':', utils);
  }
  
  /* converts item value to its string format */
  switch (item->type){
  case JSON_NULL:
      utils_buffer_fill_string("null", utils);
      break;
  case JSON_BOOLEAN:
      if (true == item->boolean){
        utils_buffer_fill_string("true", utils);
        break;
      }
      utils_buffer_fill_string("false", utils);
      break;
  case JSON_NUMBER_DOUBLE:
      utils_buffer_fill_double(item->d_number, utils);
      break;
  case JSON_NUMBER_INTEGER:
      utils_buffer_fill_integer(item->i_number, utils);
      break;
  case JSON_STRING:
      (*utils->method)('\"', utils);
      utils_buffer_fill_string(item->string, utils);
      (*utils->method)('\"', utils);
      break;
  case JSON_OBJECT:
      (*utils->method)('{', utils);
      break;
  case JSON_ARRAY:
      (*utils->method)('[', utils);
      break;
  default:
      fprintf(stderr,"ERROR: can't stringify undefined datatype\n");
      exit(EXIT_FAILURE);
  }

  /* empty num_branch means item is not of object or array type */
  if (0 == item->num_branch) return;

  /* prints (recursively) first branch that fits the type criteria */
  size_t first_index=0;
  while (first_index < item->num_branch){
    if (json_typecmp(item->branch[first_index], type|JSON_OBJECT|JSON_ARRAY)){
      json_recursive_print(item->branch[first_index], type, utils);
      break;
    }
    ++first_index;
  }

  /* prints (recursively) every consecutive branch that fits the type
      criteria, and adds a comma before it */
  for (size_t j = first_index+1; j < item->num_branch; ++j){
    /* skips branch that don't fit the criteria */
    if (!json_typecmp(item->branch[j], type|JSON_OBJECT|JSON_ARRAY))
      continue;

    (*utils->method)(',',utils);
    json_recursive_print(item->branch[j], type, utils);
  }

  /* prints object or array wrapper token */
  switch(item->type){
  case JSON_OBJECT:
      (*utils->method)('}', utils);
      break;
  case JSON_ARRAY:
      (*utils->method)(']', utils);
      break;
  default: /* this shouldn't ever happen, but just in case */
      fprintf(stderr,"ERROR: not a wrapper (object or array) json type\n");
      exit(EXIT_FAILURE);
  }
}

/* return string converted json item */
json_string_kt
json_stringify(json_item_st *root, json_type_et type)
{
  assert(NULL != root);

  struct utils_s utils = {0};

  /* count how many chars will fill the buffer with
      utils_buffer_method_count, then allocate buffer that amount */
  utils.method = &utils_buffer_method_count;
  json_recursive_print(root, type, &utils);
  utils.buffer_base = malloc(utils.buffer_offset+5);//extra +5 safety
  assert(NULL != utils.buffer_base);

  /* reset buffer_offset and proceed with utils_.method_update to
    fill allocated buffer */
  utils.buffer_offset = 0;

  utils.method = &utils_buffer_method_update;
  json_recursive_print(root, type, &utils);
  utils.buffer_base[utils.buffer_offset] = 0; //end of buffer token

  return utils.buffer_base;
}
