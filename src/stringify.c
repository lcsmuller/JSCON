#include "../json_parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static void
cat_update(char *dest, char *src, int *i)
{
  while (*src){
    dest[(*i)++] = *src++;
  }
}

static void
recursive_print(CJSON_item_t *item, CJSON_types_t datatype, char *buffer)
{
  static int i=0;

  if (item->datatype & datatype){
    if (item->key){
      buffer[i++] = '\"';
      cat_update(buffer,item->key,&i);
      buffer[i++] = '\"';
      buffer[i++] = ':';
    }
    switch (item->datatype){
      case JsonNull:
        cat_update(buffer,"null",&i);
        break;
      case JsonBoolean:
        if (item->value.boolean)
          cat_update(buffer,"true",&i);
        else
          cat_update(buffer,"false",&i);
        break;
      case JsonNumber:
        buffer[i++] = '0';
        break;
      case JsonString:
        buffer[i++] = '\"';
        cat_update(buffer,item->value.string,&i);
        buffer[i++] = '\"';
        break;
      case JsonObject:
        buffer[i++] = '{';
        break;
      case JsonArray:
        buffer[i++] = '[';
        break;
      default:
        fprintf(stderr,"ERROR: undefined datatype\n");
        exit(1);
        break;
    }
  }

  for (size_t j=0 ; j < item->n ; ++j){
    recursive_print(item->properties[j], datatype, buffer);
    buffer[i++] = ',';
  } 
   
  if (datatype & (JsonObject|JsonArray)){
    if (item->datatype == JsonObject){
      if (buffer[i-1] == ',')
        --i;
      buffer[i++] = '}';
    } else if (item->datatype == JsonArray){
      if (buffer[i-1] == ',')
        --i;
      buffer[i++] = ']';
    }
  }
}

char*
stringify_json(CJSON_t *cjson, CJSON_types_t datatype)
{
  assert(cjson);
  assert(cjson->item->n);

  char *buffer=calloc(1,cjson->memsize+50);
  recursive_print(cjson->item, datatype, buffer);
  if(buffer[strlen(buffer)-1] == ',')
    buffer[strlen(buffer)-1] = '\0';
   
  return buffer;
}
