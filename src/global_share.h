#include "../JSON.h"

#ifndef SHAREFILE_INCLUDED
#define SHAREFILE_INCLUDED
//@todo: update for thread safety (how?)
/* used for easy key tag replacement, and to spare
  allocating new memory for repeated key tags */
typedef struct json_keylist_s {
  json_string_kt **list_p_key; //stores addresses of keys
  size_t num_p_key; //amount of key addresses stored
} json_keylist_st;

#ifdef MAIN_FILE
json_keylist_st g_keylist;
#else
extern json_keylist_st g_keylist;
#endif
#endif
