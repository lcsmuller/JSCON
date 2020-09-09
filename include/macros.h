#ifndef JSONC_COMMON_H_
#define JSONC_COMMON_H_

#define MAX_DIGITS 24
#define KEY_LENGTH 50

#define STRLT(s,t) (strcmp(s,t) < 0)
#define STREQ(s,t) (strcmp(s,t) == 0)
#define STRNEQ(s,t,n) (strncmp(s,t,n) == 0)

#define IN_RANGE(n,lo,hi) (((n) > (lo)) && ((n) < (hi)))

#define DOUBLE_IS_INTEGER(d) ((d) <= LLONG_MIN || (d) >= LLONG_MAX || (d) == (long long)(d))

#endif

