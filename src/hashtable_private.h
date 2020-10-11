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

#ifndef LIBCONCORD_HASHTABLE_H_
#define LIBCONCORD_HASHTABLE_H_

//#include "libjscon.h" << implicit
#include "hashtable.h"

typedef struct jscon_htwrap_s {
  struct hashtable_s *hashtable;

  struct jscon_item_s *root; //points to root item (object or array)
  struct jscon_htwrap_s *next; //points to next composite item's htwrap
  struct jscon_htwrap_s *prev; //points to prev composite item's htwrap
} jscon_htwrap_st;

jscon_htwrap_st* Jscon_htwrap_init();
void Jscon_htwrap_destroy(jscon_htwrap_st *htwrap);
void Jscon_htwrap_link_r(struct jscon_item_s *item, jscon_htwrap_st **last_accessed_htwrap);
void Jscon_htwrap_build(struct jscon_item_s *item);
struct jscon_item_s* Jscon_htwrap_get(const char *kKey, struct jscon_item_s *item);
struct jscon_item_s* Jscon_htwrap_set(const char *kKey, struct jscon_item_s *item);

#endif
