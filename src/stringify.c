#include "../json_parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static void
cat_update(char *dest, char *src, int *i){
  while (*src)
    dest[(*i)++] = *src++;
}

static void
stringify_key(char *buffer, char *key, int *i)
{
  if(!key)

    return; //return if no key found or is array key
  buffer[(*i)++] = '\"';
  cat_update(buffer,key,i);
  buffer[(*i)++] = '\"';
  buffer[(*i)++] = ':';
}

static void
recursive_print(CJSON_item_t *item, CJSON_types_t dtype, char *buffer, int *i)
{
  if (item->dtype & dtype){
    if (!(item->parent->dtype & JsonArray))
      stringify_key(buffer,item->key,i);

    switch (item->dtype){
      case JsonNull:
        cat_update(buffer,"null",i);
        break;
      case JsonBoolean:
        if (item->value.boolean){
          cat_update(buffer,"true",i);
          break;
        }
        cat_update(buffer,"false",i);
        break;
      case JsonNumber:
        buffer[(*i)++] = '0';
        break;
      case JsonString:
        buffer[(*i)++] = '\"';
        cat_update(buffer,item->value.string,i);
        buffer[(*i)++] = '\"';
        break;
      case JsonObject:
        buffer[(*i)++] = '{';
        break;
      case JsonArray:
        buffer[(*i)++] = '[';
        break;
      default:
        fprintf(stderr,"ERROR: undefined datatype\n");
        exit(EXIT_FAILURE);
        break;
    }
  }

  for (size_t j=0 ; j < item->n ; ++j){
    recursive_print(item->properties[j], dtype, buffer,i);
    buffer[(*i)++] = ',';
  } 
   
  if ((item->dtype & dtype) & (JsonObject|JsonArray)){
    if (buffer[(*i)-1] == ',')
      --*i;

    if (item->dtype == JsonObject)
      buffer[(*i)++] = '}';
    else //is array 
      buffer[(*i)++] = ']';
  }
}

char*
stringify_json(CJSON_t *cjson, CJSON_types_t dtype)
{
  assert(cjson);
  assert(cjson->item->n);

  char *buffer=calloc(1,cjson->memsize);

  int i=0;
  recursive_print(cjson->item, dtype, buffer, &i);
  buffer[i-1] = '\0';
   
  return buffer;
}
