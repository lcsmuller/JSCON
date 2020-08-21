#include "../JSON.h"

JsonItem* Json_GetItem(Json* json);
JsonItem* Json_GetRoot(Json* json);
JsonString* Json_SearchKey(Json* json, JsonString search_key[]);
int Json_SubKey(Json* json, JsonString old_key[], JsonString new_key[]);

JsonItem* JsonItem_GetParent(JsonItem* item);
JsonItem* JsonItem_GetProperty(JsonItem* item, size_t index);
JsonItem* JsonItem_GetSibling(const JsonItem* origin, long int relative_index);
int JsonItem_DatatypeCmp(JsonItem* item, JsonDType dtype);
int JsonItem_KeyCmp(JsonItem* item, JsonString key[]);
int JsonItem_NumberCmp(JsonItem* item, JsonNumber number);
JsonString* JsonItem_GetString(JsonItem* item);

JsonString* JsonNumber_StrFormat(JsonNumber number);
