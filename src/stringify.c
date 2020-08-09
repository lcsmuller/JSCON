#include "../json_parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#define DOUBLE_LEN 21

static void
cat_update(CJSON_data *data, char *buffer, int *i)
{
  while (*data)
    buffer[(*i)++] = *data++;
}

static void
stringify_data(CJSON_data *data, char *buffer, int *i)
{
  buffer[(*i)++] = '\"';
  cat_update(data,buffer,i);
  buffer[(*i)++] = '\"';
}

static void
rtrim(CJSON_data *eval_data, int c)
{
  CJSON_data *end=eval_data+(strlen(eval_data)-1);
  while (*end == c)
    *end-- = '\0';
}

static void
ltrim(CJSON_data *eval_data, int c)
{
  while (*eval_data == c){
    int i=0;
    while (eval_data[i] != '\0'){
      eval_data[i] = eval_data[i+1];
      ++i;
    }
  }
}

static void
remove_char(CJSON_data *eval_data, int c)
{
  for (int i=0; eval_data[i] != '\0'; ++i){
    if (eval_data[i] == c){
      while (eval_data[i] != '\0'){
        eval_data[i] = eval_data[i+1];
        ++i;
      }
      return;
    }
  }
}

static void
count_decimals(CJSON_data *temp_data, int *lcount, int *rcount)
{
  int i=0;
  int rdecimals=0; //negative decimals
  while (temp_data[i++] == '0'){
    if (temp_data[i] == '.'){
      ++i; //skips '.'
      continue; //skips rdecimals decrement
    }
    --rdecimals;
  }

  if (temp_data[strlen(temp_data)-1] == '.')
    temp_data[i] = '\0'; //fix: add this somewhere else

  i=0;
  int ldecimals=0; //positive decimals
  while (temp_data[i]){
    if (temp_data[i] == '.')
      break;
    ++i;
    ++ldecimals;
  }

  *lcount = ldecimals;
  *rcount = rdecimals;
}

static CJSON_data*
atoexp(CJSON_data *eval_data, int decimals)
{
  assert(decimals);

  remove_char(eval_data,'.');
  ltrim(eval_data,'0');

  CJSON_data new_data[DOUBLE_LEN];

  *new_data = *eval_data;
  *(new_data+1) = '.';
  strncpy(new_data+2,eval_data+1,DOUBLE_LEN-3);
  sprintf(new_data,"%se%d",new_data,decimals);

  strcpy(eval_data,new_data);

  return eval_data;
}

void
eval_numdata(CJSON_data *eval_data)
{
  int ldecimals=0, rdecimals=0;

  rtrim(eval_data,'0');
  count_decimals(eval_data, &ldecimals, &rdecimals);

  /* CONVERT TO SCIENTIFIC NOTATION*/
  if (ldecimals >= 18){
    atoexp(eval_data,ldecimals);
    return;
  }
  if ((rdecimals+ldecimals) < 0){
    atoexp(eval_data,rdecimals-ldecimals);
    return; 
  }
}

static void
stringify_number(double number, char *buffer, int *i)
{
  if (number < 0.0){ //ABSOLUTE VALUE FOR EASIER HANDLING
    number *= -1; //TURN NEGATIVE NUMBER POSITIVE
    buffer[(*i)++] = '-'; //SAVE MINUS SIGN IN BUFFER
  }

  CJSON_data eval_data[DOUBLE_LEN]={'\0'};
  snprintf(eval_data,DOUBLE_LEN-1,"%f",number);

  eval_numdata(eval_data);

  cat_update(eval_data,buffer,i);
}

static void
recursive_print(CJSON_item_t *item, CJSON_types_t dtype, char *buffer, int *i)
{
  if (item->dtype & dtype){
    if ((item->key) && !(item->parent->dtype & JsonArray)){
      stringify_data(item->key,buffer,i);
      buffer[(*i)++] = ':';
    }

    switch (item->dtype){
      case JsonNull:
        cat_update("null",buffer,i);
        break;
      case JsonBoolean:
        if (item->value.boolean){
          cat_update("true",buffer,i);
          break;
        }
        cat_update("false",buffer,i);
        break;
      case JsonNumber:
        stringify_number(item->value.number,buffer,i);
        break;
      case JsonString:
        stringify_data(item->value.string,buffer,i);
        break;
      case JsonObject:
        buffer[(*i)++] = '{';
        break;
      case JsonArray:
        buffer[(*i)++] = '[';
        break;
      default:
        fprintf(stderr,"ERROR: undefined datatype\n");
        exit(EXIT_FAILURE);
        break;
    }
  }

  for (size_t j=0 ; j < item->n ; ++j){
    recursive_print(item->properties[j], dtype, buffer,i);
    buffer[(*i)++] = ',';
  } 
   
  if ((item->dtype & dtype) & (JsonObject|JsonArray)){
    if (buffer[(*i)-1] == ',')
      --*i;

    if (item->dtype == JsonObject)
      buffer[(*i)++] = '}';
    else //is array 
      buffer[(*i)++] = ']';
  }
}

char*
stringify_json(CJSON_t *cjson, CJSON_types_t dtype)
{
  assert(cjson);
  assert(cjson->item->n);

  char *buffer=calloc(1,cjson->memsize);

  int i=0;
  recursive_print(cjson->item, dtype, buffer, &i);
  buffer[i-1] = '\0';
   
  return buffer;
}
