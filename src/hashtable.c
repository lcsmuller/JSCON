/*
 * Copyright (c) 2020 Lucas Müller
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <string.h>
#include <assert.h>

#include "libjscon.h"

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

void
hashtable_remove(hashtable_st *hashtable, const char *kKey)
{
  if (0 == hashtable->num_bucket) return;

  size_t slot = hashtable_genhash(kKey, hashtable->num_bucket);

  hashtable_entry_st *entry = hashtable->bucket[slot];
  hashtable_entry_st *entry_prev = NULL;
  while (NULL != entry){
    if (STREQ(entry->key, kKey)){
      if (NULL != entry_prev){
        entry_prev->next = entry->next; 
      } else {
        hashtable->bucket[slot] = entry->next; 
      }
      free(entry->key);
      entry->key = NULL;

      free(entry);
      entry = NULL;
      return;
    }
    entry_prev = entry;
    entry = entry->next;
  }
}

//* reentrant hashtable linking function */
void
jscon_hashtable_link_r(jscon_item_st *item, jscon_htwrap_st **p_last_accessed_htwrap)
{
  assert(IS_COMPOSITE(item));

  jscon_htwrap_st *last_accessed_htwrap = *p_last_accessed_htwrap;
  if (NULL != last_accessed_htwrap){
    last_accessed_htwrap->next = &item->comp->htwrap; //item is not root
  }

  last_accessed_htwrap = &item->comp->htwrap;
  last_accessed_htwrap->root = item;

  *p_last_accessed_htwrap = last_accessed_htwrap;
}

void
jscon_hashtable_build(jscon_item_st *item)
{
  assert(IS_COMPOSITE(item));

  hashtable_build(item->comp->htwrap.hashtable, item->comp->num_branch * 1.3); //30% size increase to account for future expansions

  for (int i=0; i < item->comp->num_branch; ++i){
    jscon_hashtable_set(item->comp->branch[i]->key, item->comp->branch[i]);
  }
}

jscon_item_st*
jscon_hashtable_get(const char *kKey, jscon_item_st *item)
{
  if (!IS_COMPOSITE(item)) return NULL;

  jscon_htwrap_st *htwrap = &item->comp->htwrap;
  return hashtable_get(htwrap->hashtable, kKey);
}

jscon_item_st*
jscon_hashtable_set(const char *kKey, jscon_item_st *item)
{
  assert(!IS_ROOT(item));

  jscon_htwrap_st *htwrap = &item->parent->comp->htwrap;
  return hashtable_set(htwrap->hashtable, kKey, item);
}
