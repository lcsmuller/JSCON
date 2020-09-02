#ifndef JSONC_HASHTABLE_H
#define JSONC_HASHTABLE_H

#define HASHTABLE_SIZE 20

#include "parser.h"


typedef struct json_hasht_entry_s {
  json_string_kt key; //json item's literal key
  json_item_st *item; //value
  struct json_hasht_entry_s *next; //point to next key
} json_hasht_entry_st;

typedef struct json_hasht_s {
  json_item_st *root;
  struct json_hasht_s *next;
  json_hasht_entry_st *entries[HASHTABLE_SIZE];
} json_hasht_st;


json_hasht_st* json_hashtable_init();
void json_hashtable_destroy(json_hasht_st *hashtable);
void json_hashtable_build(json_item_st *item, json_hasht_st **last_accessed_hashtable);
json_item_st* json_hashtable_get(const json_string_kt kKey, json_item_st *item);
json_item_st* json_hashtable_set(const json_string_kt kKey, json_item_st *item);

#endif
