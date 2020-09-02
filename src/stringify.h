#ifndef JSONC_STRINGIFY_H
#define JSONC_STRINGIFY_H

#include <stdlib.h>
#include <stdio.h>

#include "parser.h"

/* converts json item into a string and returns its address */
json_string_kt json_item_stringify(json_item_st *root, json_type_et type);

#endif
