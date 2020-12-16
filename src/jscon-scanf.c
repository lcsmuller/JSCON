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
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include <libjscon.h>

#include "jscon-common.h"
#include "debug.h"
#include "strscpy.h"


struct jscon_utils_s {
    char *buffer;
    char *key; //holds key ptr to be received by item
};

/* temp struct that will be stored at jscon_scanf dictionary */
struct jscon_pair_s {
    char specifier[5];
    void *value;
};

inline static void
_jscon_skip_string(struct jscon_utils_s *utils)
{
    /* loops until null terminator or end of string are found */
    do {
        /* skips escaped characters */
        if ('\\' == *utils->buffer++){
            ++utils->buffer;
        }
    } while ('\0' != *utils->buffer && '\"' != *utils->buffer);
    DEBUG_ASSERT('\"' == *utils->buffer, "Not a String");
    ++utils->buffer; //skip double quotes
}

inline static void
_jscon_skip_composite(int ldelim, int rdelim, struct jscon_utils_s *utils)
{
    /* skips the item and all of its nests, special care is taken for any
        inner string is found, as it might contain a delim character that
        if not treated as a string will incorrectly trigger 
        depth action*/
    size_t depth = 0;
    do {
        if (ldelim == *utils->buffer){
            ++depth;
            ++utils->buffer; //skips ldelim char
        } else if (rdelim == *utils->buffer) {
            --depth;
            ++utils->buffer; //skips rdelim char
        } else if ('\"' == *utils->buffer) { //treat string separetely
            _jscon_skip_string(utils);
        } else {
            ++utils->buffer; //skips whatever char
        }

        if (0 == depth) return; //entire item has been skipped, return

    } while ('\0' != *utils->buffer);
}

