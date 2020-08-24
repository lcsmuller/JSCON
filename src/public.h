#include <stdio.h>
#include "../JSON.h"


JsonItem* Json_NextItem(Json* json);
void Json_Rewind(Json* json);
int Json_SearchKey(const Json* json, const JsonString search_key);
int Json_ReplaceKeyAll(const Json* json, const JsonString old_key, const JsonString new_key);
JsonItem* Json_GetItem(const Json* json);
JsonItem* Json_GetRoot(const Json* json);


void JsonItem_TypeOf(const JsonItem* item, FILE* stream);
int JsonItem_DatatypeCmp(const JsonItem* item, const JsonDType dtype);
int JsonItem_KeyCmp(const JsonItem* item, const JsonString key);
int JsonItem_NumberCmp(const JsonItem* item, const JsonNumber number);
JsonItem* JsonItem_GetSibling(const JsonItem* origin, const long int rel_index);
JsonItem* JsonItem_GetParent(const JsonItem* item);
JsonItem* JsonItem_GetProperty(const JsonItem* item, const size_t index);
size_t JsonItem_GetPropertyCount(const JsonItem* item);
JsonDType JsonItem_GetDatatype(const JsonItem* item);
JsonString JsonItem_GetKey(const JsonItem* item);
JsonBool JsonItem_GetBoolean(const JsonItem* item);
JsonString JsonItem_GetString(const JsonItem* item);


void JsonNumber_StrFormat(const JsonNumber number, JsonString ptr, const int digits);
