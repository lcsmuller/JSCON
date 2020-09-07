#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "hashtable.h"
#include "public.h"
#include "parser.h"
#include "stringify.h"
#include "macros.h"

/* get item with given key, successive calls will get
  the next item in line containing the same key */
json_item_st*
json_get_specific(json_item_st *item, const json_string_kt kKey)
{
  if (!(item->type & (JSON_OBJECT|JSON_ARRAY)))
    return NULL;

  item = json_hashtable_get(kKey, item);
  if (NULL != item)
    return item;

  return NULL;
}

json_item_st*
json_next_object(json_item_st *item, json_item_st **p_current_item)
{
  json_hasht_st *current_hashtable;

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

//@todo: remake this lol
json_item_st*
json_get_clone(json_item_st *item)
{
  if (NULL == item) return NULL;

  /* remove item->key temporarily, so that it can be treated
    as a root */
  json_string_kt tmp = item->key;
  item->key = NULL;
  char *buffer = json_stringify(item, JSON_ALL);
  item->key = tmp; //reattach its key

  json_item_st *clone = json_parse(buffer);
  free(buffer);

  return clone;
}

static inline json_item_st*
json_push(json_item_st* item)
{
  assert(item->last_accessed_branch < item->num_branch);//overflow assert

  ++item->last_accessed_branch;
  return item->branch[item->last_accessed_branch-1];
}

static inline json_item_st*
json_pop(json_item_st* item)
{
  assert(0 <= item->last_accessed_branch);//underflow assert

  item->last_accessed_branch = 0;
  return item->parent;
}

/*this will simulate recursive movement iteratively*/
json_item_st*
json_next(json_item_st* item)
{
  if (NULL == item) return NULL;

  /* no branch available to branch, retrieve parent
    until item with available branch found */
  if (0 == item->num_branch){
    do { //recursive walk
      item = json_pop(item);
      if ((NULL == item) || (0 == item->last_accessed_branch)){
        return NULL;
      }
     } while (item->num_branch == item->last_accessed_branch);
  }

  item = json_push(item);

  return item;
}

json_item_st*
json_get_root(json_item_st* item)
{
  json_item_st *tmp = item;
  do {
    item = tmp;
    tmp = json_get_parent(item);
  } while (NULL != tmp);

  return item;
}

void
json_typeof(const json_item_st *kItem, FILE* stream)
{
  switch (kItem->type){
  case JSON_NUMBER_DOUBLE:
      fprintf(stream,"Double");
      break;
  case JSON_NUMBER_INTEGER:
      fprintf(stream,"Integer");
      break;
  case JSON_STRING:
      fprintf(stream,"String");
      break;
  case JSON_NULL:
      fprintf(stream,"Null");
      break;
  case JSON_BOOLEAN:
      fprintf(stream,"Boolean");
      break;
  case JSON_OBJECT:
      fprintf(stream,"Object");
      break;
  case JSON_ARRAY:
      fprintf(stream,"Array");
      break;
  case JSON_UNDEFINED:
      fprintf(stream,"Undefined");
  default:
      assert(kItem->type == !kItem->type);
  }
}

int
json_typecmp(const json_item_st* kItem, const json_type_et kType){
  return kItem->type & kType;
}

int
json_keycmp(const json_item_st* kItem, const json_string_kt kKey){
  return (NULL != json_get_key(kItem)) ? STREQ(kItem->key, kKey) : 0;
}

int
json_doublecmp(const json_item_st* kItem, const json_double_kt kDouble){
  assert(JSON_NUMBER_DOUBLE == kItem->type);
  return kItem->d_number == kDouble;
}

int
json_intcmp(const json_item_st* kItem, const json_integer_kt kInteger){
  assert(JSON_NUMBER_INTEGER == kItem->type);
  return kItem->i_number == kInteger;
}

json_item_st*
json_get_sibling(const json_item_st* kOrigin, const long kRelative_index)
{
  const json_item_st* kParent = json_get_parent(kOrigin);
  if ((NULL == kParent) || (kOrigin == kParent))
    return NULL;

  long origin_index=0;
  while (kOrigin != kParent->branch[origin_index])
    ++origin_index;

  if ((0 <= (origin_index + kRelative_index)) && (kParent->num_branch > (origin_index + kRelative_index))){
    return kParent->branch[origin_index + kRelative_index];
  }

  return NULL;
}

json_item_st*
json_get_parent(const json_item_st* kItem){
  return json_pop((json_item_st*)kItem);
}

json_item_st*
json_get_property(const json_item_st* kItem, const size_t index){
  return (index < kItem->num_branch) ? kItem->branch[index] : NULL;
}

size_t
json_get_property_count(const json_item_st* kItem){
  return kItem->num_branch;
} 

json_type_et
json_get_type(const json_item_st* kItem){
  return kItem->type;
}

json_string_kt
json_get_key(const json_item_st* kItem){
  return kItem->key;
}

json_boolean_kt
json_get_boolean(const json_item_st* kItem){
  if (NULL == kItem || JSON_NULL == kItem->type)
    return 0;

  assert(JSON_BOOLEAN == kItem->type);
  return kItem->boolean;
}

json_string_kt
json_get_string(const json_item_st* kItem){
  if (NULL == kItem || JSON_NULL == kItem->type)
    return NULL;

  assert(JSON_STRING == kItem->type);
  return kItem->string;
}

json_string_kt
json_get_strdup(const json_item_st* kItem){
  json_string_kt tmp = json_get_string(kItem);
  if (NULL == tmp){
    return NULL;
  }

  json_string_kt new_string = strdup(tmp);
  assert(NULL != new_string);

  return new_string;
}

json_double_kt
json_get_double(const json_item_st* kItem){
  if (NULL == kItem || JSON_NULL == kItem->type)
    return 0.0;

  assert(JSON_NUMBER_DOUBLE == kItem->type);
  return kItem->d_number;
}

/* converts double to string and store it in p_str */
void 
json_double_tostr(const json_double_kt kDouble, json_string_kt p_str, const int kDigits)
{
  if (DOUBLE_IS_INTEGER(kDouble)){
    sprintf(p_str,"%.lf",kDouble); //convert integer to string
    return;
  }

  int decimal=0, sign=0;
  json_string_kt tmp_str = fcvt(kDouble,kDigits-1,&decimal,&sign);

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
    return;
  } else if (0 > decimal) {
    sprintf(format, "0.%0*d%%.7s", abs(decimal), 0);
    sprintf(i + p_str, format, tmp_str);
    return;
  } else {
    sprintf(format,"0.%%.7s");
    sprintf(i + p_str, format, tmp_str);
    return;
  }
}
