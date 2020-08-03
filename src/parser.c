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
  assert(filesize > 0);
  fseek(ptr_file, 0, SEEK_SET);
  return filesize;
}

static char *read_file(FILE* ptr_file, long filesize)
{
  char *buffer=malloc(filesize+1);
  assert(buffer);

  fread(buffer, sizeof(char), filesize, ptr_file);
  buffer[filesize] = '\0';

  return buffer;
}

char*
read_json_file(char file[])
{
  FILE *json_file=fopen(file, "rb");
  assert(json_file);

  //stores file size
  long filesize=fetch_filesize(json_file);
  //buffer reads file content
  char *buffer=read_file(json_file, filesize);

  fclose(json_file);

  return buffer;
}

static json_item*
new_item(json_item *item)
{
  ++item->obj.n;

  item->obj.properties = realloc(item->obj.properties, sizeof(json_item*)*item->obj.n);
  assert(item->obj.properties);

  item->obj.properties[item->obj.n-1] = calloc(1, sizeof(json_item));
  assert(item->obj.properties[item->obj.n-1]);

  item->obj.properties[item->obj.n-1]->obj.parent = item;

  return item->obj.properties[item->obj.n-1];
}

/* closes item nest (found right bracket)
 * updates length with amount of chars between brackets
 * returns to its immediate parent nest
 */
static json_item*
wrap_item(json_item *item, char *buffer)
{
  item->val.length = buffer - item->val.start;

  return item->obj.parent;
}

static json_data
datatype_token(ulong datatype, char **ptr_buffer)
{
  json_data data={NULL};
  char *temp_buffer=*ptr_buffer;

  data.start = temp_buffer-1;
  switch (datatype){
    case JsonString:
      while (*temp_buffer != DOUBLE_QUOTES){
        if (*temp_buffer++ == '\\') //skips \" char
          ++temp_buffer;
      }
      break;
    case JsonNumber:
      if (*temp_buffer == '-')
        ++temp_buffer;
      while (isdigit(*temp_buffer))
        ++temp_buffer;
      if (*temp_buffer == '.'){
        while (isdigit(*++temp_buffer))
          continue;
      }
      //check for exponent
      if ((*temp_buffer == 'e') || (*temp_buffer == 'E')){
        ++temp_buffer;
        if ((*temp_buffer == '+') || (*temp_buffer == '-'))
          ++temp_buffer;
        while (isdigit(*temp_buffer))
          ++temp_buffer;
      }
      --temp_buffer; //return from non numerical char
      break;
    case JsonTrue:
      temp_buffer += strlen("true")-2;
      break;
    case JsonFalse:
      temp_buffer += strlen("false")-2;
      break;
    case JsonNull:
      temp_buffer += strlen("null")-2;
      break;
    default:
      fprintf(stderr,"ERROR: invalid datatype %ld\n", datatype);
      exit(1);
      break;
  }
  data.length = temp_buffer+1 - data.start;

  *ptr_buffer = temp_buffer+1;

  return data;
}

static json_item*
new_property(json_item *item, json_data key, ulong datatype, char **ptr_buffer)
{
  char *temp_buffer=*ptr_buffer;

  item = new_item(item);

  item->datatype = datatype;
  item->key = key;
  item->val = datatype_token(datatype, &temp_buffer);

  item = wrap_item(item, temp_buffer);

  *ptr_buffer = temp_buffer;

  return item;
}

static json_item*
new_nested_object(json_item *item, json_data key, ulong datatype, char **ptr_buffer)
{
  char *temp_buffer=*ptr_buffer;

  item = new_item(item);

  item->val.start = temp_buffer-1;
  item->datatype = datatype;
  item->key = key;

  return item;
}


