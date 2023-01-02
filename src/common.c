#include <sys/types.h>

#include <ctype.h>
#include <dirent.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <geset.h>
#include <gevector.h>

#include "common.h"
#include "tag-vector.h"
#include "task.h"
#include "umax-set.h"

static void iterdir_helper(tagvec_t *, int, int, const char *);

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

int
voidcoll(const void *a, const void *b)
{
	return strcoll(*(const char **) a, *(const char **) b);
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

void
parseids(umaxset_t *set, char **raw, int cnt)
{
	char *p;
	uintmax_t id;

	for (int i = 0; i < cnt; i++) {
		id = strtoumax(raw[i], &p, 10);
		if (*raw[i] != '\0' && *p == '\0') {
			if (umaxset_add(set, id) == -1)
				die("umaxset_add");
		} else
			ewarnx("Invalid task ID: '%s'", raw[i]);
	}
}

void
iterdir(tagvec_t *vec, int fd)
{
	iterdir_helper(vec, fd, fd, ".");
}

void
iterdir_helper(tagvec_t *vec, int bfd, int fd, const char *base)
{
	int nfd;
	DIR *dp;
	char *buf;
	size_t len;
	struct dirent *ent;

	if ((dp = fdopendir(fd)) == NULL) {
		ewarn("fdopendir: '%s'", base);
		return;
	}
	rewinddir(dp);
	while (errno = 0, (ent = readdir(dp)) != NULL) {
		if (ent->d_type != DT_DIR || streq(ent->d_name, ".")
				|| streq(ent->d_name, ".."))
			continue;
		if (streq(base, ".")) {
			buf = xstrdup(ent->d_name);
			tagvec_push(vec, buf);
		} else {
			len = strlen(ent->d_name) + strlen(base) + 2;
			buf = xmalloc(len);
			sprintf(buf, "%s/%s", base, ent->d_name);
			tagvec_push(vec, buf);
		}
		if ((nfd = openat(bfd, buf, D_FLAGS)) == -1)
			ewarn("openat: '%s'", buf);
		else {
			iterdir_helper(vec, bfd, nfd, buf);
			close(nfd);
		}
	}
	if (errno != 0)
		ewarn("readdir: '%s'", base);
	closedir(dp);
}
