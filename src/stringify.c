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
  ulong buffer_offset; //distance in chars to buffer's base
  /*a setter method, will be either buffer_method_count or
     buffer_method_update*/
  void (*method)(char get_char, struct utils_s* utils);
};

/* increases distance to buffer's base */ 
static void
buffer_method_count(char get_char, struct utils_s *utils){
  ++utils->buffer_offset;
}

/* inserts char to current offset then increase it */ 
static void
buffer_method_update(char get_char, struct utils_s *utils)
{
  utils->buffer_base[utils->buffer_offset] = get_char;
  ++utils->buffer_offset;
}

/* evoke buffer method */
static void
buffer_execute_method(json_string_kt string, struct utils_s *utils)
{
  while (*string){
    (*utils->method)(*string,utils);
    ++string;
  }
}

/* returns number (double) converted to string */
static void
buffer_set_number(json_number_kt number, struct utils_s *utils)
{
  char get_strnum[MAX_DIGITS];
  json_number_tostr(number, get_strnum, MAX_DIGITS);

  buffer_execute_method(get_strnum,utils); //store value in utils
}

//@todo: make this iterative
/* stringify json items by going through its properties recursively */
static void
json_item_recursive_print(json_item_st *item, json_type_et type, struct utils_s *utils)
{
  /* stringify json item only if its of the same given type */
  if (json_item_typecmp(item,type)){
    if ((NULL != item->key) && !json_item_typecmp(item->parent,JSON_ARRAY)){
      (*utils->method)('\"',utils);
      buffer_execute_method(item->key,utils);
      (*utils->method)('\"',utils);
      (*utils->method)(':',utils);
    }

    switch (item->type){
    case JSON_NULL:
        buffer_execute_method("null",utils);
        break;
    case JSON_BOOLEAN:
        if (1 == item->boolean){
          buffer_execute_method("true",utils);
          break;
        }
        buffer_execute_method("false",utils);
        break;
    case JSON_NUMBER:
        buffer_set_number(item->number,utils);
        break;
    case JSON_STRING:
        (*utils->method)('\"',utils);
        buffer_execute_method(item->string,utils);
        (*utils->method)('\"',utils);
        break;
    case JSON_OBJECT:
        (*utils->method)('{',utils);
        break;
    case JSON_ARRAY:
        (*utils->method)('[',utils);
        break;
    default:
        fprintf(stderr,"ERROR: undefined datatype\n");
        exit(EXIT_FAILURE);
        break;
    }
  }

  for (size_t j=0; j < item->num_branch; ++j){
    json_item_recursive_print(item->branch[j], type, utils);
    (*utils->method)(',',utils);
  } 
   
  if (json_item_typecmp(item, type & (JSON_OBJECT|JSON_ARRAY))){
    if (0 != item->num_branch) //remove extra comma from obj/array
      --utils->buffer_offset;

    if (json_item_typecmp(item, JSON_OBJECT))
      (*utils->method)('}',utils);
    else //is array 
      (*utils->method)(']',utils);
  }
}

/* converts json item into a string and returns its address */
json_string_kt
json_item_stringify(json_item_st *root, json_type_et type)
{
  assert(NULL != root);

  struct utils_s utils = {0};
  /* count how much memory should be allocated for buffer
      with buffer_method_count, then allocate it*/
  utils.method = &buffer_method_count;
  json_item_recursive_print(root, type, &utils);
  utils.buffer_base = malloc(utils.buffer_offset+2);
  assert(NULL != utils.buffer_base);

  /* reset buffer then stringify json item with
      buffer_method_update into buffer, then return it */
  utils.buffer_offset = 0;
  utils.method = &buffer_method_update;
  json_item_recursive_print(root, type, &utils);
  utils.buffer_base[utils.buffer_offset] = 0;

  return utils.buffer_base;
}
