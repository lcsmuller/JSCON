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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libjscon.h>

#include "jscon-common.h"
#include "debug.h"


jscon_item_st*
jscon_null(const char *key)
{
  jscon_item_st *new_item = malloc(sizeof *new_item);
  if (NULL == new_item) return NULL;

  if (NULL != key){
    new_item->key = strdup(key);
    if (NULL == new_item->key){
      free(new_item);
      return NULL;
    }
  } else {
    new_item->key = NULL;
  }

  new_item->parent = NULL;
  new_item->type = JSCON_NULL;

  return new_item;
}

jscon_item_st*
jscon_boolean(const char *key, bool boolean)
{
  jscon_item_st *new_item = malloc(sizeof *new_item);
  if (NULL == new_item) return NULL;

  if (NULL != key){
    new_item->key = strdup(key);
    if (NULL == new_item->key){
      free(new_item);
      return NULL;
    }
  } else {
    new_item->key = NULL;
  }

  new_item->parent = NULL;
  new_item->type = JSCON_BOOLEAN;
  new_item->boolean = boolean;

  return new_item;
}

jscon_item_st*
jscon_integer(const char *key, long long i_number)
{
  jscon_item_st *new_item = malloc(sizeof *new_item);
  if (NULL == new_item) return NULL;

  if (NULL != key){
    new_item->key = strdup(key);
    if (NULL == new_item->key){
      free(new_item);
      return NULL;
    }
  } else {
    new_item->key = NULL;
  }

  new_item->parent = NULL;
  new_item->type = JSCON_INTEGER;
  new_item->i_number = i_number;

  return new_item;
}

jscon_item_st*
jscon_double(const char *key, double d_number)
{
  jscon_item_st *new_item = malloc(sizeof *new_item);
  if (NULL == new_item) return NULL;

  if (NULL != key){
    new_item->key = strdup(key);
    if (NULL == new_item->key){
      free(new_item);
      return NULL;
    }
  } else {
    new_item->key = NULL;
  }

  new_item->parent = NULL;
  new_item->type = JSCON_DOUBLE;
  new_item->d_number = d_number;

  return new_item;
}

jscon_item_st*
jscon_number(const char *key, double d_number)
{
  return DOUBLE_IS_INTEGER(d_number)
          ? jscon_integer(key, (long long)d_number)
          : jscon_double(key, d_number);
}

jscon_item_st*
jscon_string(const char *key, char *string)
{
  if (NULL == string) return jscon_null(key);

  jscon_item_st *new_item = malloc(sizeof *new_item);
  if (NULL == new_item) return NULL;

  if (NULL != key){
    new_item->key = strdup(key);
    if (NULL == new_item->key) goto free_key;
  } else {
    new_item->key = NULL;
  }

  new_item->parent = NULL;
  new_item->type = JSCON_STRING;

  new_item->string = strdup(string);
  if (NULL == new_item->string) goto free_string;

  return new_item;

free_string:
  free(new_item->key);
free_key:
  free(new_item);

  return NULL;
}

jscon_list_st*
jscon_list_init()
{
  jscon_list_st *new_list = malloc(sizeof *new_list);
  if (NULL == new_list) return NULL;

  new_list->first = malloc(sizeof *new_list->first);
  if (NULL == new_list->first) goto free_first;

  new_list->last = malloc(sizeof *new_list->last);
  if (NULL == new_list->last) goto free_last;

  new_list->first->next = new_list->last;
  new_list->first->prev = NULL;

  new_list->last->prev = new_list->first;
  new_list->last->next = NULL;

  new_list->num_node = 0;

  return new_list;

free_last:
  free(new_list->first);
free_first:
  free(new_list);

  return NULL;
}

void
jscon_list_destroy(jscon_list_st *list)
{
  struct jscon_node_s *node = list->first->next;

  for (size_t i=0; i < list->num_node; ++i){
    node = node->next;

    jscon_destroy(node->item);
    node->item = NULL;

    free(node->prev);
    node->prev = NULL;
  }
  free(list->first);
  list->first = NULL;

  free(list->last);
  list->last = NULL;

  free(list);
  list = NULL;
}

