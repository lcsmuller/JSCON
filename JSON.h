#ifndef JSONC_COMMON_H_
#define JSONC_COMMON_H_

#define MAX_DIGITS 24
#define KEY_LENGTH 50

//@todo: move these macros to a lib
#define STRLT(s,t) (strcmp(s,t) < 0)
#define STREQ(s,t) (strcmp(s,t) == 0)
#define STRNEQ(s,t,n) (strncmp(s,t,n) == 0)

#define IN_RANGE(n,lo,hi) (((n) > (lo)) && ((n) < (hi)))

#include "src/parser.h"
#include "src/stringify.h"
#include "src/public.h"

#endif

