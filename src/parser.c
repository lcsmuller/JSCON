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

static void
create_property(json_parse *parse, char *buffer)
{
  ++parse->obj.n;

  parse->obj.properties = realloc(parse->obj.properties, sizeof(json_parse*)*parse->obj.n);
  parse->obj.properties[parse->obj.n-1] = calloc(1, sizeof(json_parse));
  parse->obj.properties[parse->obj.n-1]->obj.parent = parse;
}

/* updates obj.properties nest amount for current nest
 * allocate memory for new obj.properties nest found
 * opens current nest (found left bracket)
 * returns address to newly created obj.properties nest
 */
static json_parse*
new_property(json_parse *parse, char *buffer)
{
  create_property(parse, buffer);
  //saves left bracket location
  parse->obj.properties[parse->obj.n-1]->val.start = buffer-1;

  return parse->obj.properties[parse->obj.n-1];
}

/* closes current nest (found right bracket)
 * updates length with amount of chars between brackets
 * returns to its immediate parent nest
 */
static json_parse*
wrap_property(json_parse *parse, char *buffer)
{
  //saves right bracket location
  parse->val.end = buffer;

  parse->val.length = parse->val.end - parse->val.start;

  return parse->obj.parent;
}

static enum json_datatype
json_data_token(json_data* data, char **ptr_buffer, enum json_datatype datatype)
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
      fprintf(stderr,"fix your code\n");
      exit(1);
      break;
  }
  data->end = buffer+1;
  data->length = data->end - data->start;

  *ptr_buffer = buffer+1;

  return datatype;
}

void
set_json_val(json_parse **ptr_obj, json_data *data, enum json_datatype datatype, char **ptr_buffer)
{
  char *buffer = *ptr_buffer;
  json_parse *parse = *ptr_obj;

  parse = new_property(parse, buffer);
  parse->key = *data;
  parse->datatype = json_data_token(data, &buffer, datatype);
  parse->val = *data;
  parse = wrap_property(parse, buffer);

  *ptr_buffer = buffer;
  *ptr_obj = parse;
}

json_parse*
parse_json(char *buffer)
{
  json_parse *root = calloc(1,sizeof(json_parse));
  enum trigger_mask mask=Found_Null;
  //iterate through buffer char by char, exits loop when '\0' found
  json_parse *parse=root;
  json_data *data=calloc(1,sizeof(json_data));
  do {
    switch (*buffer++){
      /* KEY FOUND */
      case COLON:
        BITMASK_SET(mask,Found_Key);
        break;
      /* STRING FOUND, CHECK FOR VARIABLE'S KEY OR VALUE */
      case DOUBLE_QUOTES:
        if (!(mask & Found_Key)){
          //string is value
          json_data_token(data, &buffer, JSON_String);
          break;
        }
        //string is key
        set_json_val(&parse, data, JSON_String, &buffer);
        BITMASK_CLEAR(mask,Found_Key);
        break;
      /* ARRAY OR OBJECT FOUND */
      case OPEN_BRACKET:
        BITMASK_SET(mask,Found_Object);
        /* FALLTHROUGH */
      case OPEN_SQUARE_BRACKET:
        parse = new_property(parse, buffer);
        if (mask & Found_Key){
          parse->key = *data;
          BITMASK_CLEAR(mask,Found_Key);
        }

        if (mask & Found_Object){
          parse->datatype = JSON_Object;
          BITMASK_CLEAR(mask,Found_Object);
        } else {
          parse->datatype = JSON_Array;
        }
        break;
      /* ARRAY OR OBJECT WRAPPER FOUND */
      case CLOSE_BRACKET: case CLOSE_SQUARE_BRACKET:
        parse = wrap_property(parse, buffer);
        break;
      /* CHECK FOR REMAINING DATATYPES
       *    Number, Boolean and Null   */
      default:
        if (!(mask & Found_Key)){
          //no variable to assign to, break out
          break;
        } BITMASK_CLEAR(mask,Found_Key);

        if (isdigit(*(buffer-1))){
          set_json_val(&parse, data, JSON_Number, &buffer);
          break;
        }
        break;
    }
  } while (*buffer);

  return root;
}

static void
recursive_print(json_parse *parse, enum json_datatype datatype, FILE *stream)
{
  if (parse->datatype & datatype){
    fwrite(parse->key.start, 1, parse->key.length, stream);
    fwrite(parse->val.start, 1, parse->val.length, stream);
    fputc('\n', stream);
  }

  for (size_t i=0 ; i < parse->obj.n ; ++i){
    recursive_print(parse->obj.properties[i], datatype, stream);
  }
}

void
print_json_parse(json_parse *parse, enum json_datatype datatype, FILE *stream)
{
  assert(!parse->val.length && parse->obj.n);
  recursive_print(parse->obj.properties[0], datatype, stream);
}

void
destroy_json_parse(json_parse *parse)
{
  for (size_t i=0 ; i < parse->obj.n ; ++i){
    destroy_json_parse(parse->obj.properties[i]);
  } free(parse->obj.properties);
  free(parse);
}
