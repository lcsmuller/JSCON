#ifndef JSONC_STRINGIFY_H
#define JSONC_STRINGIFY_H

#include "parser.h"

/* converts json item into a string and returns its address */
//@todo: create a default read callback
//      and add an option to modify the read callback
json_string_kt json_stringify(json_item_st *root, json_type_et type);

#endif
