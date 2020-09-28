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
#include <locale.h>

#include "libjscon.h"

FILE *select_output(int argc, char *argv[]);
char *get_buffer(char filename[]);
jscon_item_st *callback_test(jscon_item_st *item);

int main(int argc, char *argv[])
{
  char *locale = setlocale(LC_CTYPE, "");
  assert(locale);

  FILE *f_out = select_output(argc, argv);
  char *buffer = get_buffer(argv[1]);

  jscon_item_st *map1 = NULL, *map2 = NULL;
  jscon_char_kt* map3 = NULL;

  jscon_scanf(buffer, "#meta%ji,#data%ji,#string%js", &map1, &map2, &map3);
  
  if (NULL != map1){
    char *buffer1 = jscon_stringify(map1, JSCON_ANY);
    fprintf(stdout, "map1: %s\n", buffer1);
    free(buffer1);
    jscon_destroy(map1);
  }

  if (NULL != map2){
    char *buffer2 = jscon_stringify(map2, JSCON_ANY);
    fprintf(stdout, "map2: %s\n", buffer2);
    free(buffer2);
    jscon_destroy(map2);
  }

  fprintf(stdout, "map3: %s\n", map3);

  jscon_parser_callback(&callback_test);
  
  jscon_item_st *root = jscon_parse(buffer);

  jscon_item_st *item, *current_item = NULL;
  char *test1_buffer;
  jscon_item_st *walk1 = jscon_next_composite_r(root, &current_item);
  do {
    item = jscon_get_branch(walk1, "m");
    if (NULL != item){
      test1_buffer = jscon_stringify(item, JSCON_ANY);
      fwrite(test1_buffer, 1, strlen(test1_buffer), stderr);
      fputc('\n', stderr);
      free(test1_buffer);
    }

    walk1 = jscon_next_composite_r(NULL, &current_item);
  } while (NULL != walk1);

  jscon_item_st *walk2 = root;
  for (int i=0; i < 5 && walk2; ++i){
    fprintf(stderr, "%s\n", walk2->key);
    walk2 = jscon_next(walk2);
  }

  walk2 = root;
  do {
    fprintf(stderr, "%s\n", walk2->key);
    walk2 = jscon_next(walk2);
  } while (NULL != walk2);

  char *test2_buffer = jscon_stringify(root, JSCON_ANY);
  fwrite(test2_buffer, 1, strlen(test2_buffer), f_out);
  free(test2_buffer);
  jscon_destroy(root);

  free(buffer);
  fclose(f_out);

  return EXIT_SUCCESS;
}

FILE *select_output(int argc, char *argv[])
{
  char *p_arg=NULL;
  while (argc--){
    p_arg = *argv++;
    if ((*p_arg++ == '-') && (*p_arg++ == 'o') && (*p_arg == '\0')){
      assert (argc == 1); //check if theres exactly one arg left

      char *file = *argv;
      assert(access(file, W_OK)); //check if file exists

      return fopen(file, "w");
    }
  }

  return fopen("data.txt", "w");
}

/* returns file size in long format */
static long
fetch_filesize(FILE *p_file)
{
  fseek(p_file, 0, SEEK_END);
  long filesize = ftell(p_file);
  assert(filesize > 0);
  fseek(p_file, 0, SEEK_SET);

  return filesize;
}

/* returns file content */
static char*
read_file(FILE* p_file, long filesize)
{
  char *buffer = malloc(filesize+1);
  assert(NULL != buffer);

  //read file into buffer
  fread(buffer,1,filesize,p_file);
  buffer[filesize] = 0;

  return buffer;
}

/* returns buffer containing file content */
char*
get_buffer(char filename[])
{
  FILE *file = fopen(filename, "rb");
  assert(NULL != file);

  long filesize = fetch_filesize(file);
  char *buffer = read_file(file, filesize);

  fclose(file);

  return buffer;
}

jscon_item_st *callback_test(jscon_item_st *item)
{
  if (NULL != item && jscon_keycmp(item, "m")){
    fprintf(stdout, "%s\n", jscon_get_string(item));
  }
    
  return item;
}
