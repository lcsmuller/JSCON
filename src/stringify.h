#include "../CJSON.h"

#include <stdlib.h>
#include <stdio.h>

/* prints json item content and its nests */
char*
stringify_json(CJSON_t *cjson, CJSON_types_t datatype);
