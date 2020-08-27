#include <stdio.h>
#include "../JSON.h"


json_item_st* json_item_next(json_item_st* item);
int json_search_key(const json_st* json, const json_string_kt search_key);
int json_replace_key_all(const json_st* json, const json_string_kt old_key, const json_string_kt new_key);
json_item_st* json_item_get_root(json_item_st* item);


void json_item_typeof(const json_item_st* item, FILE* stream);
int json_item_typecmp(const json_item_st* item, const json_type_et type);
int json_item_keycmp(const json_item_st* item, const json_string_kt key);
int json_item_numbercmp(const json_item_st* item, const json_number_kt number);
json_item_st* json_item_get_sibling(const json_item_st* origin, const long int rel_index);
json_item_st* json_item_get_parent(const json_item_st* item);
json_item_st* json_item_get_property(const json_item_st* item, const size_t index);
size_t json_item_get_property_count(const json_item_st* item);
json_type_et json_item_get_type(const json_item_st* item);
json_string_kt json_item_get_key(const json_item_st* item);
json_boolean_kt json_item_get_boolean(const json_item_st* item);
json_string_kt json_item_get_string(const json_item_st* item);


void json_number_tostr(const json_number_kt number, json_string_kt ptr, const int digits);
