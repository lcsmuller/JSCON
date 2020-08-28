#include "../JSON.h"
#include "global_share.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static inline json_item_st*
json_item_push(json_item_st* item)
{
  assert(item->last_accessed_branch < item->num_branch);//overflow assert

  ++item->last_accessed_branch;
  return item->branch[item->last_accessed_branch-1];
}

static inline json_item_st*
json_item_pop(json_item_st* item)
{
  assert(0 <= item->last_accessed_branch);//underflow assert

  item->last_accessed_branch = 0;
  return item->parent;
}

/*this will simulate recursive movement iteratively*/
json_item_st*
json_item_next(json_item_st* item)
{
  if (NULL == item){
    return NULL;
  }

  /* no branch available to branch, retrieve parent
    until item with available branch found */
  if (0 == item->num_branch){
    do { //recursive walk
      item = json_item_pop(item);
      if (NULL == item){
        return NULL;
      }
     } while (item->last_accessed_branch == item->num_branch);
  }

  item = json_item_push(item);

  return item;
}

int
json_search_key(const json_string_kt kSearch_key)
{
  int top = g_keycache.cache_size - 1;
  int low = 0;
  int mid;

  int cmp;
  while (low <= top){
    mid = ((ulong)low + (ulong)top) >> 1;
    cmp = strcmp(kSearch_key,*g_keycache.list_keyaddr[mid]);
    if (cmp == 0)
      return mid;
    if (cmp < 0)
      top = mid-1;
    else
      low = mid+1;
  }
  
  return -1;
}

static int
cstrcmp(const void *a, const void *b)
{
  const json_string_kt**ia = (const json_string_kt**)a;
  const json_string_kt**ib = (const json_string_kt**)b;

  return strcmp(**ia, **ib);
}

int
json_replace_key_all(const json_string_kt kOld_Key, const json_string_kt kNew_Key)
{
  long found_index = json_search_key(kOld_Key);

  if (-1 != found_index){
    free(*g_keycache.list_keyaddr[found_index]);
    *g_keycache.list_keyaddr[found_index] = strdup(kNew_Key);
  
    qsort(g_keycache.list_keyaddr, g_keycache.cache_size, sizeof *g_keycache.list_keyaddr, cstrcmp);

    return 1;
  }

  return 0;
}

json_item_st*
json_item_get_root(json_item_st* item)
{
  json_item_st *tmp = item;
  do {
    item = tmp;
    tmp = json_item_get_parent(item);
  } while (NULL != tmp);

  return item;
}

void
json_item_typeof(const json_item_st *kItem, FILE* stream)
{
  switch (kItem->type){
  case JSON_NUMBER:
      fprintf(stream,"Number");
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
json_item_typecmp(const json_item_st* kItem, const json_type_et kType){
  return kItem->type & kType;
}

int
json_item_keycmp(const json_item_st* kItem, const json_string_kt kKey){
  return (NULL != json_item_get_key(kItem)) ? STREQ(*kItem->p_key, kKey) : 0;
}

int
json_item_numbercmp(const json_item_st* kItem, const json_number_kt kNumber){
  assert(kItem->type == JSON_NUMBER);
  return kItem->number == kNumber;
}

json_item_st*
json_item_get_sibling(const json_item_st* kOrigin, const long kRelative_index)
{
  const json_item_st* kParent = json_item_get_parent(kOrigin);
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
json_item_get_parent(const json_item_st* kItem){
  return json_item_pop((json_item_st*)kItem);
}

json_item_st*
json_item_get_property(const json_item_st* kItem, const size_t index){
  return (index < kItem->num_branch) ? kItem->branch[index] : NULL;
}

size_t
json_item_get_property_count(const json_item_st* kItem){
  return kItem->num_branch;
} 

json_type_et
json_item_get_type(const json_item_st* kItem){
  return kItem->type;
}

json_string_kt
json_item_get_key(const json_item_st* kItem){
  return (kItem->p_key) ? *kItem->p_key : NULL;
}

json_boolean_kt
json_item_get_boolean(const json_item_st* kItem){
  assert(kItem->type == JSON_BOOLEAN);
  return kItem->boolean;
}

json_string_kt
json_item_get_string(const json_item_st* kItem){
  assert(kItem->type == JSON_STRING);
  return kItem->string;
}

/* converts number to string and store it in p_str */
void 
json_number_tostr(const json_number_kt kNumber, json_string_kt p_str, const int kDigits)
{
  //check if value is integer
  if (kNumber <= LLONG_MIN || kNumber >= LLONG_MAX || kNumber == (long long)kNumber){
    sprintf(p_str,"%.lf",kNumber); //convert integer to string
    return;
  }

  int decimal=0, sign=0;
  json_string_kt tmp_str = fcvt(kNumber,kDigits-1,&decimal,&sign);

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
