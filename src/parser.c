#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "libjsonc.h"

struct utils_s {
  char *buffer;
  char set_key[KEY_LENGTH]; //holds key
  jsonc_htwrap_st *last_accessed_htwrap; //holds last htwrap accessed
  jsonc_callbacks_ft* parser_cb; //parser callback
};

/* function pointers used while building json items, 
  jsonc_create_value_ft points to functions prefixed by "jsonc_set_value_"
  jsonc_create_item_ft points to functions prefixed by "jsonc_set_item_" */
typedef void (jsonc_create_value_ft)(jsonc_item_st *item, struct utils_s *utils);
typedef jsonc_item_st* (jsonc_create_item_ft)(jsonc_item_st*, struct utils_s*, jsonc_create_value_ft*);

/* create a new branch to current jsonc item, and return the new branch address */
static jsonc_item_st*
jsonc_branch_create(jsonc_item_st *item)
{
  ++item->num_branch;
  item->branch = realloc(item->branch, item->num_branch * sizeof *item);
  assert(NULL != item->branch);

  item->branch[item->num_branch-1] = calloc(1, sizeof *item);
  assert(NULL != item->branch[item->num_branch-1]);

  item->branch[item->num_branch-1]->parent = item;

  return item->branch[item->num_branch-1];
}

/* destroy current item and all of its nested object/arrays */
void
jsonc_destroy(jsonc_item_st *item)
{
  for (size_t i=0; i < item->num_branch; ++i){
    jsonc_destroy(item->branch[i]);
  }

  switch (jsonc_get_type(item)){
  case JSONC_STRING:
    free(item->string);
    item->string = NULL;
    break;
  case JSONC_OBJECT:
  case JSONC_ARRAY:
    jsonc_hashtable_destroy(item->htwrap);
    free(item->branch);
    item->branch = NULL;
    break;
  default:
    break;
  }

  free(item);
  item = NULL;
}

/* fetch string type jsonc and parse into static string */
static void
jsonc_set_key(struct utils_s *utils)
{
  char *start = utils->buffer;

  char *end = ++start;
  while (('\0' != *end) && ('\"' != *end)){
    if ('\\' == *end++){ //skips escaped characters
      ++end;
    }
  }
  assert('\"' == *end); //makes sure end of string exists

  utils->buffer = end+1; //skips double quotes buffer position
  
  /* if actual length is lesser than desired length,
    use the actual length instead */
  int length;
  if (KEY_LENGTH > end - start){
    strncpy(utils->set_key, start, end - start);
    length = end-start;
  } else {
    strncpy(utils->set_key, start, KEY_LENGTH-1);
    length = KEY_LENGTH-1;
  }
  utils->set_key[length] = 0;
}

static jsonc_char_kt*
utils_buffer_set_string(struct utils_s *utils)
{
  char *start = utils->buffer;
  assert('\"' == *start); //makes sure a string is given

  char *end = ++start;
  while (('\0' != *end) && ('\"' != *end)){
    if ('\\' == *end++){ //skips escaped characters
      ++end;
    }
  }
  assert('\"' == *end); //makes sure end of string exists

  utils->buffer = end + 1; //skips double quotes buffer position

  jsonc_char_kt* set_str = strndup(start, end - start);
  assert(NULL != set_str);

  return set_str;
}

/* fetch string type jsonc and return
  allocated string */
static void
jsonc_set_value_string(jsonc_item_st *item, struct utils_s *utils)
{

  item->string = utils_buffer_set_string(utils);
  item->type = JSONC_STRING;
}