static void
_jscon_skip(struct jscon_utils_s *utils)
{
    switch (*utils->buffer){
    case '{':/*OBJECT DETECTED*/
        _jscon_skip_composite('{', '}', utils);
        return;
    case '[':/*ARRAY DETECTED*/
        _jscon_skip_composite('[', ']', utils);
        return;
    case '\"':/*STRING DETECTED*/
        _jscon_skip_string(utils);
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
_jscon_format_info(char *specifier, size_t *p_tmp)
{
    size_t *n_bytes;
    size_t discard; //throw values here if p_tmp is NULL
    if (NULL != p_tmp){
        n_bytes = p_tmp;
    } else {
        n_bytes = &discard;
    }

    if (STREQ(specifier, "c")){
        *n_bytes = sizeof(char);
        return "char*";
    }
    if (STREQ(specifier, "s")){
        *n_bytes = sizeof(char);
        return "char*";
    }
    if (STREQ(specifier, "d")){
        *n_bytes = sizeof(int);
        return "int*";
    }
    if (STREQ(specifier, "ld")){
        *n_bytes = sizeof(long);
        return "long*";
    }
    if (STREQ(specifier, "lld")){
        *n_bytes = sizeof(long long);
        return "long long*";
    }
    if (STREQ(specifier, "f")){
        *n_bytes = sizeof(float);
        return "float*";
    }
    if (STREQ(specifier, "lf")){
        *n_bytes = sizeof(double);
        return "double*";
    }
    if (STREQ(specifier, "b")){
        *n_bytes = sizeof(bool);
        return "bool*";
    }
    if (STREQ(specifier, "ji")){
        /* this will never get validated at _jscon_apply(),
          but it doesn't matter, a JSCON_NULL item is created either way */
        *n_bytes = sizeof(jscon_item_t*);
        return "jscon_item_t**";
    }

    *n_bytes = 0;
    return "NaN";
}

static void
_jscon_apply(struct jscon_utils_s *utils, struct jscon_pair_s *pair)
{
    char *specifier = pair->specifier;
    void *value = pair->value;
      

    /* if specifier is item, simply call jscon_parse at current buffer token */
    if (STREQ(specifier, "ji")){
        jscon_item_t **item = value;
        *item = jscon_parse(utils->buffer);
        _jscon_skip(utils); //skip characters parsed by jscon_parse

        /* get key, but keep in mind that this item is an "entity". The key will
          be ignored when serializing with jscon_stringify(); */
        (*item)->key = utils->key;
        utils->key = NULL;
        return;
    }

    /* specifier must be a primitive */
    char err_typeis[50];
    switch (*utils->buffer){
    case '\"':/*STRING DETECTED*/
        if (STREQ(specifier, "c")){
            char *string = Jscon_decode_string(&utils->buffer);
            strscpy(value, string, sizeof(char));
            free(string);
        } else if (STREQ(specifier, "s")){
            char *string = Jscon_decode_string(&utils->buffer);
            strscpy(value, string, strlen(string)+1);
            free(string);
        } else {
            char reason[] = "char* or jscon_item_t**";
            strscpy(err_typeis, reason, sizeof(err_typeis));
            goto type_error;
        }

        return;
    case 't':/*CHECK FOR*/
    case 'f':/* BOOLEAN */
        if (!STRNEQ(utils->buffer,"true",4) && !STRNEQ(utils->buffer,"false",5)){
            goto token_error;
        }

        if (STREQ(specifier, "b")){
            bool *boolean = value;
            *boolean = Jscon_decode_boolean(&utils->buffer);
        } else {
            char reason[] = "bool* or jscon_item_t**";
            strscpy(err_typeis, reason, sizeof(err_typeis));
            goto type_error;
        }

        return;
    case 'n':/*CHECK FOR NULL*/
     {
        if (!STRNEQ(utils->buffer,"null",4)){
            goto token_error; 
        }

        Jscon_decode_null(&utils->buffer);

        /* null conversion */
        size_t n_bytes; //get amount of bytes that should be set to 0
        _jscon_format_info(specifier, &n_bytes);
        memset(value, 0, n_bytes);
        return;
     }
    case '{':/*OBJECT DETECTED*/
    case '[':/*ARRAY DETECTED*/
     {
        char reason[] = "jscon_item_t**";
        strscpy(err_typeis, reason, sizeof(err_typeis));
        goto type_error;
     }
    default:
     { /*CHECK FOR NUMBER*/
        if (!isdigit(*utils->buffer) && ('-' != *utils->buffer)){
            goto token_error;
        }

        double tmp = Jscon_decode_double(&utils->buffer);
        if (DOUBLE_IS_INTEGER(tmp)){
            if (STREQ(specifier, "d")){
                int *number_i = value;
                *number_i = (int)tmp;
            } else if (STREQ(specifier, "ld")){
                long *number_i = value;
                *number_i = (long)tmp;
            } else if (STREQ(specifier, "lld")){
                long long *number_i = value;
                *number_i = (long long)tmp;
            } else {
                char reason[] = "short*, int*, long*, long long* or jscon_item_t**";
                strscpy(err_typeis, reason, sizeof(err_typeis));
                goto type_error;
            }
        } else {
            if (STREQ(specifier, "f")){
                float *number_d = value; 
                *number_d = (float)tmp;
            } else if (STREQ(specifier, "lf")){
                double *number_d = value; 
                *number_d = tmp;
            } else {
                char reason[] = "float*, double* or jscon_item_t**";
                strscpy(err_typeis, reason, sizeof(err_typeis));
                goto type_error;
            }
        }

        return;
     }
    }


type_error:
    DEBUG_ERR("Expected specifier %s but specifier is %s( found: \"%s\" )\n", err_typeis, _jscon_format_info(specifier, NULL), specifier);

token_error:
    DEBUG_ERR("Invalid JSON Token: %c", *utils->buffer);
}

/* count amount of keys and check for formatting errors */
static size_t
_jscon_format_analyze(char *format)
{
    size_t num_keys = 0;
    while (true) //run until end of string found
    {
        /* 1st STEP: find % occurrence */
        while (true){
            if ('%' == *format){
                ++format;
                break;
            }
            if ('\0' == *format){
                DEBUG_ASSERT(num_keys != 0, "No type specifiers from format");
                return num_keys;
            }
            DEBUG_ASSERT(']' != *format, "Found extra ']' in key specifier");

            ++format;
        }

        /* 2nd STEP: check specifier validity */
        while (true){
            if ('[' == *format){
                ++format;
                break;
            }
            if ('\0' == *format){
                DEBUG_ERR("Missing format '[' key prefix\n\t"\
                        "Found: '%c'", *format);
            }
            if (!isalpha(*format)){
                DEBUG_ERR("Unknown type specifier character\n\t"
                        "Found: '%c'", *format);
            }
            ++format;
        }

        /* 3rd STEP: check key validity */
        while (true){
            if (']' == *format){
                ++format;
                break;
            }
            if ('\0' == *format){
                DEBUG_ERR("Missing format ']' key suffix\n\t"\
                        "Found: '%c'", *format);
            }
            ++format;
        }

        ++num_keys;
    }
}

static void
_jscon_format_decode(char *format, dictionary_t *dictionary, va_list ap)
{
    /* Can't decode empty string */
    DEBUG_ASSERT('\0' != *format, "Empty format");

    char str[256];

    struct jscon_pair_s *pair;

    /* split keys from its type specifier */
    size_t i; /* str char index */
    while (true) /* run until end of string found */
    {
        /* 1st STEP: find % occurrence */
        while (true){
            if ('%' == *format){
                ++format;
                break;
            }
            if ('\0' == *format){
                return;
            }
            ++format;
        }

        i = 0;

        /* 2nd STEP: fetch type specifier */
        while (true){
            ++i;
            DEBUG_ASSERT(i <= sizeof(str), "Buffer overflow");

            if ('[' == *format){
                ++format;
                str[i-1] = '\0';
                break;
            }

            str[i-1] = *format++;
        }

        /* 3rd STEP: type specifier is formed, proceed to fetch the key */
        while(true) {
            if (']' == *format){
                str[i] = '\0';
                break;
            }

            str[i] = *format++;
            ++i;
            DEBUG_ASSERT(i <= sizeof(str), "Buffer overflow");
        }

        /* 4th STEP: store extracted specifier/value in a 
        *  struct jscon_pair_s, then assign that pair with its matching
        *  key and store it in a dictionary for later retrieval */
        pair = malloc(sizeof *pair);
        DEBUG_ASSERT(NULL != pair, "Out of memory");

        strscpy(pair->specifier, str, sizeof(pair->specifier)); //get specifier string
        if (STREQ("NaN", _jscon_format_info(pair->specifier, NULL))){
            DEBUG_ERR("Unknown type specifier %%%s", pair->specifier);
        }

        pair->value = va_arg(ap, void*);

        dictionary_set(dictionary, &str[strlen(str)+1], pair, &free);
    }
}

/* works like sscanf, will parse stuff only for the keys specified to the format string parameter. the variables assigned to ... must be in
the correct order, and type, as the requested keys.

    every key found that doesn't match any of the requested keys will be
    ignored along with all its contents. */
void
jscon_scanf(char *buffer, char *format, ...)
{
    DEBUG_ASSERT(NULL != buffer, "Missing JSON text");

    CONSUME_BLANK_CHARS(buffer);

    if ('{' != *buffer){
        DEBUG_ERR("Item type must be a JSCON_OBJECT");
    }

    struct jscon_utils_s utils = {
        .buffer = buffer,
    };

    va_list ap;
    va_start(ap, format);

    size_t num_key = _jscon_format_analyze(format);

    /* key/value dictionary fetched from format */
    dictionary_t *dictionary = dictionary_init();
    dictionary_build(dictionary, num_key);

    //return keys for freeing later
    _jscon_format_decode(format, dictionary, ap);
    DEBUG_ASSERT(num_key == dictionary->len, "Number of keys encountered is different than allocated");

    while ('\0' != *utils.buffer)
    {
        if ('\"' == *utils.buffer){
            DEBUG_ASSERT(NULL == utils.key, "utils.key wasn't freed");
            utils.key = Jscon_decode_string(&utils.buffer);
            DEBUG_ASSERT(':' == *utils.buffer, "Missing ':' token after key"); //check for key's assign token 

            ++utils.buffer; //consume ':'
            CONSUME_BLANK_CHARS(utils.buffer);

            /* check whether key found is specified */
            struct jscon_pair_s *pair = dictionary_get(dictionary, utils.key);
            if (NULL != pair){
                _jscon_apply(&utils, pair);
            } else {
                _jscon_skip(&utils);
            }

            if (NULL != utils.key){
                free(utils.key);
                utils.key = NULL;
            }
        } else {
            ++utils.buffer;
        }
    }

    dictionary_destroy(dictionary);

    va_end(ap);
}
