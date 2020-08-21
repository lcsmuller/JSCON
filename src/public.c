#include "../JSON.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define MAX_DOUBLE_DIGITS 24

JsonItem*
Json_GetItem(Json* json){
  return json->ptr;
}

JsonItem*
Json_GetRoot(Json* json){
  return json->root;
}

JsonString*
Json_SearchKey(Json* json, JsonString search_key[])
{
  int top = json->n_keylist-1;
  int low = 0;
  int mid;

  int cmp;
  while (low <= top){
    mid = ((ulong)low + (ulong)top) >> 1;
    cmp = strcmp(search_key, json->keylist[mid]);
    if (cmp == 0)
      return json->keylist[mid];

    if (cmp < 0)
      top = mid-1;
    else
      low = mid+1;
  }
  return NULL;
}

int
Json_SubKey(Json* json, JsonString old_key[], JsonString new_key[])
{
  JsonString *ptr_key = Json_SearchKey(json, old_key);
  if (ptr_key){
    free(ptr_key);
    ptr_key = strdup(new_key);
    assert(ptr_key);
    return 1;
  }
  return 0;
}

int
JsonItem_DatatypeCmp(JsonItem* item, JsonDType dtype){
  return item->dtype == dtype;
}

int
JsonItem_KeyCmp(JsonItem* item, JsonString key[]){
  if (!item->key)
    return 0;

  return !strcmp(item->key, key);
}

int
JsonItem_NumberCmp(JsonItem* item, JsonNumber number){
  return item->number == number;
}

JsonItem*
JsonItem_GetParent(JsonItem* item){
  return (item->parent != item) ? item->parent : NULL;
}

JsonItem*
JsonItem_GetProperty(JsonItem* item, size_t index){
  return (index < item->n_property) ? item->property[index] : NULL;
}

JsonItem*
JsonItem_GetSibling(const JsonItem* origin, long int relative_index)
{
  const JsonItem* parent=origin->parent;
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

JsonString*
JsonItem_GetString(JsonItem* item){
  return (item->dtype == String) ? item->string : NULL;
}

JsonString* 
JsonNumber_StrFormat(JsonNumber number)
{
  JsonString set_strnum[MAX_DOUBLE_DIGITS]={0};
  //check if value is integer
  if (number <= LLONG_MIN || number >= LLONG_MAX || number == (long long)number){
    sprintf(set_strnum,"%.lf",number); //convert integer to string
    return strdup(set_strnum);
  }


  int decimal=0, sign=0;
  JsonString *temp_str=fcvt(number,MAX_DOUBLE_DIGITS-1,&decimal,&sign);

  int i=0;
  if (sign < 0)
    set_strnum[i++] = '-';

  if ((decimal < -7) || (decimal > 17)){ //print scientific notation 
    sprintf(set_strnum+i,"%c.%.7se%d",*temp_str,temp_str+1,decimal-1);
    return strdup(set_strnum);
  }

  char format[100];
  if (decimal > 0){
    sprintf(format,"%%.%ds.%%.7s",decimal);
    sprintf(set_strnum+i,format,temp_str,temp_str+decimal);
    return strdup(set_strnum);
  }

  if (decimal < 0){
    sprintf(format,"0.%0*d%%.7s",abs(decimal),0);
    sprintf(set_strnum+i,format,temp_str);
    return strdup(set_strnum);
  }

  sprintf(format,"0.%%.7s");
  sprintf(set_strnum+i,format,temp_str);
  return strdup(set_strnum);
}
