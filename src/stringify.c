/*
 * Copyright (c) 2020 Lucas Müller
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "libjscon.h"

struct jscon_utils_s {
  char *buffer_base; //buffer's base (first position)
  size_t buffer_offset; //current distance to buffer's base (aka length)
  /*a setter method that can be either _jscon_utils_analyze or
     _jscon_utils_encode*/
  void (*method)(char get_char, struct jscon_utils_s* utils);
};

/* every time its called, it adds one position to buffer_offset,
    so that it can be used for counting how many position to be expected
    for buffer */ 
static void
_jscon_utils_analyze(char get_char, struct jscon_utils_s *utils){
  ++utils->buffer_offset;
}

/* fills allocated buffer (with its length calculated by
    _jscon_utils_analyze) with string converted jscon items */
static void
_jscon_utils_encode(char get_char, struct jscon_utils_s *utils)
{
  utils->buffer_base[utils->buffer_offset] = get_char;
  ++utils->buffer_offset;
}

/* get string value to perform buffer method calls */
static void
_jscon_utils_apply_string(jscon_char_kt* string, struct jscon_utils_s *utils)
{
  while ('\0' != *string){
    (*utils->method)(*string,utils);
    ++string;
  }
}

/* converts double to string and store it in p_str */
//@todo make this more readable
static void 
_jscon_double_tostr(const jscon_double_kt kDouble, jscon_char_kt *p_str, const int kDigits)
{
  if (DOUBLE_IS_INTEGER(kDouble)){
    sprintf(p_str,"%.lf",kDouble); //convert integer to string
    return;
  }

  int decimal=0, sign=0;
  jscon_char_kt *tmp_str = fcvt(kDouble,kDigits-1,&decimal,&sign);

  int i=0;
  if (0 > sign){ //negative sign detected
    p_str[i++] = '-';
  }

  if (IN_RANGE(decimal,-7,17)){
    //print scientific notation
    sprintf(p_str+i,"%c.%.7se%d",*tmp_str,tmp_str+1,decimal-1);
    return;
  }

  char format[100];
  if (0 < decimal){
    sprintf(format,"%%.%ds.%%.7s",decimal);
    sprintf(i + p_str, format, tmp_str, tmp_str + decimal);
  } else if (0 > decimal) {
    sprintf(format, "0.%0*d%%.7s", abs(decimal), 0);
    sprintf(i + p_str, format, tmp_str);
  } else {
    sprintf(format,"0.%%.7s");
    sprintf(i + p_str, format, tmp_str);
  }
}

/* get double converted to string and then perform buffer method calls */
static void
_jscon_utils_apply_double(jscon_double_kt d_number, struct jscon_utils_s *utils)
{
  char get_strnum[MAX_DIGITS];
  _jscon_double_tostr(d_number, get_strnum, MAX_DIGITS);

  _jscon_utils_apply_string(get_strnum,utils); //store value in utils
}

/* get int converted to string and then perform buffer method calls */
static void
_jscon_utils_apply_integer(jscon_integer_kt i_number, struct jscon_utils_s *utils)
{
  char get_strnum[MAX_DIGITS];
  snprintf(get_strnum, MAX_DIGITS-1, "%lld", i_number);

  _jscon_utils_apply_string(get_strnum,utils); //store value in utils
}

/* walk jscon item, by traversing its branches recursively,
    and perform buffer_method callback on each branch */