void
jscon_list_append(jscon_list_st *list, jscon_item_st *item)
{
  DEBUG_ASSERT(NULL != item, "NULL Item");

  struct jscon_node_s *new_node = malloc(sizeof *new_node);
  DEBUG_ASSERT(NULL != new_node, "Out of memory");

  new_node->item = item;

  ++list->num_node;

  new_node->next = list->last;
  new_node->prev = list->last->prev;

  list->last->prev->next = new_node;
  list->last->prev = new_node;
}

/* @todo add condition to stop if after linking item hwrap to a already
    formed composite. This is far from ideal, I should probably try to
    make this iteratively just so that I have a better control on whats
    going on, early break conditions etc. As it is now it will keep on
    going deeper and deeper recursively, even if not necessary */
static void
_jscon_htwrap_link_preorder(jscon_item_st *item, jscon_htwrap_st **last_accessed_htwrap)
{
  Jscon_htwrap_link_r(item, last_accessed_htwrap);

  for (size_t i=0; i < jscon_size(item); ++i){
    if (IS_COMPOSITE(item->comp->branch[i])){
      _jscon_htwrap_link_preorder(item->comp->branch[i], last_accessed_htwrap);
    }
  }
}

inline static jscon_item_st*
_jscon_composite(const char *key, jscon_list_st *list, enum jscon_type type)
{
  jscon_item_st *new_item = malloc(sizeof *new_item);
  if (NULL == new_item) return NULL;

  new_item->parent = NULL;
  new_item->type = type;
  new_item->comp = calloc(1, sizeof *new_item->comp);
  if (NULL == new_item->comp) goto comp_free;

  new_item->comp->htwrap = Jscon_htwrap_init();

  if (NULL == list){ //empty object/array
    new_item->comp->branch = malloc(sizeof(jscon_item_st*));
    if (NULL == new_item->comp->branch) goto branch_free;
  } else {
    new_item->comp->branch = malloc((1+list->num_node) * sizeof(jscon_item_st*));
    if (NULL == new_item->comp->branch) goto branch_free;

    struct jscon_node_s *node = list->first->next;
    for (size_t i=0; i < list->num_node; ++i){
      new_item->comp->branch[i] = node->item;
      node->item->parent = new_item;

      node = node->next;

      free(node->prev);
      node->prev = NULL;
    }

    new_item->comp->num_branch = list->num_node;

    list->num_node = 0;

    list->first->next = list->last;
    list->first->prev = NULL;

    list->last->prev = list->first;
    list->last->next = NULL;
  }

  if (NULL != key){
    new_item->key = strdup(key);
    if (NULL == new_item->key) goto key_free;
  } else {
    new_item->key = NULL;
  }

  jscon_htwrap_st *last_accessed_htwrap = NULL;
  _jscon_htwrap_link_preorder(new_item, &last_accessed_htwrap);

  Jscon_htwrap_build(new_item);

  return new_item;


key_free:
  free(new_item->key);
  free(new_item->comp->branch);
branch_free:
  free(new_item->comp);
comp_free:
  free(new_item);

  return NULL;
}

jscon_item_st*
jscon_object(const char *key, jscon_list_st *list){
  return _jscon_composite(key, list, JSCON_OBJECT);
}

jscon_item_st*
jscon_array(const char *key, jscon_list_st *list){
  return _jscon_composite(key, list, JSCON_ARRAY);
}

/* total branches the item possess, returns 0 if item type is primitive */
size_t
jscon_size(const jscon_item_st *item){
  return IS_COMPOSITE(item) ? item->comp->num_branch : 0;
} 

/* get the last htwrap relative to the item */
static jscon_htwrap_st*
_jscon_get_last_htwrap(jscon_item_st *item)
{
  DEBUG_ASSERT(IS_COMPOSITE(item), "Item is not an Object or Array");

  size_t item_depth = jscon_get_depth(item);

  /* get the deepest nested composite relative to item */
  jscon_htwrap_st *htwrap_last = item->comp->htwrap;
  while(NULL != htwrap_last->next && item_depth < jscon_get_depth(htwrap_last->next->root)){
    htwrap_last = htwrap_last->next;
  }

  return htwrap_last;
}

/* remake hashtable on functions that deal with increasing branches */
/* @todo move to jscon-hashtable.c */
static void
_jscon_hashtable_remake(jscon_item_st *item)
{
  hashtable_destroy(item->comp->htwrap->hashtable);

  item->comp->htwrap->hashtable = hashtable_init();
  DEBUG_ASSERT(NULL != item->comp->htwrap->hashtable, "Out of memory");

  Jscon_htwrap_build(item);
}

