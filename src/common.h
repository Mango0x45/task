#ifndef COMMON_H
#define COMMON_H

#define die(...) err(EXIT_FAILURE, __VA_ARGS__)
#define streq(x, y) (strcmp(x, y) == 0)

void  errtoolong(const char *, ...);
char *strstrip(char *);

#endif /* !COMMON_H */
