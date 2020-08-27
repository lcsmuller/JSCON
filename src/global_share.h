#include "../JSON.h"

//fix: update for thread safety (how?)
typedef struct json_keylist_s {
  json_string_kt **list_ptr_key; //stores addresses of keys
  size_t num_ptr_key; //amount of key addresses stored
} json_keylist_st;

#ifndef SHAREFILE_INCLUDED
#define SHAREFILE_INCLUDED
#ifdef MAIN_FILE
/* used for easy key tag replacement, and to spare
  allocating new memory for repeated key tags */
json_keylist_st g_keylist;
#else
extern json_keylist_st g_keylist;
#endif
#endif
