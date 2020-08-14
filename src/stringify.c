#include "../CJSON.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#define DOUBLE_LEN 24

static void
cat_update(CjsonString *data, char *buffer, int *i) {
  while (*data)
    buffer[(*i)++] = *data++;
}

static void
stringify_data(CjsonString *data, char *buffer, int *i)
{
  buffer[(*i)++] = '\"';
  cat_update(data,buffer,i);
  buffer[(*i)++] = '\"';
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
stringify_number(CjsonNumber number, char *buffer, int *i)
{
  CjsonString new_data[DOUBLE_LEN]={'\0'};
  format_number(new_data, number);

  cat_update(new_data,buffer,i); //store value in buffer
}

static void
CjsonItem_recprint(CjsonItem *item, CjsonDType dtype, char *buffer, int *i)
{
  if (item->dtype & dtype){
    if ((item->key) && !(item->parent->dtype & Array)){
      stringify_data(item->key,buffer,i);
      buffer[(*i)++] = ':';
    }

    switch (item->dtype){
      case Null:
        cat_update("null",buffer,i);
        break;
      case Boolean:
        if (item->value.boolean){
          cat_update("true",buffer,i);
          break;
        }
        cat_update("false",buffer,i);
        break;
      case Number:
        stringify_number(item->value.number,buffer,i);
        break;
      case String:
        stringify_data(item->value.string,buffer,i);
        break;
      case Object:
        buffer[*i] = '{';
        ++*i;
        break;
      case Array:
        buffer[*i] = '[';
        ++*i;
        break;
      default:
        fprintf(stderr,"ERROR: undefined datatype\n");
        exit(EXIT_FAILURE);
        break;
    }
  }

  for (size_t j=0 ; j < item->n ; ++j){
    CjsonItem_recprint(item->property[j], dtype, buffer,i);
    buffer[*i] = ',';
    ++*i;
  } 
   
  if ((item->dtype & dtype) & (Object|Array)){
    if (buffer[(*i)-1] == ',')
      --*i;

    if (item->dtype == Object)
      buffer[*i] = '}';
    else //is array 
      buffer[*i] = ']';
    ++*i;
  }
}

char*
Cjson_stringify(Cjson *cjson, CjsonDType dtype)
{
  assert(cjson);
  assert(cjson->item->n);

  char *buffer=calloc(1,611097);

  int i=0;
  CjsonItem_recprint(cjson->item, dtype, buffer, &i);
  buffer[i-1] = '\0';
   
  return buffer;
}
