#include <string.h>
#include <assert.h>

#include "libjsonc.h"

jsonc_hasht_st*
jsonc_hashtable_init()
{
  jsonc_hasht_st *new_hashtable = calloc(1, sizeof *new_hashtable);
  assert(NULL != new_hashtable);

  return new_hashtable;
}

void
jsonc_hashtable_destroy(jsonc_hasht_st *hashtable)
{
  for (size_t i=0; i < hashtable->num_bucket; ++i){
    if (NULL == hashtable->bucket[i])
      continue;

    jsonc_hasht_entry_st *entry = hashtable->bucket[i];
    jsonc_hasht_entry_st *entry_prev;
    while (NULL != entry){
      entry_prev = entry;
      entry = entry->next;

      free(entry_prev->key);
      entry_prev->key = NULL;
      free(entry_prev);
      entry_prev = NULL;
    }
  }
  free(hashtable->bucket);
  hashtable->bucket = NULL;
  free(hashtable);
  hashtable = NULL;
}

//* reentrant hashtable linking function */
void
jsonc_hashtable_link_r(jsonc_item_st *item, jsonc_hasht_st **p_last_accessed_hashtable)
{
  jsonc_hasht_st *last_accessed_hashtable = *p_last_accessed_hashtable;
  if (NULL != last_accessed_hashtable){
    last_accessed_hashtable->next = item->hashtable; //item is not root
  }

  last_accessed_hashtable = item->hashtable;
  last_accessed_hashtable->root = item;

  *p_last_accessed_hashtable = last_accessed_hashtable;
}

void
jsonc_hashtable_build(jsonc_item_st *item)
{
  assert(item->type & (JSONC_OBJECT|JSONC_ARRAY));

  jsonc_hasht_st *hashtable = item->hashtable;
  hashtable->num_bucket = item->num_branch * 1.3;

  hashtable->bucket = calloc(1, hashtable->num_bucket * sizeof *hashtable->bucket);
  assert(NULL != hashtable->bucket);

  for (int i=0; i < item->num_branch; ++i){
    jsonc_hashtable_set(item->branch[i]->key, item->branch[i]);
  }
}

static size_t
jsonc_generate_hash(const char *kKey, const size_t kNum_bucket)
{
  size_t slot = 0;
  size_t key_len = strlen(kKey);

  for (size_t i=0; i < key_len; ++i){
    slot = slot * 37 + kKey[i];
  }

  slot %= kNum_bucket;

  return slot;
}

static jsonc_hasht_entry_st*
jsonc_hashtable_pair(const char *kKey, jsonc_item_st* item)
{
  jsonc_hasht_entry_st *entry = calloc(1, sizeof *entry);
  assert(NULL != entry);

  entry->key = item->key;
  entry->item = item;

  return entry;
}

jsonc_item_st*
jsonc_hashtable_get(const char *kKey, jsonc_item_st *root)
{
  if (!(root->type & (JSONC_OBJECT|JSONC_ARRAY)))
    return NULL;

  jsonc_hasht_st *hashtable = root->hashtable;
  if (0 == hashtable->num_bucket)
    return NULL;

  size_t slot = jsonc_generate_hash(kKey, hashtable->num_bucket);

  jsonc_hasht_entry_st *entry = hashtable->bucket[slot];
  while (NULL != entry){ //try to find key and return it
    if (STREQ(entry->key, kKey)){
      return entry->item;
    }
    entry = entry->next;
  }

  return NULL;
}

jsonc_item_st*
jsonc_hashtable_set(const char *kKey, jsonc_item_st *item)
{
  assert((item->parent->type) & (JSONC_OBJECT|JSONC_ARRAY));

  jsonc_hasht_st *hashtable = item->parent->hashtable;
  size_t slot = jsonc_generate_hash(kKey, hashtable->num_bucket);

  jsonc_hasht_entry_st *entry = hashtable->bucket[slot];

  if (NULL == entry){
    hashtable->bucket[slot] = jsonc_hashtable_pair(kKey, item);
    return hashtable->bucket[slot]->item;
  }

  jsonc_hasht_entry_st *entry_prev;
  while (NULL != entry){
    if (STREQ(entry->key, kKey)){
      return entry->item;
    }
    entry_prev = entry;
    entry = entry->next;
  }

  entry_prev->next = jsonc_hashtable_pair(kKey, item);

  return item;
}
