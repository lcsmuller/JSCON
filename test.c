#include <assert.h>
#include <stdio.h>
#include <unistd.h> //for access()
#include <string.h>
#include <locale.h>

#include "src/public.h"
#include "src/stringify.h"


FILE *select_output(int argc, char *argv[]);
char *get_buffer(char filename[]);
void reviver_test(jsonc_item_st *item);

int main(int argc, char *argv[])
{
  char *locale = setlocale(LC_CTYPE, "");
  assert(locale);

  FILE *f_out = select_output(argc, argv);
  char *buffer = get_buffer(argv[1]);

  jsonc_item_st *root = jsonc_parse_reviver(buffer, NULL);

  jsonc_item_st *walk = root;
  jsonc_item_st *item, *current_item = NULL;
  char *test1_buffer;
  walk = jsonc_foreach_object_r(walk, &current_item);
  do {
    item = jsonc_foreach_specific(walk, "m");
    if (NULL != item){
      test1_buffer = jsonc_stringify(item, JSONC_ALL);
      fwrite(test1_buffer, 1, strlen(test1_buffer), stderr);
      fputc('\n', stderr);
      free(test1_buffer);
    }

    walk = jsonc_foreach_object_r(NULL, &current_item);
  } while (NULL != walk);

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

void reviver_test(jsonc_item_st *item){
  if (jsonc_keycmp(item,"u") && jsonc_intcmp(item,3)){
        jsonc_item_st *sibling = jsonc_get_sibling(item,2);
        if (jsonc_keycmp(sibling,"m")){
          fputs(jsonc_get_string(sibling),stdout);
          fputc('\n',stdout);
        }
  }
}
