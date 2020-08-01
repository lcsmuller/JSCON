#include "../json_parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

static long fetch_filesize(FILE *ptr_file)
{
  fseek(ptr_file, 0, SEEK_END);
  long filesize=ftell(ptr_file);
  fseek(ptr_file, 0, SEEK_SET);
  return filesize;
}

static char *read_file(FILE* ptr_file, long filesize)
{
  char *buffer = malloc(filesize+1);
  fread(buffer, sizeof(char), filesize, ptr_file);
  buffer[filesize] = '\0';

  return buffer;
}

char*
read_json_file(char file[])
{
  FILE *json_file = fopen(file, "rb");
  assert(json_file);

  //stores file size
  long filesize = fetch_filesize(json_file);
  //buffer reads file content
  char *buffer = read_file(json_file, filesize);

  fclose(json_file);

  return buffer;
}

static json_item*
new_nested_object(json_item *item)
{
  ++item->obj.n;

  item->obj.properties = realloc(item->obj.properties, sizeof(json_item*)*item->obj.n);
  assert(item->obj.properties);

  item->obj.properties[item->obj.n-1] = calloc(1, sizeof(json_item));
  assert(item->obj.properties[item->obj.n-1]);

  item->obj.properties[item->obj.n-1]->obj.parent = item;

  return item->obj.properties[item->obj.n-1];
}

static void
set_json_object(json_item *item, json_data *data, unsigned long datatype, char *buffer)
{
  item->val.start = buffer-1;
  item->datatype = datatype;
  item->key = *data;
}

/* closes item nest (found right bracket)
 * updates length with amount of chars between brackets
 * returns to its immediate parent nest
 */
static json_item*
wrap_nested_object(json_item *item, char *buffer)
{
  item->val.end = buffer;
  item->val.length = item->val.end - item->val.start;

  return item->obj.parent;
}

static void
json_data_token(json_data *data, unsigned long datatype, char **ptr_buffer)
{
  char *buffer = *ptr_buffer;

  data->start = buffer-1;
  switch (datatype){
    case JSON_String:
      while (*buffer != DOUBLE_QUOTES){
        if (*buffer++ == '\\')
          ++buffer;
      }
      break;
    case JSON_Number:
      while (isdigit(*buffer)){
        ++buffer;
      } --buffer; //nondigit char
      break;
    case JSON_True:
      buffer += strlen("true")-2;
      break;
    case JSON_False:
      buffer += strlen("false")-2;
      break;
    case JSON_Null:
      buffer += strlen("null")-2;
      break;
    default:
      fprintf(stderr,"ERROR: invalid datatype %ld\n", datatype);
      exit(1);
      break;
  }
  data->end = buffer+1;
  data->length = data->end - data->start;

  *ptr_buffer = buffer+1;
}

static json_item*
new_property(json_item *item, json_data *data, unsigned long datatype, char **ptr_buffer)
{
  char *buffer = *ptr_buffer;

  item = new_nested_object(item);

  item->datatype = datatype;
  item->key = *data;
  json_data_token(data, datatype, &buffer);
  item->val = *data;

  item = wrap_nested_object(item, buffer);

  *ptr_buffer = buffer;

  return item;
}