static void
_jscon_stringify_preorder(jscon_item_st *item, jscon_type_et type, struct jscon_utils_s *utils)
{
  /* 1st STEP: stringify jscon item only if it match the type
      given as parameter or is a composite type item */
  if (!jscon_typecmp(item, type) && !IS_COMPOSITE(item))
    return;

  /* 2nd STEP: prints item key only if its a object's property
      (array's numerical keys printing doesn't conform to standard)*/
  if (!IS_ROOT(item) && IS_PROPERTY(item)){
    (*utils->method)('\"', utils);
    _jscon_utils_apply_string(item->key, utils);
    (*utils->method)('\"', utils);
    (*utils->method)(':', utils);
  }
  
  /* 3rd STEP: converts item to its string format and append to buffer */
  switch (item->type){
  case JSCON_NULL:
      _jscon_utils_apply_string("null", utils);
      break;
  case JSCON_BOOLEAN:
      if (true == item->boolean){
        _jscon_utils_apply_string("true", utils);
        break;
      }
      _jscon_utils_apply_string("false", utils);
      break;
  case JSCON_DOUBLE:
      _jscon_utils_apply_double(item->d_number, utils);
      break;
  case JSCON_INTEGER:
      _jscon_utils_apply_integer(item->i_number, utils);
      break;
  case JSCON_STRING:
      (*utils->method)('\"', utils);
      _jscon_utils_apply_string(item->string, utils);
      (*utils->method)('\"', utils);
      break;
  case JSCON_OBJECT:
      (*utils->method)('{', utils);
      break;
  case JSCON_ARRAY:
      (*utils->method)('[', utils);
      break;
  default:
      fprintf(stderr,"ERROR: can't stringify undefined datatype\n");
      exit(EXIT_FAILURE);
  }

  /* 4th STEP: if item is is a branch's leaf (defined at macros.h),
      the 5th step can be ignored and returned */
  if (IS_LEAF(item)){
    switch(item->type){
    case JSCON_OBJECT:
        (*utils->method)('}', utils);
        return;
    case JSCON_ARRAY:
        (*utils->method)(']', utils);
        return;
    default: //is a primitive, just return
        return;
    }
  }

  /* 5th STEP: find first item's branch that matches the given type, and 
      calls the write function on it */
  size_t first_index=0;
  while (first_index < item->comp->num_branch){
    if (jscon_typecmp(item->comp->branch[first_index], type) || IS_COMPOSITE(item->comp->branch[first_index])){
      _jscon_stringify_preorder(item->comp->branch[first_index], type, utils);
      break;
    }
    ++first_index;
  }

  /* 6th STEP: calls the write function on every consecutive branch
      that matches the type criteria, with an added comma before it */
  for (size_t j = first_index+1; j < item->comp->num_branch; ++j){
    /* skips branch that don't fit the criteria */
    if (!jscon_typecmp(item, type) && !IS_COMPOSITE(item)){
      continue;
    }
    (*utils->method)(',',utils);
    _jscon_stringify_preorder(item->comp->branch[j], type, utils);
  }

  /* 7th STEP: write the composite's type item wrapper token */
  switch(item->type){
  case JSCON_OBJECT:
      (*utils->method)('}', utils);
      break;
  case JSCON_ARRAY:
      (*utils->method)(']', utils);
      break;
  default: /* this shouldn't ever happen, but just in case */
      fprintf(stderr,"ERROR: not a wrapper (object or array) jscon type\n");
      exit(EXIT_FAILURE);
  }
}

/* converts a jscon item to a json formatted text, and return it */
jscon_char_kt*
jscon_stringify(jscon_item_st *root, jscon_type_et type)
{
  assert(NULL != root);

  struct jscon_utils_s utils = {0};

  /* 1st STEP: remove root->key and root->parent temporarily to make
      sure the given item is treated as a root when printing, in the
      case that given item isn't already a root (roots donesn't have
      keys or parents) */
  jscon_char_kt *hold_key = root->key;
  root->key = NULL;
  jscon_item_st *hold_parent = root->parent;
  root->parent = NULL;

  /* 2nd STEP: count how many chars will fill the buffer with
      _jscon_utils_analyze, then allocate the buffer to that amount */
  utils.method = &_jscon_utils_analyze;
  _jscon_stringify_preorder(root, type, &utils);
  utils.buffer_base = malloc(utils.buffer_offset+5);//+5 for extra safety
  assert(NULL != utils.buffer_base);

  /* 3rd STEP: reset buffer_offset and proceed with
      _jscon_utils_encode to fill allocated buffer */
  utils.buffer_offset = 0;
  utils.method = &_jscon_utils_encode;
  _jscon_stringify_preorder(root, type, &utils);
  utils.buffer_base[utils.buffer_offset] = 0; //end of buffer token

  /* 4th STEP: reattach key and parents from step 1 */
  root->key = hold_key;
  root->parent = hold_parent;

  return utils.buffer_base;
}
