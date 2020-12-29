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


struct _jscon_utils_s {
    char *buffer;   /* the json string to be parsed */
    char key[256];  /* holds key ptr to be received by item */
    long offset;    /* key offset used for concatenating unique keys for nested objects */
    bool is_nest;   /* condition to form nested keys */
};

/* temp struct that will be stored at jscon_scanf dictionary */
struct _jscon_pair_s {
    char specifier[5];
    void *value; /* NULL value means its a parent */
};

inline static void
_jscon_skip_string(struct _jscon_utils_s *utils)
{
    /* loops until null terminator or end of string are found */
    do {
        /* skips escaped characters */
        if ('\\' == *utils->buffer++){
            ++utils->buffer;
        }
    } while ('\0' != *utils->buffer && '\"' != *utils->buffer);
    ASSERT_S('\"' == *utils->buffer, "Not a String");
    ++utils->buffer; /* skip double quotes */
}

inline static void
_jscon_skip_composite(int ldelim, int rdelim, struct _jscon_utils_s *utils)
{
    /* skips the item and all of its nests, special care is taken for any
     *  inner string is found, as it might contain a delim character that
     *  if not treated as a string will incorrectly trigger depth action*/
    size_t depth = 0;
    do {
        if (ldelim == *utils->buffer){
            ++depth;
            ++utils->buffer; /* skips ldelim char */
        } else if (rdelim == *utils->buffer) {
            --depth;
            ++utils->buffer; /* skips rdelim char */
        } else if ('\"' == *utils->buffer) { /* treat string separately */
            _jscon_skip_string(utils);
        } else {
            ++utils->buffer; /* skips whatever token */
        }

        if (0 == depth) return; /* entire item has been skipped, return */

    } while ('\0' != *utils->buffer);
}

static void
_jscon_skip(struct _jscon_utils_s *utils)
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
        /* skip tokens while not end of string or not new key */
        while ('\0' != *utils->buffer && ',' != *utils->buffer){
            ++utils->buffer;
        }
        return;
    }
}

static char*
_jscon_format_info(char *specifier, size_t *p_tmp)
{
    size_t discard; /* throw values here if p_tmp is NULL */
    size_t *n_bytes = (p_tmp != NULL) ? p_tmp : &discard;

    if (STREQ(specifier, "s") || STREQ(specifier, "c")){
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
        *n_bytes = sizeof(jscon_item_t*);
        return "jscon_item_t**";
    }

    *n_bytes = 0;
    return "";
}