static void
eval_json_token(json_data *data, unsigned long *datatype, unsigned long *mask, char **ptr_buffer)
{
  char *buffer=*ptr_buffer;
  switch (*buffer++){
    /* KEY DETECTED */
    case COLON:
      BITMASK_SET(*mask,Found_Key);
      break;
    /* KEY or STRING DETECTED */
    case DOUBLE_QUOTES:
      if (!(*mask & Found_Key)){
        //stores string in data
        json_data_token(data, JSON_String, &buffer);
        break;
      }
      BITMASK_SET(*mask,Found_String);
      *datatype = JSON_String;
      break;
    /* OBJECT DETECTED */
    case OPEN_BRACKET:
      BITMASK_SET(*mask,Found_Object);
      *datatype = JSON_Object;
      break;
    /* ARRAY DETECTED */
    case OPEN_SQUARE_BRACKET:
      BITMASK_SET(*mask,Found_Array);
      *datatype = JSON_Array;
      break;
    /* OBJECT OR ARRAY WRAPPER DETECTED */
    case CLOSE_BRACKET: case CLOSE_SQUARE_BRACKET:
      BITMASK_SET(*mask,Found_Wrapper);
      break;
    /* CHECK FOR REMAINING DATATYPES
     *    Boolean(TRUE/FALSE), Null and Number  */
    case 't':
      if (strncmp(buffer-1,"true",strlen("true")) != 0)
        break;
      BITMASK_SET(*mask, Found_True);
      *datatype = JSON_True;
      break;
    case 'f':
      if (strncmp(buffer-1,"false",strlen("false")) != 0)
        break;
      BITMASK_SET(*mask, Found_False);
      *datatype = JSON_False;
      break;
    case 'n':
      if (strncmp(buffer-1,"null",strlen("null")) != 0)
        break;
      BITMASK_SET(*mask, Found_Null);
      *datatype = JSON_Null;
      break;
    default:
      if (!isdigit(*(buffer-1)))
        break;
      BITMASK_SET(*mask, Found_Number);
      *datatype = JSON_Number;
      break;
  }

  *ptr_buffer = buffer;
}

static json_item*
apply_json_token(json_item *item, json_data data, unsigned long datatype, unsigned long *mask, char **ptr_buffer)
{
  char *buffer=*ptr_buffer;

  if (*mask & Found_Wrapper){
    item = wrap_nested_object(item, buffer);
    BITMASK_CLEAR(*mask,Found_Wrapper);
  }
  else if (*mask & Found_Object){
    item = new_nested_object(item);
    set_json_object(item, &data, datatype, buffer);
    BITMASK_CLEAR(*mask,Found_Object|Found_Key);
  }
  else if (*mask & Found_Array){
    item = new_nested_object(item);
    set_json_object(item, &data, datatype, buffer);
    BITMASK_CLEAR(*mask,Found_Array|Found_Key);
  }
  else if (*mask & Found_String){
    item = new_property(item, &data, datatype, &buffer);
    BITMASK_CLEAR(*mask,Found_String|Found_Key);
  }
  else if (*mask & Found_Number){
    item = new_property(item, &data, datatype, &buffer);
    BITMASK_CLEAR(*mask,Found_Number|Found_Key);
  }
  else if (*mask & Found_True){
    item = new_property(item, &data, datatype, &buffer);
    BITMASK_CLEAR(*mask,Found_True|Found_Key);
  }
  else if (*mask & Found_False){
    item = new_property(item, &data, datatype, &buffer);
    BITMASK_CLEAR(*mask,Found_False|Found_Key);
  }
  else if (*mask & Found_Null){
    item = new_property(item, &data, datatype, &buffer);
    BITMASK_CLEAR(*mask,Found_Null|Found_Key);
  }

  *ptr_buffer = buffer;

  return item;
}

json_item*
parse_json(char *buffer)
{
  json_item *global = calloc(1,sizeof(json_item));
  assert(global);
  json_item *item=global;

  json_data data={NULL};

  unsigned long mask=0;
  unsigned long datatype=0;
  while (*buffer){ //while not null terminator character
    //get data, mask and datatype with item buffer's char evaluation
    eval_json_token(&data, &datatype, &mask, &buffer);
    //applies evaluation to item's json_item entity
    item = apply_json_token(item, data, datatype, &mask, &buffer);
  }

  return global;
}

static void
recursive_print(json_item *item, unsigned long datatype, FILE *stream)
{
  if (item->datatype & datatype){
    fwrite(item->key.start, 1, item->key.length, stream);
    fwrite(item->val.start, 1, item->val.length, stream);
    fputc('\n', stream);
  }

  for (size_t i=0 ; i < item->obj.n ; ++i){
    recursive_print(item->obj.properties[i], datatype, stream);
  }
}

void
print_json_item(json_item *item, unsigned long datatype, FILE *stream)
{
  assert(!item->val.length && item->obj.n);
  recursive_print(item, datatype, stream);
}

void
destroy_json_item(json_item *item)
{
  for (size_t i=0 ; i < item->obj.n ; ++i){
    destroy_json_item(item->obj.properties[i]);
  } free(item->obj.properties);
  free(item);
}
