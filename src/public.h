#include <stdio.h>
#include "../JSON.h"


json_item_st* json_item_get_specific(const json_string_kt kKey);
json_item_st* json_item_next(json_item_st* item);
json_item_st* json_item_get_root(json_item_st* item);
void json_item_typeof(const json_item_st* kItem, FILE* stream);
int json_item_typecmp(const json_item_st* kItem, const json_type_et kType);
int json_item_keycmp(const json_item_st* kItem, const json_string_kt kKey);
int json_item_numbercmp(const json_item_st* kItem, const json_number_kt kNumber);
json_item_st* json_item_get_sibling(const json_item_st* kOrigin, const long kRelative_index);
json_item_st* json_item_get_parent(const json_item_st* kItem);
json_item_st* json_item_get_property(const json_item_st* kItem, const size_t kIndex);
size_t json_item_get_property_count(const json_item_st* kItem);
json_type_et json_item_get_type(const json_item_st* kItem);
json_string_kt json_item_get_key(const json_item_st* kItem);
json_boolean_kt json_item_get_boolean(const json_item_st* kItem);
json_string_kt json_item_get_string(const json_item_st* kItem);
json_number_kt json_item_get_number(const json_item_st* kItem);


void json_number_tostr(const json_number_kt kNumber, json_string_kt p_str, const int kDigits);
