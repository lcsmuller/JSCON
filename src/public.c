#include "../JSON.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
