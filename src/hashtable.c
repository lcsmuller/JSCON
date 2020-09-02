#include <string.h>
#include <assert.h>

#include "hashtable.h"
#include "parser.h"
#include "macros.h"

json_hasht_st*
json_hashtable_init()
{
  json_hasht_st *new_hashtable = calloc(1, sizeof *new_hashtable);
  assert(NULL != new_hashtable);

  return new_hashtable;
}

void
json_hashtable_destroy(json_hasht_st *hashtable)
{
  for (uint i=0; i < HASHTABLE_SIZE; ++i){
    if (NULL == hashtable->entries[i])
      continue;

    json_hasht_entry_st *entry = hashtable->entries[i];
    json_hasht_entry_st *entry_prev;
    while (NULL != entry){
      entry_prev = entry;
      entry = entry->next;

      free(entry_prev->key);
      entry_prev->key = NULL;
      free(entry_prev);
      entry_prev = NULL;
    }
  }

  free(hashtable);
  hashtable = NULL;
}

void
json_hashtable_build(json_item_st *item)
{
  static json_hasht_st *hashtable_end;

  if (NULL != hashtable_end){
    hashtable_end->next = item->hashtable;
  }
  hashtable_end = item->hashtable;
  hashtable_end->root = item;
}

static uint
json_hash(const json_string_kt kKey)
{
  ulong slot = 0;
  uint key_len = strlen(kKey);

  for (uint i=0; i < key_len; ++i){
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

  item->key = entry->key;

  entry->item = item;

  return entry;
}

json_item_st*
json_hashtable_get(const json_string_kt kKey, json_item_st *root)
{
  if (!(root->type & (JSON_OBJECT|JSON_ARRAY)))
    return NULL;

  json_hasht_st *hashtable = root->hashtable;
  uint slot = json_hash(kKey);

  json_hasht_entry_st *entry = hashtable->entries[slot];
  while (NULL != entry){ //try to find key and return it
    if (STREQ(entry->key, kKey)){
      return entry->item;
    }
    entry = entry->next;
  }

  return NULL;
}

json_item_st*
json_hashtable_set(const json_string_kt kKey, json_item_st *item)
{
  assert((item->parent->type) & (JSON_OBJECT|JSON_ARRAY));

  uint slot = json_hash(kKey);
  json_hasht_st *hashtable = item->parent->hashtable; //object type

  json_hasht_entry_st *entry = hashtable->entries[slot];

  if (NULL == entry){
    hashtable->entries[slot] = json_hashtable_pair(kKey, item);
    return hashtable->entries[slot]->item;
  }

  json_hasht_entry_st *entry_prev;
  while (NULL != entry){
    if (STREQ(entry->key, kKey)){
      return entry->item;
    }
    entry_prev = entry;
    entry = entry->next;
  }

  entry_prev->next = json_hashtable_pair(kKey, item);

  return item;
}
