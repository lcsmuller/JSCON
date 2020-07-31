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

static json_parse*
new_nested_object(json_parse *current)
{
  ++current->obj.n;

  current->obj.properties = realloc(current->obj.properties, sizeof(json_parse*)*current->obj.n);
  assert(current->obj.properties);

  current->obj.properties[current->obj.n-1] = calloc(1, sizeof(json_parse));
  assert(current->obj.properties[current->obj.n-1]);

  current->obj.properties[current->obj.n-1]->obj.parent = current;

  return current->obj.properties[current->obj.n-1];
}

static void
set_json_object(json_parse *current, json_data *data, enum json_datatype set_datatype, char *buffer)
{
  current->val.start = buffer-1;
  current->datatype = set_datatype;
  current->key = *data;
}

/* closes current nest (found right bracket)
 * updates length with amount of chars between brackets
 * returns to its immediate parent nest
 */
static json_parse*
wrap_nested_object(json_parse *current, char *buffer)
{
  current->val.end = buffer;
  current->val.length = current->val.end - current->val.start;

  return current->obj.parent;
}

static void
json_data_token(json_data *data, enum json_datatype datatype, char **ptr_buffer)
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
    case JSON_Boolean:
      break;
    case JSON_Null:
      break;
    default:
      fprintf(stderr,"ERROR: invalid datatype\n");
      exit(1);
      break;
  }
  data->end = buffer+1;
  data->length = data->end - data->start;

  *ptr_buffer = buffer+1;
}

static void
new_property(json_parse *current, json_data *data, enum json_datatype set_datatype, char **ptr_buffer)
{
  char *buffer = *ptr_buffer;

  current = new_nested_object(current);

  current->datatype = set_datatype;
  current->key = *data;
  json_data_token(data, set_datatype, &buffer);
  current->val = *data;

  current = wrap_nested_object(current, buffer);

  *ptr_buffer = buffer;
}

static void
eval_json_token(json_data *data, enum json_datatype *get_datatype, enum trigger_mask *mask, char **ptr_buffer)
{
    char *buffer=*ptr_buffer;
    switch (*buffer++){
      /* KEY DETECTED */
      case COLON:
        BITMASK_SET(*mask,Found_Key);
        break;
      /* KEY or STRING DETECTED */
      case DOUBLE_QUOTES:
        if (*mask & Found_Key){
          BITMASK_SET(*mask,Found_String);
          *get_datatype = JSON_String;
          break;
        }
        //stores string in data
        json_data_token(data, JSON_String, &buffer);
        break;
      /* OBJECT DETECTED */
      case OPEN_BRACKET:
        BITMASK_SET(*mask,Found_Object);
        *get_datatype = JSON_Object;
        break;
      /* ARRAY DETECTED */
      case OPEN_SQUARE_BRACKET:
        BITMASK_SET(*mask,Found_Array);
        *get_datatype = JSON_Array;
        break;
      /* OBJECT OR ARRAY WRAPPER DETECTED */
      case CLOSE_BRACKET: case CLOSE_SQUARE_BRACKET:
        BITMASK_SET(*mask,Found_Wrapper);
        break;
      /* CHECK FOR REMAINING DATATYPES
       *    Number, Boolean and Null   */
      default:
        if (isdigit(*(buffer-1))){
          BITMASK_SET(*mask, Found_Number);
          *get_datatype = JSON_Number;
          break;
        }
        break;
    }
    *ptr_buffer = buffer;
}

json_parse*
parse_json(char *buffer)
{
  json_parse *global = calloc(1,sizeof(json_parse));
  assert(global);

  json_parse *current=global;

  json_data *data=calloc(1,sizeof(json_data));
  assert(data);

  enum trigger_mask mask=Found_Null;
  enum json_datatype set_datatype=JSON_Null;
  while (*buffer){ //while not null terminator character
    //updates data, mask and datatype with current buffer's char evaluation
    eval_json_token(data, &set_datatype, &mask, &buffer);

    if (mask & Found_Wrapper){
      current = wrap_nested_object(current, buffer);

      BITMASK_CLEAR(mask,Found_Wrapper);
      continue;
    }

    if (mask & Found_Object){
      current = new_nested_object(current);
      set_json_object(current, data, set_datatype, buffer);

      BITMASK_CLEAR(mask,Found_Object|Found_Key);
      continue;
    }

    if (mask & Found_Array){
      current = new_nested_object(current);
      set_json_object(current, data, set_datatype, buffer);

      BITMASK_CLEAR(mask,Found_Array|Found_Key);
      continue;
    }

    if (mask & Found_String){
      new_property(current, data, set_datatype, &buffer);

      BITMASK_CLEAR(mask,Found_String|Found_Key);
      continue;
    }

    if (mask & Found_Number){
      new_property(current, data, set_datatype, &buffer);

      BITMASK_CLEAR(mask,Found_Number|Found_Key);
      continue;
    }
  }

  return global;
}

static void
recursive_print(json_parse *current, enum json_datatype datatype, FILE *stream)
{
  if (current->datatype & datatype){
    fwrite(current->key.start, 1, current->key.length, stream);
    fwrite(current->val.start, 1, current->val.length, stream);
    fputc('\n', stream);
  }

  for (size_t i=0 ; i < current->obj.n ; ++i){
    recursive_print(current->obj.properties[i], datatype, stream);
  }
}

void
print_json_parse(json_parse *current, enum json_datatype datatype, FILE *stream)
{
  assert(!current->val.length && current->obj.n);
  recursive_print(current, datatype, stream);
}

void
destroy_json_parse(json_parse *current)
{
  for (size_t i=0 ; i < current->obj.n ; ++i){
    destroy_json_parse(current->obj.properties[i]);
  } free(current->obj.properties);
  free(current);
}
