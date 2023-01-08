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

#include <geset.h>
#include <gevector.h>

#include "common.h"
#include "tagset.h"
#include "task.h"
#include "task-parser.h"
#include "umaxset.h"

GEVECTOR_DEF(struct task, taskvec)

bool aflag, dflag, lflag, sflag, tflag;

enum pipe_ends {
	READ,
	WRITE
};

static void printbody(FILE *, FILE *);
static int  qsort_helper(const void *, const void *);
static void queuetasks(taskvec_t *, tagset_t *, enum task_status, umaxset_t *);
static void outputlist(taskvec_t *);

void
subcmdlist(int argc, char **argv)
{
	int opt;
	umaxset_t ids = {0};
	tagset_t tags = {0};
	taskvec_t tasks;
	const char *opts = "adlst:";
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
				if (tagset_new(&tags, 0, 0) == -1)
					die("tagset_new");
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
	if (argc != 0) {
		if (umaxset_new(&ids, argc, 0) == -1)
			die("umaxset_new");
		parseids(&ids, argv, argc);
	}

	if (taskvec_new(&tasks, (size_t) argc, 0) == -1)
		die("taskvec_new");
	if (aflag || !dflag)
		queuetasks(&tasks, &tags, TODO, &ids);
	if (aflag ||  dflag)
		queuetasks(&tasks, &tags, DONE, &ids);

	tagset_free(&tags);
	umaxset_free(&ids);
	qsort(tasks.data, tasks.len, sizeof(struct task), &qsort_helper);
	outputlist(&tasks);
	free(tasks.data);
}

void
queuetasks(taskvec_t *tasks, tagset_t *tags, enum task_status status,
           umaxset_t *ids)
{
	DIR *dp;
	struct task task = {0};
	struct dirent *ent;

	if ((dp = opendir(".")) == NULL)
		die("opendir");
	while (errno = 0, (ent = readdir(dp)) != NULL) {
		if (ent->d_type == DT_DIR)
			continue;
		task = parsetask(ent->d_name, false);
		if (task.status == status && (tagset_empty(tags)
				|| tagset_intersects(tags, &task.tags))
				&& (umaxset_empty(ids)
				|| umaxset_has(ids, task.id)))
			taskvec_push(tasks, task);
		else {
			free(task.title);
			tagset_deep_free(&task.tags);
		}
	}
	if (errno != 0)
		die("readdir");
	closedir(dp);
}

void
outputlist(taskvec_t *tasks)
{
	int fds[2];
	pid_t pid;
	char buf[21];
	size_t pad;
	bool tty = true;
	FILE *fp, *pager = stdout;
	struct task task;

	if (tasks->len == 0)
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
	pad = uintmaxlen(tasks->data[tasks->len - 1].id);
	if (!lflag) {
		for (size_t i = 0; i < tasks->len; i++) {
			task = tasks->data[i];
			fprintf(pager, "%*ju. %s\n", (int) pad, task.id,
			        task.title);
			free(task.title);
			tagset_deep_free(&task.tags);
		}
	} else {
		for (size_t i = 0; i < tasks->len; i++) {
			task = tasks->data[i];

			if (tty == true || sflag)
				fprintf(pager, "\033[1mTask Title:\033[0m "
				               "\033[4m%s\033[0m\n"
				               "\033[1mTask ID:\033[0m    "
				               "\033[4m%ju\033[0m\n",
				        task.title, task.id);
			else
				printf("Task Title: %s\nTask ID:    %ju\n",
				       task.title, task.id);

			sprintf(buf, "%ju", task.id);

			if ((fp = fopen(buf, "r")) == NULL)
				ewarn("fopen: '%s'", task.title);

			printbody(pager, fp);
			fclose(fp);

			if (i < tasks->len - 1)
				fputc('\n', pager);

			free(task.title);
			tagset_deep_free(&task.tags);
		}
	}

	if (tty == true) {
		fclose(pager);
		if (waitpid(pid, NULL, 0) == -1)
			die("waitpid");
	}
}

void
printbody(FILE *ofp, FILE *ifp)
{
	/* TODO: Make line and len static to avoid extra allocations? */
	char *line = NULL;
	size_t len = 0;
	ssize_t nr;

	while ((nr = getline(&line, &len, ifp)) != -1) {
		if (nr <= 1)
			break;
	}
	if (ferror(ifp))
		die("getline");
	if (feof(ifp)) {
		free(line);
		return;
	}

	fputs(line, ofp);
	while ((nr = getline(&line, &len, ifp)) != -1) {
		if (line[nr - 1] == '\n')
			line[nr - 1] = '\0';
		fprintf(ofp, "    %s\n", line);
	}
	if (ferror(ifp))
		die("getline");

	free(line);
}

int
qsort_helper(const void *x, const void *y)
{
	return ((struct task *) x)->id - ((struct task *) y)->id;
}