static jsonc_double_kt
utils_buffer_set_double(struct utils_s *utils)
{
  char *start = utils->buffer;
  char *end = start;

  if ('-' == *end){
    ++end; //skips minus sign
  }

  assert(isdigit(*end)); //interrupt if char isn't digit

  while (isdigit(*++end))
    continue; //skips while char is digit

  if ('.' == *end){
    while (isdigit(*++end))
      continue;
  }

  //if exponent found skips its signal and numbers
  if (('e' == *end) || ('E' == *end)){
    ++end;
    if (('+' == *end) || ('-' == *end)){ 
      ++end;
    }
    assert(isdigit(*end));
    while (isdigit(*++end))
      continue;
  }

  char get_numstr[MAX_DIGITS] = {0};
  strncpy(get_numstr, start, end-start);

  jsonc_double_kt set_double;
  sscanf(get_numstr,"%lf",&set_double); //TODO: replace sscanf?

  utils->buffer = end; //skips entire length of number

  return set_double;
}

/* fetch number jsonc type by parsing string,
  find out whether its a integer or double and assign*/
static void
jsonc_set_value_number(jsonc_item_st *item, struct utils_s *utils)
{
  double set_double = utils_buffer_set_double(utils);
  if (DOUBLE_IS_INTEGER(set_double)){
    item->i_number = (jsonc_integer_kt)set_double;
    item->type = JSONC_NUMBER_INTEGER;
  } else {
    item->d_number = set_double;
    item->type = JSONC_NUMBER_DOUBLE;
  }
}

static jsonc_boolean_kt
utils_buffer_set_boolean(struct utils_s *utils)
{
  if ('t' == *utils->buffer){
    utils->buffer += 4; //skips length of "true"
    return true;
  }
  utils->buffer += 5; //skips length of "false"
  return false;
}

static void
jsonc_set_value_boolean(jsonc_item_st *item, struct utils_s *utils)
{
  item->boolean = utils_buffer_set_boolean(utils);
  item->type = JSONC_BOOLEAN;
}

static void
utils_buffer_set_null(struct utils_s *utils){
  utils->buffer += 4; //skips length of "null"
}

static void
jsonc_set_value_null(jsonc_item_st *item, struct utils_s *utils)
{
  utils_buffer_set_null(utils);
  item->type = JSONC_NULL;
}

//executed inside jsonc_set_value_object and jsonc_set_value_array routines
inline static void
jsonc_set_hashtable(jsonc_item_st *item, struct utils_s *utils)
{
  item->htwrap = jsonc_hashtable_init();
  jsonc_hashtable_link_r(item, &utils->last_accessed_htwrap);
}

static void
jsonc_set_value_object(jsonc_item_st *item, struct utils_s *utils)
{
  jsonc_set_hashtable(item, utils);
  ++utils->buffer; //skips '{' token
  item->type = JSONC_OBJECT;
}

static void
jsonc_set_value_array(jsonc_item_st *item, struct utils_s *utils)
{
  jsonc_set_hashtable(item, utils);
  ++utils->buffer; //skips '[' token
  item->type = JSONC_ARRAY;
}

/* Create nested object and return the nested object address. 
  This is used for arrays and objects type jsonc */
static jsonc_item_st*
jsonc_set_incomplete_item(jsonc_item_st *item, struct utils_s *utils, jsonc_create_value_ft *value_setter)
{
  item = jsonc_branch_create(item);
  item->key = strdup(utils->set_key);
  assert(NULL != item->key);

  (*value_setter)(item, utils);

  return (utils->parser_cb)(item);
}

/* Wrap array or object type jsonc, which means
  all of its properties have been created */
static jsonc_item_st*
jsonc_wrap_incomplete_item(jsonc_item_st *item, struct utils_s *utils)
{
  ++utils->buffer; //skips '}' or ']'
  jsonc_hashtable_build(item);
  return jsonc_get_parent(item);
}

/* Create a property. The "complete" means all
  of its value is created at once, as opposite
  of array or object type jsonc */
static jsonc_item_st*
jsonc_set_complete_item(jsonc_item_st *item, struct utils_s *utils, jsonc_create_value_ft *value_setter)
{
  item = jsonc_set_incomplete_item(item, utils, value_setter);
  return jsonc_get_parent(item);
}

