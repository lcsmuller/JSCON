#ifndef JSONC_PUBLIC_H
#define JSONC_PUBLIC_H

#include "parser.h"

/* JSONC UTILITIES */
jsonc_item_st* jsonc_foreach_specific(jsonc_item_st *item, const jsonc_string_kt kKey);
jsonc_item_st* jsonc_foreach_object_r(jsonc_item_st *item, jsonc_item_st **p_current_item);
jsonc_item_st* jsonc_foreach(jsonc_item_st* item);
jsonc_item_st* jsonc_clone(jsonc_item_st *item);
jsonc_string_kt jsonc_typeof(const jsonc_item_st* kItem);
jsonc_string_kt jsonc_strdup(const jsonc_item_st* kItem);
int jsonc_typecmp(const jsonc_item_st* kItem, const jsonc_type_et kType);
int jsonc_keycmp(const jsonc_item_st* kItem, const jsonc_string_kt kKey);
int jsonc_doublecmp(const jsonc_item_st* kItem, const jsonc_double_kt kDouble);
int jsonc_intcmp(const jsonc_item_st* kItem, const jsonc_integer_kt kInteger);
void jsonc_double_tostr(const jsonc_double_kt kDouble, jsonc_string_kt p_str, const int kDigits);

/* JSONC GETTERS */
jsonc_item_st* jsonc_get_root(jsonc_item_st* item);
jsonc_item_st* jsonc_get_sibling(const jsonc_item_st* kOrigin, const size_t kRelative_index);
jsonc_item_st* jsonc_get_parent(const jsonc_item_st* kItem);
jsonc_item_st* jsonc_get_branch(const jsonc_item_st* kItem, const size_t kIndex);
size_t jsonc_get_num_branch(const jsonc_item_st* kItem);
jsonc_type_et jsonc_get_type(const jsonc_item_st* kItem);
jsonc_string_kt jsonc_get_key(const jsonc_item_st* kItem);
jsonc_boolean_kt jsonc_get_boolean(const jsonc_item_st* kItem);
jsonc_string_kt jsonc_get_string(const jsonc_item_st* kItem);
jsonc_double_kt jsonc_get_double(const jsonc_item_st* kItem);
jsonc_integer_kt jsonc_get_integer(const jsonc_item_st* kItem);




#endif
