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

#include <gevector.h>

#include "common.h"
#include "task.h"

GEVECTOR_API(struct task, taskvec)
GEVECTOR_IMPL(struct task, taskvec)

bool aflag, dflag, lflag, sflag;

enum pipe_ends {
	READ,
	WRITE
};

static int  printbody(FILE *, int);
static int  qsort_helper(const void *, const void *);
static void queuetasks(struct taskvec *, int, int, uintmax_t *);
static void outputlist(struct taskvec);

void
subcmdlist(int argc, char **argv)
{
	int opt;
	char *p;
	uintmax_t id, *ids = NULL;
	struct taskvec tasks;
	static struct option longopts[] = {
		{"all",     no_argument, NULL, 'a'},
		{"done",    no_argument, NULL, 'd'},
		{"long",    no_argument, NULL, 'l'},
		{"special", no_argument, NULL, 's'},
		{ NULL,     0,           NULL,  0 }
	};

	while ((opt = getopt_long(argc, argv, "adls", longopts, NULL)) != -1) {
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
		default:
			fprintf(stderr, "Usage: %s ls [-adl] [id ...]\n",
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

	if (taskvec_new(&tasks, (size_t) argc, 0) == -1)
		die("taskvec_new");
	if (aflag || !dflag)
		queuetasks(&tasks, dfds[TODO], argc, ids);
	if (aflag ||  dflag)
		queuetasks(&tasks, dfds[DONE], argc, ids);
	free(ids);

	qsort(tasks.items, tasks.size, sizeof(struct task), &qsort_helper);

	if (tasks.size > 0)
		outputlist(tasks);
	free(tasks.items);
}

void
queuetasks(struct taskvec *tasks, int dfd, int idcnt, uintmax_t *ids)
{
	int ddfd;
	DIR *dp;
	struct dirent *ent;
	struct task tsk;

	if ((ddfd = dup(dfd)) == -1)
		die("dup");
	if ((dp = fdopendir(ddfd)) == NULL)
		die("fdopendir");
	while ((ent = readdir(dp)) != NULL) {
		if (streq(ent->d_name, ".") || streq(ent->d_name, ".."))
			continue;
		if (sscanf(ent->d_name, "%ju-", &tsk.id) != 1) {
			/* TODO: Make the program exit with EXIT_FAILURE if this
			 * warning is issued.
			 */
			warnx("%s: Couldn't parse task ID", ent->d_name);
			continue;
		}

		tsk.dfd = dfd;
		tsk.title = strchr(ent->d_name, '-') + 1;

		if (tsk.title[0] == '\0') {
			/* TODO: Same here as the above comment */
			warnx("%s: Task title is empty", ent->d_name);
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

		tsk.filename = xstrdup(ent->d_name);
		tsk.title = strchr(tsk.filename, '-') + 1;
		if (taskvec_append(tasks, tsk) == -1)
			die("taskvec_append");
	}

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
