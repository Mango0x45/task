#ifndef COMMON_H
#define COMMON_H

#include <string.h>

#define streq(x, y) (strcmp(x, y) == 0)

void  errtoolong(const char *, ...);
char *strstrip(char *);

#endif /* !COMMON_H */
