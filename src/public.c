#include "../JSON.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static inline JsonItem*
JsonItem_Push(JsonItem* item)
{
  assert(item->top < item->n_property);//overflow assert

  ++item->top;
  return item->property[item->top-1];
}

static inline JsonItem*
JsonItem_Pop(JsonItem* item)
{
  assert(item->top >= 0);//underflow assert

  item->top = 0;
  return item->parent;
}

/*this will simulate recursive movement iteratively*/
JsonItem*
JsonItem_Next(JsonItem* item)
{
  if (!item){
    return NULL;
  }
  /* no property available to branch, retrieve parent
    until item with available property found */
  if (item->n_property == 0){
    do //recursive walk
     {
      item = JsonItem_Pop(item);
      if (!item){
        return NULL;
      }
     }
    while (item->top == item->n_property);
  }
  item = JsonItem_Push(item);

  return item;
}

int
Json_SearchKey(const Json* json, const JsonString search_key)
{
  int top = json->n_list-1;
  int low = 0;
  int mid;

  int cmp;
  while (low <= top){
    mid = ((ulong)low + (ulong)top) >> 1;
    cmp = strcmp(search_key,*json->list_ptr_key[mid]);
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
  const JsonString**ia = (const JsonString**)a;
  const JsonString**ib = (const JsonString**)b;

  return strcmp(**ia, **ib);
}

int
Json_ReplaceKeyAll(const Json* json, const JsonString old_key, const JsonString new_key)
{
  int found_index = Json_SearchKey(json, old_key);

  if (found_index != -1){
    free(*json->list_ptr_key[found_index]);
    *json->list_ptr_key[found_index] = strdup(new_key);
  
    qsort(json->list_ptr_key,json->n_list,sizeof(JsonString*),cstrcmp);

    return 1;
  }

  return 0;
}

JsonItem*
JsonItem_GetRoot(JsonItem* item){
  while (item->parent != NULL){
    item = item->parent;
  }
  return item;
}

void
JsonItem_TypeOf(const JsonItem *item, FILE* stream)
{
  switch (item->dtype){
    case Number:
      fprintf(stream,"Number");
      break;
    case String:
      fprintf(stream,"String");
      break;
    case Null:
      fprintf(stream,"Null");
      break;
    case Boolean:
      fprintf(stream,"Boolean");
      break;
    case Object:
      fprintf(stream,"Object");
      break;
    case Array:
      fprintf(stream,"Array");
      break;
    case Undefined:
      fprintf(stream,"Undefined");
    default:
      assert(item->dtype == !item->dtype);
  }
}

int
JsonItem_DatatypeCmp(const JsonItem* item, const JsonDType dtype){
  return item->dtype & dtype;
}

int
JsonItem_KeyCmp(const JsonItem* item, const JsonString key){
  return (item->ptr_key != NULL) ? !strcmp(*item->ptr_key, key) : 0;
}

int
JsonItem_NumberCmp(const JsonItem* item, const JsonNumber number){
  return item->number == number;
}

JsonItem*
JsonItem_GetSibling(const JsonItem* origin, const long int relative_index)
{
  const JsonItem* parent = JsonItem_GetParent(origin);
  if ((parent == NULL) || (parent == origin))
    return NULL;

  long int origin_index=0;
  while (parent->property[origin_index] != origin)
    ++origin_index;

  if (((origin_index+relative_index) >= 0)
      && ((origin_index+relative_index) < parent->n_property)){
    return parent->property[origin_index+relative_index];
  }

  return NULL;
}

JsonItem*
JsonItem_GetParent(const JsonItem* item){
  return (item->parent != item) ? item->parent : NULL;
}

JsonItem*
JsonItem_GetProperty(const JsonItem* item, const size_t index){
  return (index < item->n_property) ? item->property[index] : NULL;
}

size_t
JsonItem_GetPropertyCount(const JsonItem* item){
  return item->n_property;
} 

JsonDType
JsonItem_GetDatatype(const JsonItem* item){
  return item->dtype;
}

JsonString
JsonItem_GetKey(const JsonItem* item){
  return (item->ptr_key) ? *item->ptr_key : NULL;
}

JsonBool
JsonItem_GetBoolean(const JsonItem* item){
  return item->boolean;
}

JsonString
JsonItem_GetString(const JsonItem* item){
  return item->string;
}

void 
JsonNumber_StrFormat(const JsonNumber number, JsonString ptr, const int digits)
{
  //check if value is integer
  if (number <= LLONG_MIN || number >= LLONG_MAX || number == (long long)number){
    sprintf(ptr,"%.lf",number); //convert integer to string
    return;
  }

  int decimal=0, sign=0;
  JsonString temp_str=fcvt(number,digits-1,&decimal,&sign);

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
