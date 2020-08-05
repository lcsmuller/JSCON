#include "json_parser.h"

#include <assert.h>
#include <stdio.h>
#include <unistd.h> //for access()
#include <limits.h> //for MAX_INT


FILE *select_output(int argc, char *argv[]);

int main(int argc, char *argv[])
{
  FILE *f_out = select_output(argc, argv);
  char *json_file = read_json_file(argv[1]);

  CJSON_item_t *item = parse_json(json_file);

  print_json(item, JsonAll, f_out);
  destroy_json(item);

  free(json_file);
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
