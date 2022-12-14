#include <err.h>
#include <stdlib.h>
#include <string.h>

#include <geset.h>

#include "common.h"
#include "tagset.h"

GESET_DEF_IMPL(char *, tagset)

bool
tagset_elem_iseq(char *a, char *b)
{
	return strcoll(a, b) == 0;
}

size_t
tagset_elem_hash(char *s)
{
	char c;
	size_t x = 5381;

	while ((c = *s++))
		x = ((x << 5) + x) + c;

	return x;
}

void
tagset_deep_free(tagset_t *set)
{
	GESET_FOREACH(tagset, char *, tag, *set)
		free(tag);
	tagset_free(set);
}

void
parsetags(tagset_t *set, char *tagstr)
{
	char *start, *end;
	bool done = false;

	start = tagstr;
	while ((end = strchrnul(start, ','))) {
		if (*end == '\0')
			done = true;
		else
			*end = '\0';

		for (int i = -1; end + i >= start && end[i] == '/'; i--)
			end[i] = '\0';
		if (tagset_add(set, start) == -1)
			die("tagset_add");
		start = end + 1;

		if (done)
			break;
	}
}
