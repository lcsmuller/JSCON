#include <string.h>
#include <assert.h>

#include "libjsonc.h"

hashtable_st*
hashtable_init()
{
  hashtable_st *new_hashtable = calloc(1, sizeof *new_hashtable);
  assert(NULL != new_hashtable);

  return new_hashtable;
}

void
hashtable_destroy(hashtable_st *hashtable)
{
  for (size_t i=0; i < hashtable->num_bucket; ++i){
    if (NULL == hashtable->bucket[i])
      continue;

    hashtable_entry_st *entry = hashtable->bucket[i];
    hashtable_entry_st *entry_prev;
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

static size_t
hashtable_genhash(const char *kKey, const size_t kNum_bucket)
{
  size_t slot = 0;
  size_t key_len = strlen(kKey);

  //TODO: look for different algorithms, learn if there are any improvements
  for (size_t i=0; i < key_len; ++i){
    slot = slot * 37 + kKey[i];
  }

  slot %= kNum_bucket;

  return slot;
}

static hashtable_entry_st*
hashtable_pair(const char *kKey, const void *kValue)
{
  hashtable_entry_st *entry = calloc(1, sizeof *entry);
  assert(NULL != entry);

  entry->key = (char*)kKey;
  entry->value = (void*)kValue;

  return entry;
}

void
hashtable_build(hashtable_st *hashtable, const size_t kNum_index)
{
  hashtable->num_bucket = kNum_index;

  hashtable->bucket = calloc(1, hashtable->num_bucket * sizeof *hashtable->bucket);
  assert(NULL != hashtable->bucket);
}

hashtable_entry_st*
hashtable_get_entry(hashtable_st *hashtable, const char *kKey)
{
  if (0 == hashtable->num_bucket) return NULL;

  size_t slot = hashtable_genhash(kKey, hashtable->num_bucket);

  hashtable_entry_st *entry = hashtable->bucket[slot];
  while (NULL != entry){ //try to find key and return it
    if (STREQ(entry->key, kKey)){
      return entry;
    }
    entry = entry->next;
  }

  return NULL;
}

void*
hashtable_get(hashtable_st *hashtable, const char *kKey)
{
  hashtable_entry_st *entry = hashtable_get_entry(hashtable, kKey);
  return (NULL != entry) ? entry->value : NULL;
}

void*
hashtable_set(hashtable_st *hashtable, const char *kKey, const void *kValue)
{
  size_t slot = hashtable_genhash(kKey, hashtable->num_bucket);

  hashtable_entry_st *entry = hashtable->bucket[slot];

  if (NULL == entry){
    hashtable->bucket[slot] = hashtable_pair(kKey, kValue);
    return hashtable->bucket[slot]->value;
  }

  hashtable_entry_st *entry_prev;
  while (NULL != entry){
    if (STREQ(entry->key, kKey)){
      return entry->value;
    }
    entry_prev = entry;
    entry = entry->next;
  }

  entry_prev->next = hashtable_pair(kKey, kValue);

  return (void*)kValue;
}

//* reentrant hashtable linking function */
void
jsonc_hashtable_link_r(jsonc_item_st *item, jsonc_htwrap_st **p_last_accessed_htwrap)
{
  assert(IS_COMPOSITE(item));

  jsonc_htwrap_st *last_accessed_htwrap = *p_last_accessed_htwrap;
  if (NULL != last_accessed_htwrap){
    last_accessed_htwrap->next = &item->comp->htwrap; //item is not root
  }

  last_accessed_htwrap = &item->comp->htwrap;
  last_accessed_htwrap->root = item;

  *p_last_accessed_htwrap = last_accessed_htwrap;
}

void
jsonc_hashtable_build(jsonc_item_st *item)
{
  assert(IS_COMPOSITE(item));

  hashtable_build(item->comp->htwrap.hashtable, item->comp->num_branch * 1.3); //30% size increase to account for future expansions

  for (int i=0; i < item->comp->num_branch; ++i){
    jsonc_hashtable_set(item->comp->branch[i]->key, item->comp->branch[i]);
  }
}

jsonc_item_st*
jsonc_hashtable_get(const char *kKey, jsonc_item_st *item)
{
  if (!IS_COMPOSITE(item)) return NULL;

  jsonc_htwrap_st *htwrap = &item->comp->htwrap;
  return hashtable_get(htwrap->hashtable, kKey);
}

jsonc_item_st*
jsonc_hashtable_set(const char *kKey, jsonc_item_st *item)
{
  assert(!IS_ROOT(item));

  jsonc_htwrap_st *htwrap = &item->parent->comp->htwrap;
  return hashtable_set(htwrap->hashtable, kKey, item);
}
