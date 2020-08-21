#include "../JSON.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

struct Buffer {
  char *ptr;
  ulong offset;
  void (*method)(char get_char, struct Buffer* buffer);
};

static void
BufferMethod_Count(char get_char, struct Buffer *buffer){
  ++buffer->offset;
}

static void
BufferMethod_Update(char get_char, struct Buffer *buffer)
{
  buffer->ptr[buffer->offset] = get_char;
  ++buffer->offset;
}

static void
Buffer_SetString(JsonString *string, struct Buffer *buffer)
{
  while (*string){
    (*buffer->method)(*string,buffer);
    ++string;
  }
}

static void
Buffer_SetNumber(JsonNumber number, struct Buffer *buffer)
{
  JsonString *get_strnum = JsonNumber_StrFormat(number);
  assert(get_strnum);

  Buffer_SetString(get_strnum,buffer); //store value in buffer

  free(get_strnum);
}

static void
JsonItem_RecPrint(JsonItem *item, JsonDType dtype, struct Buffer *buffer)
{
  if (item->dtype & dtype){
    if ((item->key) && !(item->parent->dtype & Array)){
      (*buffer->method)('\"',buffer);
      Buffer_SetString(item->key,buffer);
      (*buffer->method)('\"',buffer);
      (*buffer->method)(':',buffer);
    }

    switch (item->dtype){
      case Null:
        Buffer_SetString("null",buffer);
        break;
      case Boolean:
        if (item->boolean){
          Buffer_SetString("true",buffer);
          break;
        }
        Buffer_SetString("false",buffer);
        break;
      case Number:
        Buffer_SetNumber(item->number,buffer);
        break;
      case String:
        (*buffer->method)('\"',buffer);
        Buffer_SetString(item->string,buffer);
        (*buffer->method)('\"',buffer);
        break;
      case Object:
        (*buffer->method)('{',buffer);
        break;
      case Array:
        (*buffer->method)('[',buffer);
        break;
      default:
        fprintf(stderr,"ERROR: undefined datatype\n");
        exit(EXIT_FAILURE);
        break;
    }
  }

  for (size_t j=0; j < item->n_property; ++j){
    JsonItem_RecPrint(item->property[j], dtype, buffer);
    (*buffer->method)(',',buffer);
  } 
   
  if ((item->dtype & dtype) & (Object|Array)){
    if (item->n_property != 0) //remove extra comma from obj/array
      --buffer->offset;

    if (item->dtype == Object)
      (*buffer->method)('}',buffer);
    else //is array 
      (*buffer->method)(']',buffer);
  }
}

char*
Json_Stringify(Json *json, JsonDType dtype)
{
  assert(json);

  struct Buffer buffer={0};
  /* COUNT HOW MUCH MEMORY SHOULD BE ALLOCATED FOR BUFFER 
      WITH BUFFER_COUNT METHOD */
  buffer.method = &BufferMethod_Count;
  JsonItem_RecPrint(json->root, dtype, &buffer);
  /* ALLOCATE BY CALCULATED AMOUNT */
  buffer.ptr = malloc(buffer.offset+1);
  assert(buffer.ptr);
  /* RESET OFFSET */ 
  buffer.offset = 0;
  /* STRINGIFY JSON SAFELY WITH BUFFER_UPDATE METHOD */
  buffer.method = &BufferMethod_Update;
  JsonItem_RecPrint(json->root, dtype, &buffer);
  buffer.ptr[buffer.offset] = 0;

  return buffer.ptr;
}