/* this will be active if the current item is of array type jsonc,
  whatever item is created here will be this array's property.
  if a ']' token is found then the array is wrapped up */
static jsonc_item_st*
jsonc_build_array(jsonc_item_st *item, struct utils_s *utils)
{
  jsonc_create_item_ft *item_setter;
  jsonc_create_value_ft *value_setter;

  switch (*utils->buffer){
  case ']':/*ARRAY WRAPPER DETECTED*/
      return jsonc_wrap_incomplete_item(item, utils);
  case '{':/*OBJECT DETECTED*/
      item_setter = &jsonc_set_incomplete_item;
      value_setter = &jsonc_set_value_object;
      break;
  case '[':/*ARRAY DETECTED*/
      item_setter = &jsonc_set_incomplete_item;
      value_setter = &jsonc_set_value_array;
      break;
  case '\"':/*STRING DETECTED*/
      item_setter = &jsonc_set_complete_item;
      value_setter = &jsonc_set_value_string;
      break;
  case 't':/*CHECK FOR*/
  case 'f':/* BOOLEAN */
      if (!STRNEQ(utils->buffer,"true",4) && !STRNEQ(utils->buffer,"false",5)){
        goto error;
      }
      item_setter = &jsonc_set_complete_item;
      value_setter = &jsonc_set_value_boolean;
      break;
  case 'n':/*CHECK FOR NULL*/
      if (!STRNEQ(utils->buffer,"null",4)){
        goto error;
      }
      item_setter = &jsonc_set_complete_item;
      value_setter = &jsonc_set_value_null;
      break;
  case ',': /*NEXT ELEMENT TOKEN*/
      ++utils->buffer; //skips ','
      return item;
  default:
      /*SKIPS IF CONTROL CHARACTER*/
      if (isspace(*utils->buffer) || iscntrl(*utils->buffer)){
        ++utils->buffer;
        return item;
      }
      /*CHECK FOR NUMBER*/
      if (!isdigit(*utils->buffer) && ('-' != *utils->buffer)){
        goto error;
      }
      item_setter = &jsonc_set_complete_item;
      value_setter = &jsonc_set_value_number;
      break;
  }

  //creates numerical key for the array element
  snprintf(utils->set_key, MAX_DIGITS-1, "%ld", item->num_branch);

  return (*item_setter)(item, utils, value_setter);


  error:
    fprintf(stderr,"ERROR: invalid json token %c\n", *utils->buffer);
    exit(EXIT_FAILURE);
}

static jsonc_item_st*
jsonc_set_object_property(jsonc_item_st *item, struct utils_s *utils)
{
  jsonc_set_key(utils);
  assert(':' == *utils->buffer);

  /* skips ':' and consecutive space and any control characters */
  do {
    ++utils->buffer;
  } while (isspace(*utils->buffer) || iscntrl(*utils->buffer));

  jsonc_create_item_ft *item_setter;
  jsonc_create_value_ft *value_setter;

  switch (*utils->buffer){
  case '{':/*OBJECT DETECTED*/
      item_setter = &jsonc_set_incomplete_item;
      value_setter = &jsonc_set_value_object;
      break;
  case '[':/*ARRAY DETECTED*/
      item_setter = &jsonc_set_incomplete_item;
      value_setter = &jsonc_set_value_array;
      break;
  case '\"':/*STRING DETECTED*/
      item_setter = &jsonc_set_complete_item;
      value_setter = &jsonc_set_value_string;
      break;
  case 't':/*CHECK FOR*/
  case 'f':/* BOOLEAN */
      if (!STRNEQ(utils->buffer,"true",4) && !STRNEQ(utils->buffer,"false",5)){
        goto error;
      }
      item_setter = &jsonc_set_complete_item;
      value_setter = &jsonc_set_value_boolean;
      break;
  case 'n':/*CHECK FOR NULL*/
      if (!STRNEQ(utils->buffer,"null",4)){
        goto error; 
      }
      item_setter = &jsonc_set_complete_item;
      value_setter = &jsonc_set_value_null;
      break;
  default:
      /*CHECK FOR NUMBER*/
      if (!isdigit(*utils->buffer) && ('-' != *utils->buffer)){
        goto error;
      }
      item_setter = &jsonc_set_complete_item;
      value_setter = &jsonc_set_value_number;
      break;
  }

  return (*item_setter)(item, utils, value_setter);


  error:
    fprintf(stderr,"ERROR: invalid json token %c\n", *utils->buffer);
    exit(EXIT_FAILURE);
}

