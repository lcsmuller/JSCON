#ifndef JSCON_COMMON_H_
#define JSCON_COMMON_H_

#define MAX_DIGITS 24
#define KEY_LENGTH 50

#define STRLT(s,t) (strcmp(s,t) < 0)
#define STREQ(s,t) (strcmp(s,t) == 0)
#define STRNEQ(s,t,n) (strncmp(s,t,n) == 0)

#define IN_RANGE(n,lo,hi) (((n) > (lo)) && ((n) < (hi)))

#define JSCON_VERSION "0.0"

#define DOUBLE_IS_INTEGER(d) ((d) <= LLONG_MIN || (d) >= LLONG_MAX || (d) == (long long)(d))

//TODO: add escaped characters
#define ALLOWED_JSON_CHAR(c) (isspace(c) || isalnum(c) || ('_' == (c)) || ('-' == (c)))

#define IS_COMPOSITE(i) ((i->type) & (JSCON_OBJECT|JSCON_ARRAY))
#define IS_EMPTY_COMPOSITE(i) (0 == i->comp->num_branch)
#define IS_PRIMITIVE(i) !IS_COMPOSITE(i)
#define IS_PROPERTY(i) (jscon_typecmp(i->parent, JSCON_OBJECT))
#define IS_LEAF(i) (IS_PRIMITIVE(i) || IS_EMPTY_COMPOSITE(i))
#define IS_ROOT(i) (NULL == i->parent)

#endif

