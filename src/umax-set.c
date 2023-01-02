#include <stdbool.h>
#include <stdint.h>

#include <geset.h>

#include "umax-set.h"

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
