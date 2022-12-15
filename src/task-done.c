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

void taskdone(int *, uintmax_t);

void
subcmddone(int argc, char **argv, int *dfds)
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
			taskdone(dfds, id);
		else
			/* TODO: Make the program exit with EXIT_FAILURE if this
			 * warning is issued.
			 */
			warnx("Invalid task ID: '%s'", argv[i]);
	}
}

void
taskdone(int *dfds, uintmax_t tid)
{
	int fd;
	uintmax_t id;
	FILE *fp;
	DIR *dp;
	struct dirent *ent;

	if ((dp = fdopendir(dfds[TODO])) == NULL)
		die("fdopendir");
	while ((ent = readdir(dp)) != NULL) {
		if (streq(ent->d_name, ".") || streq(ent->d_name, ".."))
			continue;
		if ((fd = openat(dfds[TODO], ent->d_name, O_RDONLY)) == -1)
			die("openat: '%s'", ent->d_name);
		if ((fp = fdopen(fd, "r")) == NULL)
			die("fdopen: '%s'", ent->d_name);
		if (fscanf(fp, "Task ID: %ju", &id) != 1) {
			/* TODO: Make the program exit with EXIT_FAILURE if this
			 * warning is issued.
			 */
			warnx("%s: Couldn't parse task ID", ent->d_name);
			fclose(fp);
		} else if (id == tid) {
			fclose(fp);
			if (renameat(dfds[TODO], ent->d_name,
					dfds[DONE], ent->d_name) == -1)
				die("renameat: '%s'", ent->d_name);
			return;
		}
	}

	errx(EXIT_FAILURE, "No task with id '%ju' found", tid);
}
