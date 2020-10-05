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
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "libjscon.h"

/* TODO: change some of these functions to macros */

jscon_item_st*
jscon_null(const char *kKey)
{
  jscon_item_st *new_item = malloc(sizeof *new_item);
  assert(NULL != new_item);

  if (NULL != kKey){
    new_item->key = strdup(kKey);
    assert(NULL != new_item->key);
  } else {
    new_item->key = NULL;
  }

  new_item->parent = NULL;
  new_item->type = JSCON_NULL;

  return new_item;
}

jscon_item_st*
jscon_boolean(jscon_boolean_kt boolean, const char *kKey)
{
  jscon_item_st *new_item = malloc(sizeof *new_item);
  assert(NULL != new_item);

  if (NULL != kKey){
    new_item->key = strdup(kKey);
    assert(NULL != new_item->key);
  } else {
    new_item->key = NULL;
  }

  new_item->parent = NULL;
  new_item->type = JSCON_BOOLEAN;
  new_item->boolean = boolean;

  return new_item;
}

jscon_item_st*
jscon_integer(jscon_integer_kt i_number, const char *kKey)
{
  jscon_item_st *new_item = malloc(sizeof *new_item);
  assert(NULL != new_item);

  if (NULL != kKey){
    new_item->key = strdup(kKey);
    assert(NULL != new_item->key);
  } else {
    new_item->key = NULL;
  }

  new_item->parent = NULL;
  new_item->type = JSCON_INTEGER;
  new_item->i_number = i_number;

  return new_item;
}

jscon_item_st*
jscon_double(jscon_double_kt d_number, const char *kKey)
{
  jscon_item_st *new_item = malloc(sizeof *new_item);
  assert(NULL != new_item);

  if (NULL != kKey){
    new_item->key = strdup(kKey);
    assert(NULL != new_item->key);
  } else {
    new_item->key = NULL;
  }

  new_item->parent = NULL;
  new_item->type = JSCON_DOUBLE;
  new_item->d_number = d_number;

  return new_item;
}

jscon_item_st*
jscon_number(jscon_double_kt d_number, const char *kKey)
{
  return DOUBLE_IS_INTEGER(d_number)
          ? jscon_integer((jscon_integer_kt)d_number, kKey)
          : jscon_double(d_number, kKey);
}

jscon_item_st*
jscon_string(jscon_char_kt *string, const char *kKey)
{
  if (NULL == string) return jscon_null(kKey);

  jscon_item_st *new_item = malloc(sizeof *new_item);
  assert(NULL != new_item);

  if (NULL != kKey){
    new_item->key = strdup(kKey);
    assert(NULL != new_item->key);
  } else {
    new_item->key = NULL;
  }

  new_item->parent = NULL;
  new_item->type = JSCON_STRING;

  new_item->string = strdup(string);
  assert(NULL != new_item->string);

  return new_item;
}

