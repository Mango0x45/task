#include <sys/types.h>
#include <sys/wait.h>

#include <dirent.h>
#include <err.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "task.h"

struct task_vector {
	size_t bufsize, len;
	struct task *tasks;
};

enum pipe_ends {
	READ,
	WRITE
};

static int  sizetlen(size_t);
static int  qsort_helper(const void *, const void *);
static void lstasks(int);
static void outputlist(struct task_vector);
static void append(struct task_vector *, struct task);

void
subcmdlist(int argc, char **argv, int *dfds)
{
	int opt;
	bool lflag = false;
	static struct option longopts[] = {
		{"long", no_argument, NULL, 'l'},
		{ NULL,  0,           NULL,  0 }
	};
	(void)lflag;
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
	lstasks(dfds[NDONE]);
}

void
lstasks(int dfd)
{
	int fd;
	DIR *dp;
	FILE *fp;
	struct dirent *ent;
	struct task tsk;
	struct task_vector vec = {
		.len = 0,
		.bufsize = 16,
	};

	if ((vec.tasks = malloc(vec.bufsize * sizeof(struct task))) == NULL)
		err(EXIT_FAILURE, "malloc");

	if ((dp = fdopendir(dfd)) == NULL)
		err(EXIT_FAILURE, "fdopendir");
	while ((ent = readdir(dp)) != NULL) {
		if (streq(ent->d_name, ".") || streq(ent->d_name, ".."))
			continue;
		if ((fd = openat(dfd, ent->d_name, O_RDONLY)) == -1)
			err(EXIT_FAILURE, "openat: '%s'", ent->d_name);
		if ((fp = fdopen(fd, "r")) == NULL)
			err(EXIT_FAILURE, "fdopen: '%s'", ent->d_name);
		tsk.title = ent->d_name;
		if (fscanf(fp, "Task ID: %zu", &tsk.id) != 1)
			warnx("%s: Counldn't parse task ID", ent->d_name);
		append(&vec, tsk);
		fclose(fp);
	}

	qsort(vec.tasks, vec.len, sizeof(struct task), &qsort_helper);

	if (vec.len > 0)
		outputlist(vec);

	free(vec.tasks);
	closedir(dp);
}

void
outputlist(struct task_vector vec)
{
	int pad, fds[2];
	pid_t pid;
	FILE *fp;

	if (pipe(fds) == -1)
		err(EXIT_FAILURE, "pipe");
	if ((pid = fork()) == -1)
		err(EXIT_FAILURE, "fork");
	if (pid == 0) {
		close(fds[WRITE]);
		close(STDIN_FILENO);
		if (dup2(fds[READ], STDIN_FILENO) == -1)
			err(EXIT_FAILURE, "dup2");
		close(fds[READ]);
		execlp("less", "less", "-FS", NULL);
		err(EXIT_FAILURE, "execlp");
	}

	close(fds[READ]);
	pad = sizetlen(vec.tasks[vec.len - 1].id);
	if ((fp = fdopen(fds[WRITE], "w")) == NULL)
		err(EXIT_FAILURE, "fdopen: Pipe to pager");
	for (size_t i = 0; i < vec.len; i++)
		fprintf(fp, "%*zu. %s\n", pad,
		        vec.tasks[i].id, vec.tasks[i].title);
	fclose(fp);
	if (waitpid(pid, NULL, 0) == -1)
		err(EXIT_FAILURE, "waitpid");
}

void
append(struct task_vector *vec, struct task tsk)
{
	if (vec->len == vec->bufsize) {
		vec->bufsize *= 2;
		vec->tasks = realloc(vec->tasks,
		                     vec->bufsize * sizeof(struct task));
		if (vec->tasks == NULL)
			err(EXIT_FAILURE, "realloc");
	}
	vec->tasks[vec->len++] = tsk;
}

static int
sizetlen(size_t n)
{
	return n < 10 ? 1
	     : n < 100 ? 2
	     : n < 1000 ? 3
	     : n < 10000 ? 4
	     : n < 100000 ? 5
	     : n < 1000000 ? 6
	     : n < 10000000 ? 7
	     : n < 100000000 ? 8
	     : n < 1000000000 ? 9
	     : n < 10000000000 ? 10
	     : n < 100000000000 ? 11
	     : n < 1000000000000 ? 12
	     : n < 10000000000000 ? 13
	     : n < 100000000000000 ? 14
	     : n < 1000000000000000 ? 15
	     : n < 10000000000000000 ? 16
	     : n < 100000000000000000 ? 17
	     : n < 1000000000000000000 ? 18
	     :                            19;
}

int
qsort_helper(const void *x, const void *y)
{
	return ((struct task *) x)->id - ((struct task *) y)->id;
}
