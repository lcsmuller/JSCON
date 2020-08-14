#include "../CJSON.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#define DOUBLE_LEN 24

static void
cat_update(CjsonString *data, char *json_text, int *i) {
  while (*data)
    json_text[(*i)++] = *data++;
}

static void
stringify_data(CjsonString *data, char *json_text, int *i)
{
  json_text[(*i)++] = '\"';
  cat_update(data,json_text,i);
  json_text[(*i)++] = '\"';
}

static void 
format_number(CjsonString new_data[], CjsonNumber number)
{
  int decimal=0, sign=0;
  CjsonString *temp_data=fcvt(number,DOUBLE_LEN-1,&decimal,&sign);

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
stringify_number(CjsonNumber number, char *json_text, int *i)
{
  CjsonString new_data[DOUBLE_LEN]={'\0'};
  format_number(new_data, number);

  cat_update(new_data,json_text,i); //store value in json_text
}

static void
CjsonItem_recprint(CjsonItem *item, CjsonDType dtype, char *json_text, int *i)
{
  if (item->dtype & dtype){
    if ((item->key) && !(item->parent->dtype & Array)){
      stringify_data(item->key,json_text,i);
      json_text[(*i)++] = ':';
    }

    switch (item->dtype){
      case Null:
        cat_update("null",json_text,i);
        break;
      case Boolean:
        if (item->value.boolean){
          cat_update("true",json_text,i);
          break;
        }
        cat_update("false",json_text,i);
        break;
      case Number:
        stringify_number(item->value.number,json_text,i);
        break;
      case String:
        stringify_data(item->value.string,json_text,i);
        break;
      case Object:
        json_text[*i] = '{';
        ++*i;
        break;
      case Array:
        json_text[*i] = '[';
        ++*i;
        break;
      default:
        fprintf(stderr,"ERROR: undefined datatype\n");
        exit(EXIT_FAILURE);
        break;
    }
  }

  for (size_t j=0 ; j < item->n ; ++j){
    CjsonItem_recprint(item->property[j], dtype, json_text,i);
    json_text[*i] = ',';
    ++*i;
  } 
   
  if ((item->dtype & dtype) & (Object|Array)){
    if (json_text[(*i)-1] == ',')
      --*i;

    if (item->dtype == Object)
      json_text[*i] = '}';
    else //is array 
      json_text[*i] = ']';
    ++*i;
  }
}

char*
Cjson_stringify(Cjson *cjson, CjsonDType dtype)
{
  assert(cjson);
  assert(cjson->item->n);

  char *json_text=calloc(1,611097);

  int i=0;
  CjsonItem_recprint(cjson->item, dtype, json_text, &i);
  json_text[i-1] = '\0';
   
  return json_text;
}
