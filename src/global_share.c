#define MAIN_FILE
#include "global_share.h"
#include <string.h>
#include <assert.h>

static unsigned int
json_hash(const json_string_kt kKey)
{
  unsigned long slot = 0;
  unsigned int key_len = strlen(kKey);

  for (unsigned int i=0; i < key_len; ++i){
    slot = slot * 37 + kKey[i];
  }
  slot %= HASHTABLE_SIZE;

  return slot;
}

static json_hasht_entry_st*
json_hashtable_pair(const json_string_kt kKey, json_item_st* item)
{
  json_hasht_entry_st *entry = calloc(1, sizeof *entry);
  assert(NULL != entry);

  entry->key = strdup(kKey);
  assert(NULL != entry->key);

  entry->start = calloc(1, sizeof *entry->start);
  assert(NULL != entry->start);

  entry->start->item = item;
  entry->last_access_shared = entry->start;
  entry->end = entry->start;

  return entry;
}

json_hasht_entry_st*
json_hashtable_get(json_hasht_st *hashtable, const json_string_kt kKey)
{
  unsigned int slot = json_hash(kKey);

  json_hasht_entry_st *entry = hashtable->entries[slot];

  while (NULL != entry){ //try to find key and return it
    if (STREQ(entry->key, kKey)){
      return entry;
    }
    entry = entry->next;
  }

  return NULL;
}

static void
json_hashtable_new_shared_item(json_hasht_entry_st* entry, json_item_st* item)
{
  json_hasht_shared_st *new_shared = malloc(sizeof *new_shared);
  assert(NULL != new_shared);

  new_shared->item = item;
  new_shared->next = NULL;

  entry->end->next = new_shared;
  entry->end = new_shared;
}

json_hasht_entry_st*
json_hashtable_set(json_hasht_st* hashtable, const json_string_kt kKey, json_item_st *item)
{
  unsigned int slot = json_hash(kKey);

  json_hasht_entry_st *entry = hashtable->entries[slot];

  if (NULL == entry){
    hashtable->entries[slot] = json_hashtable_pair(kKey, item);
    return hashtable->entries[slot];
  }

  json_hasht_entry_st *entry_prev;
  while (NULL != entry){
    if (STREQ(entry->key, kKey)){
      json_hashtable_new_shared_item(entry, item);
      return entry;
    }
    entry_prev = entry;
    entry = entry->next;
  }

  entry_prev->next = json_hashtable_pair(kKey, item);

  return entry_prev->next;
}

json_hasht_st*
json_hashtable_destroy(json_hasht_st *hashtable)
{
  for (unsigned int i=0; i < HASHTABLE_SIZE; ++i){
    if (NULL == hashtable->entries[i])
      continue;

    json_hasht_entry_st *entry = hashtable->entries[i];
    json_hasht_entry_st *entry_prev;
    while (NULL != entry){
      entry_prev = entry;
      entry = entry->next;
      
      json_hasht_shared_st *shared = entry_prev->start;
      json_hasht_shared_st *shared_prev;
      while (NULL != shared){
        shared_prev = shared;
        shared = shared->next;
        free(shared_prev);
      }
      
      free(entry_prev->key);
      entry_prev->key = NULL;

      free(entry_prev);
      entry_prev = NULL;
    }
  }
  json_hasht_st *ht_prev = hashtable;
  hashtable = hashtable->next;

  free(ht_prev);
  return hashtable;
}
