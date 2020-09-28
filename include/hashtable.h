#ifndef JSCON_HASHTABLE_H
#define JSCON_HASHTABLE_H

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

/* JSCON SPECIFIC IMPLEMENTATIONS */
struct jscon_item_s; //forward declaration

typedef struct jscon_htwrap_s {
  hashtable_st *hashtable;

  struct jscon_item_s *root; //points to root item (object or array)
  struct jscon_htwrap_s *next; //points to linked hashtable
} jscon_htwrap_st;

void jscon_hashtable_link_r(struct jscon_item_s *item, jscon_htwrap_st **last_accessed_htwrap);
void jscon_hashtable_build(struct jscon_item_s *item);
struct jscon_item_s* jscon_hashtable_get(const char *kKey, struct jscon_item_s *item);
struct jscon_item_s* jscon_hashtable_set(const char *kKey, struct jscon_item_s *item);

#endif
