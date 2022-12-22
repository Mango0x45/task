#include <sys/types.h>
#include <sys/wait.h>

#include <dirent.h>
#include <err.h>
#include <fcntl.h>
#include <getopt.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "task.h"

struct task_vector {
	size_t bufsize, len;
	struct task *tasks;
};

enum pipe_ends {
	READ,
	WRITE
};

static int  printbody(FILE *, int);
static int  qsort_helper(const void *, const void *);
static void lstasks(int, int, uintmax_t *, bool);
static void outputlist(struct task_vector, int, bool);
static void append(struct task_vector *, struct task);

void
subcmdlist(int argc, char **argv)
{
	int opt;
	char *p;
	bool lflag = false;
	uintmax_t id, *ids = NULL;
	static struct option longopts[] = {
		{"long", no_argument, NULL, 'l'},
		{ NULL,  0,           NULL,  0 }
	};

	while ((opt = getopt_long(argc, argv, "l", longopts, NULL)) != -1) {
		switch (opt) {
		case 'l':
			lflag = true;
			break;
		default:
			fprintf(stderr, "Usage: %s add [-l] [id ...]\n",
			        argv0);
			exit(EXIT_FAILURE);
		}
	}

	argc -= optind, argv += optind;
	if (argc != 0) {
		ids = xmalloc(argc * sizeof(uintmax_t));
		for (int i = 0; i < argc; i++) {
			id = strtoumax(argv[i], &p, 10);
			if (*argv[i] != '\0' && *p == '\0')
				ids[i] = id;
			else {
				ids[i] = -1;
				/* TODO: Make the program exit with EXIT_FAILURE
				 * if this warning is issued.
				 */
				warnx("Invalid task ID: '%s'", argv[i]);
			}
		}
	}

	lstasks(dfds[TODO], argc, ids, lflag);
	free(ids);
}

void
lstasks(int dfd, int idcnt, uintmax_t *ids, bool lflag)
{
	DIR *dp;
	struct dirent *ent;
	struct task tsk;
	struct task_vector vec = {
		.len = 0,
		.bufsize = 16,
	};

	vec.tasks = xmalloc(vec.bufsize * sizeof(struct task));

	if ((dp = fdopendir(dfd)) == NULL)
		die("fdopendir");
	while ((ent = readdir(dp)) != NULL) {
		if (streq(ent->d_name, ".") || streq(ent->d_name, ".."))
			continue;
		if (sscanf(ent->d_name, "%ju-", &tsk.id) != 1)
			/* TODO: Make the program exit with EXIT_FAILURE if this
			 * warning is issued.
			 */
			warnx("%s: Couldn't parse task ID", ent->d_name);
		else {
			tsk.filename = ent->d_name;
			tsk.title = strchr(ent->d_name, '-') + 1;
			if (tsk.title[0] == '\0')
				/* TODO: Same here as the above comment */
				warnx("%s: Task title is empty", ent->d_name);
			else if (ids != NULL) {
				for (int i = 0; i < idcnt; i++) {
					if (tsk.id == ids[i])
						append(&vec, tsk);
				}
			} else
				append(&vec, tsk);
		}
	}

	qsort(vec.tasks, vec.len, sizeof(struct task), &qsort_helper);

	if (vec.len > 0)
		outputlist(vec, dfd, lflag);

	free(vec.tasks);
	closedir(dp);
}

void
outputlist(struct task_vector vec, int dfd, bool lflag)
{
	int fd, fds[2];
	pid_t pid;
	size_t pad;
	FILE *pager;
	struct task tsk;

	if (pipe(fds) == -1)
		die("pipe");
	if ((pid = fork()) == -1)
		die("fork");
	if (pid == 0) {
		close(fds[WRITE]);
		close(STDIN_FILENO);
		if (dup2(fds[READ], STDIN_FILENO) == -1)
			die("dup2");
		close(fds[READ]);
		execlp("less", "less", "-FSr", NULL);
		die("execlp");
	}

	close(fds[READ]);
	pad = uintmaxlen(vec.tasks[vec.len - 1].id);
	if ((pager = fdopen(fds[WRITE], "w")) == NULL)
		die("fdopen: Pipe to pager");
	if (!lflag) {
		for (size_t i = 0; i < vec.len; i++) {
			tsk = vec.tasks[i];
			fprintf(pager, "%*ju. %s\n",
			       (int) pad, tsk.id, tsk.title);
		}
	} else {
		for (size_t i = 0; i < vec.len; i++) {
			tsk = vec.tasks[i];
			if ((fd = openat(dfd, tsk.filename, O_RDONLY)) == -1)
				die("openat: '%s'", tsk.filename);
			fprintf(pager, "\033[1mTask Title:\033[0m "
			               "\033[4m%s\033[0m\n"
			               "\033[1mTask ID:\033[0m    "
			               "\033[4m%ju\033[0m\n",
			        tsk.title, tsk.id);
			if (printbody(pager, fd) == -1)
				warn("printbody: '%s'", tsk.filename);
			if (i < vec.len - 1)
				fputc('\n', pager);
			close(fd);
		}
	}
	fclose(pager);
	if (waitpid(pid, NULL, 0) == -1)
		die("waitpid");
}

int
printbody(FILE *stream, int fd)
{
	bool init, wasnl;
	ssize_t nr;
	char buf[BUFSIZ];

	init = wasnl = true;
	while ((nr = read(fd, buf, BUFSIZ)) != -1 && nr != 0) {
		for (ssize_t i = 0; i < nr; i++) {
			if (init) {
				fputc('\n', stream);
				init = false;
			}
			if (buf[i] == '\n')
				wasnl = true;
			else if (wasnl) {
				fputs("    ", stream);
				wasnl = false;
			}
			fputc(buf[i], stream);
		}
	}

	return nr == -1 ? -1 : 0;
}

void
append(struct task_vector *vec, struct task tsk)
{
	if (vec->len == vec->bufsize) {
		vec->bufsize *= 2;
		vec->tasks = xrealloc(vec->tasks,
		                      vec->bufsize * sizeof(struct task));
	}
	vec->tasks[vec->len++] = tsk;
}

int
qsort_helper(const void *x, const void *y)
{
	return ((struct task *) x)->id - ((struct task *) y)->id;
}