jscon_item_st*
jscon_append(jscon_item_st *item, jscon_item_st *new_branch)
{
  DEBUG_ASSERT(IS_COMPOSITE(item), "Item is not an Object or Array");

  if (new_branch == item){
    DEBUG_ASSERT(NULL != jscon_get_key(item), "Can't perform circular append of item without a key");
    new_branch = jscon_clone(item);
    if (NULL == new_branch) return NULL;
  }

  ++item->comp->num_branch;
  /* realloc parent references to match new size */
  jscon_item_st **tmp = realloc(item->comp->branch, jscon_size(item) * sizeof(jscon_item_st*));
  if (NULL == tmp) return NULL;

  item->comp->branch = tmp;

  item->comp->branch[jscon_size(item)-1] = new_branch;
  new_branch->parent = item;

  if (jscon_size(item) <= item->comp->htwrap->hashtable->num_bucket){
    Jscon_htwrap_set(jscon_get_key(new_branch), new_branch);
  } else {
    _jscon_hashtable_remake(item);
  }

  if (IS_PRIMITIVE(new_branch)) return new_branch;

  /* get the last htwrap relative to item */
  jscon_htwrap_st *htwrap_last = _jscon_get_last_htwrap(item);
  htwrap_last->next = new_branch->comp->htwrap;
  new_branch->comp->htwrap->prev = htwrap_last;

  return new_branch;
}

jscon_item_st*
jscon_dettach(jscon_item_st *item)
{
  //can't dettach root from nothing
  if (NULL == item || IS_ROOT(item)) return item;

  /* get the item index reference from its parent */
  jscon_item_st *item_parent = item->parent;

  /* dettach the item from its parent and reorder keys */
  for (size_t i = jscon_get_index(item_parent, item->key); i < jscon_size(item_parent)-1; ++i){
    item_parent->comp->branch[i] = item_parent->comp->branch[i+1]; 
  }
  item_parent->comp->branch[jscon_size(item_parent)-1] = NULL;
  --item_parent->comp->num_branch;

  /* realloc parent references to match new size */
  jscon_item_st **tmp = realloc(item_parent->comp->branch, (1+jscon_size(item_parent)) * sizeof(jscon_item_st*));
  if (NULL == tmp) return NULL;

  item_parent->comp->branch = tmp;

  /* parent hashtable has to be remade, to match reordered keys */
  _jscon_hashtable_remake(item_parent);

  /* get the immediate previous htwrap relative to the item */
  jscon_htwrap_st *htwrap_prev = item->comp->htwrap->prev;
  /* get the last htwrap relative to item */
  jscon_htwrap_st *htwrap_last = _jscon_get_last_htwrap(item);

  /* remove tree references to the item */
  htwrap_prev->next = htwrap_last->next;

  /* remove item references to the tree */
  item->parent = NULL;
  htwrap_last->next = NULL;
  item->comp->htwrap->prev = NULL;

  return item;
}

void
jscon_delete(jscon_item_st *item, const char *key)
{
  jscon_item_st *branch = jscon_get_branch(item, key);
  if (NULL == branch) return;

  jscon_dettach(branch); 
  jscon_destroy(branch);
}


/* reentrant function, works similar to strtok. the starting point is set
    by doing the function call before the main iteration loop, then
    consecutive function calls inside the loop will continue the iteration
    from then on if item is then set to NULL.
    
    p_current_item allows for thread safety reentrancy, it should not be
    modified outside of this routine*/
jscon_item_st*
jscon_iter_composite_r(jscon_item_st *item, jscon_item_st **p_current_item)
{
  /* if item is not NULL, set p_current_item to item, otherwise
      fetch next iteration */ 
  if (NULL != item){
    DEBUG_ASSERT(IS_COMPOSITE(item), "Item is not an Object or Array");
    *p_current_item = item;
    return item;
  }

  /* if p_current_item is NULL, it needs to be set back with item parameter */
  if (NULL == *p_current_item) return NULL;

  /* get next htwrap in line, if NULL it means there are no more
      composite datatype items to iterate through */
  jscon_htwrap_st *next_htwrap = (*p_current_item)->comp->htwrap->next;
  if (NULL == next_htwrap){
    *p_current_item = NULL;
    return NULL;
  }

  *p_current_item = next_htwrap->root;

  return *p_current_item;
}

