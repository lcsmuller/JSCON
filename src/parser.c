#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "libjscon.h"

struct utils_s {
  char *buffer;
  char set_key[KEY_LENGTH]; //holds key temporarily
  jscon_htwrap_st *last_accessed_htwrap; //holds last htwrap accessed
  jscon_callbacks_ft* parser_cb; //parser callback
};

/* function pointers used while building json items, 
  jscon_create_value_ft points to functions prefixed by "jscon_value_set_"
  jscon_create_item_ft points to functions prefixed by "jscon_set_item_" */
typedef void (jscon_create_value_ft)(jscon_item_st *item, struct utils_s *utils);
typedef jscon_item_st* (jscon_create_item_ft)(jscon_item_st*, struct utils_s*, jscon_create_value_ft*);

/* create a new branch to current jscon object item, and return
  the new branch address */
static jscon_item_st*
jscon_new_branch(jscon_item_st *item)
{
  ++item->comp->num_branch;
  item->comp->branch = realloc(item->comp->branch, item->comp->num_branch * sizeof *item);
  assert(NULL != item->comp->branch);

  item->comp->branch[item->comp->num_branch-1] = calloc(1, sizeof *item);
  assert(NULL != item->comp->branch[item->comp->num_branch-1]);

  item->comp->branch[item->comp->num_branch-1]->parent = item;

  return item->comp->branch[item->comp->num_branch-1];
}

/* destroy current item and all of its nested object/arrays */
void
jscon_destroy(jscon_item_st *item)
{
  switch (jscon_get_type(item)){
  case JSCON_OBJECT:
  case JSCON_ARRAY:
      for (size_t i=0; i < item->comp->num_branch; ++i){
        jscon_destroy(item->comp->branch[i]);
      }
      hashtable_destroy(item->comp->htwrap.hashtable);
      free(item->comp->branch);
      item->comp->branch = NULL;
      free(item->comp);
      item->comp = NULL;
      break;
  case JSCON_STRING:
      free(item->string);
      item->string = NULL;
      break;
  default:
      break;
  }

  free(item);
  item = NULL;
}

/* fetch string type jscon and parse into static string */
static void
jscon_set_key(struct utils_s *utils)
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

  utils->buffer = end+1; //skips double quotes buffer position
  
  /* if actual length is lesser than desired length,
    use the actual length instead */
  int length;
  if (KEY_LENGTH > end - start){
    length = end-start;
  } else {
    length = KEY_LENGTH-1;
  }
  strncpy(utils->set_key, start, length);
  utils->set_key[length] = '\0';
}

static jscon_char_kt*
utils_eval_string(struct utils_s *utils)
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

  jscon_char_kt* set_str = strndup(start, end - start);
  assert(NULL != set_str);

  return set_str;
}

/* fetch string type jscon and return
  allocated string */
static void
jscon_value_set_string(jscon_item_st *item, struct utils_s *utils)
{
  item->type = JSCON_STRING;
  item->string = utils_eval_string(utils);
}

static jscon_double_kt
utils_eval_double(struct utils_s *utils)
{
  char *start = utils->buffer;
  char *end = start;

  /* 1st STEP: check for a minus sign and skip it */
  if ('-' == *end){
    ++end; //skips minus sign
  }

  /* 2nd STEP: skips until a non digit char found */
  assert(isdigit(*end)); //interrupt if char isn't digit
  while (isdigit(*++end)){
    continue; //skips while char is digit
  }

  /* 3rd STEP: if non-digit char is not a comma then it must be
      an integer*/
  if ('.' == *end){
    while (isdigit(*++end)){
      continue;
    }
  }

  /* 4th STEP: if exponent found skips its tokens */
  if (('e' == *end) || ('E' == *end)){
    ++end;
    if (('+' == *end) || ('-' == *end)){ 
      ++end;
    }
    assert(isdigit(*end));
    while (isdigit(*++end)){
      continue;
    }
  }

  /* 5th STEP: convert string to double and return its value */
  char get_numstr[MAX_DIGITS] = {0};
  strncpy(get_numstr, start, end-start);

  jscon_double_kt set_double;
  sscanf(get_numstr,"%lf",&set_double); //TODO: replace sscanf?

  utils->buffer = end; //skips entire length of number

  return set_double;
}

