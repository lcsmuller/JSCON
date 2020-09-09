#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "libjsonc.h"

/* get item with given key, successive calls will get
  the next item in line containing the same key */
jsonc_item_st*
jsonc_foreach_specific(jsonc_item_st *item, const char *kKey)
{
  if (!(item->type & (JSONC_OBJECT|JSONC_ARRAY)))
    return NULL;

  item = jsonc_hashtable_get(kKey, item);

  if (NULL != item) return item;

  return NULL;
}

jsonc_item_st*
jsonc_foreach_object_r(jsonc_item_st *item, jsonc_item_st **p_current_item)
{
  jsonc_hasht_st *current_hashtable;

  if (NULL != item){
    current_hashtable = item->hashtable;
    *p_current_item = current_hashtable->root;
    return *p_current_item;
  }

  current_hashtable = (*p_current_item)->hashtable->next;
  if (NULL == current_hashtable){
    *p_current_item = NULL;
    return NULL;
  }

  *p_current_item = current_hashtable->root;
  return *p_current_item;
}

static inline jsonc_item_st*
jsonc_push(jsonc_item_st* item)
{
  assert(item->last_accessed_branch < item->num_branch); //overflow assert

  ++item->last_accessed_branch;
  return item->branch[item->last_accessed_branch-1];
}

static inline jsonc_item_st*
jsonc_pop(jsonc_item_st* item)
{
  assert(0 <= item->last_accessed_branch); //underflow assert

  item->last_accessed_branch = 0;
  return item->parent;
}

/* this will simulate recursive movement iteratively, by checking the
    current item last_accessed_branch value, under no circumstance 
    should you modify last_accessed_branch value directly */
jsonc_item_st*
jsonc_foreach(jsonc_item_st* item)
{
  if (NULL == item) return NULL;

  /* no branch available to branch, retrieve parent
    until item with available branch found */
  if (0 == item->num_branch){
    do { //recursive walk
      item = jsonc_pop(item);
      if ((NULL == item) || (0 == item->last_accessed_branch)){
        return NULL;
      }
     } while (item->num_branch == item->last_accessed_branch);
  }

  item = jsonc_push(item);

  return item;
}

/* This is not the most effective way to clone a item, but it is
    the safest one, because it automatically accounts for any
    new feature I might add in the future. By first stringfying the
    (to be cloned) jsonc_item and then parsing the resulting string into
    a new (clone) jsonc_item, it's guaranteed that it will be a perfect 
    clone, with its own addressed hashtable, strings, etc */
jsonc_item_st*
jsonc_get_clone(jsonc_item_st *item)
{
  if (NULL == item) return NULL;

  char *tmp_buffer = jsonc_stringify(item, JSONC_ALL);
  jsonc_item_st *clone = jsonc_parse(tmp_buffer);
  free(tmp_buffer);

  return clone;
}

jsonc_item_st*
jsonc_get_root(jsonc_item_st* item)
{
  jsonc_item_st *tmp = item;
  do {
    item = tmp;
    tmp = jsonc_get_parent(item);
  } while (NULL != tmp);

  return item;
}

jsonc_string_kt
jsonc_typeof(const jsonc_item_st *kItem)
{
  switch (kItem->type){
  case JSONC_NUMBER_DOUBLE:
      return "Double";
  case JSONC_NUMBER_INTEGER:
      return "Integer";
  case JSONC_STRING:
      return "String";
  case JSONC_NULL:
      return "Null";
  case JSONC_BOOLEAN:
      return "Boolean";
  case JSONC_OBJECT:
      return "Object";
  case JSONC_ARRAY:
      return "Array";
  case JSONC_UNDEFINED:
      return "Undefined";
  default:
      return "NaN";
  }
}

int
jsonc_typecmp(const jsonc_item_st* kItem, const jsonc_type_et kType){
  return kItem->type & kType;
}

int
jsonc_keycmp(const jsonc_item_st* kItem, const char *kKey){
  return (NULL != jsonc_get_key(kItem)) ? STREQ(kItem->key, kKey) : 0;
}

int
jsonc_doublecmp(const jsonc_item_st* kItem, const jsonc_double_kt kDouble){
  assert(JSONC_NUMBER_DOUBLE == kItem->type);
  return kItem->d_number == kDouble;
}

int
jsonc_intcmp(const jsonc_item_st* kItem, const jsonc_integer_kt kInteger){
  assert(JSONC_NUMBER_INTEGER == kItem->type);
  return kItem->i_number == kInteger;
}

jsonc_item_st*
jsonc_get_sibling(const jsonc_item_st* kOrigin, const size_t kRelative_index)
{
  const jsonc_item_st* kParent = jsonc_get_parent(kOrigin);
  if (NULL == kParent) return NULL; //kOrigin is root

  size_t origin_index=0;
  while (kOrigin != kParent->branch[origin_index])
    ++origin_index;

  if ((0 <= (origin_index + kRelative_index)) && (kParent->num_branch > (origin_index + kRelative_index))){
    return kParent->branch[origin_index + kRelative_index];
  }

  return NULL;
}

jsonc_item_st*
jsonc_get_parent(const jsonc_item_st* kItem){
  return jsonc_pop((jsonc_item_st*)kItem);
}

jsonc_item_st*
jsonc_get_branch(const jsonc_item_st* kItem, const size_t index){
  return (index < kItem->num_branch) ? kItem->branch[index] : NULL;
}

size_t
jsonc_get_num_branch(const jsonc_item_st* kItem){
  return kItem->num_branch;
} 

jsonc_type_et
jsonc_get_type(const jsonc_item_st* kItem){
  return kItem->type;
}

jsonc_string_kt
jsonc_get_key(const jsonc_item_st* kItem){
  return kItem->key;
}

jsonc_boolean_kt
jsonc_get_boolean(const jsonc_item_st* kItem){
  if (NULL == kItem || JSONC_NULL == kItem->type)
    return false;

  assert(JSONC_BOOLEAN == kItem->type);
  return kItem->boolean;
}

jsonc_string_kt
jsonc_get_string(const jsonc_item_st* kItem){
  if (NULL == kItem || JSONC_NULL == kItem->type)
    return NULL;

  assert(JSONC_STRING == kItem->type);
  return kItem->string;
}

jsonc_double_kt
jsonc_get_double(const jsonc_item_st* kItem){
  if (NULL == kItem || JSONC_NULL == kItem->type)
    return 0.0;

  assert(JSONC_NUMBER_DOUBLE == kItem->type);
  return kItem->d_number;
}

jsonc_integer_kt
jsonc_get_integer(const jsonc_item_st* kItem){
  if (NULL == kItem || JSONC_NULL == kItem->type)
    return 0;

  assert(JSONC_NUMBER_INTEGER == kItem->type);
  return kItem->i_number;
}

jsonc_string_kt
jsonc_strdup(const jsonc_item_st* kItem){
  jsonc_string_kt tmp = jsonc_get_string(kItem);
  if (NULL == tmp){
    return NULL;
  }

  jsonc_string_kt new_string = strdup(tmp);
  assert(NULL != new_string);

  return new_string;
}

/* converts double to string and store it in p_str */
//@todo: try to make this more readable
void 
jsonc_double_tostr(const jsonc_double_kt kDouble, jsonc_string_kt p_str, const int kDigits)
{
  if (DOUBLE_IS_INTEGER(kDouble)){
    sprintf(p_str,"%.lf",kDouble); //convert integer to string
    return;
  }

  int decimal=0, sign=0;
  jsonc_string_kt tmp_str = fcvt(kDouble,kDigits-1,&decimal,&sign);

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
