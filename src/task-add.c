#include <sys/wait.h>

#include <dirent.h>
#include <err.h>
#include <errno.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "task.h"

static void      mktaske(char *);
static void      mktaskt(char *);
static void      mktask_from_file(FILE *);
static void      spawneditor(char *);
static uintmax_t mktaskid(void);

void
subcmdadd(int argc, char **argv)
{
	int opt;
	bool eflag = false;
	static struct option longopts[] = {
		{"edit", no_argument, NULL, 'e'},
		{ NULL,  0,           NULL,  0 }
	};
	while ((opt = getopt_long(argc, argv, "e", longopts, NULL)) != -1) {
		switch (opt) {
		case 'e':
			eflag = true;
			break;
		default:
			fprintf(stderr, "Usage: %s add [-e] [title]\n",
			        argv0);
			exit(EXIT_FAILURE);
		}
	}

	argc -= optind, argv += optind;
	(!eflag && *argv != NULL ? mktaskt : mktaske)(*argv);
}

void
mktaske(char *s)
{
	int fd;
	FILE *fp;
	char path[] = "/tmp/task-add.XXXXXX";

	if ((fd = mkstemp(path)) == -1)
		die("mkstemp: '%s'", path);
	if ((fp = fdopen(fd, "w+")) == NULL)
		die("fdopen: '%s'", path);

	fprintf(fp, "%s\n\nEnter the task description here\n",
	        s != NULL ? s : "Enter the task title here");
	fflush(fp);
	spawneditor(path);
	rewind(fp);
	mktask_from_file(fp);

	unlink(path);
	fclose(fp);
}

void
mktaskt(char *s)
{
	FILE *fp;
	char buf[21];
	uintmax_t id;

	sprintf(buf, "%ju", (id = mktaskid()));
	if ((fp = fopen(buf, "w")) == NULL)
		die("fopen: '%s'", buf);
	s = strstrip(s);
	fprintf(fp, "Title: %s\nStatus: todo\n", s);
	fprintf(stderr, "Created task %ju: %s\n", id, s);

	fclose(fp);
}

void
mktask_from_file(FILE *ifp)
{
	char *s, *buf, *line, path[21];
	size_t bbs = 1, lbs = 0;
	ssize_t nr;
	FILE *ofp;
	uintmax_t id;

	buf = line = NULL;

	while ((nr = getline(&line, &lbs, ifp)) != -1) {
		s = strstrip(line);
		if (*s == '\0')
			break;
		bbs += strlen(s) + 1;
		buf = buf == NULL ? xcalloc(bbs, sizeof(char))
		                  : xrealloc(buf, bbs);
		strlcat(buf, " ", bbs);
		strlcat(buf, s, bbs);
	}
	if (ferror(ifp))
		die("getline");

	if (buf == NULL)
		errx(EXIT_FAILURE, "Empty task provided; aborting");

	sprintf(path, "%ju", (id = mktaskid()));
	if ((ofp = fopen(path, "w")) == NULL)
		die("fopen: '%s'", path);

	s = strstrip(buf);
	fprintf(ofp, "Title: %s\nStatus: todo\n\n", s);

	while ((nr = getline(&line, &lbs, ifp)) != -1)
		fputs(line, ofp);
	if (ferror(ifp))
		die("getline: '%s'", path);

	fprintf(stderr, "Created task %ju: %s\n", id, s);
	free(buf);
	free(line);
	fclose(ofp);
}

void
spawneditor(char *path)
{
	char *e;
	pid_t pid;

	if ((e = getenv("VISUAL")) == NULL) {
		if ((e = getenv("EDITOR")) == NULL)
			e = "vi";
	}

	if ((pid = fork()) == -1)
		die("fork");
	else if (pid == 0) {
		execlp(e, e, path, NULL);
		die("execlp");
	}
	if (waitpid(pid, NULL, 0) == -1)
		die("waitpid");
}

uintmax_t
mktaskid(void)
{
	DIR *dp;
	uintmax_t id = 0;
	struct dirent *ent;

	if ((dp = opendir(".")) == NULL)
		die("opendir");
	while (errno = 0, (ent = readdir(dp)) != NULL) {
		if (ent->d_type != DT_DIR)
			id++;
	}
	if (errno != 0)
		die("readdir");

	closedir(dp);
	return id;
}
