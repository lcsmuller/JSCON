#ifndef JSONC_STRINGIFY_H
#define JSONC_STRINGIFY_H

#include "parser.h"

/* converts jsonc item into a string and returns its address */
//@todo: create a default read callback
//      and add an option to modify the read callback
jsonc_string_kt jsonc_stringify(jsonc_item_st *root, jsonc_type_et type);

#endif
