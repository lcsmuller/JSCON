#ifndef JSONC_PUBLIC_H
#define JSONC_PUBLIC_H

#include <stdio.h>

#include "parser.h"

json_item_st* json_get_specific(json_item_st *item, const json_string_kt kKey);
json_item_st* json_next_object(json_item_st *item, json_item_st **p_current_item);
json_item_st* json_get_clone(json_item_st *item);
json_item_st* json_next(json_item_st* item);
json_item_st* json_get_root(json_item_st* item);
void json_typeof(const json_item_st* kItem, FILE* stream);
int json_typecmp(const json_item_st* kItem, const json_type_et kType);
int json_keycmp(const json_item_st* kItem, const json_string_kt kKey);
int json_doublecmp(const json_item_st* kItem, const json_double_kt kDouble);
int json_intcmp(const json_item_st* kItem, const json_integer_kt kInteger);
json_item_st* json_get_sibling(const json_item_st* kOrigin, const long kRelative_index);
json_item_st* json_get_parent(const json_item_st* kItem);
json_item_st* json_get_property(const json_item_st* kItem, const size_t kIndex);
size_t json_get_property_count(const json_item_st* kItem);
json_type_et json_get_type(const json_item_st* kItem);
json_string_kt json_get_key(const json_item_st* kItem);
json_boolean_kt json_get_boolean(const json_item_st* kItem);
json_string_kt json_get_string(const json_item_st* kItem);
json_string_kt json_get_strdup(const json_item_st* kItem);
json_double_kt json_get_double(const json_item_st* kItem);


void json_double_tostr(const json_double_kt kDouble, json_string_kt p_str, const int kDigits);

#endif
