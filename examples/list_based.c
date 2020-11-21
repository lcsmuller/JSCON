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
#include <string.h>
#include <stdbool.h>
#include <locale.h>

#include <libjscon.h>


int main(void)
{
  char *locale = setlocale(LC_CTYPE, "");
  assert(locale);

  jscon_list_t *list = jscon_list_init();
  jscon_list_t *main_list = jscon_list_init();

  jscon_list_append(list, jscon_string("0", "Dog"));
  jscon_list_append(list, jscon_string("1", "Cat"));
  jscon_list_append(list, jscon_string("2", "Fish"));
  jscon_list_append(list, jscon_array("pets", list));

  jscon_list_append(list, jscon_string("name", "Mario"));
  jscon_list_append(list, jscon_number("age", 28));
  jscon_list_append(list, jscon_boolean("retired", false));
  jscon_list_append(list, jscon_boolean("married", true));
  jscon_list_append(main_list, jscon_object("person1", list));

  jscon_list_append(list, jscon_string("0", "Moose"));
  jscon_list_append(list, jscon_string("1", "Mouse"));
  jscon_list_append(list, jscon_array("pets", list));

  jscon_list_append(list, jscon_string("name", "Joana"));
  jscon_list_append(list, jscon_number("age", 58));
  jscon_list_append(list, jscon_boolean("retired", true));
  jscon_list_append(list, jscon_boolean("married", false));
  jscon_list_append(main_list, jscon_object("person2", list));

  jscon_item_t *root = jscon_object("root", main_list);

  jscon_item_t *curr_item = NULL;
  jscon_item_t *item = jscon_iter_composite_r(root, &curr_item);
  do {
    fprintf(stderr, "Hey, a composite %s!\n", jscon_get_key(item));
  } while (NULL != (item = jscon_iter_composite_r(NULL, &curr_item)));

  char *buffer = jscon_stringify(root, JSCON_ANY);
  fprintf(stderr, "%s\n", buffer);

  free(buffer);
  jscon_list_destroy(list);
  jscon_list_destroy(main_list);
  jscon_destroy(root);

  return EXIT_SUCCESS;
}

