#ifndef JSONC_HASHTABLE_H
#define JSONC_HASHTABLE_H

#include "parser.h"


typedef struct jsonc_hasht_entry_s {
  jsonc_string_kt key;
  jsonc_item_st *item; //this entry's value
  struct jsonc_hasht_entry_s *next; //next entry pointer for when keys don't match
} jsonc_hasht_entry_st;

typedef struct jsonc_hasht_s {
  jsonc_item_st *root;
  struct jsonc_hasht_s *next; //@todo: make this point to differente buckets instead ?
  jsonc_hasht_entry_st **bucket;
  size_t num_bucket;
} jsonc_hasht_st;


jsonc_hasht_st* jsonc_hashtable_init();
void jsonc_hashtable_destroy(jsonc_hasht_st *hashtable);
void jsonc_hashtable_link_r(jsonc_item_st *item, jsonc_hasht_st **last_accessed_hashtable);
void jsonc_hashtable_build(jsonc_item_st *item);
jsonc_item_st* jsonc_hashtable_get(const jsonc_string_kt kKey, jsonc_item_st *item);
jsonc_item_st* jsonc_hashtable_set(const jsonc_string_kt kKey, jsonc_item_st *item);

#endif
