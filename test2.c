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

#include <assert.h>
#include <stdio.h>
#include <unistd.h> //for access()
#include <string.h>
#include <stdbool.h>
#include <locale.h>

#include "libjscon.h"

int main(int argc, char *argv[])
{
  char *locale = setlocale(LC_CTYPE, "");
  assert(locale);

  jscon_item_st *root = jscon_object(NULL, "root");
  jscon_list_st *list = jscon_list_init();

  jscon_list_append(list, jscon_string("Mario", "name"));
  jscon_list_append(list, jscon_number(28, "age"));
  jscon_list_append(list, jscon_boolean(false, "retired"));
  jscon_list_append(list, jscon_boolean(true, "married"));
  jscon_attach(root, jscon_object(list, "person1"));

  jscon_list_append(list, jscon_string("Joana", "name"));
  jscon_list_append(list, jscon_number(58, "age"));
  jscon_list_append(list, jscon_boolean(true, "retired"));
  jscon_list_append(list, jscon_boolean(false, "married"));
  jscon_attach(root, jscon_object(list, "person2"));

  jscon_item_st *curr_item = NULL;
  jscon_item_st *item = jscon_iter_composite_r(root, &curr_item);
  do {
    fprintf(stderr, "Hey, a composite %s!\n", item->key);
  } while (NULL != (item = jscon_iter_composite_r(NULL, &curr_item)));

  char *buffer = jscon_stringify(root, JSCON_ANY);
  fprintf(stderr, "%s\n", buffer);

  free(buffer);
  jscon_list_destroy(list);
  jscon_destroy(root);

  return EXIT_SUCCESS;
}

