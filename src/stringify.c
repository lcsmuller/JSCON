  #include "../JSON.h"

  #include <stdio.h>
  #include <stdlib.h>
  #include <string.h>
  #include <ctype.h>
  #include <assert.h>


struct buffer_s {
  char *base; //buffer's base (first position)
  ulong offset; //distance in chars to buffer's base
  /*a setter method, will be either buffer_method_count or
     buffer_method_update*/
  void (*method)(char get_char, struct buffer_s* buffer);
};

/* increases distance to buffer's base */ 
static void
buffer_method_count(char get_char, struct buffer_s *buffer){
  ++buffer->offset;
}

/* inserts char to current offset then increase it */ 
static void
buffer_method_update(char get_char, struct buffer_s *buffer)
{
  buffer->base[buffer->offset] = get_char;
  ++buffer->offset;
}

/* evoke buffer method */
static void
buffer_execute_method(json_string_kt string, struct buffer_s *buffer)
{
  while (*string){
    (*buffer->method)(*string,buffer);
    ++string;
  }
}

/* returns number (double) converted to string */
static void
buffer_set_number(json_number_kt number, struct buffer_s *buffer)
{
  char get_strnum[MAX_DIGITS];
  json_number_tostr(number, get_strnum, MAX_DIGITS);

  buffer_execute_method(get_strnum,buffer); //store value in buffer
}

//@todo: make this iterative
/* stringify json items by going through its properties recursively */
static void
json_item_recursive_print(json_item_st *item, json_type_et type, struct buffer_s *buffer)
{
  /* stringify json item only if its of the same given type */
  if (json_item_typecmp(item,type)){
    if ((NULL != json_item_get_key(item)) && !json_item_typecmp(item->parent,JSON_ARRAY)){
      (*buffer->method)('\"',buffer);
      buffer_execute_method(*item->p_key,buffer);
      (*buffer->method)('\"',buffer);
      (*buffer->method)(':',buffer);
    }

    switch (item->type){
    case JSON_NULL:
        buffer_execute_method("null",buffer);
        break;
    case JSON_BOOLEAN:
        if (1 == item->boolean){
          buffer_execute_method("true",buffer);
          break;
        }
        buffer_execute_method("false",buffer);
        break;
    case JSON_NUMBER:
        buffer_set_number(item->number,buffer);
        break;
    case JSON_STRING:
        (*buffer->method)('\"',buffer);
        buffer_execute_method(item->string,buffer);
        (*buffer->method)('\"',buffer);
        break;
    case JSON_OBJECT:
        (*buffer->method)('{',buffer);
        break;
    case JSON_ARRAY:
        (*buffer->method)('[',buffer);
        break;
    default:
        fprintf(stderr,"ERROR: undefined datatype\n");
        exit(EXIT_FAILURE);
        break;
    }
  }

  for (size_t j=0; j < item->num_branch; ++j){
    json_item_recursive_print(item->branch[j], type, buffer);
    (*buffer->method)(',',buffer);
  } 
   
  if (json_item_typecmp(item, type & (JSON_OBJECT|JSON_ARRAY))){
    if (0 != item->num_branch) //remove extra comma from obj/array
      --buffer->offset;

    if (json_item_typecmp(item, JSON_OBJECT))
      (*buffer->method)('}',buffer);
    else //is array 
      (*buffer->method)(']',buffer);
  }
}

/* converts json item into a string and returns its address */
json_string_kt
json_item_stringify(json_item_st *root, json_type_et type)
{
  assert(NULL != root);

  struct buffer_s buffer={0};
  /* count how much memory should be allocated for buffer
      with buffer_method_count, then allocate it*/
  buffer.method = &buffer_method_count;
  json_item_recursive_print(root, type, &buffer);
  buffer.base = malloc(buffer.offset+2);
  assert(NULL != buffer.base);

  /* reset buffer then stringify json item with
      buffer_method_update into buffer, then return it */
  buffer.offset = 0;
  buffer.method = &buffer_method_update;
  json_item_recursive_print(root, type, &buffer);
  buffer.base[buffer.offset] = 0;

  return buffer.base;
}