/* fetch number jscon type by parsing string,
  find out whether its a integer or double and assign*/
static void
jscon_value_set_number(jscon_item_st *item, struct utils_s *utils)
{
  double set_double = utils_eval_double(utils);
  if (DOUBLE_IS_INTEGER(set_double)){
    item->type = JSCON_NUMBER_INTEGER;
    item->i_number = (jscon_integer_kt)set_double;
  } else {
    item->type = JSCON_NUMBER_DOUBLE;
    item->d_number = set_double;
  }
}

static jscon_boolean_kt
utils_eval_boolean(struct utils_s *utils)
{
  if ('t' == *utils->buffer){
    utils->buffer += 4; //skips length of "true"
    return true;
  }
  utils->buffer += 5; //skips length of "false"
  return false;
}

static void
jscon_value_set_boolean(jscon_item_st *item, struct utils_s *utils)
{
  item->type = JSCON_BOOLEAN;
  item->boolean = utils_eval_boolean(utils);
}

static void
utils_eval_null(struct utils_s *utils){
  utils->buffer += 4; //skips length of "null"
}

static void
jscon_value_set_null(jscon_item_st *item, struct utils_s *utils)
{
  item->type = JSCON_NULL;
  utils_eval_null(utils);
}

static jscon_composite_st*
utils_eval_composite(struct utils_s *utils){
  jscon_composite_st* new_comp = calloc(1, sizeof *new_comp);
  assert(NULL != new_comp);

  new_comp->htwrap.hashtable = hashtable_init();

  utils->buffer += 1; //skips composite delim '{' '}' or '[' ']'

  return new_comp;
}

static void
jscon_value_set_object(jscon_item_st *item, struct utils_s *utils)
{
  item->type = JSCON_OBJECT;

  item->comp = utils_eval_composite(utils);
  jscon_hashtable_link_r(item, &utils->last_accessed_htwrap);
}

static void
jscon_value_set_array(jscon_item_st *item, struct utils_s *utils)
{
  item->type = JSCON_ARRAY;

  item->comp = utils_eval_composite(utils);
  jscon_hashtable_link_r(item, &utils->last_accessed_htwrap);
}

/* create nested composite type (object/array) and return 
    the address. */
static jscon_item_st*
jscon_new_composite(jscon_item_st *item, struct utils_s *utils, jscon_create_value_ft *value_setter)
{
  item = jscon_new_branch(item);
  item->key = strdup(utils->set_key);
  assert(NULL != item->key);

  (*value_setter)(item, utils);

  return (utils->parser_cb)(item);
}

/* wrap array or object type jscon, which means
    all of its branches have been created */
static jscon_item_st*
jscon_wrap_composite(jscon_item_st *item, struct utils_s *utils)
{
  ++utils->buffer; //skips '}' or ']'
  jscon_hashtable_build(item);
  return jscon_get_parent(item);
}

/* create a primitive data type branch. */
static jscon_item_st*
jscon_append_primitive(jscon_item_st *item, struct utils_s *utils, jscon_create_value_ft *value_setter)
{
  item = jscon_new_branch(item);
  item->key = strdup(utils->set_key);
  assert(NULL != item->key);

  (*value_setter)(item, utils);
  item = (utils->parser_cb)(item);

  return jscon_get_parent(item);
}

/* this routine is called when setting a branch of a composite type
    (object and array) item. */
