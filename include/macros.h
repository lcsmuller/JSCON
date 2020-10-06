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

#ifndef JSCON_COMMON_H_
#define JSCON_COMMON_H_

#define JSCON_VERSION "0.0"

#define MAX_DIGITS 17

#define STRLT(s,t) (strcmp(s,t) < 0)
#define STREQ(s,t) (strcmp(s,t) == 0)
#define STRNEQ(s,t,n) (strncmp(s,t,n) == 0)

#define IN_RANGE(n,lo,hi) (((n) > (lo)) && ((n) < (hi)))

#define DOUBLE_IS_INTEGER(d) ((d) <= LLONG_MIN||(d) >= LLONG_MAX||(d) == (long long)(d))

//@todo add escaped characters
#define ALLOWED_JSON_CHAR(c) (isspace(c)||isalnum(c)||'_' == (c)||'-' == (c))
#define CONSUME_BLANK_CHARS(str) for(;(isspace(*str)||iscntrl(*str)); ++str)

#define IS_COMPOSITE(i) ((i) && jscon_typecmp(i, JSCON_OBJECT|JSCON_ARRAY))
#define IS_EMPTY_COMPOSITE(i) (IS_COMPOSITE(i) && 0 == jscon_size(i))
#define IS_PRIMITIVE(i) ((i) && !jscon_typecmp(i, JSCON_OBJECT|JSCON_ARRAY))
#define IS_PROPERTY(i) (jscon_typecmp(i->parent, JSCON_OBJECT))
#define IS_ELEMENT(i) (jscon_typecmp(i->parent, JSCON_ARRAY))
#define IS_LEAF(i) (IS_PRIMITIVE(i) || IS_EMPTY_COMPOSITE(i))
#define IS_ROOT(i) (NULL == i->parent)

#endif

