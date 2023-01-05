#include <err.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>

#include <geset.h>

#include "common.h"
#include "umaxset.h"

GESET_DEF_IMPL(uintmax_t, umaxset)

bool
umaxset_elem_iseq(uintmax_t a, uintmax_t b)
{
	return a == b;
}

size_t
umaxset_elem_hash(uintmax_t n)
{
	return n;
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
