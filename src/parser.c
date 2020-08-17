#include "../JSON.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

Json*
Json_Create()
{
  Json *new_json=calloc(1,sizeof(Json));
  assert(new_json);

  new_json->root=calloc(1,sizeof(JsonItem));
  assert(new_json->root);

  return new_json;
}

/* destroy current item and all of its nested object/arrays */
static void
JsonItem_destroy(JsonItem *item)
{
  for (size_t i=0; i < item->n_property; ++i)
    JsonItem_destroy(item->property[i]);
  free(item->property);

  if (item->dtype == String)
    free(item->string);
  free(item);
}

/* destroy json struct */
void
Json_destroy(Json *json)
{
  JsonItem_destroy(json->root);
  
  while (json->n_keylist){
    free(json->keylist[--json->n_keylist]);
  }
  free(json->keylist);
  free(json);
}

/* create new json item and return it's address */
static JsonItem*
JsonItem_create(JsonItem *item)
{
  ++item->n_property; //update object's property count
  //update memory space for property's list
  item->property = realloc(item->property, item->n_property*sizeof(JsonItem*));
  assert(item->property);
  //allocate memory space for new property (which is a nested item)
  item->property[item->n_property-1] = calloc(1,sizeof(JsonItem));
  assert(item->property[item->n_property-1]);
  //get parent address of the new property
  item->property[item->n_property-1]->parent = item;
  //return new property address
  return item->property[item->n_property-1];
}

static JsonString*
JsonString_cache_key(Json *json, JsonString *cache_entry)
{
  ++json->n_keylist;
  json->keylist = realloc(json->keylist,json->n_keylist*sizeof(char*));
  assert(json->keylist);

  int i=json->n_keylist-1;
  while ((i > 0) && (strcmp(cache_entry, json->keylist[i-1]) < 0)){
    json->keylist[i] = json->keylist[i-1];
    --i;
  }
  json->keylist[i] = cache_entry;

  return json->keylist[i];
}

static JsonString*
JsonString_GetKey(Json *json, char *cache_entry)
{
  int top=json->n_keylist-1;
  int low=0;
  int mid;

  int cmp;
  while (low <= top){
    mid = ((ulong)low + (ulong)top) >> 1;
    cmp=strcmp(cache_entry, json->keylist[mid]);
    if (cmp == 0){
      free(cache_entry);
      return json->keylist[mid];
    }
    if (cmp < 0)
      top = mid-1;
    else
      low = mid+1;
  }

  return JsonString_cache_key(json, cache_entry);
}

/* get numerical key for array type
    json data, in string format */
static JsonString*
JsonString_SetArrKey(Json *json, JsonItem *item)
{
  const int len=25;
  //will be free'd inside jsonString_GetKey if necessary
  JsonString *cache_entry=malloc(len);
  assert(cache_entry);

  snprintf(cache_entry,len-1,"%ld",item->n_property);

  return JsonString_GetKey(json, cache_entry);
}

static JsonString*
JsonString_Set(char **ptr_buffer)
{
  char *start=*ptr_buffer;
  char *end=start;

  while (*end != '\"'){
    if (*end++ == '\\'){ //skips \" char
      ++end;
    }
  }

  JsonString *data=strndup(start, end-start);

  *ptr_buffer = end+1;

  return data;
}

static JsonNumber
JsonNumber_set(char **ptr_buffer)
{
  char *start=*ptr_buffer-1;
  char *end=start;

  if (*end == '-')
    ++end;

  while (isdigit(*end))
    ++end;

  if (*end == '.'){
    while (isdigit(*++end))
      continue;
  }
  //check for exponent
  if ((*end == 'e') || (*end == 'E')){
    ++end;
    if ((*end == '+') || (*end == '-'))
      ++end;
    while (isdigit(*end))
      ++end;
  }

  JsonString *data=strndup(start, end-start);
  assert(data);

  JsonNumber number;
  sscanf(data,"%lf",&number);

  free(data);

  *ptr_buffer = end;

  return number;
}