static jscon_item_st*
jscon_build_branch(jscon_item_st *item, struct utils_s *utils)
{
  jscon_create_item_ft *item_setter;
  jscon_create_value_ft *value_setter;

  switch (*utils->buffer){
  case '{':/*OBJECT DETECTED*/
      item_setter = &jscon_new_composite;
      value_setter = &jscon_value_set_object;
      break;
  case '[':/*ARRAY DETECTED*/
      item_setter = &jscon_new_composite;
      value_setter = &jscon_value_set_array;
      break;
  case '\"':/*STRING DETECTED*/
      item_setter = &jscon_append_primitive;
      value_setter = &jscon_value_set_string;
      break;
  case 't':/*CHECK FOR*/
  case 'f':/* BOOLEAN */
      if (!STRNEQ(utils->buffer,"true",4) && !STRNEQ(utils->buffer,"false",5)){
        goto error;
      }
      item_setter = &jscon_append_primitive;
      value_setter = &jscon_value_set_boolean;
      break;
  case 'n':/*CHECK FOR NULL*/
      if (!STRNEQ(utils->buffer,"null",4)){
        goto error; 
      }
      item_setter = &jscon_append_primitive;
      value_setter = &jscon_value_set_null;
      break;
  default:
      /*CHECK FOR NUMBER*/
      if (!isdigit(*utils->buffer) && ('-' != *utils->buffer)){
        goto error;
      }
      item_setter = &jscon_append_primitive;
      value_setter = &jscon_value_set_number;
      break;
  }

  return (*item_setter)(item, utils, value_setter);


  error:
    fprintf(stderr,"ERROR: invalid json token %c\n", *utils->buffer);
    exit(EXIT_FAILURE);
}

/* this will be active if the current item is of array type jscon,
  whatever item is created here will be this array's property.
  if a ']' token is found then the array is wrapped up */
static jscon_item_st*
jscon_build_array(jscon_item_st *item, struct utils_s *utils)
{
  switch (*utils->buffer){
  case ']':/*ARRAY WRAPPER DETECTED*/
      return jscon_wrap_composite(item, utils);
  case ',': /*NEXT ELEMENT TOKEN*/
      /* skips ',' and consecutive space and/or control characters */
      do {
        ++utils->buffer;
      } while (isspace(*utils->buffer) || iscntrl(*utils->buffer));

      return item;
  default:
      //creates numerical key for the array element
      snprintf(utils->set_key, MAX_DIGITS-1, "%ld", item->comp->num_branch);
      return jscon_build_branch(item, utils);
  }

  //token error checking done inside jscon_build_branch
}

/* this will be active if the current item is of object type jscon,
  whatever item is created here will be this object's property.
  if a '}' token is found then the object is wrapped up */
