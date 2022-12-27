#include <string.h>

#include <gevector.h>

#include "tag-vector.h"

GEVECTOR_IMPL(char *, tagvec)

void
parsetags(struct tagvec *vec, char *tagstr)
{
	char *start, *end;

	start = tagstr;
	while ((end = strchr(start, ',')) != NULL) {
		*end = '\0';
		tagvec_append(vec, start);
		start = end + 1;
	}
	tagvec_append(vec, start);
}