jscon_list_st*
jscon_list_init()
{
  jscon_list_st *new_list = malloc(sizeof *new_list);
  assert(NULL != new_list);

  new_list->first = malloc(sizeof *new_list->first);
  assert(NULL != new_list->first);

  new_list->last = malloc(sizeof *new_list->last);
  assert(NULL != new_list->last);

  new_list->first->next = new_list->last;
  new_list->first->prev = NULL;

  new_list->last->prev = new_list->first;
  new_list->last->next = NULL;

  new_list->num_node = 0;

  return new_list;
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

/* TODO: return error value */
void
jscon_list_append(jscon_list_st *list, jscon_item_st *item)
{
  assert(NULL != item);

  struct jscon_node_s *new_node = malloc(sizeof *new_node);
  assert(NULL != new_node);

  new_node->item = item;

  ++list->num_node;

  new_node->next = list->last;
  new_node->prev = list->last->prev;

  list->last->prev->next = new_node;
  list->last->prev = new_node;
}

/* TODO: add condition to stop if after linking item hwrap to a already
    formed composite. This is far from ideal, I should probably try to
    make this iteratively just so that I have a better control on whats
    going on, early break conditions etc. As it is now it will keep on
    going deeper and deeper recursively, even if not necessary */
static void
jscon_htwrap_link_preorder(jscon_item_st *item, jscon_htwrap_st **last_accessed_htwrap)
{
  jscon_htwrap_link_r(item, last_accessed_htwrap);

  for (size_t i=0; i < jscon_size(item); ++i){
    if (IS_COMPOSITE(item->comp->branch[i])){
      jscon_htwrap_link_preorder(item->comp->branch[i], last_accessed_htwrap);
    }
  }
}

inline static jscon_item_st*
jscon_composite(jscon_list_st *list, const char *kKey, jscon_type_et type)
{
  jscon_item_st *new_item = malloc(sizeof *new_item);
  assert(NULL != new_item);

  if (NULL != kKey){
    new_item->key = strdup(kKey);
    assert(NULL != new_item->key);
  } else {
    new_item->key = NULL;
  }

  new_item->parent = NULL;
  new_item->type = type;
  new_item->comp = calloc(1, sizeof *new_item->comp);
  assert(NULL != new_item->comp);

  jscon_htwrap_init(&new_item->comp->htwrap);

  if (NULL == list){ //empty object/array
    new_item->comp->branch = malloc(sizeof(jscon_item_st*));
    assert(NULL != new_item->comp->branch);
  } else {
    new_item->comp->branch = malloc((1+list->num_node) * sizeof(jscon_item_st*));
    assert(NULL != new_item->comp->branch);

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

  jscon_htwrap_st *last_accessed_htwrap = NULL;
  jscon_htwrap_link_preorder(new_item, &last_accessed_htwrap);

  jscon_htwrap_build(new_item);

  return new_item;
}

jscon_item_st*
jscon_object(jscon_list_st *list, const char *kKey){
  return jscon_composite(list, kKey, JSCON_OBJECT);
}

jscon_item_st*
jscon_array(jscon_list_st *list, const char *kKey){
  return jscon_composite(list, kKey, JSCON_ARRAY);
}

/* total branches the item possess, returns -1 if primitive*/
size_t
jscon_size(const jscon_item_st* kItem){
  return IS_COMPOSITE(kItem) ? kItem->comp->num_branch : -1;
} 

/* get the last htwrap relative to the item */
static jscon_htwrap_st*
jscon_get_last_htwrap(jscon_item_st *item)
{
  assert(IS_COMPOSITE(item));

  size_t item_depth = jscon_get_depth(item);

  /* get the deepest nested composite relative to item */
  jscon_htwrap_st *htwrap_last = &item->comp->htwrap;
  while(NULL != htwrap_last->next && item_depth < jscon_get_depth(htwrap_last->next->root)){
    htwrap_last = htwrap_last->next;
  }

  return htwrap_last;
}

/* remake hashtable on functions that deal with increasing branches */
static void
jscon_hashtable_remake(jscon_item_st *item)
{
  hashtable_destroy(item->comp->htwrap.hashtable);
  item->comp->htwrap.hashtable = hashtable_init();
  jscon_htwrap_build(item);
}

jscon_item_st*
jscon_append(jscon_item_st *item, jscon_item_st *new_branch)
{
  assert(IS_COMPOSITE(item) && (NULL != new_branch));

  if (new_branch == item){
    assert(NULL != jscon_get_key(item));
    new_branch = jscon_clone(item);
  }

  ++item->comp->num_branch;
  /* realloc parent references to match new size */
  item->comp->branch = realloc(item->comp->branch, jscon_size(item) * sizeof(jscon_item_st*));
  assert(NULL != item->comp->branch);
  item->comp->branch[jscon_size(item)-1] = new_branch;
  new_branch->parent = item;

  if (jscon_size(item) <= item->comp->htwrap.hashtable->num_bucket){
    jscon_htwrap_set(jscon_get_key(new_branch), new_branch);
  } else {
    jscon_hashtable_remake(item);
  }

  if (IS_PRIMITIVE(new_branch)) return new_branch;

  /* get the last htwrap relative to item */
  jscon_htwrap_st *htwrap_last = jscon_get_last_htwrap(item);
  htwrap_last->next = &new_branch->comp->htwrap;
  new_branch->comp->htwrap.prev = htwrap_last;

  return new_branch;
}

jscon_item_st*
jscon_dettach(jscon_item_st *item)
{
  //can't dettach root from nothing
  if (IS_ROOT(item)) return item;

  /* get the item index reference from its parent */
  jscon_item_st *item_parent = item->parent;

  /* dettach the item from its parent and reorder keys */
  for (long i = jscon_get_index(item_parent, item->key); i < jscon_size(item_parent)-1; ++i){
    item_parent->comp->branch[i] = item_parent->comp->branch[i+1]; 
  }
  item_parent->comp->branch[jscon_size(item_parent)-1] = NULL;
  --item_parent->comp->num_branch;

  /* realloc parent references to match new size */
  item_parent->comp->branch = realloc(item_parent->comp->branch, (1+jscon_size(item_parent)) * sizeof(jscon_item_st*));
  assert(NULL != item_parent->comp->branch);

  /* parent hashtable has to be remade, to match reordered keys */
  jscon_hashtable_remake(item_parent);

  /* get the immediate previous htwrap relative to the item */
  jscon_htwrap_st *htwrap_prev = item->comp->htwrap.prev;
  /* get the last htwrap relative to item */
  jscon_htwrap_st *htwrap_last = jscon_get_last_htwrap(item);

  /* remove tree references to the item */
  htwrap_prev->next = htwrap_last->next;

  /* remove item references to the tree */
  item->parent = NULL;
  htwrap_last->next = NULL;
  item->comp->htwrap.prev = NULL;

  return item;
}

void
jscon_delete(jscon_item_st *item, const char *kKey)
{
  jscon_item_st *branch = jscon_get_branch(item, kKey);

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
    assert(IS_COMPOSITE(item));
    *p_current_item = item;
    return item;
  }

  /* if p_current_item is NULL, it needs to be set back with item parameter */
  if (NULL == *p_current_item) return NULL;

  /* get next htwrap in line, if NULL it means there are no more
      composite datatype items to iterate through */
  jscon_htwrap_st *next_htwrap = (*p_current_item)->comp->htwrap.next;
  if (NULL == next_htwrap){
    *p_current_item = NULL;
    return NULL;
  }

  *p_current_item = next_htwrap->root;

  return *p_current_item;
}

/* return next (not yet accessed) item, by using item->comp->last_accessed_branch as the branch index */
static inline jscon_item_st*
jscon_push(jscon_item_st* item)
{
  assert(IS_COMPOSITE(item));//item has to be of Object type to fetch a branch
  assert(item->comp->last_accessed_branch < jscon_size(item));//overflow assert

  ++item->comp->last_accessed_branch; //update last_accessed_branch to next
  jscon_item_st *next_item = item->comp->branch[item->comp->last_accessed_branch-1];

  //resets incase its already set because of a different run
  if (IS_COMPOSITE(next_item)){
    next_item->comp->last_accessed_branch = 0;
  }

  return next_item; //return item from next branch in line
}

static inline jscon_item_st*
jscon_pop(jscon_item_st* item)
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
jscon_iter_next(jscon_item_st* item)
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
      item = jscon_pop(item);
      if ((NULL == item) || (0 == item->comp->last_accessed_branch)){
        return NULL; //return NULL if exceeded root
      }
     } while (jscon_size(item) == item->comp->last_accessed_branch);
  }

  return jscon_push(item);
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
  if (NULL != item->key){
    clone->key = strdup(item->key);
    assert(NULL != clone->key);
  }
  free(tmp_buffer);

  return clone;
}

