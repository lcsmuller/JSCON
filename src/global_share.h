#include "../JSON.h"

#ifndef SHAREFILE_INCLUDED
#define SHAREFILE_INCLUDED

#define HASHTABLE_SIZE 10000

/* linked list of items sharing the same key */
typedef struct json_ht_shared_s {
  json_item_st *item;
  struct json_ht_shared_s *next;
} json_ht_shared_st;

typedef struct json_ht_entry_s {
  json_string_kt key; //json item's literal key
  json_ht_shared_st *start; //linked list of items sharing the same key
  json_ht_shared_st *end; //point to end of linked list
  json_ht_shared_st *last_access_shared; //items sharing the same key
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
