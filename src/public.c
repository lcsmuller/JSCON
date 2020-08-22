#include "../JSON.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

JsonItem*
Json_GetItem(Json* json){
  return json->item_ptr;
}

JsonItem*
Json_GetRoot(Json* json){
  return json->root;
}

static int
push(Json* json)
{
  if ((json->stack.top - json->stack.trace) >= json->stack.max_depth){
    fprintf(stderr,"ERR: Stack overflow\n");
    exit(EXIT_FAILURE);
  }
  
  json->item_ptr = json->item_ptr->property[++*json->stack.top];
  ++json->stack.top; //update top

  return 1;
}

static int
pop(Json* json)
{
  if (json->stack.top < json->stack.trace){
    fprintf(stderr,"ERR: Stack underflow\n");
    exit(EXIT_FAILURE);
  }

  json->item_ptr = json->item_ptr->parent;
  --json->stack.top; //update top
  --*json->stack.top;

  return 1;
}

static int
branch(Json* json)
{
  ++*json->stack.top;

  return 1;
}

JsonItem*
Json_NextItem(Json* json)
{
  if (!json->item_ptr)
    return NULL;

  int c, i=0;
  printf("\nMAX_DEPTH: %d", json->stack.max_depth);
  do {
    printf("\nPUSH:1, POP:2, BRANCH:3, EXIT:4 ");
    scanf("%d",&c);
    switch(c){
      case 1:
        push(json);
        fprintf(stderr, "key: %s depth: %d", json->item_ptr->key, ++i);
        break;
      case 2:
        pop(json);
        fprintf(stderr, "key: %s depth: %d", json->item_ptr->key, --i);
        break;
      case 3:
        branch(json);
        break;
      case 4:
      default:
        break;
    }
    printf("TOP: ");
    for (int j=0; j < json->stack.max_depth; ++j){
      printf("%d ",json->stack.trace[j]);
    }
    fprintf(stderr,"\n\n");
  } while(c != 4);

  return NULL;
}

JsonString*
Json_SearchKey(Json* json, const JsonString search_key[])
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
  const JsonItem* parent = origin->parent;
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

void 
JsonNumber_StrFormat(JsonNumber number, JsonString* ptr, const int digits)
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
