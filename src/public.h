#include "../JSON.h"

int JsonItem_DatatypeCmp(JsonItem* item, JsonDType dtype);
int JsonItem_KeyCmp(JsonItem* item, JsonString key[]);
int JsonItem_NumberCmp(JsonItem* item, JsonNumber number);

JsonItem* JsonItem_GetParent(JsonItem* item);
JsonItem* JsonItem_GetProperty(JsonItem* item, size_t index);
JsonItem* JsonItem_GetSibling(const JsonItem* origin, long int relative_index);

JsonString* JsonItem_GetString(JsonItem* item);
