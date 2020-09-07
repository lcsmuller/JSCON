#ifndef JSONC_HASHTABLE_H
#define JSONC_HASHTABLE_H

#include "parser.h"


typedef struct json_hasht_entry_s {
  json_string_kt key;
  json_item_st *item; //this entry's value
  struct json_hasht_entry_s *next; //next entry pointer for when keys don't match
} json_hasht_entry_st;

typedef struct json_hasht_s {
  json_item_st *root;
  struct json_hasht_s *next; //@todo: make this point to differente buckets instead ?
  json_hasht_entry_st **bucket;
  size_t num_bucket;
} json_hasht_st;


json_hasht_st* json_hashtable_init();
void json_hashtable_destroy(json_hasht_st *hashtable);
void json_hashtable_link_r(json_item_st *item, json_hasht_st **last_accessed_hashtable);
void json_hashtable_build(json_item_st *item);
json_item_st* json_hashtable_get(const json_string_kt kKey, json_item_st *item);
json_item_st* json_hashtable_set(const json_string_kt kKey, json_item_st *item);

#endif
