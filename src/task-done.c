#include <sys/types.h>

#include <dirent.h>
#include <err.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "task.h"

void taskdone(uintmax_t);

void
subcmddone(int argc, char **argv)
{
	char *p;
	uintmax_t id;

	if (argc == 1) {
		fprintf(stderr, "Usage: %s done id ...\n", argv0);
		exit(EXIT_FAILURE);
	}
	for (int i = 1; i < argc; i++) {
		id = strtoumax(argv[i], &p, 10);
		if (*argv[i] != '\0' && *p == '\0')
			taskdone(id);
		else
			/* TODO: Make the program exit with EXIT_FAILURE if this
			 * warning is issued.
			 */
			warnx("Invalid task ID: '%s'", argv[i]);
	}
}

void
taskdone(uintmax_t tid)
{
	DIR *dp;
	uintmax_t id;
	struct dirent *ent;

	if ((dp = fdopendir(dfds[TODO])) == NULL)
		die("fdopendir");
	while ((ent = readdir(dp)) != NULL) {
		if (sscanf(ent->d_name, "%ju", &id) == 1 && id == tid) {
			if (renameat(dfds[TODO], ent->d_name,
				     dfds[DONE], ent->d_name) == -1)
				die("renameat: '%s'", ent->d_name);
			return;
		}
	}

	errx(EXIT_FAILURE, "No task with id '%ju' found", tid);
}
