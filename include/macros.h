#ifndef JSONC_COMMON_H_
#define JSONC_COMMON_H_

#define MAX_DIGITS 24
#define KEY_LENGTH 50

#define STRLT(s,t) (strcmp(s,t) < 0)
#define STREQ(s,t) (strcmp(s,t) == 0)
#define STRNEQ(s,t,n) (strncmp(s,t,n) == 0)

#define IN_RANGE(n,lo,hi) (((n) > (lo)) && ((n) < (hi)))

#define DOUBLE_IS_INTEGER(d) ((d) <= LLONG_MIN || (d) >= LLONG_MAX || (d) == (long long)(d))

//allowed characters for key naming
#define ALLOWED_KEY_CHAR(c) (isalnum(c) || ('_' == (c)) || ('-' == (c)))

#define IS_OBJECT(i) ((i->type) & (JSONC_OBJECT|JSONC_ARRAY))
#define IS_EMPTY_OBJECT(i) (0 == i->num_branch)
#define IS_PRIMITIVE(i) !IS_OBJECT(i)
#define IS_PROPERTY(i) ((NULL != (i->key)) && jsonc_typecmp(i->parent, JSONC_OBJECT))
#define IS_LEAF(i) (IS_PRIMITIVE(i) || IS_EMPTY_OBJECT(i))

#endif