/* get and return data from appointed datatype */
static void
JsonItem_SetValue(JsonItem *item, char **ptr_buffer)
{
  switch (item->dtype){
    case String:
      item->string = JsonString_Set(ptr_buffer);
      break;
    case Number:
      item->number = JsonNumber_set(ptr_buffer);
      break;
    case Boolean:
      if ((*ptr_buffer)[-1] == 't'){
        *ptr_buffer += 3; //length of "true"-1
        item->boolean = 1;
        break;
      }
      *ptr_buffer += 4; //length of "false"-1
      item->boolean = 0;
      break;
    case Null:
      *ptr_buffer += 3; //length of "null"-1
      break;
    default:
      fprintf(stderr,"ERROR: invalid datatype %ld\n", item->dtype);
      exit(EXIT_FAILURE);
  }
}

/* create nested object and return
    the nested object address. */
static JsonItem*
JsonItem_SetIncomplete(JsonItem *item, JsonString *get_key, JsonDType get_dtype, char **ptr_buffer)
{
  item = JsonItem_create(item);
  item->key = get_key;
  item->dtype = get_dtype;

  get_key = NULL;

  return item;
}

/* create property from appointed JSON datatype
    and return the item containing it */
static JsonItem*
JsonItem_SetComplete(JsonItem *item, JsonString *get_key, JsonDType get_dtype, char **ptr_buffer)
{
  item = JsonItem_SetIncomplete(item, get_key, get_dtype, ptr_buffer);
  JsonItem_SetValue(item, ptr_buffer);

  return item->parent; //wraps property in item
}

static JsonItem*
JsonItem_BuildArray(Json *json, JsonItem *item, short *found_key, char **ptr_buffer)
{
    switch (*(*ptr_buffer)++){
      case ']':/*ARRAY WRAPPER DETECTED*/
        return item->parent;
      case '\"':/*STRING DETECTED*/
        return JsonItem_SetComplete(item,JsonString_SetArrKey(json,item),String,ptr_buffer);
      case '{':/*OBJECT DETECTED*/
        *found_key = 1;
        return JsonItem_SetIncomplete(item,JsonString_SetArrKey(json,item),Object,ptr_buffer);
      case '[':/*ARRAY DETECTED*/
        return JsonItem_SetIncomplete(item,JsonString_SetArrKey(json,item),Array,ptr_buffer);
      case 't':/*CHECK FOR*/
      case 'f':/* BOOLEAN */
        if (strncmp(*ptr_buffer-1,"true",4) 
            && strncmp(*ptr_buffer-1,"false",5))
          return item;
        return JsonItem_SetComplete(item,JsonString_SetArrKey(json,item),Boolean,ptr_buffer);
      case 'n':/*CHECK FOR NULL*/
        if (strncmp(*ptr_buffer-1,"null",4))
          return item;
        return JsonItem_SetComplete(item,JsonString_SetArrKey(json,item),Null,ptr_buffer);
      default:/*CHECK FOR NUMBER*/
        if (!isdigit((*ptr_buffer)[-1])
            && ((*ptr_buffer)[-1] != '-'))
          return item;
        return JsonItem_SetComplete(item,JsonString_SetArrKey(json,item),Number,ptr_buffer);
    }
}