/* this will be active if the current item is of object type jsonc,
  whatever item is created here will be this object's property.
  if a '}' token is found then the object is wrapped up */
static jsonc_item_st*
jsonc_build_object(jsonc_item_st *item, struct utils_s *utils)
{
  switch (*utils->buffer){
  case '}':/*OBJECT WRAPPER DETECTED*/
      return jsonc_wrap_incomplete_item(item, utils);
  case '\"':/*KEY STRING DETECTED*/
      return jsonc_set_object_property(item, utils);
  case ',': /*NEXT PROPERTY TOKEN*/
      ++utils->buffer; //skips ','
      return item;
  default:
      /*SKIPS IF CONTROL CHARACTER*/
      if (!(isspace(*utils->buffer) || iscntrl(*utils->buffer))){
        goto error;
      }
      ++utils->buffer;
      return item;
  }


  error:
    fprintf(stderr,"ERROR: invalid json token %c\n", *utils->buffer);
    exit(EXIT_FAILURE);
}

/* this call will only be used once, at the first iteration,
  it also allows the creation of a jsonc that's not part of an
  array or object. ex: jsonc_item_parse("10") */
static jsonc_item_st*
jsonc_build_entity(jsonc_item_st *item, struct utils_s *utils)
{
  switch (*utils->buffer){
  case '{':/*OBJECT DETECTED*/
      jsonc_set_value_object(item, utils);
      break;
  case '[':/*ARRAY DETECTED*/
      jsonc_set_value_array(item, utils);
      break;
  case '\"':/*STRING DETECTED*/
      jsonc_set_value_string(item, utils);
      break;
  case 't':/*CHECK FOR*/
  case 'f':/* BOOLEAN */
      if (!STRNEQ(utils->buffer,"true",4) && !STRNEQ(utils->buffer,"false",5)){
        goto error;
      }
      jsonc_set_value_boolean(item, utils);
      break;
  case 'n':/*CHECK FOR NULL*/
      if (!STRNEQ(utils->buffer,"null",4)){
        goto error;
      }
      jsonc_set_value_null(item, utils);
      break;
  default:
      /*SKIPS IF CONTROL CHARACTER*/
      if (isspace(*utils->buffer) || iscntrl(*utils->buffer)){
        ++utils->buffer;
        break;
      }
      /*CHECK FOR NUMBER*/
      if (!isdigit(*utils->buffer) && ('-' != *utils->buffer)){
        goto error;
      }
      jsonc_set_value_number(item, utils);
      break;
  }

  return item;


  error:
    fprintf(stderr,"ERROR: invalid json token %c\n", *utils->buffer);
    exit(EXIT_FAILURE);
}

static inline jsonc_item_st*
jsonc_default_callback(jsonc_item_st *item){
  return item;
}

jsonc_callbacks_ft*
jsonc_parser_callback(jsonc_callbacks_ft *new_cb)
{
  jsonc_callbacks_ft *parser_cb = &jsonc_default_callback;

  if (NULL != new_cb){
    parser_cb = new_cb;
  }

  return parser_cb;
}

/* parse contents from buffer into a jsonc item object
  and return its root */
