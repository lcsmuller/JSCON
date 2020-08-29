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

static json_ht_entry_st*
json_ht_pair(const json_string_kt kKey, json_item_st* item)
{
  json_ht_entry_st *entry = calloc(1, sizeof *entry);
  assert(NULL != entry);

  entry->key = strdup(kKey);
  assert(NULL != entry->key);

  entry->shared_items = malloc(sizeof *entry->shared_items);
  assert(NULL != entry->shared_items);

  entry->shared_items[0] = item;

  ++entry->num_shared_items;

  entry->next = NULL;

  return entry;
}

json_ht_entry_st*
json_ht_get(const json_string_kt kKey)
{
  unsigned int slot = json_hash(kKey);

  json_ht_entry_st *entry = g_hashtable.entries[slot];

  while (NULL != entry){ //try to find key and return it
    if (STREQ(entry->key, kKey)){
      return entry;
    }
    entry = entry->next;
  }

  return NULL;
}

static void
json_ht_new_shared_item(json_ht_entry_st* entry, json_item_st* item)
{
  ++entry->num_shared_items;

  entry->shared_items = realloc(entry->shared_items, (entry->num_shared_items) * (sizeof *entry->shared_items));
  assert(NULL != entry->shared_items);

  entry->shared_items[entry->num_shared_items-1] = item;
}

json_ht_entry_st*
json_ht_set(const json_string_kt kKey, json_item_st *item)
{
  unsigned int slot = json_hash(kKey);

  json_ht_entry_st *entry = g_hashtable.entries[slot];

  if (NULL == entry){
    g_hashtable.entries[slot] = json_ht_pair(kKey, item);
    return g_hashtable.entries[slot];
  }

  json_ht_entry_st *prev;
  while (NULL != entry){
    if (STREQ(entry->key, kKey)){
      json_ht_new_shared_item(entry, item);
      return entry;
    }
    prev = entry;
    entry = entry->next;
  }

  prev->next = json_ht_pair(kKey, item);
  return prev->next;
}

void
json_ht_destroy()
{
  for (unsigned int i=0; i < HASHTABLE_SIZE; ++i){
    if (NULL == g_hashtable.entries[i])
      continue;

    json_ht_entry_st *entry = g_hashtable.entries[i];
    json_ht_entry_st *prev;
    while (NULL != entry){
      prev = entry;
      entry = entry->next;

      free(prev->shared_items);
      free(prev->key);
      free(prev);
    }
  }
}
