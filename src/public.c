#include "../JSON.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static inline void
JsonStack_Push(Json* json)
{
  assert((json->stack.top - json->stack.trace) < json->stack.max_depth);//overflow assert
  ++json->stack.top; //update top

  json->item_ptr = json->item_ptr->property[++json->stack.top[-1]];
}

static inline void
JsonStack_Pop(Json* json)
{
  assert(json->stack.top > json->stack.trace);//underflow assert
  //avoid writing at offset memory
  if(json->stack.top < json->stack.trace + json->stack.max_depth)
    *json->stack.top = -1;
  --json->stack.top; //update top

  json->item_ptr = json->item_ptr->parent;
}

/*this will simulate recursive movement iteratively*/
JsonItem*
Json_NextItem(Json* json)
{
  if (!json->item_ptr)
    return NULL;

  /*no branching possible, pop stack until an item with branch found
    (available property = available branch)*/
  if (json->item_ptr->n_property == 0){
    do /* "recursively" walk json items */
     {
      //return NULL to avoid underflow
      if (json->stack.top <= json->stack.trace){
        json->item_ptr = NULL;
        return json->item_ptr;
      }
      JsonStack_Pop(json);
     }
    while (*json->stack.top == json->item_ptr->n_property-1);
  }
  assert(json->item_ptr->n_property > 0); //overflow
  JsonStack_Push(json);

  return json->item_ptr;
}

void
Json_Rewind(Json* json){
  memset(json->stack.trace,-1,json->stack.max_depth); 
  json->stack.top = json->stack.trace;
  json->item_ptr = json->root;
}

int
Json_SearchKey(const Json* json, const JsonString search_key[])
{
  int top = json->n_keylist-1;
  int low = 0;
  int mid;

  int cmp;
  while (low <= top){
    mid = ((ulong)low + (ulong)top) >> 1;
    cmp = strcmp(search_key, json->keylist[mid]);
    if (cmp == 0)
      return mid;

    if (cmp < 0)
      top = mid-1;
    else
      low = mid+1;
  }
  return -1;
}

int
Json_ReplaceKeyAll(const Json* json, const JsonString old_key[], const JsonString new_key[])
{
  int found_index = Json_SearchKey(json, old_key);
  if (found_index != -1){
    strncpy(json->keylist[found_index],new_key,KEY_LENGTH-1);
    return 1;
  }

  return 0;
}

JsonItem*
Json_GetItem(const Json* json){
  return json->item_ptr;
}

JsonItem*
Json_GetRoot(const Json* json){
  return json->root;
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
      fprintf(stream,"ArrayObject");
      break;
    case Undefined:
      fprintf(stream,"Undefined");
    default:
      fprintf(stream,"ERROR");
      exit(EXIT_FAILURE);
  }
}

int
JsonItem_DatatypeCmp(const JsonItem* item, const JsonDType dtype){
  return item->dtype & dtype;
}

int
JsonItem_KeyCmp(const JsonItem* item, const JsonString key[]){
  return (item->key != NULL) ? !strcmp(item->key, key) : 0;
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

JsonString*
JsonItem_GetKey(const JsonItem* item){
  return item->key;
}

JsonBool
JsonItem_GetBoolean(const JsonItem* item){
  return item->boolean;
}

JsonString*
JsonItem_GetString(const JsonItem* item){
  return item->string;
}

void 
JsonNumber_StrFormat(const JsonNumber number, JsonString* ptr, const int digits)
{
  //check if value is integer
  if (number <= LLONG_MIN || number >= LLONG_MAX || number == (long long)number){
    sprintf(ptr,"%.lf",number); //convert integer to string
    return;
  }

  int decimal=0, sign=0;
  JsonString *temp_str=fcvt(number,digits-1,&decimal,&sign);

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