jsonc_item_st*
jsonc_parse(char *buffer)
{
  jsonc_item_st *root = calloc(1, sizeof *root);
  assert(NULL != root);

  struct utils_s utils = {
    .buffer = buffer,
    .parser_cb = jsonc_parser_callback(NULL),
  };

  //build while item and buffer aren't nulled
  jsonc_item_st *item = root;
  while ((NULL != item) && ('\0' != *utils.buffer)){
    switch(item->type){
    case JSONC_OBJECT:
        item = jsonc_build_object(item, &utils);
        break;
    case JSONC_ARRAY:
        item = jsonc_build_array(item, &utils);
        break;
    case JSONC_UNDEFINED://this should be true only at the first call
        item = jsonc_build_entity(item, &utils);
        break;
    default: //nothing else to build, check buffer for potential error
        if (!(isspace(*utils.buffer) || iscntrl(*utils.buffer))){
          goto error;
        }
        ++utils.buffer; //moves if cntrl character found ('\n','\b',..)
        break;
    }
  }

  return root;


  error:
    fprintf(stderr,"ERROR: invalid json token %c\n",*utils.buffer);
    exit(EXIT_FAILURE);
}

inline static void
jsonc_sscanf_skip_string(struct utils_s *utils)
{
  /* loops until null terminator or end of string are found */
  do {
    /* skips escaped characters */
    if ('\\' == *utils->buffer++){
      ++utils->buffer;
    }
  } while ('\0' != *utils->buffer && '\"' != *utils->buffer);
  assert('\"' == *utils->buffer);
  ++utils->buffer; //skips last comma
}

inline static void
jsonc_sscanf_skip_item(int ldelim, int rdelim, struct utils_s *utils)
{
  /* skips the item and all of its nests, special care is taken for any
      inner string is found, as it might contain a delim character that
      if not treated as a string will incorrectly trigger 
      num_nest action*/
  int num_nest = 0;

  do {
    if (ldelim == *utils->buffer){
      ++num_nest;
      ++utils->buffer; //skips ldelim char
    } else if (rdelim == *utils->buffer) {
      --num_nest;
      ++utils->buffer; //skips rdelim char
    } else if ('\"' == *utils->buffer) { //treat string separetely
      jsonc_sscanf_skip_string(utils);
    } else {
      ++utils->buffer; //skips whatever char
    }


    if (0 == num_nest) return; //entire item has been skipped, return

  } while ('\0' != *utils->buffer);
}

static void
jsonc_sscanf_skip(struct utils_s *utils)
{
  switch (*utils->buffer){
  case '{':/*OBJECT DETECTED*/
      jsonc_sscanf_skip_item('{', '}', utils);
      return;
  case '[':/*ARRAY DETECTED*/
      jsonc_sscanf_skip_item('[', ']', utils);
      return;
  case '\"':/*STRING DETECTED*/
      jsonc_sscanf_skip_string(utils);
      return;
  default:
      //consume characters while not end of string or not new key
      while ('\0' != *utils->buffer && ',' != *utils->buffer){
        ++utils->buffer;
      }
      return;
  }
}

