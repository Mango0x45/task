#include "compat.h"

#ifdef __GNUC__
void dummy(void) {}
#endif

#ifndef __GNUC__
char *
strchrnul(const char *s, int c)
{
	for (; *s; s++) {
		if (*s == c)
			break;
	}
	return (char *) s;
}
#endif
