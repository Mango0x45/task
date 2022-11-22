#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

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