static void
_jscon_apply(struct _jscon_utils_s *utils, struct _jscon_pair_s *pair)
{
    /* first thing, we check if this pair has no value assigned to */
    if (NULL == pair->value){
        utils->is_nest = true;
        return;
    }

    /* is not nest or reached last item from nest that can be fetched */

    utils->is_nest = false;

    /* if specifier is item, simply call jscon_parse at current buffer token */
    if (STREQ(pair->specifier, "ji")){
        jscon_item_t **item = pair->value;
        *item = jscon_parse(utils->buffer);

        (*item)->key = strdup(&utils->key[utils->offset]);
        ASSERT_S(NULL != (*item)->key, "Out of memory");

        _jscon_skip(utils); /* skip characters parsed by jscon_parse */

        return;
    }

    /* specifier must be a primitive */
    char err_typeis[50];
    switch (*utils->buffer){
    case '\"':/*STRING DETECTED*/
        if (STREQ(pair->specifier, "c")){
            char *string = Jscon_decode_string(&utils->buffer);
            strscpy(pair->value, string, sizeof(char));
            free(string);
        } else if (STREQ(pair->specifier, "s")){
            char *string = Jscon_decode_string(&utils->buffer);
            strscpy(pair->value, string, strlen(string)+1);
            free(string);
        } else {
            strscpy(err_typeis, "char* or jscon_item_t**", sizeof(err_typeis));
            goto type_error;
        }

        return;
    case 't':/*CHECK FOR*/
    case 'f':/* BOOLEAN */
        if (!STRNEQ(utils->buffer,"true",4) && !STRNEQ(utils->buffer,"false",5))
            goto token_error;

        if (STREQ(pair->specifier, "b")){
            bool *boolean = pair->value;
            *boolean = Jscon_decode_boolean(&utils->buffer);
        } else {
            strscpy(err_typeis, "bool* or jscon_item_t**", sizeof(err_typeis));
            goto type_error;
        }

        return;
    case 'n':/*CHECK FOR NULL*/
     {
        if (!STRNEQ(utils->buffer,"null",4))
            goto token_error; 

        Jscon_decode_null(&utils->buffer);

        /* null conversion */
        size_t n_bytes; /* get amount of bytes that should be set to 0 */
        _jscon_format_info(pair->specifier, &n_bytes);
        memset(pair->value, 0, n_bytes);
        return;
     }
    case '{':/*OBJECT DETECTED*/
    case '[':/*ARRAY DETECTED*/
        strscpy(err_typeis, "jscon_item_t**", sizeof(err_typeis));
        goto type_error;
    default:
     { /*CHECK FOR NUMBER*/
        if (!isdigit(*utils->buffer) && ('-' != *utils->buffer))
            goto token_error;

        double tmp = Jscon_decode_double(&utils->buffer);
        if (DOUBLE_IS_INTEGER(tmp)){
            if (STREQ(pair->specifier, "d")){
                int *number_i = pair->value;
                *number_i = (int)tmp;
            } else if (STREQ(pair->specifier, "ld")){
                long *number_i = pair->value;
                *number_i = (long)tmp;
            } else if (STREQ(pair->specifier, "lld")){
                long long *number_i = pair->value;
                *number_i = (long long)tmp;
            } else {
                strscpy(err_typeis, "short*, int*, long*, long long* or jscon_item_t**", sizeof(err_typeis));
                goto type_error;
            }
        } else {
            if (STREQ(pair->specifier, "f")){
                float *number_d = pair->value; 
                *number_d = (float)tmp;
            } else if (STREQ(pair->specifier, "lf")){
                double *number_d = pair->value; 
                *number_d = tmp;
            } else {
                strscpy(err_typeis, "float*, double* or jscon_item_t**", sizeof(err_typeis));
                goto type_error;
            }
        }

        return;
     }
    }


type_error:
    ERROR("Expected specifier %s but specifier is %s( found: \"%s\" )\n", err_typeis, _jscon_format_info(pair->specifier, NULL), pair->specifier);

token_error:
    ERROR("Invalid JSON Token: %c", *utils->buffer);
}

/* count amount of keys and check for formatting errors */
static size_t
_jscon_format_analyze(char *format)
{
    size_t num_keys = 0;
    while (true) /* run until end of string found */
    {
        /* 1st STEP: find % occurrence */
        while (true)
        {
            if ('%' == *format){
                ++format;
                break;
            }
            if ('\0' == *format){
                ASSERT_S(num_keys != 0, "No type specifiers from format");
                return num_keys;
            }
            ASSERT_S(']' != *format, "Found extra ']' in key specifier");

            ++format;
        }

        /* 2nd STEP: check specifier validity */
        while (true)
        {
            if ('[' == *format){
                ++format;
                break;
            }

            if ('\0' == *format)
                ERROR("Missing format '[' key prefix\n\tFound: '%c'", *format);
            if (!isalpha(*format))
                ERROR("Unknown type specifier character\n\tFound: '%c'", *format);

            ++format;
        }

        /* 3rd STEP: check key validity */
        while (true)
        {
            if (']' == *format)
            {
                if (*++format != '[')
                    break;

                /* we've got a parent of a nested object. */

                ++format;
                ++num_keys;

                continue;
            }

            if ('\0' == *format)
                ERROR("Missing format ']' key suffix\n\tFound: '%c'", *format);

            ++format;
        }

        ++num_keys;
    }
}

