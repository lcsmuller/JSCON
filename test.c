#include "JSON.h"

#include <assert.h>
#include <stdio.h>
#include <unistd.h> //for access()
#include <limits.h> //for MAX_INT
#include <string.h>


FILE *select_output(int argc, char *argv[]);
char *get_buffer(char filename[]);
void reviver_test(JsonItem *item);

int main(int argc, char *argv[])
{
  FILE *f_out = select_output(argc, argv);
  char *buffer = get_buffer(argv[1]);

  Json *json = Json_ParseReviver(buffer, NULL);

  char *new_buffer=Json_stringify(json, All);
  fwrite(new_buffer,1,strlen(new_buffer),f_out);
  free(new_buffer);

  Json_Destroy(json);

  free(buffer);
  fclose(f_out);

  return EXIT_SUCCESS;
}

FILE *select_output(int argc, char *argv[])
{
  char *arg_ptr=NULL;
  while (argc--){
    arg_ptr=*argv++;
    if ((*arg_ptr++ == '-') && (*arg_ptr++ == 'o') && (*arg_ptr == '\0')){
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
fetch_filesize(FILE *ptr_file)
{
  fseek(ptr_file, 0, SEEK_END);
  long filesize=ftell(ptr_file);
  assert(filesize > 0);
  fseek(ptr_file, 0, SEEK_SET);

  return filesize;
}

/* returns file content */
static char*
read_file(FILE* ptr_file, long filesize)
{
  char *buffer=malloc(filesize+1);
  assert(buffer);
  //read file into buffer
  fread(buffer,sizeof(char),filesize,ptr_file);
  buffer[filesize] = '\0';

  return buffer;
}

/* returns buffer containing file content */
char*
get_buffer(char filename[])
{
  FILE *file=fopen(filename, "rb");
  assert(file);

  long filesize=fetch_filesize(file);
  char *buffer=read_file(file, filesize);

  fclose(file);

  return buffer;
}

void reviver_test(JsonItem *item){
  if (item->dtype == Number){
        fprintf(stdout,"%s",item->key);
        fprintf(stdout,"%f",item->number);
        fputc('\n',stdout);
  }
}
