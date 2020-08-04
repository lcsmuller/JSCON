#include "../json_parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

/*numerical key for arrays will be generated and
  stored here*/
struct global_numlist {
  char **list;
  long n; 
} numlist;

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
array_datatype(json_item *item)
{
  json_data data={NULL};

  if (numlist.n < item->obj.n+1){
    char number[25];
    snprintf(number,24,"\"%ld\"",item->obj.n);
    numlist.list = realloc(numlist.list,(item->obj.n+1)*sizeof(struct global_numlist));
    numlist.list[item->obj.n] = strdup(number);
    numlist.n = item->obj.n+1; 
  }

  data.start = numlist.list[item->obj.n];
  data.length = strlen(data.start);

  return data;
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
    case COMMA:/*NEW PROPERTY DETECTED*/
      BITMASK_SET(*mask,FoundKey);
      break;
    case COLON:/*KEY DETECTED*/
      BITMASK_SET(*mask,FoundAssign);
      break;
    case DOUBLE_QUOTES:/*STRING DETECTED*/
      BITMASK_SET(*mask,FoundString);
      *datatype = JsonString;
      break;
    case OPEN_BRACKET:/*OBJECT DETECTED*/
      BITMASK_SET(*mask,FoundObject|FoundKey);
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
apply_json_token(json_item *item, json_data *key, ulong datatype, ulong *mask, char **ptr_buffer)
{
  //early exit if mask is 0 or set with only FoundKey or FoundAssign
  if (BITMASK_EQUALITY(*mask,FoundKey)      || 
        BITMASK_EQUALITY(*mask,FoundAssign) ||
        BITMASK_EQUALITY(*mask,0)){
    return item;
  }

  char *temp_buffer=*ptr_buffer;

  if (*mask & FoundProperty){
    item = new_property(item, *key, datatype, &temp_buffer);
    BITMASK_CLEAR(*mask,FoundProperty);
  }
  else if (*mask & (FoundObject|FoundArray)){
    item = new_nested_object(item, *key, datatype, &temp_buffer);
    BITMASK_CLEAR(*mask,(FoundObject|FoundArray));
  }
  else if (*mask & FoundWrapper){
    item = wrap_item(item, temp_buffer);
    BITMASK_CLEAR(*mask,FoundWrapper);
  }


  if (*mask & FoundAssign)
    BITMASK_CLEAR(*mask,FoundAssign);


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

  ulong mask=0,datatype=0;
  while (*buffer){ //while not null terminator character
    //get mask and datatype with item buffer's char evaluation
    eval_json_token(&datatype, &mask, buffer);
    ++buffer;
    //get string for key
    if (item->datatype == JsonArray)
      key = array_datatype(item);
    else if (BITMASK_EQUALITY(mask,FoundString|FoundKey)){
      key = datatype_token(datatype, &buffer);
      BITMASK_CLEAR(mask,mask);
    }

    item = apply_json_token(item, &key, datatype, &mask, &buffer);
  }

  return global;
}

static void
destroy_json_item(json_item *item)
{
  for (size_t i=0 ; i < item->obj.n ; ++i){
    destroy_json_item(item->obj.properties[i]);
  } free(item->obj.properties);
  free(item);
}

void
destroy_json(json_item *item)
{
  if (numlist.n){
    while (numlist.n >= 0){
      free(numlist.list[numlist.n]);
      --numlist.n;
    } free(numlist.list);
  }

  assert(!item->val.length && item->obj.n);
  destroy_json_item(item);
}