/* return next (not yet accessed) item, by using item->comp->last_accessed_branch as the branch index */
static inline jscon_item_st*
_jscon_push(jscon_item_st *item)
{
  DEBUG_ASSERT(IS_COMPOSITE(item), "Item is not an Object or Array");
  DEBUG_ASSERT(item->comp->last_accessed_branch < jscon_size(item), "Overflow, trying to access forbidden memory");

  ++item->comp->last_accessed_branch; //update last_accessed_branch to next
  jscon_item_st *next_item = item->comp->branch[item->comp->last_accessed_branch-1];

  //resets incase its already set because of a different run
  if (IS_COMPOSITE(next_item)){
    next_item->comp->last_accessed_branch = 0;
  }

  return next_item; //return item from next branch in line
}

static inline jscon_item_st*
_jscon_pop(jscon_item_st *item)
{
  //resets object's last_accessed_branch
  if (IS_COMPOSITE(item)){
    item->comp->last_accessed_branch = 0;
  }

  return item->parent; //return item's parent
}

/* this will simulate tree preorder traversal iteratively, by using 
    item->comp->last_accessed_branch like a stack frame. under no circumstance 
    should you modify last_accessed_branch value directly */
jscon_item_st*
jscon_iter_next(jscon_item_st *item)
{
  if (NULL == item) return NULL;

  /* resets root's last_accessed_branch in case its set from a different run */
  if (IS_COMPOSITE(item)){
    item->comp->last_accessed_branch = 0;
  }

  /* item is a leaf, fetch parent until found a item with any branch
      left to be accessed */
  if (IS_LEAF(item)){
    /* fetch parent until a item with available branch is found */
    do {
      item = _jscon_pop(item);
      if ((NULL == item) || (0 == item->comp->last_accessed_branch)){
        return NULL; //return NULL if exceeded root
      }
     } while (jscon_size(item) == item->comp->last_accessed_branch);
  }

  return _jscon_push(item);
}

/* This is not the most effective way to clone a item, but it is
    the most reliable, because it automatically accounts for any
    new feature I might add in the future. By first stringfying the
    (to be cloned) jscon_item and then parsing the resulting string into
    a new (clone) jscon_item, it's guaranteed that it will be a perfect 
    clone, with its own addressed htwrap, strings, etc */
jscon_item_st*
jscon_clone(jscon_item_st *item)
{
  if (NULL == item) return NULL;

  char *tmp_buffer = jscon_stringify(item, JSCON_ANY);
  jscon_item_st *clone = jscon_parse(tmp_buffer);
  free(tmp_buffer);

  if (NULL != item->key){
    clone->key = strdup(item->key);
    if (NULL == clone->key){
      jscon_destroy(clone);
      clone = NULL;
    }
  }

  return clone;
}

char*
jscon_typeof(const jscon_item_st *item)
{
  switch (item->type){
    case JSCON_DOUBLE: return "Double";
    case JSCON_INTEGER: return "Integer";
    case JSCON_STRING: return "String";
    case JSCON_NULL: return "Null";
    case JSCON_BOOLEAN: return "Boolean";
    case JSCON_OBJECT: return "Object";
    case JSCON_ARRAY: return "Array";
    case JSCON_UNDEFINED: return "Undefined";
    default: return "NaN";
  }
}

char*
jscon_strdup(const jscon_item_st *item)
{
  char *src = jscon_get_string(item);
  if (NULL == src) return NULL;

  char *dest = strdup(src);
  return dest;
}

char*
jscon_strcpy(char *dest, const jscon_item_st *item)
{
  char *src = jscon_get_string(item);
  if (NULL == src) return NULL;

  ssize_t ret = strscpy(dest, src, strlen(src));
  DEBUG_ASSERT(ret != -1, "Overflow occured");

  return dest;
}

int
jscon_typecmp(const jscon_item_st *item, const enum jscon_type type){
  return item->type & type; //BITMASK AND
}

int
jscon_keycmp(const jscon_item_st *item, const char *key){
  return (NULL != jscon_get_key(item)) ? STREQ(item->key, key) : 0;
}

