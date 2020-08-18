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

/* Destroy current item and all of its nested object/arrays */
static void
JsonItem_Destroy(JsonItem *item)
{
  for (size_t i=0; i < item->n_property; ++i)
    JsonItem_Destroy(item->property[i]);
  free(item->property);

  if (item->dtype == String)
    free(item->string);
  free(item);
}

/* Destroy json struct */
void
Json_Destroy(Json *json)
{
  JsonItem_Destroy(json->root);
  
  while (json->n_keylist){
    free(json->keylist[--json->n_keylist]);
  }
  free(json->keylist);
  free(json);
}

/* create new json item and return it's address */
static JsonItem*
JsonItem_Create(JsonItem *item)
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
JsonString_CacheKey(Json *json, JsonString *cache_entry)
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
    cmp = strcmp(cache_entry, json->keylist[mid]);
    if (cmp == 0){
      free(cache_entry);
      return json->keylist[mid];
    }
    if (cmp < 0)
      top = mid-1;
    else
      low = mid+1;
  }

  return JsonString_CacheKey(json, cache_entry);
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
JsonNumber_Set(char **ptr_buffer)
{
  char *start=*ptr_buffer-1;
  char *end=start;

  if (*end == '-'){
    ++end;
  }
  while (isdigit(*end)){
    ++end;
  }
  if (*end == '.'){
    while (isdigit(*++end))
      continue;
  }
  //check for exponent
  if ((*end == 'e') || (*end == 'E')){
    ++end;
    if ((*end == '+') || (*end == '-')){
      ++end;
    }
    while (isdigit(*end)){
      ++end;
    }
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
static JsonItem*
JsonItem_SetValue(JsonDType get_dtype, JsonItem *item, char **ptr_buffer)
{
  item->dtype = get_dtype;
  switch (item->dtype){
    case String:
      item->string = JsonString_Set(ptr_buffer);
      break;
    case Number:
      item->number = JsonNumber_Set(ptr_buffer);
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
    case Array:
    case Object:
      break;
    default:
      fprintf(stderr,"ERROR: invalid datatype %ld\n", item->dtype);
      exit(EXIT_FAILURE);
  }

  return item;
}

/* create nested object and return
    the nested object address. */
static JsonItem*
JsonItem_SetIncomplete(JsonItem *item, JsonString *get_key, JsonDType get_dtype, char **ptr_buffer)
{
  item = JsonItem_Create(item);
  item->key = get_key;

  item = JsonItem_SetValue(get_dtype, item, ptr_buffer);

  get_key = NULL;

  return item;
}

/* create property from appointed JSON datatype
    and return the item containing it */
static JsonItem*
JsonItem_SetComplete(JsonItem *item, JsonString *get_key, JsonDType get_dtype, char **ptr_buffer)
{
  item = JsonItem_SetIncomplete(item, get_key, get_dtype, ptr_buffer);
  return item->parent; //wraps property in item (completes it)
}

static JsonItem*
JsonItem_BuildArray(Json *json, JsonItem *item, char **ptr_buffer)
{
  JsonItem* (*item_setter)(JsonItem*,JsonString*,JsonDType,char**);
  JsonDType set_dtype;
  switch (*(*ptr_buffer)++){
    case ']':/*ARRAY WRAPPER DETECTED*/
      return item->parent;
    case '{':/*OBJECT DETECTED*/
      set_dtype = Object;
      item_setter = &JsonItem_SetIncomplete;
      break;
    case '[':/*ARRAY DETECTED*/
      set_dtype = Array;
      item_setter = &JsonItem_SetIncomplete;
      break;
    case '\"':/*STRING DETECTED*/
      set_dtype = String;
      item_setter = &JsonItem_SetComplete;
      break;
    case 't':/*CHECK FOR*/
    case 'f':/* BOOLEAN */
      if (strncmp(*ptr_buffer-1,"true",4) 
          && strncmp(*ptr_buffer-1,"false",5)){
        goto error;
      }
      set_dtype = Boolean;
      item_setter = &JsonItem_SetComplete;
      break;
    case 'n':/*CHECK FOR NULL*/
      if (strncmp(*ptr_buffer-1,"null",4)){
        goto error;
      }
      set_dtype = Null;
      item_setter = &JsonItem_SetComplete;
      break;
    case ',': /*ignore comma*/
      return item;
    default:
      /*IGNORE CONTROL CHARACTER*/
      if (isspace((*ptr_buffer)[-1]) || iscntrl((*ptr_buffer)[-1])){
        return item;
      }
      /*CHECK FOR NUMBER*/
      if (!isdigit((*ptr_buffer)[-1]) && ((*ptr_buffer)[-1] != '-')){
        goto error;
      }
      set_dtype = Number;
      item_setter = &JsonItem_SetComplete;
      break;
  }

  return (*item_setter)(item,JsonString_SetArrKey(json,item),set_dtype,ptr_buffer);

  error:
    fprintf(stderr,"ERROR: invalid json token\n");
    exit(EXIT_FAILURE);
}

static JsonItem*
JsonItem_BuildObject(Json *json, JsonItem *item, JsonString **ptr_key, char **ptr_buffer)
{
  JsonItem* (*item_setter)(JsonItem*,JsonString*,JsonDType,char**);
  JsonDType set_dtype;
  switch (*(*ptr_buffer)++){
    case '}':/*OBJECT WRAPPER DETECTED*/
      return item->parent;
    case '\"':/*KEY STRING DETECTED*/
        *ptr_key = JsonString_GetKey(json,JsonString_Set(ptr_buffer));
        return item;
    case ':':/*VALUE DETECTED*/
        switch (*(*ptr_buffer)++){ //fix: move to function
          case '{':/*OBJECT DETECTED*/
            set_dtype = Object;
            item_setter = &JsonItem_SetIncomplete;
            break;
          case '[':/*ARRAY DETECTED*/
            set_dtype = Array;
            item_setter = &JsonItem_SetIncomplete;
            break;
          case '\"':/*STRING DETECTED*/
            set_dtype = String;
            item_setter = &JsonItem_SetComplete;
            break;
          case 't':/*CHECK FOR*/
          case 'f':/* BOOLEAN */
            if (strncmp(*ptr_buffer-1,"true",4)
                && strncmp(*ptr_buffer-1,"false",5)){
              goto error;
            }
            set_dtype = Boolean;
            item_setter = &JsonItem_SetComplete;
            break;
          case 'n':/*CHECK FOR NULL*/
            if (strncmp(*ptr_buffer-1,"null",4)){
              goto error; 
            }
            set_dtype = Null;
            item_setter = &JsonItem_SetComplete;
            break;
          default:
            /*CHECK FOR NUMBER*/
            if (!isdigit((*ptr_buffer)[-1])
                && ((*ptr_buffer)[-1] != '-')){
              goto error;
            }
            set_dtype = Number;
            item_setter = &JsonItem_SetComplete;
            break;
        }
      return (*item_setter)(item,*ptr_key,set_dtype,ptr_buffer);
    case ',': //ignore comma
      return item;
    default:
      /*IGNORE CONTROL CHARACTER*/
      if (isspace((*ptr_buffer)[-1]) || iscntrl((*ptr_buffer)[-1])){
        return item;
      }
      goto error;
  }

  error:
    fprintf(stderr,"ERROR: invalid json token %c\n", (*ptr_buffer)[-1]);
    exit(EXIT_FAILURE);
}

static JsonItem*
JsonItem_BuildEntity(Json *json, JsonItem *item, JsonString **ptr_key, char **ptr_buffer)
{
  JsonDType set_dtype=0;
  switch (*(*ptr_buffer)++){
    case '{':/*OBJECT DETECTED*/
      item->parent = item;
      set_dtype = Object;
      break;
    case '[':/*ARRAY DETECTED*/
      item->parent = item;
      set_dtype = Array;
      break;
    case '\"':/*STRING DETECTED*/
      set_dtype = String;
      break;
    case 't':/*CHECK FOR*/
    case 'f':/* BOOLEAN */
      if (strncmp(*ptr_buffer-1,"true",4) && strncmp(*ptr_buffer-1,"false",5)){
        goto error;
      }
      set_dtype = Boolean;
      break;
    case 'n':/*CHECK FOR NULL*/
      if (strncmp(*ptr_buffer-1,"null",4)){
        goto error;
      }
      set_dtype = Null;
      break;
    default:
      /*IGNORE CONTROL CHARACTER*/
      if (isspace((*ptr_buffer)[-1]) || iscntrl((*ptr_buffer)[-1])){
        return item;
      }
      /*CHECK FOR NUMBER*/
      if (!isdigit((*ptr_buffer)[-1]) && ((*ptr_buffer)[-1] != '-')){
        goto error;
      }
      set_dtype = Number;
      break;
  }

  return JsonItem_SetValue(set_dtype, item, ptr_buffer);

  error:
    fprintf(stderr,"ERROR: invalid json token\n");
    exit(EXIT_FAILURE);
}

/* get json  by evaluating buffer's current position */
static JsonItem*
JsonItem_Build(Json *json, JsonItem *item, JsonString **ptr_key, char **ptr_buffer)
{
  switch(item->dtype){
    case Object:
      return JsonItem_BuildObject(json,item,ptr_key,ptr_buffer);
    case Array:
      return JsonItem_BuildArray(json,item,ptr_buffer);
    case Undefined:
      return JsonItem_BuildEntity(json,item,ptr_key,ptr_buffer);
    default: //nothing else to build, check buffer for potential error
      if (isspace(**ptr_buffer) || iscntrl(**ptr_buffer)){
        ++*ptr_buffer; //moves if cntrl character found ('\n','\b',..)
        return item;
      }
      goto error;
  }

  error:
    fprintf(stderr,"ERROR: invalid json token %c\n",**ptr_buffer);
    exit(EXIT_FAILURE);
}

Json*
Json_Parse(char *buffer)
{
  Json *json=Json_Create();

  JsonItem *item=json->root;
  JsonString *set_key=NULL;

  while (*buffer){ //while not null terminator char
    item = JsonItem_Build(json,item,&set_key,&buffer);
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
Json_ParseReviver(char *buffer, void (*reviver)(JsonItem*))
{
  Json *json=Json_Parse(buffer);

  if (reviver){
    apply_reviver(json->root, reviver);
  }

  return json;
}

