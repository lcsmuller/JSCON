#include "../json_parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

static void
recursive_print(CJSON_item_t *item, CJSON_types_t datatype, FILE *stream)
{
  if (item->datatype & datatype){
    fwrite(item->key.start, 1, item->key.length, stream);
    fputc(':', stream);
    switch (item->datatype){
      case JsonNull:
        fprintf(stream, "null");
        break;
      case JsonBoolean:
        if (item->value.boolean)
          fprintf(stream, "true");
        else
          fprintf(stream, "false");
        break;
      case JsonNumber:
        fprintf(stream,"%f",item->value.number);
        break;
      case JsonString:
        fputs(item->value.string, stream);
        break;
      case JsonObject:
        fputc('{', stream);
        break;
      case JsonArray:
        fputc('[', stream);
        break;
      default:
        fprintf(stderr,"ERROR: undefined datatype\n");
        exit(1);
        break;
    }
  }

  for (size_t i=0 ; i < item->n ; ++i){
    recursive_print(item->properties[i], datatype, stream);
  }

  if (datatype & (JsonObject|JsonArray)){
    if (item->datatype == JsonObject)
      fputc('}',stream);
    else if (item->datatype == JsonArray)
      fputc(']',stream);
    fputc(',', stream);
  }
}

void
print_json(CJSON_item_t *item, CJSON_types_t datatype, FILE *stream)
{
  assert(item->n);
  recursive_print(item, datatype, stream);
}
