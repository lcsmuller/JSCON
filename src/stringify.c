#include "../json_parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#define DOUBLE_LEN 24

static void
cat_update(CJSON_data *data, char *buffer, int *i) {
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
format_number(CJSON_data new_data[], double number)
{
  int decimal=0, sign=0;
  CJSON_data *temp_data=fcvt(number,DOUBLE_LEN-1,&decimal,&sign);

  //check if value is integer
  if (number <= LLONG_MIN || number >= LLONG_MAX || number == (long long)number){
    sprintf(new_data,"%.lf",number); //convert integer to string
    return;
  }

  int i=0;
  if (sign < 0)
    new_data[i++] = '-';

  if ((decimal < -7) || (decimal > 17)){ //print scientific notation 
    sprintf(new_data+i,"%c.%.7se%d",*temp_data,temp_data+1,decimal-1);
    return;
  }

  char format[100];
  if (decimal > 0){
    sprintf(format,"%%.%ds.%%.7s",decimal);
    sprintf(new_data+i,format,temp_data,temp_data+decimal);
    return;
  }

  if (decimal < 0){
    sprintf(format,"0.%0*d%%.7s",abs(decimal),0);
    sprintf(new_data+i,format,temp_data);
    return;
  }

  sprintf(format,"0.%%.7s");
  sprintf(new_data+i,format,temp_data);
}

static void
stringify_number(double number, char *buffer, int *i)
{
  CJSON_data new_data[DOUBLE_LEN]={'\0'};
  format_number(new_data, number);

  cat_update(new_data,buffer,i); //store value in buffer
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

  char *buffer=calloc(1,cjson->memsize+200);

  int i=0;
  recursive_print(cjson->item, dtype, buffer, &i);
  buffer[i-1] = '\0';
   
  return buffer;
}
