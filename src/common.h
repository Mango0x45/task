#ifndef COMMON_H
#define COMMON_H

#define TASK_NAME_MAX 255
#define TASK_PATH_MAX 4096

#define die(...) err(EXIT_FAILURE, __VA_ARGS__)
#define streq(x, y) (strcmp(x, y) == 0)

char  *xstrdup(const char *);
void  *xmalloc(size_t);
void  *xrealloc(void *, size_t);
void  *xcalloc(size_t, size_t);
void   errtoolong(const char *, ...);
char  *strstrip(char *);
size_t uintmaxlen(uintmax_t);
long   fgetnamemax(int);
long   getpathmax(const char *);

#endif /* !COMMON_H */