int
jscon_doublecmp(const jscon_item_st *item, const double d_number){
  DEBUG_ASSERT(JSCON_DOUBLE == item->type, "Item type is not a Double");

  return item->d_number == d_number;
}

int
jscon_intcmp(const jscon_item_st *item, const long long i_number){
  DEBUG_ASSERT(JSCON_INTEGER == item->type, "Item type is not a Integer");

  return item->i_number == i_number;
}

size_t
jscon_get_depth(jscon_item_st *item)
{
  size_t depth = 0;
  while (!IS_ROOT(item)){
    item = item->parent;
    ++depth;
  }

  return depth;
}

jscon_item_st*
jscon_get_root(jscon_item_st *item)
{
  while (!IS_ROOT(item)){
    item = jscon_get_parent(item);
  }

  return item;
}


/* get item branch with given key */
jscon_item_st*
jscon_get_branch(jscon_item_st *item, const char *key)
{
  DEBUG_ASSERT(IS_COMPOSITE(item), "Item is not an Object or Array");
  /* search for entry with given key at item's htwrap,
    and retrieve found or not found(NULL) item */
  return Jscon_htwrap_get(key, item);
}

/* get origin item sibling by the relative index, if origin item is of index 3 (from parent's perspective), and relative index is -1, then this function will return item of index 2 (from parent's perspective) */
jscon_item_st*
jscon_get_sibling(const jscon_item_st* origin, const size_t kRelative_index)
{
  DEBUG_ASSERT(!IS_ROOT(origin), "Origin is root (has no siblings)");

  const jscon_item_st* kParent = jscon_get_parent(origin);

  //get parent's branch index of the origin item
  size_t origin_index= jscon_get_index(kParent, origin->key);

  /* if relative index given doesn't exceed kParent branch amount,
    or dropped below 0, return branch at given relative index */
  if ((0 <= (int)(origin_index + kRelative_index)) && jscon_size(kParent) > (origin_index + kRelative_index)){
    return jscon_get_byindex(kParent, origin_index + kRelative_index);
  }

  return NULL;
}

/* return parent */
jscon_item_st*
jscon_get_parent(const jscon_item_st *item){
  return _jscon_pop((jscon_item_st*)item);
}

jscon_item_st*
jscon_get_byindex(const jscon_item_st *item, const size_t index)
{
  DEBUG_ASSERT(IS_COMPOSITE(item), "Item is not an Object or Array");
  return (index < jscon_size(item)) ? item->comp->branch[index] : NULL;
}

/* returns -1 if item not found */
long
jscon_get_index(const jscon_item_st *item, const char *key)
{
  DEBUG_ASSERT(IS_COMPOSITE(item), "Item is not an Object or Array");

  jscon_item_st *lookup_item = Jscon_htwrap_get(key, (jscon_item_st*)item);

  if (NULL == lookup_item) return -1;

  /* @todo can this be done differently? */
  for (size_t i=0; i < jscon_size(item); ++i){
    if (lookup_item == item->comp->branch[i]){
      return i;
    }
  }

  DEBUG_ERR("Item exists in hashtable but is not referenced by parent");
  return -1;
}

enum jscon_type
jscon_get_type(const jscon_item_st *item){
  return item->type;
}

char*
jscon_get_key(const jscon_item_st *item){
  return item->key;
}

bool
jscon_get_boolean(const jscon_item_st *item)
{
  if (NULL == item || JSCON_NULL == item->type) return false;

  DEBUG_ASSERT(JSCON_BOOLEAN == item->type, "Item type is not a Boolean");
  return item->boolean;
}

char*
jscon_get_string(const jscon_item_st *item)
{
  if (NULL == item || JSCON_NULL == item->type) return NULL;

  DEBUG_ASSERT(JSCON_STRING == item->type, "Item type is not a String");
  return item->string;
}

double
jscon_get_double(const jscon_item_st *item)
{
  if (NULL == item || JSCON_NULL == item->type) return 0.0;

  DEBUG_ASSERT(JSCON_DOUBLE == item->type, "Item type is not a Double");
  return item->d_number;
}

long long
jscon_get_integer(const jscon_item_st *item)
{
  if (NULL == item || JSCON_NULL == item->type) return 0;

  DEBUG_ASSERT(JSCON_INTEGER == item->type, "Item type is not a Integer");
  return item->i_number;
}
