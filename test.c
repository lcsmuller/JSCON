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
  char *json_text = get_json_text(argv[1]);

  Cjson *cjson = Cjson_parse_reviver(json_text, NULL);

  char *new_json_text=Cjson_stringify(cjson, All);
  fwrite(new_json_text,1,strlen(new_json_text),f_out);
  free(new_json_text);

  Cjson_destroy(cjson);

  free(json_text);
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