jscon_char_kt*
jscon_typeof(const jscon_item_st *kItem)
{
  switch (kItem->type){
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

jscon_char_kt*
jscon_strdup(const jscon_item_st* kItem)
{
  jscon_char_kt *src = jscon_get_string(kItem);

  if (NULL == src) return NULL;

  jscon_char_kt *dest = strdup(src);
  assert(NULL != dest);

  return dest;
}

jscon_char_kt*
jscon_strcpy(char *dest, const jscon_item_st* kItem)
{
  jscon_char_kt *src = jscon_get_string(kItem);

  return (NULL != src) ? strcpy(dest, src) : NULL;
}

int
jscon_typecmp(const jscon_item_st* kItem, const jscon_type_et kType){
  return kItem->type & kType; //BITMASK AND
}

int
jscon_keycmp(const jscon_item_st* kItem, const char *kKey){
  return (NULL != jscon_get_key(kItem)) ? STREQ(kItem->key, kKey) : 0;
}

int
jscon_doublecmp(const jscon_item_st* kItem, const jscon_double_kt kDouble){
  assert(JSCON_DOUBLE == kItem->type); //check if given item is double

  return kItem->d_number == kDouble;
}

int
jscon_intcmp(const jscon_item_st* kItem, const jscon_integer_kt kInteger){
  assert(JSCON_INTEGER == kItem->type); //check if given item is integer

  return kItem->i_number == kInteger;
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
jscon_get_root(jscon_item_st* item)
{
  while (!IS_ROOT(item)){
    item = jscon_get_parent(item);
  }

  return item;
}


/* get item branch with given key */
jscon_item_st*
jscon_get_branch(jscon_item_st *item, const char *kKey)
{
  assert(IS_COMPOSITE(item));
  /* search for entry with given key at item's htwrap,
    and retrieve found or not found(NULL) item */
  return jscon_htwrap_get(kKey, item);
}

/* get origin item sibling by the relative index, if origin item is of index 3 (from parent's perspective), and relative index is -1, then this function will return item of index 2 (from parent's perspective) */
jscon_item_st*
jscon_get_sibling(const jscon_item_st* kOrigin, const size_t kRelative_index)
{
  assert(!IS_ROOT(kOrigin));

  const jscon_item_st* kParent = jscon_get_parent(kOrigin);

  //get parent's branch index of the kOrigin item
  size_t origin_index= jscon_get_index(kParent, kOrigin->key);

  /* if relative index given doesn't exceed kParent branch amount,
    or dropped below 0, return branch at given relative index */
  if ((0 <= (origin_index + kRelative_index)) && jscon_size(kParent) > (origin_index + kRelative_index)){
    return jscon_get_byindex(kParent, origin_index + kRelative_index);
  }

  return NULL;
}

/* return parent */
jscon_item_st*
jscon_get_parent(const jscon_item_st* kItem){
  return jscon_pop((jscon_item_st*)kItem);
}

jscon_item_st*
jscon_get_byindex(const jscon_item_st* kItem, const size_t index)
{
  assert(IS_COMPOSITE(kItem));
  return (index < jscon_size(kItem)) ? kItem->comp->branch[index] : NULL;
}

/* returns -1 if item not found */
size_t
jscon_get_index(const jscon_item_st* kItem, const char *kKey)
{
  assert(IS_COMPOSITE(kItem));

  jscon_item_st *lookup_item = jscon_htwrap_get(kKey, (jscon_item_st*)kItem);
  if (NULL == lookup_item) return -1;

  /* TODO: can this be done differently? */
  for (size_t i=0; i < jscon_size(kItem); ++i){
    if (lookup_item == kItem->comp->branch[i]){
      return i;
    }
  }

  fprintf(stderr, "\n\nERROR: item exists in hashtable but is not referenced by the parent\n");
  exit(EXIT_FAILURE);
}

jscon_type_et
jscon_get_type(const jscon_item_st* kItem){
  return kItem->type;
}

jscon_char_kt*
jscon_get_key(const jscon_item_st* kItem){
  return kItem->key;
}

jscon_boolean_kt
jscon_get_boolean(const jscon_item_st* kItem)
{
  if (NULL == kItem || JSCON_NULL == kItem->type) return false;

  assert(JSCON_BOOLEAN == kItem->type);
  return kItem->boolean;
}

jscon_char_kt*
jscon_get_string(const jscon_item_st* kItem)
{
  if (NULL == kItem || JSCON_NULL == kItem->type) return NULL;

  assert(JSCON_STRING == kItem->type);
  return kItem->string;
}

jscon_double_kt
jscon_get_double(const jscon_item_st* kItem)
{
  if (NULL == kItem || JSCON_NULL == kItem->type) return 0.0;

  assert(JSCON_DOUBLE == kItem->type);
  return kItem->d_number;
}

jscon_integer_kt
jscon_get_integer(const jscon_item_st* kItem)
{
  if (NULL == kItem || JSCON_NULL == kItem->type) return 0;

  assert(JSCON_INTEGER == kItem->type);
  return kItem->i_number;
}
