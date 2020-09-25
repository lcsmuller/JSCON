#ifndef JSONC_HASHTABLE_H
#define JSONC_HASHTABLE_H

/* GENERAL USE IMPLEMENTATIONS */
typedef struct hashtable_entry_s {
  char *key; //this entry key tag
  void *value; //this entry value
  struct hashtable_entry_s *next; //next entry pointer for when keys don't match
} hashtable_entry_st;

typedef struct hashtable_s {
  hashtable_entry_st **bucket;
  size_t num_bucket;
} hashtable_st;

hashtable_st* hashtable_init();
void hashtable_destroy(hashtable_st *hashtable);
void hashtable_build(hashtable_st *hashtable, const size_t kNum_index);
hashtable_entry_st *hashtable_get_entry(hashtable_st *hashtable, const char *kKey);
void *hashtable_get(hashtable_st *hashtable, const char *kKey);
void *hashtable_set(hashtable_st *hashtable, const char *kKey, const void *kValue);

/* JSONC SPECIFIC IMPLEMENTATIONS */
struct jsonc_item_s; //forward declaration

typedef struct jsonc_htwrap_s {
  hashtable_st *hashtable;

  struct jsonc_item_s *root; //points to root item (object or array)
  struct jsonc_htwrap_s *next; //points to linked hashtable
} jsonc_htwrap_st;

void jsonc_hashtable_destroy(jsonc_htwrap_st *htwrap);
void jsonc_hashtable_link_r(struct jsonc_item_s *item, jsonc_htwrap_st **last_accessed_htwrap);
void jsonc_hashtable_build(struct jsonc_item_s *item);
struct jsonc_item_s* jsonc_hashtable_get(const char *kKey, struct jsonc_item_s *item);
struct jsonc_item_s* jsonc_hashtable_set(const char *kKey, struct jsonc_item_s *item);

#endif
