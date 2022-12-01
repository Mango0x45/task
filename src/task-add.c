#include <sys/types.h>
#include <sys/wait.h>

#include <dirent.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "task.h"

#define OPENATFLAGS (O_CREAT | O_EXCL | O_WRONLY)

static void   mktaske(int, char *);
static void   mktaskt(int, char *);
static void   mktask_from_file(FILE *, int);
static void   spawneditor(char *);
static size_t mktaskid(int);

void
subcmdadd(int argc, char **argv, int *dfds)
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
	(!eflag && *argv != NULL ? mktaskt : mktaske)(dfds[NDONE], *argv);
}

void
mktaske(int dfd, char *s)
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

	mktask_from_file(fp, dfd);

	unlink(path);
	fclose(fp);
}

void
mktaskt(int dfd, char *s)
{
	int fd;
	FILE *fp;

	if ((fd = openat(dfd, strstrip(s), OPENATFLAGS, 0644)) == -1)
		die("openat: '%s'", s);
	if ((fp = fdopen(fd, "w")) == NULL)
		die("fdopen: '%s'", s);
	fprintf(fp, "Task ID: %zu\n", mktaskid(dfd));

	fclose(fp);
}

void
mktask_from_file(FILE *ifp, int dfd)
{
	int fd;
	char *s, *buf, *line;
	size_t bbs = 1, lbs = 0;
	ssize_t nr;
	FILE *ofp;

	buf = line = NULL;

	while ((nr = getline(&line, &lbs, ifp)) != -1) {
		s = strstrip(line);
		if (*s == '\0')
			break;
		bbs += strlen(s) + 1;
		if (buf == NULL) {
			if ((buf = calloc(bbs, sizeof(char))) == NULL)
				die("calloc");
		} else {
			if ((buf = realloc(buf, bbs)) == NULL)
				die("realloc");
		}
		strlcat(buf, " ", bbs);
		strlcat(buf, s, bbs);
	}
	if (ferror(ifp))
		die("getline");

	s = strstrip(buf);
	if ((fd = openat(dfd, s, OPENATFLAGS, 0644)) == -1)
		die("openat: '%s'", s);

	if ((ofp = fdopen(fd, "w")) == NULL)
		die("fdopen: '%s'", s);
	fprintf(ofp, "Task ID: %zu\n\n", mktaskid(dfd));
	while ((nr = getline(&line, &lbs, ifp)) != -1)
		fputs(line, ofp);
	if (ferror(ifp))
		die("getline: '%s'", s);

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

size_t
mktaskid(int fd)
{
	DIR *dp;
	size_t cnt = 0;
	struct dirent *ent;

	if ((dp = fdopendir(fd)) == NULL)
		die("fdopendir");
	while ((ent = readdir(dp)) != NULL) {
		if (!streq(ent->d_name, ".") && !streq(ent->d_name, ".."))
			cnt++;
	}

	closedir(dp);
	return cnt;
}
