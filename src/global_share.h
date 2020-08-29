#include "../JSON.h"

#ifndef SHAREFILE_INCLUDED
#define SHAREFILE_INCLUDED

#define HASHTABLE_SIZE 10000

typedef struct json_ht_entry_s {
  json_string_kt key; //json item's literal key
  json_item_st **shared_items; //item addresses that share the same key
  size_t num_shared_items; //amount of items sharing the same key
  size_t last_accessed_item;
  struct json_ht_entry_s *next; //point to next key
} json_ht_entry_st;

typedef struct json_ht_s {
  json_ht_entry_st *entries[HASHTABLE_SIZE];
} json_ht_st;

#ifdef MAIN_FILE
json_ht_st g_hashtable = {NULL};
#else
extern json_ht_st g_hashtable;
json_ht_entry_st* json_ht_get(const json_string_kt kKey);
json_ht_entry_st* json_ht_set(const json_string_kt kKey, json_item_st *item);
void json_ht_destroy();
#endif
#endif
