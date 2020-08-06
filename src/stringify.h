#include "../json_parser.h"

#include <stdlib.h>
#include <stdio.h>

/* prints json item content and its nests */
void print_json(CJSON_item_t *item, CJSON_types_t datatype, FILE *stream);
