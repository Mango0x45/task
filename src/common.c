#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <gevector.h>

#include "common.h"
#include "tag-vector.h"
#include "task.h"

char *
xstrdup(const char *s)
{
	char *r;
	if ((r = strdup(s)) == NULL)
		die("strdup");
	return r;
}

void *
xmalloc(size_t size)
{
	void *r;
	if ((r = malloc(size)) == NULL)
		die("malloc");
	return r;
}

void *
xrealloc(void *p, size_t size)
{
	void *r;
	if ((r = realloc(p, size)) == NULL)
		die("realloc");
	return r;
}

void *
xcalloc(size_t nmemb, size_t size)
{
	void *r;
	if ((r = calloc(nmemb, size)) == NULL)
		die("calloc");
	return r;
}

void
errtoolong(const char *fmt, ...)
{
	va_list va;
	va_start(va, fmt);

	errno = ENAMETOOLONG;
	verr(EXIT_FAILURE, fmt, va);
}

char *
strstrip(char *s)
{
	char *e;

	/* If the string is empty, just return */
	if (*s == '\0')
		return s;

	/* Skip all leading whitespace */
	while (isspace(*s))
		s++;

	/* Find the last non-whitespace character */
	for (e = strchr(s, '\0'); e > s && isspace(*--e); )
		;
	if (*e != '\0')
		e[1] = '\0';

	for (e = s; *e; e++) {
		if (*e == '\n')
			*e = ' ';
	}
	return s;
}

size_t
uintmaxlen(uintmax_t n)
{
	return n < 10ULL ? 1
	     : n < 100ULL ? 2
	     : n < 1000ULL ? 3
	     : n < 10000ULL ? 4
	     : n < 100000ULL ? 5
	     : n < 1000000ULL ? 6
	     : n < 10000000ULL ? 7
	     : n < 100000000ULL ? 8
	     : n < 1000000000ULL ? 9
	     : n < 10000000000ULL ? 10
	     : n < 100000000000ULL ? 11
	     : n < 1000000000000ULL ? 12
	     : n < 10000000000000ULL ? 13
	     : n < 100000000000000ULL ? 14
	     : n < 1000000000000000ULL ? 15
	     : n < 10000000000000000ULL ? 16
	     : n < 100000000000000000ULL ? 17
	     : n < 1000000000000000000ULL ? 18
	     :                               19;
}

long
fgetnamemax(int fd)
{
	long namemax;

	errno = 0;
	if ((namemax = fpathconf(fd, _PC_NAME_MAX)) == -1) {
		if (errno != 0)
			die("fpathconf");
		return TASK_NAME_MAX;
	}

	return namemax;
}

long
getpathmax(const char *s)
{
	long pathmax;

	errno = 0;
	if ((pathmax = pathconf(s, _PC_PATH_MAX)) == -1) {
		if (errno != 0)
			die("pathconf");
		return TASK_PATH_MAX;
	}

	return pathmax;
}

uintmax_t *
parseids(char **raw, int cnt)
{
	char *p;
	uintmax_t id, *ids;

	ids = xmalloc(cnt * sizeof(uintmax_t));
	for (int i = 0; i < cnt; i++) {
		id = strtoumax(raw[i], &p, 10);
		if (*raw[i] != '\0' && *p == '\0')
			ids[i] = id;
		else {
			ids[i] = -1;
			ewarnx("Invalid task ID: '%s'", raw[i]);
		}
	}

	return ids;
}
