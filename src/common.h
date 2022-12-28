#ifndef COMMON_H
#define COMMON_H

#include <gevector.h>

#include "tag-vector.h"

#define TASK_NAME_MAX 255
#define TASK_PATH_MAX 4096

#define die(...)    err(EXIT_FAILURE, __VA_ARGS__)
#define ewarn(...)  do { rv = EXIT_FAILURE; warn(__VA_ARGS__); } while (0)
#define ewarnx(...) do { rv = EXIT_FAILURE; warnx(__VA_ARGS__); } while (0)
#define streq(x, y) (strcmp(x, y) == 0)

char      *xstrdup(const char *);
void      *xmalloc(size_t);
void      *xrealloc(void *, size_t);
void      *xcalloc(size_t, size_t);
int        voidcoll(const void *, const void *);
void       errtoolong(const char *, ...);
char      *strstrip(char *);
size_t     uintmaxlen(uintmax_t);
long       fgetnamemax(int);
long       getpathmax(const char *);
uintmax_t *parseids(char **, int);
void       iterdir(struct tagvec *, int);

#endif /* !COMMON_H */
