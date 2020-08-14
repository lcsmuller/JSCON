#include "CJSON.h"

#include <assert.h>
#include <stdio.h>
#include <unistd.h> //for access()
#include <limits.h> //for MAX_INT
#include <string.h>


FILE *select_output(int argc, char *argv[]);
void reviver_test(CjsonItem *item);

int main(int argc, char *argv[])
{
  FILE *f_out = select_output(argc, argv);
  char *buffer = get_buffer(argv[1]);

  Cjson *cjson = Cjson_parse_reviver(buffer, NULL);

  char *new_buffer=Cjson_stringify(cjson, All);
  fwrite(new_buffer,1,strlen(new_buffer),f_out);
  free(new_buffer);

  Cjson_destroy(cjson);

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

void reviver_test(CjsonItem *item){
  if (item->dtype == Number){
        fprintf(stdout,"%s",item->key);
        fprintf(stdout,"%f",item->value.number);
        fputc('\n',stdout);
  }
}

