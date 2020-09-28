#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "libjscon.h"

/* TODO: define some of these functions as a libjscon.h #define */

/* reentrant function, works similar to strtok. the starting point is set
    by doing the function call before the main iteration loop, then
    consecutive function calls inside the loop will continue the iteration
    from then on if item is then set to NULL.
    
    p_current_item allows for thread safety reentrancy, it should not be
    modified outside of this routine*/
jscon_item_st*
jscon_next_composite_r(jscon_item_st *item, jscon_item_st **p_current_item)
{
  jscon_htwrap_st *current_htwrap;

  if (NULL != item){
    assert(IS_COMPOSITE(item));
    current_htwrap = &item->comp->htwrap;
    *p_current_item = current_htwrap->root;
    return *p_current_item;
  }

  current_htwrap = (*p_current_item)->comp->htwrap.next;
  if (NULL == current_htwrap){
    *p_current_item = NULL;
    return NULL;
  }

  *p_current_item = current_htwrap->root;
  return *p_current_item;
}

/* return next (not yet accessed) item, by using item->comp->last_accessed_branch as the branch index */
static inline jscon_item_st*
jscon_push(jscon_item_st* item)
{
  assert(IS_COMPOSITE(item));//item has to be of Object type to fetch a branch
  assert(item->comp->last_accessed_branch < item->comp->num_branch);//overflow assert

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
    item->comp->last_accessed_branch like a stack trace. under no circumstance 
    should you modify last_accessed_branch value directly */
jscon_item_st*
jscon_next(jscon_item_st* item)
{
  if (NULL == item) return NULL;

  //resets root's last_accessed_branch in case its set from a different run
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
     } while (item->comp->num_branch == item->comp->last_accessed_branch);
  }

  item = jscon_push(item);

  return item;
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

  return clone;
}

jscon_char_kt*
jscon_typeof(const jscon_item_st *kItem)
{
  switch (kItem->type){
    case JSCON_NUMBER_DOUBLE: return "Double";
    case JSCON_NUMBER_INTEGER: return "Integer";
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
  jscon_char_kt *tmp = jscon_get_string(kItem);

  if (NULL == tmp) return NULL;

  jscon_char_kt *new_string = strdup(tmp);
  assert(NULL != new_string);

  return new_string;
}

jscon_char_kt*
jscon_strncpy(char *dest, const jscon_item_st* kItem, size_t n)
{
  jscon_char_kt *tmp = jscon_get_string(kItem);

  if (NULL == tmp) return NULL;

  strncpy(dest, tmp, n);

  return dest;
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
  assert(JSCON_NUMBER_DOUBLE == kItem->type); //check if given item is double
  return kItem->d_number == kDouble;
}

int
jscon_intcmp(const jscon_item_st* kItem, const jscon_integer_kt kInteger){
  assert(JSCON_NUMBER_INTEGER == kItem->type); //check if given item is integer
  return kItem->i_number == kInteger;
}

/* converts double to string and store it in p_str */
//TODO: try to make this more readable
void 
jscon_double_tostr(const jscon_double_kt kDouble, jscon_char_kt *p_str, const int kDigits)
{
  if (DOUBLE_IS_INTEGER(kDouble)){
    sprintf(p_str,"%.lf",kDouble); //convert integer to string
    return;
  }

  int decimal=0, sign=0;
  jscon_char_kt *tmp_str = fcvt(kDouble,kDigits-1,&decimal,&sign);

  int i=0;
  if (0 > sign){ //negative sign detected
    p_str[i++] = '-';
  }

  if (IN_RANGE(decimal,-7,17)){
    //print scientific notation
    sprintf(p_str+i,"%c.%.7se%d",*tmp_str,tmp_str+1,decimal-1);
    return;
  }

  char format[100];
  if (0 < decimal){
    sprintf(format,"%%.%ds.%%.7s",decimal);
    sprintf(i + p_str, format, tmp_str, tmp_str + decimal);
  } else if (0 > decimal) {
    sprintf(format, "0.%0*d%%.7s", abs(decimal), 0);
    sprintf(i + p_str, format, tmp_str);
  } else {
    sprintf(format,"0.%%.7s");
    sprintf(i + p_str, format, tmp_str);
  }
}

jscon_item_st*
jscon_get_root(jscon_item_st* item)
{
  jscon_item_st *tmp = item;
  do {
    item = tmp;
    tmp = jscon_get_parent(item);
  } while (NULL != tmp);

  return item;
}

/* get item branch with given key,
  successive calls will get the next item in
  line containing the same key (if there are any) */
jscon_item_st*
jscon_get_branch(jscon_item_st *item, const char *kKey)
{
  //return NULL if item is not of Object type
  if (!IS_COMPOSITE(item)) return NULL;

  /* search for entry with given key at item's htwrap,
    and retrieve found or not found(NULL) item */
  return jscon_hashtable_get(kKey, item);
}

/* get origin item sibling by the relative index, if origin item is of index 3 (from parent's perspective), and relative index is -1, then this function will return item of index 2 (from parent's perspective) */
jscon_item_st*
jscon_get_sibling(const jscon_item_st* kOrigin, const size_t kRelative_index)
{
  const jscon_item_st* kParent = jscon_get_parent(kOrigin);

  //if NULL kOrigin is a root, not a member of any Object
  if (NULL == kParent) return NULL; 

  //get parent's branch index of the kOrigin item
  size_t origin_index=0;
  while (kOrigin != kParent->comp->branch[origin_index]){
    ++origin_index;
  }

  /* if relative index given doesn't exceed kParent branch amount,
    or dropped below 0, return branch at given relative index */
  if ((0 <= (origin_index + kRelative_index)) && (kParent->comp->num_branch > (origin_index + kRelative_index))){
    return kParent->comp->branch[origin_index + kRelative_index];
  }

  return NULL;
}

/* return parent safely */
jscon_item_st*
jscon_get_parent(const jscon_item_st* kItem){
  return jscon_pop((jscon_item_st*)kItem);
}

jscon_item_st*
jscon_get_byindex(const jscon_item_st* kItem, const size_t index)
{
  assert(IS_COMPOSITE(kItem));
  return (index < kItem->comp->num_branch) ? kItem->comp->branch[index] : NULL;
}

size_t
jscon_get_num_branch(const jscon_item_st* kItem)
{
  assert(IS_COMPOSITE(kItem));
  return kItem->comp->num_branch;
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
jscon_get_boolean(const jscon_item_st* kItem){
  if (NULL == kItem || JSCON_NULL == kItem->type)
    return false;

  assert(JSCON_BOOLEAN == kItem->type);
  return kItem->boolean;
}

jscon_char_kt*
jscon_get_string(const jscon_item_st* kItem){
  if (NULL == kItem || JSCON_NULL == kItem->type)
    return NULL;

  assert(JSCON_STRING == kItem->type);
  return kItem->string;
}

jscon_double_kt
jscon_get_double(const jscon_item_st* kItem){
  if (NULL == kItem || JSCON_NULL == kItem->type)
    return 0.0;

  assert(JSCON_NUMBER_DOUBLE == kItem->type);
  return kItem->d_number;
}

jscon_integer_kt
jscon_get_integer(const jscon_item_st* kItem){
  if (NULL == kItem || JSCON_NULL == kItem->type)
    return 0;

  assert(JSCON_NUMBER_INTEGER == kItem->type);
  return kItem->i_number;
}
