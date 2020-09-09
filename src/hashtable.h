#ifndef JSONC_HASHTABLE_H
#define JSONC_HASHTABLE_H

struct jsonc_item_s; //forward declaration


typedef struct jsonc_hasht_entry_s {
  char *key;
  struct jsonc_item_s *item; //this entry's value
  struct jsonc_hasht_entry_s *next; //next entry pointer for when keys don't match
} jsonc_hasht_entry_st;

typedef struct jsonc_hasht_s {
  struct jsonc_item_s *root;
  struct jsonc_hasht_s *next; //@todo: make this point to differente buckets instead ?
  jsonc_hasht_entry_st **bucket;
  size_t num_bucket;
} jsonc_hasht_st;


jsonc_hasht_st* jsonc_hashtable_init();
void jsonc_hashtable_destroy(jsonc_hasht_st *hashtable);
void jsonc_hashtable_link_r(struct jsonc_item_s *item, jsonc_hasht_st **last_accessed_hashtable);
void jsonc_hashtable_build(struct jsonc_item_s *item);
struct jsonc_item_s* jsonc_hashtable_get(const char *kKey, struct jsonc_item_s *item);
struct jsonc_item_s* jsonc_hashtable_set(const char *kKey, struct jsonc_item_s *item);

#endif
