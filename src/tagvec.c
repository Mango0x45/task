#include <string.h>

#include <gevector.h>

#include "tagvec.h"

GEVECTOR_DEF_IMPL(char *, tagvec)

void
parsetags(tagvec_t *vec, char *tagstr)
{
	char *start, *end;

	start = tagstr;
	while ((end = strchr(start, ',')) != NULL) {
		*end = '\0';
		tagvec_push(vec, start);
		start = end + 1;
	}
	tagvec_push(vec, start);
}