static JsonItem*
JsonItem_BuildObject(Json *json, JsonItem *item, short *found_key, JsonString **ptr_key, char **ptr_buffer)
{
  switch (*(*ptr_buffer)++){
    case '}':/*OBJECT WRAPPER DETECTED*/
      return item->parent;
    case ',':/*KEY DETECTED*/
      *found_key = 1;
      return item;
    case '\"':/*STRING DETECTED*/
      if (*found_key){
        /*key string is set*/
        *ptr_key = JsonString_GetKey(json,JsonString_Set(ptr_buffer));
        *found_key = 0;
        return item;
      }
      /*value string is set*/
      return JsonItem_SetComplete(item,*ptr_key,String,ptr_buffer);
    case '{':/*OBJECT DETECTED*/
      *found_key = 1;
      return JsonItem_SetIncomplete(item,*ptr_key,Object,ptr_buffer);
    case '[':/*ARRAY DETECTED*/
      return JsonItem_SetIncomplete(item,*ptr_key,Array,ptr_buffer);
    case 't':/*CHECK FOR*/
    case 'f':/* BOOLEAN */
      if (strncmp(*ptr_buffer-1,"true",4) 
          && strncmp(*ptr_buffer-1,"false",5))
        return item;
      return JsonItem_SetComplete(item,*ptr_key,Boolean,ptr_buffer);
    case 'n':/*CHECK FOR NULL*/
      if (strncmp(*ptr_buffer-1,"null",4))
        return item;
      return JsonItem_SetComplete(item,*ptr_key,Null,ptr_buffer);
    default:/*CHECK FOR NUMBER*/
      if (!isdigit((*ptr_buffer)[-1])
          && ((*ptr_buffer)[-1] != '-'))
        return item;
      return JsonItem_SetComplete(item,*ptr_key,Number,ptr_buffer);
  }
}

static JsonItem*
JsonItem_BuildEntity(Json *json, JsonItem *item, short *found_key, JsonString **ptr_key, char **ptr_buffer)
{
  switch (*(*ptr_buffer)++){
    case '{':/*OBJECT DETECTED*/
      *found_key = 1;
      return JsonItem_SetIncomplete(item,*ptr_key,Object,ptr_buffer);
    case '[':/*ARRAY DETECTED*/
      return JsonItem_SetIncomplete(item,*ptr_key,Array,ptr_buffer);
    case '\"':/*STRING DETECTED*/
      item->dtype = String;
      break;
    case 't':/*CHECK FOR*/
    case 'f':/* BOOLEAN */
      if (strncmp(*ptr_buffer-1,"true",4) 
          && strncmp(*ptr_buffer-1,"false",5))
        return item;
      item->dtype = Boolean;
      break;
    case 'n':/*CHECK FOR NULL*/
      if (strncmp(*ptr_buffer-1,"null",4))
        return item;
      item->dtype = Null;
      break;
    default:/*CHECK FOR NUMBER*/
      if (!isdigit((*ptr_buffer)[-1])
          && ((*ptr_buffer)[-1] != '-'))
        return item;
      item->dtype = Number;
      break;
  }

  JsonItem_SetValue(item, ptr_buffer);
  return item;
}

/* get json  by evaluating buffer's current position */
static JsonItem*
JstonItem_Build(Json *json, JsonItem *item, short *found_key, JsonString **ptr_key, char **ptr_buffer)
{
  switch(item->dtype){
    case Array:
      return JsonItem_BuildArray(json,item,found_key,ptr_buffer);
    case Object:
      return JsonItem_BuildObject(json,item,found_key,ptr_key,ptr_buffer);
    case Undefined:
      return JsonItem_BuildEntity(json,item,found_key,ptr_key,ptr_buffer);
    default: //nothing else to do
      ++*ptr_buffer;
      return item;
  }
}

Json*
Json_parse(char *buffer)
{
  Json *json=Json_Create();

  JsonItem *item=json->root;
  JsonString *set_key=NULL;
  short found_key=0;

  while (*buffer){ //while not null terminator char
    item = JstonItem_Build(json,item,&found_key,&set_key,&buffer);
  }

  return json;
}

static void
apply_reviver(JsonItem *item, void (*reviver)(JsonItem*))
{
  (*reviver)(item);
  for (size_t i=0; i < item->n_property; ++i){
    apply_reviver(item->property[i], reviver);
  }
}

Json*
Json_parse_reviver(char *buffer, void (*reviver)(JsonItem*))
{
  Json *json=Json_parse(buffer);

  if (reviver){
    apply_reviver(json->root, reviver);
  }

  return json;
}

