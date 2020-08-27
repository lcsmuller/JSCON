  #include "../JSON.h"

  #include <stdio.h>
  #include <stdlib.h>
  #include <string.h>
  #include <ctype.h>
  #include <assert.h>

struct buffer_s {
  char *ptr;
  ulong offset;
  void (*method)(char get_char, struct buffer_s* buffer);
};

static void
buffer_method_count(char get_char, struct buffer_s *buffer){
  ++buffer->offset;
}

static void
buffer_method_update(char get_char, struct buffer_s *buffer)
{
  buffer->ptr[buffer->offset] = get_char;
  ++buffer->offset;
}

static void
buffer_set_string(json_string_kt string, struct buffer_s *buffer)
{
  while (*string){
    (*buffer->method)(*string,buffer);
    ++string;
  }
}

static void
buffer_set_number(json_number_kt number, struct buffer_s *buffer)
{
  char get_strnum[MAX_DIGITS];
  json_number_tostr(number, get_strnum, MAX_DIGITS);

  buffer_set_string(get_strnum,buffer); //store value in buffer
}

static void
json_item_recursive_print(json_item_st *item, json_type_et type, struct buffer_s *buffer)
{
  if (json_item_typecmp(item,type)){
    if ((item->ptr_key) && !(json_item_typecmp(item->parent,JSON_ARRAY))){
      (*buffer->method)('\"',buffer);
      buffer_set_string(*item->ptr_key,buffer);
      (*buffer->method)('\"',buffer);
      (*buffer->method)(':',buffer);
    }

    switch (item->type){
      case JSON_NULL:
        buffer_set_string("null",buffer);
        break;
      case JSON_BOOLEAN:
        if (item->boolean){
          buffer_set_string("true",buffer);
          break;
        }
        buffer_set_string("false",buffer);
        break;
      case JSON_NUMBER:
        buffer_set_number(item->number,buffer);
        break;
      case JSON_STRING:
        (*buffer->method)('\"',buffer);
        buffer_set_string(item->string,buffer);
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
   
  if (json_item_typecmp(item,type&(JSON_OBJECT|JSON_ARRAY))){
    if (item->num_branch != 0) //remove extra comma from obj/array
      --buffer->offset;

    if (json_item_typecmp(item, JSON_OBJECT))
      (*buffer->method)('}',buffer);
    else //is array 
      (*buffer->method)(']',buffer);
  }
}

json_string_kt
json_item_stringify(json_item_st *root, json_type_et type)
{
  assert(root);

  struct buffer_s buffer={0};
  /* COUNT HOW MUCH MEMORY SHOULD BE ALLOCATED FOR BUFFER 
      BY BUFFER_COUNT METHOD */
  buffer.method = &buffer_method_count;
  json_item_recursive_print(root, type, &buffer);
  /* ALLOCATE BY CALCULATED AMOUNT */
  buffer.ptr = malloc(buffer.offset+2);
  assert(buffer.ptr);
  /* RESET OFFSET */ 
  buffer.offset = 0;
  /* STRINGIFY JSON SAFELY WITH BUFFER_UPDATE METHOD */
  buffer.method = &buffer_method_update;
  json_item_recursive_print(root, type, &buffer);
  buffer.ptr[buffer.offset] = '\0';

  return buffer.ptr;
}
