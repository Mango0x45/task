#ifndef TASK_H
#define TASK_H

#include "common.h"

#define  DONEDIR "done"
#define NDONEDIR "notdone"

extern const char *argv0;

void subcmdadd(int, char **, int *);
void subcmdlist(int);
void subcmddone(int);

enum fd_type {
	DONE,
	NDONE,
	/* Number of enum members */
	FD_COUNT
};

#endif /* !TASK_H */