static jscon_item_st*
jscon_build_object(jscon_item_st *item, struct utils_s *utils)
{
  switch (*utils->buffer){
  case '}':/*OBJECT WRAPPER DETECTED*/
      return jscon_wrap_composite(item, utils);
  case '\"':/*KEY STRING DETECTED*/
      jscon_set_key(utils);
      assert(':' == *utils->buffer); //check for key's assign token 

      /* skips ':' and consecutive space and/or control characters */
      do {
        ++utils->buffer;
      } while (isspace(*utils->buffer) || iscntrl(*utils->buffer));

      return jscon_build_branch(item, utils);
  case ',': /*NEXT PROPERTY TOKEN*/
      /* skips ',' and consecutive space and/or control characters */
      do {
        ++utils->buffer;
      } while (isspace(*utils->buffer) || iscntrl(*utils->buffer));

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
  it also allows the creation of a jscon that's not part of an
  array or object. ex: jscon_item_parse("10") */
static jscon_item_st*
jscon_build_entity(jscon_item_st *item, struct utils_s *utils)
{
  switch (*utils->buffer){
  case '{':/*OBJECT DETECTED*/
      jscon_value_set_object(item, utils);
      break;
  case '[':/*ARRAY DETECTED*/
      jscon_value_set_array(item, utils);
      break;
  case '\"':/*STRING DETECTED*/
      jscon_value_set_string(item, utils);
      break;
  case 't':/*CHECK FOR*/
  case 'f':/* BOOLEAN */
      if (!STRNEQ(utils->buffer,"true",4) && !STRNEQ(utils->buffer,"false",5)){
        goto error;
      }
      jscon_value_set_boolean(item, utils);
      break;
  case 'n':/*CHECK FOR NULL*/
      if (!STRNEQ(utils->buffer,"null",4)){
        goto error;
      }
      jscon_value_set_null(item, utils);
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
      jscon_value_set_number(item, utils);
      break;
  }

  return item;


  error:
    fprintf(stderr,"ERROR: invalid json token %c\n", *utils->buffer);
    exit(EXIT_FAILURE);
}

static inline jscon_item_st*
jscon_default_callback(jscon_item_st *item){
  return item;
}

jscon_callbacks_ft*
jscon_parser_callback(jscon_callbacks_ft *new_cb)
{
  jscon_callbacks_ft *parser_cb = &jscon_default_callback;

  if (NULL != new_cb){
    parser_cb = new_cb;
  }

  return parser_cb;
}

/* parse contents from buffer into a jscon item object
  and return its root */
jscon_item_st*
jscon_parse(char *buffer)
{
  jscon_item_st *root = calloc(1, sizeof *root);
  assert(NULL != root);

  struct utils_s utils = {
    .buffer = buffer,
    .parser_cb = jscon_parser_callback(NULL),
  };

  //build while item and buffer aren't nulled
  jscon_item_st *item = root;
  while ((NULL != item) && ('\0' != *utils.buffer)){
    switch(item->type){
    case JSCON_OBJECT:
        item = jscon_build_object(item, &utils);
        break;
    case JSCON_ARRAY:
        item = jscon_build_array(item, &utils);
        break;
    case JSCON_UNDEFINED://this should be true only at the first call
        item = jscon_build_entity(item, &utils);
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
jscon_scanf_skip_string(struct utils_s *utils)
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
jscon_scanf_skip_composite(int ldelim, int rdelim, struct utils_s *utils)
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
      jscon_scanf_skip_string(utils);
    } else {
      ++utils->buffer; //skips whatever char
    }


    if (0 == num_nest) return; //entire item has been skipped, return

  } while ('\0' != *utils->buffer);
}

static void
jscon_scanf_skip(struct utils_s *utils)
{
  switch (*utils->buffer){
  case '{':/*OBJECT DETECTED*/
      jscon_scanf_skip_composite('{', '}', utils);
      return;
  case '[':/*ARRAY DETECTED*/
      jscon_scanf_skip_composite('[', ']', utils);
      return;
  case '\"':/*STRING DETECTED*/
      jscon_scanf_skip_string(utils);
      return;
  default:
      //consume characters while not end of string or not new key
      while ('\0' != *utils->buffer && ',' != *utils->buffer){
        ++utils->buffer;
      }
      return;
  }
}

static char*
jscon_format_eval(char *tmp_format, size_t *p_tmp)
{
  size_t *n_bytes;
  size_t discard; //throw values here if p_tmp is NULL
  if (NULL != p_tmp){
    n_bytes = p_tmp;
  } else {
    n_bytes = &discard;
  }

  if (STREQ(tmp_format, "js")){
    *n_bytes = sizeof(jscon_char_kt*);
    return "jscon_char_kt*";
  }
  if (STREQ(tmp_format, "ji")){
    *n_bytes = sizeof(jscon_item_st*);
    return "jscon_item_st**";
  }
  if (STREQ(tmp_format, "jd")){
    *n_bytes = sizeof(jscon_integer_kt);
    return "jscon_integer_kt*";
  }
  if (STREQ(tmp_format, "jf")){
    *n_bytes = sizeof(jscon_double_kt);
    return "jscon_double_kt*";
  }
  if (STREQ(tmp_format, "jb")){
    *n_bytes = sizeof(jscon_boolean_kt);
    return "jscon_boolean_kt*";
  }
  *n_bytes = 0;
  return "NaN";
}

static void
jscon_scanf_apply(struct utils_s *utils, hashtable_st *hashtable)
{
  hashtable_entry_st *entry = hashtable_get_entry(hashtable, utils->set_key);
  assert(NULL != entry);

  char *tmp_format = &entry->key[strlen(entry->key)+1];

  char err_typeis[25];
  switch (*utils->buffer){
  case '{':/*OBJECT DETECTED*/
   {
      if (!STREQ(tmp_format, "ji")){
        strcpy(err_typeis, "jscon_item_st**");
        goto type_error;
      }

      jscon_item_st **item = entry->value;
      *item = jscon_parse(utils->buffer);
      jscon_scanf_skip_composite('{', '}', utils);
      return;
   }
  case '[':/*ARRAY DETECTED*/
   {
      if (!STREQ(tmp_format, "ji")){
        strcpy(err_typeis, "jscon_item_st**");
        goto type_error;
      }

      jscon_item_st **item = entry->value;
      *item = jscon_parse(utils->buffer);
      jscon_scanf_skip_composite('[', ']', utils);
      return;
   }
  case '\"':/*STRING DETECTED*/
   {
      if (!STREQ(tmp_format, "js")){
        strcpy(err_typeis, "jscon_char_kt*");
        goto type_error;
      }

      jscon_char_kt *string = utils_eval_string(utils);
      strcpy(entry->value, string);
      free(string);

      return;
   }
  case 't':/*CHECK FOR*/
  case 'f':/* BOOLEAN */
   {
      if (!STRNEQ(utils->buffer,"true",4) && !STRNEQ(utils->buffer,"false",5)){
        goto token_error;
      }
      if (!STREQ(tmp_format, "jb")){
        strcpy(err_typeis, "jscon_boolean_kt*");
        goto type_error;
      }

      jscon_boolean_kt *boolean = entry->value;
      *boolean = utils_eval_boolean(utils);
      return;
   }
  case 'n':/*CHECK FOR NULL*/
   {
      if (!STRNEQ(utils->buffer,"null",4)){
        goto token_error; 
      }

      utils_eval_null(utils);

      /* nullify pointer contents */
      size_t n_bytes;
      jscon_format_eval(tmp_format, &n_bytes);
      memset(entry->value, 0, n_bytes);
      return;
   }
  default:
   { /*CHECK FOR NUMBER*/
      if (!isdigit(*utils->buffer) && ('-' != *utils->buffer)){
        goto token_error;
      }
      
      double tmp = utils_eval_double(utils);
      if (DOUBLE_IS_INTEGER(tmp)){
        if (!STREQ(tmp_format, "jd")){
          strcpy(err_typeis, "jscon_double_kt*");
          goto type_error;
        }
        jscon_integer_kt *number_i = entry->value;
        *number_i = (jscon_integer_kt)tmp;
      } else {
        if (!STREQ(tmp_format, "jf")){
          strcpy(err_typeis, "jscon_integer_kt*");
          goto type_error;
        }
        jscon_double_kt *number_d = entry->value; 
        *number_d = tmp;
      }
      return;
   }
  }


  type_error:
    fprintf(stderr,"ERROR: type is: %s, expected: %s\n",err_typeis, jscon_format_eval(tmp_format, NULL));
    exit(EXIT_FAILURE);

  token_error:
    fprintf(stderr,"ERROR: invalid json token %c\n",*utils->buffer);
    exit(EXIT_FAILURE);
}

/* count amount of keys and check for formatting errors */
static size_t
jscon_scanf_eval_keys(char *format)
{
  /* count each "word" composed of allowed key characters */
  size_t num_keys = 0;
  char c;
  while (true) //run until end of string found
  {
    c = *format;

    //consume any space or control characters sequence
    while (isspace(c) || iscntrl(c)){
      c = *++format;
    }

    /*1st STEP: check for key '#' prefix existence */
    if ('#' != c){
      fprintf(stderr, "\nERROR: missing key '#' prefix\n\n");
      exit(EXIT_FAILURE);
    }
    c = *++format;

    /* TODO: check for escaped characters */
    /*2nd STEP: check if key matches a criteria for being a key */
    if (!ALLOWED_JSON_CHAR(c)){
      fprintf(stderr, "\nERROR: key's char not allowed '%c'\n\n", c);
      exit(EXIT_FAILURE);
    }

    ++num_keys; //found key, increment num_keys
    do { //consume remaining key characters
      c = *++format;
    } while (ALLOWED_JSON_CHAR(c));

    /*3rd STEP: check if key's type is specified */
    if ('%' != c){
      fprintf(stderr, "\nERROR: missing key datatype\n\n");
      exit(EXIT_FAILURE);
    }

    /*TODO: error check for available unknown characters */
    do { /* consume type formatting characters */
      c = *++format;
    } while (isalpha(c));

    if ('\0' == c) return num_keys; //return if found end of string

    ++format; //start next key
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
jscon_scanf_split_keys(char *format, hashtable_st *hashtable, va_list ap)
{
  assert('\0' != *format); //can't be empty string

  /* split individual keys from format and make
      each a key_array element */
  char c;
  char key[KEY_LENGTH], *tmp, *tmp_format;
  size_t char_index = 0;
  while (true) //run until end of string found
  {
    c = *format;

    //consume any space or control characters sequence
    while (isspace(c) || iscntrl(c)){
      c = *++format;
    }
    assert('#' == c); //make sure key prefix is there
    c = *++format; //skip '#'
    
    /* 1st STEP: build key specific characters */
    while ('%' != c){
      key[char_index] = c;
      ++char_index;
      assert(char_index <= KEY_LENGTH);
      
      c = *++format;
    }
    key[char_index] = '\0'; //key is formed
    ++char_index;
    assert(char_index <= KEY_LENGTH);

    /* 2nd STEP: key is formed, fetch its datatype */
    do { //isolate datatype characters
      c = *++format; //skips '%'
      key[char_index] = c;
      ++char_index;
      assert(char_index <= KEY_LENGTH);
    } while(isalpha(c));

    key[char_index-1] = '\0';

    /* 3rd STEP: store '#key\0%format\0' string and its variable
        address (given at ... parameter) in hashtable */
    tmp = malloc(char_index);
    assert(NULL != tmp);
    memcpy(tmp, key, char_index); //get the string in its entirety

    tmp_format = &tmp[strlen(tmp)+1]; //get format specific string
    
    //fprintf(stderr, "SPLIT: %s TYPE: %s\n", tmp, tmp_format);
    if ( STREQ("NaN", jscon_format_eval(tmp_format, NULL)) ){
      fprintf(stderr, "\n\nERROR: unknown type format %%%s\n", tmp_format);
      exit(EXIT_FAILURE);
    }
    hashtable_set(hashtable, tmp, va_arg(ap, void*));

    if ('\0' == c) return; //end of string, return

    ++format; //start next key

    char_index = 0; //resets char_index for starting next key from scratch
  }
}

/* works like sscanf, will parse stuff only for the keys specified to the format string parameter. the variables assigned to ... must be in
the correct order, and type, as the requested keys.

  every key found that doesn't match any of the requested keys will be
  ignored along with all its contents. */
void
jscon_scanf(char *buffer, char *format, ...)
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
  va_start(ap, format);

  hashtable_st *hashtable = hashtable_init();
  hashtable_build(hashtable, jscon_scanf_eval_keys(format));
  jscon_scanf_split_keys(format, hashtable, ap);

  while ('\0' != *utils.buffer){
    switch (*utils.buffer){
    case '\"':
        jscon_set_key(&utils);
        assert(':' == *utils.buffer);
        /* consume ':' and consecutive space and any control characters */
        do {
          ++utils.buffer;
        } while (isspace(*utils.buffer) || iscntrl(*utils.buffer));

        /* check wether key found is wanted or not */
        if (NULL != hashtable_get(hashtable, utils.set_key)){
          jscon_scanf_apply(&utils, hashtable);
          //fprintf(stderr, "ASSIGN: %s\n", utils.set_key);
        } else {
          jscon_scanf_skip(&utils);
          //fprintf(stderr, "SKIP: %s\n", utils.set_key);
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