static void
jsonc_sscanf_assign(struct utils_s *utils, hashtable_st *hashtable)
{
  hashtable_entry_st *entry = hashtable_get_entry(hashtable, utils->set_key);
  assert(NULL != entry);

  char *tmp_type = &entry->key[strlen(entry->key)+1];

  switch (*utils->buffer){
  case '{':/*OBJECT DETECTED*/
   {
      if (!STREQ(tmp_type, "p")){
        goto type_error;
      }

      jsonc_item_st **item = entry->value;
      *item = jsonc_parse(utils->buffer);
      jsonc_sscanf_skip_item('{', '}', utils);
      return;
   }
  case '[':/*ARRAY DETECTED*/
   {
      if (!STREQ(tmp_type, "p")){
        goto type_error;
      }

      jsonc_item_st **item = entry->value;
      *item = jsonc_parse(utils->buffer);
      jsonc_sscanf_skip_item('[', ']', utils);
      return;
   }
  case '\"':/*STRING DETECTED*/
      if (!STREQ(tmp_type, "s")){
        goto type_error;
      }
      strcpy(entry->value, utils_buffer_set_string(utils));
      return;
  case 't':/*CHECK FOR*/
  case 'f':/* BOOLEAN */
   {
      if (!STRNEQ(utils->buffer,"true",4) && !STRNEQ(utils->buffer,"false",5)){
        goto token_error;
      }
      if (!STREQ(tmp_type, "d")){
        goto type_error;
      }

      jsonc_boolean_kt *boolean = entry->value;
      *boolean = utils_buffer_set_boolean(utils);
      return;
   }
  case 'n':/*CHECK FOR NULL*/
      if (!STRNEQ(utils->buffer,"null",4)){
        goto token_error; 
      }

      utils_buffer_set_null(utils);
      return;
  default:
   { /*CHECK FOR NUMBER*/
      if (!isdigit(*utils->buffer) && ('-' != *utils->buffer)){
        goto token_error;
      }
      
      double tmp = utils_buffer_set_double(utils);
      if (DOUBLE_IS_INTEGER(tmp)){
        if (!STREQ(tmp_type, "lld")){
          goto type_error;
        }
        jsonc_integer_kt *number_i = entry->value;
        *number_i = (jsonc_integer_kt)tmp;
      } else {
        if (!STREQ(tmp_type, "lf")){
          goto type_error;
        }
        jsonc_double_kt *number_d = entry->value; 
        *number_d = tmp;
      }
      return;
   }
  }

  return;


  token_error:
    fprintf(stderr,"ERROR: invalid json token %c\n",*utils->buffer);
    exit(EXIT_FAILURE);
  /* TODO: make specific errors relating to type, such as:
      "ERROR: type is: String\nexpected: Object" */
  type_error:
    fprintf(stderr,"ERROR: invalid type %%%s\n",tmp_type);
    exit(EXIT_FAILURE);
}

/* count amount of keys and check for formatting errors */
static size_t
jsonc_sscanf_count_keys(char *arg_keys)
{
  /* count each "word" composed of allowed key characters */
  size_t num_keys = 0;
  char c;
  while (true) //run until end of string found
  {
    c = *arg_keys;

    /*1st STEP: check if key matches a criteria for being a key */
    if (!ALLOWED_KEY_CHAR(c)){
      fprintf(stderr, "\nERROR: key's char not allowed '%c'\n\n", c);
      exit(EXIT_FAILURE);
    }

    ++num_keys; //found key, increment num_keys
    do { //consume remaining key characters
      c = *++arg_keys;
    } while (ALLOWED_KEY_CHAR(c));

    /*2nd STEP: check if key's type is specified */
    if ('%' != c){
      fprintf(stderr, "\nERROR: missing key datatype\n\n");
      exit(EXIT_FAILURE);
    }

    /*TODO: check for available formatting characters */
    c = *++arg_keys; //get datatype formatting character
    if(!isalpha(c)){
      fprintf(stderr, "\nERROR: invalid datatype format %c\n\n", c);
      exit(EXIT_FAILURE);
    }

    do { /* consume type formatting characters */
      c = *++arg_keys;
    } while (isalpha(c));

    if ('\0' == c) return num_keys; //return if found end of string

    /*3rd STEP: check if delimiter fits the criteria of being a comma */
    if (',' != c){
      fprintf(stderr, "\nERROR: invalid jsonc_sscanf delimiter '%c'\n\n", c);
      exit(EXIT_FAILURE);
    }

    ++arg_keys; //start next key
  }
}

/* this can be a little confusing, basically it stores the key and the
    datatype identification characters at the same array, with a null
    terminator character inbetween, to make sure just the key is read,
    but the type can be easily accessed by skipping the last char
    
    an approximate resulting format:
      input:    'phones%s\0'
      output:   'phones\0s\0'   */
