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
  assert(item->last_accessed_branch >= 0);//underflow assert

  item->last_accessed_branch = 0;
  return item->parent;
}

/*this will simulate recursive movement iteratively*/
json_item_st*
json_item_next(json_item_st* item)
{
  if (!item){
    return NULL;
  }
  /* no branch available to branch, retrieve parent
    until item with available branch found */
  if (item->num_branch == 0){
    do //recursive walk
     {
      item = json_item_pop(item);
      if (!item){
        return NULL;
      }
     }
    while (item->last_accessed_branch == item->num_branch);
  }
  item = json_item_push(item);

  return item;
}

// fix: no need for json_item_st call
int
json_item_search_key(const json_item_st* item, const json_string_kt search_key)
{
  int top = g_keylist.num_ptr_key-1;
  int low = 0;
  int mid;

  int cmp;
  while (low <= top){
    mid = ((ulong)low + (ulong)top) >> 1;
    cmp = strcmp(search_key,*g_keylist.list_ptr_key[mid]);
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

// fix: no need for json_item_st call
int
json_item_replace_key_all(const json_item_st* item, const json_string_kt old_key, const json_string_kt new_key)
{
  int found_index = json_item_search_key(item, old_key);

  if (found_index != -1){
    free(*g_keylist.list_ptr_key[found_index]);
    *g_keylist.list_ptr_key[found_index] = strdup(new_key);
  
    qsort(g_keylist.list_ptr_key,g_keylist.num_ptr_key,sizeof(json_string_kt*),cstrcmp);

    return 1;
  }

  return 0;
}

json_item_st*
json_item_get_root(json_item_st* item){
  while (item->parent != NULL){
    item = item->parent;
  }
  return item;
}

void
json_item_typeof(const json_item_st *item, FILE* stream)
{
  switch (item->type){
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
      assert(item->type == !item->type);
  }
}

int
json_item_typecmp(const json_item_st* item, const json_type_et type){
  return item->type & type;
}

int
json_item_keycmp(const json_item_st* item, const json_string_kt key){
  return (item->ptr_key != NULL) ? !strcmp(*item->ptr_key, key) : 0;
}

int
json_item_numbercmp(const json_item_st* item, const json_number_kt number){
  return item->number == number;
}

json_item_st*
json_item_get_sibling(const json_item_st* origin, const long int relative_index)
{
  const json_item_st* parent = json_item_get_parent(origin);
  if ((parent == NULL) || (parent == origin))
    return NULL;

  long int origin_index=0;
  while (parent->branch[origin_index] != origin)
    ++origin_index;

  if (((origin_index+relative_index) >= 0)
      && ((origin_index+relative_index) < parent->num_branch)){
    return parent->branch[origin_index+relative_index];
  }

  return NULL;
}

json_item_st*
json_item_get_parent(const json_item_st* item){
  return (item->parent != item) ? item->parent : NULL;
}

json_item_st*
json_item_get_property(const json_item_st* item, const size_t index){
  return (index < item->num_branch) ? item->branch[index] : NULL;
}

size_t
json_item_get_property_count(const json_item_st* item){
  return item->num_branch;
} 

json_type_et
json_item_get_type(const json_item_st* item){
  return item->type;
}

json_string_kt
json_item_get_key(const json_item_st* item){
  return (item->ptr_key) ? *item->ptr_key : NULL;
}

json_boolean_kt
json_item_get_boolean(const json_item_st* item){
  return item->boolean;
}

json_string_kt
json_item_get_string(const json_item_st* item){
  return item->string;
}

void 
json_number_tostr(const json_number_kt number, json_string_kt ptr, const int digits)
{
  //check if value is integer
  if (number <= LLONG_MIN || number >= LLONG_MAX || number == (long long)number){
    sprintf(ptr,"%.lf",number); //convert integer to string
    return;
  }

  int decimal=0, sign=0;
  json_string_kt temp_str=fcvt(number,digits-1,&decimal,&sign);

  int i=0;
  if (sign < 0)
    ptr[i++] = '-';

  if ((decimal < -7) || (decimal > 17)){ //print scientific notation 
    sprintf(ptr+i,"%c.%.7se%d",*temp_str,temp_str+1,decimal-1);
    return;
  }

  char format[100];
  if (decimal > 0){
    sprintf(format,"%%.%ds.%%.7s",decimal);
    sprintf(ptr+i,format,temp_str,temp_str+decimal);
    return;
  }
  if (decimal < 0){
    sprintf(format,"0.%0*d%%.7s",abs(decimal),0);
    sprintf(ptr+i,format,temp_str);
    return;
  }
  sprintf(format,"0.%%.7s");
  sprintf(ptr+i,format,temp_str);

  return;
}
