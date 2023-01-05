#include <sys/types.h>

#include <ctype.h>
#include <err.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include <geset.h>

#include "common.h"
#include "tagset.h"
#include "task.h"
#include "task-parser.h"

void taskdone(uintmax_t);
bool lineempty(const char *);

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
			ewarnx("Invalid task ID: '%s'", argv[i]);
	}
}

void
taskdone(uintmax_t tid)
{
	FILE *fp;
	char filename[21];
	struct task task;

	sprintf(filename, "%ju", tid);
	task = parsetask(filename, true);

	if ((fp = fopen(filename, "w")) == NULL)
		ewarn("fopen: '%s'", filename);

	fprintf(fp, "Title: %s\nStatus: done\n\n%s", task.title, task.body);
	free(task.body);
	free(task.title);
	tagset_free(&task.tags);
	fclose(fp);
}
