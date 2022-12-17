#include <sys/types.h>
#include <sys/wait.h>

#include <dirent.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "task.h"

#define OPENATFLAGS (O_CREAT | O_EXCL | O_WRONLY)
#define TASK_PATH_MAX 4096

static void      mktaske(int, char *);
static void      mktaskt(int, char *);
static void      mktask_from_file(FILE *, int);
static void      spawneditor(char *);
static uintmax_t mktaskid(int);
static char     *getpath(int, char *);
static long      getpathmax(int);

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
	(!eflag && *argv != NULL ? mktaskt : mktaske)(dfds[TODO], *argv);
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
	char *path;
	FILE *fp;

	path = getpath(dfd, s);
	if ((fd = openat(dfd, path, OPENATFLAGS, 0644)) == -1)
		die("openat: '%s'", path);
	if ((fp = fdopen(fd, "w")) == NULL)
		die("fdopen: '%s'", path);

	free(path);
	fclose(fp);
}

void
mktask_from_file(FILE *ifp, int dfd)
{
	int fd;
	char *s, *buf, *line, *path;
	size_t bbs = 1, lbs = 0;
	ssize_t nr;
	FILE *ofp;

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

	path = getpath(dfd, buf);
	if ((fd = openat(dfd, path, OPENATFLAGS, 0644)) == -1)
		die("openat: '%s'", path);
	if ((ofp = fdopen(fd, "w")) == NULL)
		die("fdopen: '%s'", path);

	while ((nr = getline(&line, &lbs, ifp)) != -1)
		fputs(line, ofp);
	if (ferror(ifp))
		die("getline: '%s'", path);

	free(buf);
	free(line);
	free(path);
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
mktaskid(int fd)
{
	int nfd;
	DIR *dp;
	uintmax_t tmp, max = 0;
	struct dirent *ent;

	/* From the fdopendir() manual page:
	 *
	 *     After a successful call to fdopendir(), fd is used internally by
	 *     the implementation, and should not otherwise be used by the
	 *     application.
	 *
	 * We do want to keep using ‘fd’ afterwards, so we need to duplicate the
	 * file descriptor.
	 */
	if ((nfd = dup(fd)) == -1)
		die("dup");

	if ((dp = fdopendir(nfd)) == NULL)
		die("fdopendir");
	while ((ent = readdir(dp)) != NULL) {
		if (streq(ent->d_name, ".") || streq(ent->d_name, ".."))
			continue;
		sscanf(ent->d_name, "%ju", &tmp);
		if (tmp > max)
			max = tmp;
	}

	closedir(dp);
	return max + 1;
}

char *
getpath(int dfd, char *s)
{
	char *path;
	size_t pathmax, pathlen;
	uintmax_t tid;

	s = strstrip(s);
	tid = mktaskid(dfd);
	pathmax = getpathmax(dfd);
	pathlen = strlen(s) + uintmaxlen(tid) + 2;

	if (pathlen > (size_t) pathmax) {
		errno = ENAMETOOLONG;
		die("mktaskt: %ju-%s", tid, s);
	}

	path = xmalloc(pathlen);
	sprintf(path, "%ju-%s", tid, s);
	path[pathlen - 1] = '\0';

	return path;
}

long
getpathmax(int fd)
{
	long pathmax;

	errno = 0;
	if ((pathmax = fpathconf(fd, _PC_NAME_MAX)) == -1) {
		if (errno != 0)
			die("fpathconf");
		return TASK_PATH_MAX;
	}

	return pathmax;
}
