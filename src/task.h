#ifndef TASK_H
#define TASK_H

#include <geset.h>

#include "tagset.h"

#define D_FLAGS (O_RDONLY | O_DIRECTORY)

enum task_status {
	DONE,
	TODO
};

struct task {
	uintmax_t id;
	enum task_status status;
	tagset_t tags;
	char *title, *body;
};

extern const char *argv0;

void subcmdadd(int, char **);
void subcmddone(int, char **);
void subcmdlist(int, char **);
void subcmdtag(int, char **);

#endif /* !TASK_H */
