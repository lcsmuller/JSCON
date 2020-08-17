#include "../JSON.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

Json*
Json_create()
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
JsonString_getkey(Json *json, char *cache_entry)
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
JsonString_setarray_key(Json *json, JsonItem *item)
{
  const int len=25;
  //will be free'd inside jsonString_getkey if necessary
  JsonString *cache_entry=malloc(len);
  assert(cache_entry);

  snprintf(cache_entry,len-1,"%ld",item->n_property);

  return JsonString_getkey(json, cache_entry);
}

static JsonString*
JsonString_set(char **ptr_buffer)
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
JsonItem_setvalue(JsonItem *item, char **ptr_buffer)
{
  switch (item->dtype){
    case String:
      item->string = JsonString_set(ptr_buffer);
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

/* create nested object/array and return
    the nested object/array address. */
static JsonItem*
JsonItem_setobject(JsonItem *item, JsonString *get_key, JsonDType get_dtype, char **ptr_buffer)
{
  item = JsonItem_create(item);
  item->key = get_key;
  item->dtype = get_dtype;

  return item;
}


/* create property from appointed JSON datatype
    and return the item containing it */
static JsonItem*
JsonItem_setproperty(JsonItem *item, JsonString *get_key, JsonDType get_dtype, char **ptr_buffer)
{
  item = JsonItem_setobject(item, get_key, get_dtype, ptr_buffer);
  JsonItem_setvalue(item, ptr_buffer);

  return item->parent; //wraps item
}

/* get json  by evaluating buffer's current position */
static JsonItem*
JsonItem_build(Json *json, JsonItem *item, short *found_key, JsonString **ptr_set_key, char **ptr_buffer)
{
  JsonItem* (*setter)(JsonItem *item, JsonString *key, JsonDType dtype, char **ptr_buffer) = NULL;

  JsonDType set_dtype=0;

  switch (*(*ptr_buffer)++){
    case ',':/*KEY DETECTED*/
      *found_key = 1;
      return item;
    case '\"':/*STRING DETECTED*/
      if ((item->dtype != Array) && (*found_key)){
        /* is a key string */
        *ptr_set_key = JsonString_getkey(json,JsonString_set(ptr_buffer));
        *found_key = 0;
        return item;
      }
      /* is normal string  */
      setter = &JsonItem_setproperty;
      set_dtype = String;
      break;
    case '{':/*OBJECT DETECTED*/
      setter = &JsonItem_setobject;
      set_dtype = Object;
      *found_key = 1;
      break;
    case '[':/*ARRAY DETECTED*/
      setter = &JsonItem_setobject;
      set_dtype = Array;
      break;
    case '}': /*WRAPPER*/
    case ']':/*DETECTED*/
      //WRAPS OBJECT or ARRAY, ALL OF ITS MEMBERS HAVE BEEN FOUND
      return item->parent;
    case 't': /* CHECK FOR */
    case 'f':/*TRUE or FALSE*/
      if (strncmp(*ptr_buffer-1,"true",4) && strncmp(*ptr_buffer-1,"false",5))
        return item;
      setter = &JsonItem_setproperty;
      set_dtype = Boolean;
      break;
    case 'n':/*CHECK FOR NULL*/
      if (strncmp(*ptr_buffer-1,"null",4))
        return item;
      setter = &JsonItem_setproperty;
      set_dtype = Null;
      break;
    default:/*CHECK FOR NUMBER*/
      if ((!isdigit((*ptr_buffer)[-1])) && ((*ptr_buffer)[-1] != '-'))
        return item;
      setter = &JsonItem_setproperty;
      set_dtype = Number;
      break;
  }

  /* if hasn't reach a return thus far, it means the json item
    is ready to be created (all of its composing information
    have been found) */

  if (item->dtype == Array) //creates key for array element
    *ptr_set_key = JsonString_setarray_key(json,item);

  item = (*setter)(item, *ptr_set_key, set_dtype, ptr_buffer);
  *ptr_set_key = NULL;

  return item;
}

Json*
Json_parse(char *buffer)
{
  Json *json=Json_create();

  JsonItem *item=json->root;
  JsonString *set_key=NULL;
  short found_key=0;
  while (*buffer){ //while not null terminator char
    item = JsonItem_build(json, item, &found_key, &set_key, &buffer);
  }

  return json;
}

static void
apply_reviver(JsonItem *item, void (*reviver)(JsonItem*))
{
  (*reviver)(item);
  for (size_t i=0; i < item->n_property; ++i)
    apply_reviver(item->property[i], reviver);
}

Json*
Json_parse_reviver(char *buffer, void (*reviver)(JsonItem*))
{
  Json *json=Json_parse(buffer);

  if (reviver)
    apply_reviver(json->root, reviver);

  return json;
}

