#include "../JSON.h"

#ifndef SHAREFILE_INCLUDED
#define SHAREFILE_INCLUDED

#define HASHTABLE_SIZE 10000

/* linked list of items sharing the same key */
typedef struct json_hasht_shared_s {
  json_item_st *item;
  struct json_hasht_shared_s *next;
} json_hasht_shared_st;

typedef struct json_hasht_entry_s {
  json_string_kt key; //json item's literal key
  struct json_hasht_entry_s *next; //point to next key

  json_hasht_shared_st *start; //linked list of items sharing the same key
  json_hasht_shared_st *end; //point to end of linked list
  json_hasht_shared_st *last_access_shared;
} json_hasht_entry_st;

typedef struct json_hasht_s {
  json_item_st *root_tag; //assign root address for tag
  struct json_hasht_s *next; //points to next hashtable

  json_hasht_entry_st *entries[HASHTABLE_SIZE];
} json_hasht_st;

typedef struct json_utils_st {
  json_hasht_st *first_hasht;
} json_utils_st;

#ifdef MAIN_FILE
json_utils_st g_utils = {NULL};
#else
extern json_utils_st g_utils;
json_hasht_entry_st* json_hashtable_get(json_hasht_st *hashtable, const json_string_kt kKey);
json_hasht_entry_st* json_hashtable_set(json_hasht_st *hashtable, const json_string_kt kKey, json_item_st *item);
json_hasht_st* json_hashtable_destroy(json_hasht_st *hashtable);
#endif
#endif
