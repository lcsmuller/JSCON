#include <stdio.h>
#include <stdlib.h>

#include <libjscon.h>

#include "jscon-common.h"
#include "debug.h"

jscon_htwrap_st*
Jscon_htwrap_init()
{
  jscon_htwrap_st *new_htwrap = calloc(1, sizeof *new_htwrap);
  DEBUG_ASSERT(NULL != new_htwrap, "Out of memory");

  new_htwrap->hashtable = hashtable_init();

  return new_htwrap;
}

void
Jscon_htwrap_destroy(jscon_htwrap_st *htwrap)
{
  hashtable_destroy(htwrap->hashtable);
  free(htwrap);
}

//* reentrant hashtable linking function */
void
Jscon_htwrap_link_r(jscon_item_st *item, jscon_htwrap_st **p_last_accessed_htwrap)
{
  DEBUG_ASSERT(IS_COMPOSITE(item), "Item is not an Object or Array");

  jscon_htwrap_st *last_accessed_htwrap = *p_last_accessed_htwrap;
  if (NULL != last_accessed_htwrap){
    last_accessed_htwrap->next = item->comp->htwrap; //item is not root
    item->comp->htwrap->prev = last_accessed_htwrap;
  }

  last_accessed_htwrap = item->comp->htwrap;

  *p_last_accessed_htwrap = last_accessed_htwrap;
}

void
Jscon_htwrap_build(jscon_item_st *item)
{
  DEBUG_ASSERT(IS_COMPOSITE(item), "Item is not an Object or Array");

  hashtable_build(item->comp->htwrap->hashtable, 2 + jscon_size(item) * 1.3); //30% size increase to account for future expansions, and a default bucket size of 2

  item->comp->htwrap->root = item;

  for (size_t i=0; i < jscon_size(item); ++i){
    Jscon_htwrap_set(item->comp->branch[i]->key, item->comp->branch[i]);
  }
}

jscon_item_st*
Jscon_htwrap_get(const char *key, jscon_item_st *item)
{
  if (!IS_COMPOSITE(item)) return NULL;

  jscon_htwrap_st *htwrap = item->comp->htwrap;
  return hashtable_get(htwrap->hashtable, key);
}

jscon_item_st*
Jscon_htwrap_set(const char *key, jscon_item_st *item)
{
  DEBUG_ASSERT(!IS_ROOT(item), "Can't add to parent hashtable if Item is root");

  jscon_htwrap_st *htwrap = item->parent->comp->htwrap;
  return hashtable_set(htwrap->hashtable, key, item);
}
