#include "../json_parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

static void
recursive_print(CJSON_item_t *item, CJSON_types_t datatype, FILE *stream)
{
  if (item->datatype & datatype){
    fwrite(item->key.start, 1, item->key.length, stream);
    fputc(' ', stream);
    fwrite(item->val.start, 1, item->val.length, stream);
    fputc(' ', stream);
    switch (item->datatype){
      case JsonNull:
        fputs("Null", stream);
        break;
      case JsonTrue:
      case JsonFalse:
        fputs("Boolean", stream);
        break;
      case JsonNumber:
        fputs("Number", stream);
        break;
      case JsonString:
        fputs("String", stream);
        break;
      case JsonObject:
        fputs("Object", stream);
        break;
      case JsonArray:
        fputs("Array", stream);
        break;
      default:
        fprintf(stderr,"undefined datatype\n");
        exit(1);
        break;
    }
    fputc('\n', stream);
  }

  for (size_t i=0 ; i < item->obj.n ; ++i){
    recursive_print(item->obj.properties[i], datatype, stream);
  }
}

void
print_json(CJSON_item_t *item, CJSON_types_t datatype, FILE *stream)
{
  assert(!item->val.length && item->obj.n);
  recursive_print(item, datatype, stream);
}
