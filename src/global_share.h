#include "../JSON.h"

#ifndef SHAREFILE_INCLUDED
#define SHAREFILE_INCLUDED
//@todo: update for thread safety (how?)
// take a look at pthread_getspecific()

/* used for easy key tag replacement, and to spare
  allocating new memory for repeated key tags */
typedef struct json_keycache_s {
  json_string_kt **list_keyaddr; //list of key addresses
  size_t cache_size; //amount of key addresses stored
} json_keycache_st;

#ifdef MAIN_FILE
json_keycache_st g_keycache;
#else
extern json_keycache_st g_keycache;
#endif
#endif