static void
eval_json_token(ulong *datatype, ulong *mask, char *json_token)
{
  switch (*json_token){
    case COLON:/*KEY DETECTED*/
      BITMASK_SET(*mask,FoundKey);
      break;
    case DOUBLE_QUOTES:/*STRING DETECTED*/
      BITMASK_SET(*mask,FoundString);
      *datatype = JsonString;
      break;
    case OPEN_BRACKET:/*OBJECT DETECTED*/
      BITMASK_SET(*mask,FoundObject);
      *datatype = JsonObject;
      break;
    case OPEN_SQUARE_BRACKET:/*ARRAY DETECTED*/
      BITMASK_SET(*mask,FoundArray);
      *datatype = JsonArray;
      break;
    case CLOSE_BRACKET:       /*WRAPPER*/
    case CLOSE_SQUARE_BRACKET:/*DETECTED*/
      BITMASK_SET(*mask,FoundWrapper);
      break;
    case 't':/*CHECK FOR TRUE*/
      if (strncmp(json_token,"true",strlen("true")) != 0)
        break;
      BITMASK_SET(*mask, FoundProperty);
      *datatype = JsonTrue;
      break;
    case 'f':/*CHECK FOR FALSE*/
      if (strncmp(json_token,"false",strlen("false")) != 0)
        break;
      BITMASK_SET(*mask, FoundProperty);
      *datatype = JsonFalse;
      break;
    case 'n':/*CHECK FOR NULL*/
      if (strncmp(json_token,"null",strlen("null")) != 0)
        break;
      BITMASK_SET(*mask, FoundProperty);
      *datatype = JsonNull;
      break;
    default:/*CHECK FOR NUMBER*/
      if ((!isdigit(*json_token)) && (*json_token != '-'))
        break;
      BITMASK_SET(*mask, FoundProperty);
      *datatype = JsonNumber;
      break;
  }
}

static json_item*
apply_json_token(json_item *item, json_data key, ulong datatype, ulong *mask, char **ptr_buffer)
{
  //early exit if mask is unset or set with only FoundKey
  if (BITMASK_EQUALITY(*mask,FoundKey) || BITMASK_EQUALITY(*mask,0))
    return item;

  char *temp_buffer=*ptr_buffer;

  if (*mask & FoundProperty)
    item = new_property(item, key, datatype, &temp_buffer);
  else if (*mask & (FoundObject|FoundArray))
    item = new_nested_object(item, key, datatype, &temp_buffer);
  else if (*mask & FoundWrapper)
    item = wrap_item(item, temp_buffer);

  BITMASK_CLEAR(*mask,*mask);

  *ptr_buffer = temp_buffer;

  return item;
}

json_item*
parse_json(char *buffer)
{
  json_item *global=calloc(1,sizeof(json_item));
  assert(global);
  json_item *item=global;

  json_data key={NULL};

  ulong mask=0;
  ulong datatype=0;
  while (*buffer){ //while not null terminator character
    //get mask and datatype with item buffer's char evaluation
    eval_json_token(&datatype, &mask, buffer);
    ++buffer; //fix this (ignore this comment)
    //get string for key
    if (BITMASK_EQUALITY(mask,FoundString)){
      key = datatype_token(datatype, &buffer);
      BITMASK_CLEAR(mask,mask);
      continue;
    }
    item = apply_json_token(item, key, datatype, &mask, &buffer);
  }

  return global;
}

static void
recursive_print(json_item *item, ulong datatype, FILE *stream)
{
  if (item->datatype & datatype){
    fwrite(item->key.start, 1, item->key.length, stream);
    fputc(' ', stream);
    fwrite(item->val.start, 1, item->val.length, stream);
    fputc(' ', stream);
    switch (item->datatype){
      case JsonNull:
        fputs("Null", stream);
        break;
      case JsonTrue:
      case JsonFalse:
        fputs("Boolean", stream);
        break;
      case JsonNumber:
        fputs("Number", stream);
        break;
      case JsonString:
        fputs("String", stream);
        break;
      case JsonObject:
        fputs("Object", stream);
        break;
      case JsonArray:
        fputs("Array", stream);
        break;
      default:
        fprintf(stderr,"undefined datatype\n");
        exit(1);
        break;
    }
    fputc('\n', stream);
  }

  for (size_t i=0 ; i < item->obj.n ; ++i){
    recursive_print(item->obj.properties[i], datatype, stream);
  }
}

void
print_json_item(json_item *item, ulong datatype, FILE *stream)
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