static void
jsonc_sscanf_split_keys(char *arg_keys, hashtable_st *hashtable, va_list ap)
{
  assert('\0' != *arg_keys); //can't be empty string

  /* split individual keys from arg_keys and make
      each a key_array element */
  char c;
  char key[KEY_LENGTH], *tmp, *tmp_type;
  size_t char_index = 0;
  while (true) //run until end of string found
  {
    /* 1st STEP: build key specific characters */
    while ('%' != (c = *arg_keys)){
      key[char_index] = c;
      ++char_index;
      assert(char_index <= KEY_LENGTH);
      
      ++arg_keys;
    }
    key[char_index] = '\0'; //key is formed
    ++char_index;
    assert(char_index <= KEY_LENGTH);

    /* 2nd STEP: key is formed, fetch its datatype */
    do { //isolate datatype characters
      c = *++arg_keys; //skip %
      key[char_index] = c;
      ++char_index;
      assert(char_index <= KEY_LENGTH);
    } while(isalpha(c));

    key[char_index-1] = '\0';

    tmp = malloc(char_index);
    assert(NULL != tmp);
    memcpy(tmp, key, char_index);

    tmp_type = &tmp[strlen(tmp)+1];

    fprintf(stderr, "SPLIT: %s TYPE: %s\n", tmp, tmp_type);
    /* TODO: this is ugly fixit */
    if (STREQ(tmp_type, "s")){
      hashtable_set(hashtable, tmp, va_arg(ap, jsonc_char_kt*));
    } else if (STREQ(tmp_type, "p")) {
      hashtable_set(hashtable, tmp, va_arg(ap, jsonc_item_st**));
    } else if (STREQ(tmp_type, "lld")) {
      hashtable_set(hashtable, tmp, va_arg(ap, jsonc_integer_kt*));
    } else if (STREQ(tmp_type, "lf")) {
      hashtable_set(hashtable, tmp, va_arg(ap, jsonc_double_kt*));
    } else if (STREQ(tmp_type, "d")) {
      hashtable_set(hashtable, tmp, va_arg(ap, jsonc_boolean_kt*));
    } else {
      fprintf(stderr, "\n\nERROR: invalid type %%%s\n", tmp_type);
      exit(EXIT_FAILURE);
    }

    if ('\0' == c) return; //end of string, return

    ++arg_keys; //start next key

    char_index = 0; //resets char_index for starting next key from scratch
  }
}

/* works like sscanf, will parse stuff only for the keys specified to the arg_keys string parameter. the variables assigned to ... must be in
the correct order, and type, as the requested keys.

  every key found that doesn't match any of the requested keys will be
  ignored along with all its contents. */
void
jsonc_sscanf(char *buffer, char *arg_keys, ...)
{
  //skip any space and control characters at start of buffer
  while (isspace(*buffer) || iscntrl(*buffer)){
    ++buffer;
  }
  assert('{' == *buffer); //must be a json object

  struct utils_s utils = {
    .buffer = buffer,
  };

  va_list ap;
  va_start(ap, arg_keys);

  hashtable_st *hashtable = hashtable_init();
  hashtable_build(hashtable, jsonc_sscanf_count_keys(arg_keys));
  jsonc_sscanf_split_keys(arg_keys, hashtable, ap);

  while ('\0' != *utils.buffer){
    switch (*utils.buffer){
    case '\"':
        jsonc_set_key(&utils);
        assert(':' == *utils.buffer);
        /* consume ':' and consecutive space and any control characters */
        do {
          ++utils.buffer;
        } while (isspace(*utils.buffer) || iscntrl(*utils.buffer));

        /* check wether key found is wanted or not */
        if (NULL != hashtable_get(hashtable, utils.set_key)){
          jsonc_sscanf_assign(&utils, hashtable);
          fprintf(stderr, "ASSIGN: %s\n", utils.set_key);
        } else {
          jsonc_sscanf_skip(&utils);
          fprintf(stderr, "SKIP: %s\n", utils.set_key);
        }
        break;
    default:
        ++utils.buffer; //moves if cntrl character found ('\n','\b',..)
        break;
    }
  }

  hashtable_destroy(hashtable);

  va_end(ap);
}
