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

#ifndef JSCON_STRSCPY_H_
#define JSCON_STRSCPY_H_

#include <limits.h>
#include <stddef.h>
#include <inttypes.h>
#include <stdint.h>

#if SIZE_MAX == UINT_MAX
typedef int ssize_t;        /* common 32 bit case */
#define SSIZE_MIN  INT_MIN
#define SSIZE_MAX  INT_MAX
#elif SIZE_MAX == ULONG_MAX
typedef long ssize_t;       /* linux 64 bits */
#define SSIZE_MIN  LONG_MIN
#define SSIZE_MAX  LONG_MAX
#elif SIZE_MAX == ULLONG_MAX
typedef long long ssize_t;  /* windows 64 bits */
#define SSIZE_MIN  LLONG_MIN
#define SSIZE_MAX  LLONG_MAX
#elif SIZE_MAX == USHRT_MAX
typedef short ssize_t;      /* is this even possible? */
#define SSIZE_MIN  SHRT_MIN
#define SSIZE_MAX  SHRT_MAX
#elif SIZE_MAX == UINTMAX_MAX
typedef uintmax_t ssize_t;  /* last resort, chux suggestion */
#define SSIZE_MIN  INTMAX_MIN
#define SSIZE_MAX  INTMAX_MAX
#else
#error platform has exotic SIZE_MAX
#endif

ssize_t strscpy(char *d, const char *s, size_t n);

#endif
