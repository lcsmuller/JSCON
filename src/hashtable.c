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
  for (size_t i=0; i < hashtable->num_bucket; ++i){
    if (NULL == hashtable->bucket[i])
      continue;

    json_hasht_entry_st *entry = hashtable->bucket[i];
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
  free(hashtable->bucket);
  hashtable->bucket = NULL;
  free(hashtable);
  hashtable = NULL;
}

//* reentrant hashtable linking function */
void
json_hashtable_link_r(json_item_st *item, json_hasht_st **p_last_accessed_hashtable)
{
  json_hasht_st *last_accessed_hashtable = *p_last_accessed_hashtable;
  if (NULL != last_accessed_hashtable){
    last_accessed_hashtable->next = item->hashtable; //item is not root
  }

  last_accessed_hashtable = item->hashtable;
  last_accessed_hashtable->root = item;

  *p_last_accessed_hashtable = last_accessed_hashtable;
}

void
json_hashtable_build(json_item_st *item)
{
  assert(item->type & (JSON_OBJECT|JSON_ARRAY));

  json_hasht_st *hashtable = item->hashtable;
  hashtable->num_bucket = item->num_branch * 1.3;

  hashtable->bucket = calloc(1, hashtable->num_bucket * sizeof *hashtable->bucket);
  assert(NULL != hashtable->bucket);

  for (int i=0; i < item->num_branch; ++i){
    json_hashtable_set(item->branch[i]->key, item->branch[i]);
  }
}

static size_t
json_generate_hash(const json_string_kt kKey, const size_t kNum_bucket)
{
  size_t slot = 0;
  size_t key_len = strlen(kKey);

  for (size_t i=0; i < key_len; ++i){
    slot = slot * 37 + kKey[i];
  }

  slot %= kNum_bucket;

  return slot;
}

static json_hasht_entry_st*
json_hashtable_pair(const json_string_kt kKey, json_item_st* item)
{
  json_hasht_entry_st *entry = calloc(1, sizeof *entry);
  assert(NULL != entry);

  entry->key = item->key;
  entry->item = item;

  return entry;
}

json_item_st*
json_hashtable_get(const json_string_kt kKey, json_item_st *root)
{
  if (!(root->type & (JSON_OBJECT|JSON_ARRAY)))
    return NULL;

  json_hasht_st *hashtable = root->hashtable;
  if (0 == hashtable->num_bucket)
    return NULL;

  size_t slot = json_generate_hash(kKey, hashtable->num_bucket);

  json_hasht_entry_st *entry = hashtable->bucket[slot];
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

  json_hasht_st *hashtable = item->parent->hashtable;
  size_t slot = json_generate_hash(kKey, hashtable->num_bucket);

  json_hasht_entry_st *entry = hashtable->bucket[slot];

  if (NULL == entry){
    hashtable->bucket[slot] = json_hashtable_pair(kKey, item);
    return hashtable->bucket[slot]->item;
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
