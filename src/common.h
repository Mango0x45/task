#ifndef COMMON_H
#define COMMON_H

#define die(...) err(EXIT_FAILURE, __VA_ARGS__)
#define streq(x, y) (strcmp(x, y) == 0)

void  *xmalloc(size_t);
void  *xrealloc(void *, size_t);
void  *xcalloc(size_t, size_t);
void   errtoolong(const char *, ...);
char  *strstrip(char *);
size_t uintmaxlen(uintmax_t);

#endif /* !COMMON_H */
