#include <sys/types.h>
#include <sys/wait.h>

#include <dirent.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <gevector.h>

#include "common.h"
#include "tag-vector.h"
#include "task.h"

GEVECTOR_API(struct task, taskvec)
GEVECTOR_IMPL(struct task, taskvec)

bool aflag, dflag, lflag, sflag, tflag;

enum pipe_ends {
	READ,
	WRITE
};

static int  printbody(FILE *, int);
static int  qsort_helper(const void *, const void *);
static void queuetasks(struct taskvec *, struct tagvec *, int, uintmax_t *,
                       int);
static void queuetasks_helper(struct taskvec *, char *, int, uintmax_t *, int);
static void outputlist(struct taskvec);

void
subcmdlist(int argc, char **argv)
{
	int opt;
	uintmax_t *ids = NULL;
	struct taskvec tasks;
	const char *opts = "adlst:";
	struct tagvec tags = { .items = NULL };
	static struct option longopts[] = {
		{"all",     no_argument,       NULL, 'a'},
		{"done",    no_argument,       NULL, 'd'},
		{"long",    no_argument,       NULL, 'l'},
		{"special", no_argument,       NULL, 's'},
		{"tags",    required_argument, NULL, 't'},
		{ NULL,     0,                 NULL,  0 }
	};

	while ((opt = getopt_long(argc, argv, opts, longopts, NULL)) != -1) {
		switch (opt) {
		case 'a':
			aflag = true;
			break;
		case 'd':
			dflag = true;
			break;
		case 'l':
			lflag = true;
			break;
		case 's':
			sflag = true;
			break;
		case 't':
			if (!tflag) {
				if (tagvec_new(&tags, 0, 0) == -1)
					die("tagvec_new");
				tflag = true;
			}
			parsetags(&tags, optarg);
			break;
		default:
			fprintf(stderr,
			        "Usage: %s ls [-adls] [-t tags] [id ...]\n",
			        argv0);
			exit(EXIT_FAILURE);
		}
	}

	argc -= optind, argv += optind;
	if (argc != 0)
		ids = parseids(argv, argc);

	if (taskvec_new(&tasks, (size_t) argc, 0) == -1)
		die("taskvec_new");
	if (aflag || !dflag)
		queuetasks(&tasks, &tags, dfds[TODO], ids, argc);
	if (aflag ||  dflag)
		queuetasks(&tasks, &tags, dfds[DONE], ids, argc);
	free(ids);

	qsort(tasks.items, tasks.length, sizeof(struct task), &qsort_helper);
	outputlist(tasks);
	free(tasks.items);
	free(tags.items);
}

void
queuetasks(struct taskvec *tasks, struct tagvec *tags, int dfd, uintmax_t *ids,
           int idcnt)
{
	if (tags->items == NULL)
		queuetasks_helper(tasks, ".", dfd, ids, idcnt);
	else for (size_t i = 0; i < tags->length; i++)
		queuetasks_helper(tasks, tags->items[i], dfd, ids, idcnt);
}

void
queuetasks_helper(struct taskvec *tasks, char *dirpath, int dfd, uintmax_t *ids,
                  int idcnt)
{
	int fd;
	DIR *dp;
	struct dirent *ent;
	struct task tsk;

	if ((fd = openat(dfd, dirpath, D_FLAGS)) == -1)
		die("openat: '%s'", dirpath);
	if ((dp = fdopendir(fd)) == NULL)
		die("fdopendir");
	while (errno = 0, (ent = readdir(dp)) != NULL) {
		if (streq(ent->d_name, ".") || streq(ent->d_name, ".."))
			continue;
		if (ent->d_type == DT_DIR) {
			queuetasks_helper(tasks, ent->d_name, fd, ids, idcnt);
			continue;
		}
		if (sscanf(ent->d_name, "%ju-", &tsk.id) != 1) {
			ewarnx("%s: Couldn't parse task ID", ent->d_name);
			continue;
		}

		tsk.dfd = dfd;
		tsk.title = strchr(ent->d_name, '-') + 1;

		if (tsk.title[0] == '\0') {
			ewarnx("%s: Task title is empty", ent->d_name);
			continue;
		}
		if (ids != NULL) {
			int i;
			for (i = 0; i < idcnt; i++) {
				if (tsk.id == ids[i])
					break;
			}
			if (i == idcnt)
				continue;
		}

		for (size_t i = 0; i < tasks->length; i++) {
			if (tasks->items[i].id == tsk.id)
				goto duplicate;
		}

		tsk.filename = xstrdup(ent->d_name);
		tsk.title = strchr(tsk.filename, '-') + 1;
		if (taskvec_append(tasks, tsk) == -1)
			die("taskvec_append");
duplicate:;
	}
	if (errno != 0)
		die("readdir");

	closedir(dp);
}

void
outputlist(struct taskvec tasks)
{
	bool tty = true;
	int fd, fds[2];
	pid_t pid;
	size_t pad;
	FILE *pager = stdout;
	struct task tsk;

	if (tasks.length == 0)
		return;

	if (!isatty(STDOUT_FILENO)) {
		tty = false;
		goto not_a_tty;
	}

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
	if ((pager = fdopen(fds[WRITE], "w")) == NULL)
		die("fdopen: Pipe to pager");

not_a_tty:
	pad = uintmaxlen(tasks.items[tasks.length - 1].id);
	if (!lflag) {
		for (size_t i = 0; i < tasks.length; i++) {
			tsk = tasks.items[i];
			fprintf(pager, "%*ju. %s\n",
			       (int) pad, tsk.id, tsk.title);
			free(tsk.filename);
		}
	} else {
		for (size_t i = 0; i < tasks.length; i++) {
			tsk = tasks.items[i];
			if ((fd = openat(tsk.dfd, tsk.filename, O_RDONLY))
					== -1)
				die("openat: '%s'", tsk.filename);
			if (tty == true || sflag)
				fprintf(pager, "\033[1mTask Title:\033[0m "
				               "\033[4m%s\033[0m\n"
				               "\033[1mTask ID:\033[0m    "
				               "\033[4m%ju\033[0m\n",
				        tsk.title, tsk.id);
			else
				printf("Task Title: %s\nTask ID:    %ju\n",
				       tsk.title, tsk.id);
			if (printbody(pager, fd) == -1)
				warn("printbody: '%s'", tsk.filename);
			if (i < tasks.length - 1)
				fputc('\n', pager);
			free(tsk.filename);
			close(fd);
		}
	}

	if (tty == true) {
		fclose(pager);
		if (waitpid(pid, NULL, 0) == -1)
			die("waitpid");
	}
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

int
qsort_helper(const void *x, const void *y)
{
	return ((struct task *) x)->id - ((struct task *) y)->id;
}
