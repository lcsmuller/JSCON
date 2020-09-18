#include <assert.h>
#include <stdio.h>
#include <unistd.h> //for access()
#include <string.h>
#include <locale.h>

#include "libjsonc.h"

FILE *select_output(int argc, char *argv[]);
char *get_buffer(char filename[]);
jsonc_item_st *callback_test(jsonc_item_st *item);

int main(int argc, char *argv[])
{
  char *locale = setlocale(LC_CTYPE, "");
  assert(locale);

  FILE *f_out = select_output(argc, argv);
  char *buffer = get_buffer(argv[1]);

  jsonc_parser_callback(&callback_test);
  jsonc_item_st *root = jsonc_parse(buffer);

  jsonc_item_st *item, *current_item = NULL;
  char *test1_buffer;
  jsonc_item_st *walk1 = jsonc_next_object_r(root, &current_item);
  do {
    item = jsonc_get_branch(walk1, "m");
    if (NULL != item){
      test1_buffer = jsonc_stringify(item, JSONC_ALL);
      fwrite(test1_buffer, 1, strlen(test1_buffer), stderr);
      fputc('\n', stderr);
      free(test1_buffer);
    }

    walk1 = jsonc_next_object_r(NULL, &current_item);
  } while (NULL != walk1);

  jsonc_item_st *walk2 = root;
  for (int i=0; i < 5 && walk2; ++i){
    fprintf(stderr, "%s\n", walk2->key);
    walk2 = jsonc_next(walk2);
  }

  walk2 = root;
  do {
    fprintf(stderr, "%s\n", walk2->key);
    walk2 = jsonc_next(walk2);
  } while (NULL != walk2);

  char *test2_buffer = jsonc_stringify(root, JSONC_ALL);
  fwrite(test2_buffer,1,strlen(test2_buffer),f_out);
  free(test2_buffer);
  jsonc_destroy(root);

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
  assert(file);

  long filesize = fetch_filesize(file);
  char *buffer = read_file(file, filesize);

  fclose(file);

  return buffer;
}

jsonc_item_st *callback_test(jsonc_item_st *item)
{
  if (NULL != item && jsonc_keycmp(item, "m")){
    fprintf(stdout, "%s\n", jsonc_get_string(item));
  }
    
  return item;
}