static void
_jscon_store_pair(char buf[], dictionary_t *dict, va_list ap)
{
    struct _jscon_pair_s *new_pair = malloc(sizeof *new_pair);
    ASSERT_S(new_pair != NULL, "Out of memory");

    strscpy(new_pair->specifier, buf, sizeof(new_pair->specifier)); /* get specifier string */

    if (STREQ("", _jscon_format_info(new_pair->specifier, NULL)))
        ERROR("Unknown type specifier %%%s", new_pair->specifier);

    if (NULL != ap)
        new_pair->value = va_arg(ap, void*);
    else
        new_pair->value = NULL;

    dictionary_set(dict, &buf[strlen(buf)+1], new_pair, &free);
}

static void
_jscon_format_decode(char *format, dictionary_t *dict, va_list ap)
{
    /* Can't decode empty string */
    ASSERT_S('\0' != *format, "Empty format");

    char buf[256];

    /* split keys from its type specifier */
    size_t i; /* buf char index */
    while (true) /* run until end of string found */
    {
        /* 1st STEP: find % occurrence */
        while (true){
            if ('%' == *format){
                ++format;
                break;
            }

            if ('\0' == *format) return;

            ++format;
        }

        i = 0;

        /* 2nd STEP: fetch type specifier */
        while (true){
            ++i;
            ASSERT_S(i <= sizeof(buf), "Buffer overflow");

            if ('[' == *format){
                ++format;
                buf[i-1] = '\0';
                break;
            }

            buf[i-1] = *format++;
        }

        /* 3rd STEP: type specifier is formed, proceed to fetch the key and store
         *  it in a dictionary */
        while (true)
        {
            if (']' == *format)
            {
                buf[i] = '\0';

                if (*++format != '['){
                    /* most significand key */
                    _jscon_store_pair(buf, dict, ap);

                    break;
                }

                /* we've got a parent of a nested object.
                 *  it will be identified by its pair->value
                 *  being NULL */

                _jscon_store_pair(buf, dict, NULL);

                ++format; /* skips '[' token */

                continue;
            }

            buf[i] = *format++;

            ++i;
            ASSERT_S(i <= sizeof(buf), "Buffer overflow");
        }
    }
}

/* works like sscanf, will parse stuff only for the keys specified to the format string parameter.
 *  the variables assigned to ... must be in
 *  the correct order, and type, as the requested keys.  
 *
 * every key found that doesn't match any of the requested keys will be ignored along with all of 
 *  its contents. */
void
jscon_scanf(char *buffer, char *format, ...)
{
    ASSERT_S(buffer != NULL, "Missing JSON text");

    CONSUME_BLANK_CHARS(buffer);

    if (*buffer != '{')
        ERROR("Item type must be a JSCON_OBJECT");

    struct _jscon_utils_s utils = {
        .key     = "",
        .buffer  = buffer
    };

    va_list ap;
    va_start(ap, format);

    size_t num_key = _jscon_format_analyze(format);

    /* key/value dictionary fetched from format */
    dictionary_t *dict = dictionary_init();
    dictionary_build(dict, num_key);

    _jscon_format_decode(format, dict, ap);
    ASSERT_S(num_key == dict->len, "Number of keys encountered is different than allocated");

    while (*utils.buffer != '\0')
    {
        if ('\"' == *utils.buffer)
        {
            if (true == utils.is_nest)
                utils.offset = strlen(utils.key);
            else
                utils.offset = 0;

            Jscon_decode_static_string(&utils.buffer, sizeof(utils.key), utils.offset, utils.key);

            /* is key token, check if key has a match from given format */
            ASSERT_S(':' == *utils.buffer, "Missing ':' token after key"); /* check for key's assign token  */
            ++utils.buffer; /* consume ':' */

            CONSUME_BLANK_CHARS(utils.buffer);

            /* check if key found has a match in dictionary */
            struct _jscon_pair_s *pair = dictionary_get(dict, utils.key);
            if (pair != NULL){
                _jscon_apply(&utils, pair); /* match, fetch value and apply to corresponding arg */
            } else {
                _jscon_skip(&utils); /* doesn't match, skip tokens until different key is detected */
                utils.key[utils.offset] = '\0'; /* resets unmatched key */
            }
        }
        else {
            /* not a key token, skip it */
            ++utils.buffer;
        }
    }

    dictionary_destroy(dict);

    va_end(ap);
}
