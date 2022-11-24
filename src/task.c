#include <sys/stat.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "task.h"

#ifndef O_PATH
	#define O_PATH (O_RDONLY | O_DIRECTORY)
#endif

const char *argv0;

static void mkdatadirs(int *);
static void usage(void);

int
main(int argc, char *argv[])
{
	int fds[FD_COUNT];
	argv0 = getprogname();

	if (argc == 1)
		usage();
	mkdatadirs(fds);

	argc--, argv++;
	if (streq(argv[0], "add"))
		subcmdadd(argc, argv, fds);
	else if (streq(argv[0], "ls"))
		subcmdlist(argc, argv, fds);
	else if (streq(argv[0], "done"))
		subcmddone(0);
	else {
		warnx("invalid subcommand -- '%s'", argv[0]);
		usage();
	}

	close(fds[DONE]);
	close(fds[NDONE]);
	return EXIT_SUCCESS;
}

void
mkdatadirs(int *fds)
{
	int fd;
	bool home = false;
	char *base, buf[PATH_MAX + 1];
	const size_t n = sizeof(buf);

	if ((base = getenv("XDG_DATA_HOME")) == NULL) {
		if ((base = getenv("HOME")) == NULL)
			errx(EXIT_FAILURE,
			     "The HOME environment variable is not set");
		home = true;
	}

	if (strlcpy(buf, base, n) >= n)
		errtoolong("%s", base);
	if (home && strlcat(buf, "/.local/share", n) >= n)
		errtoolong("%s/.local/share", buf);
	if (strlcat(buf, "/task", n) >= n)
		errtoolong("%s/task", buf);

	if (mkdir(buf, 0777) == -1 && errno != EEXIST)
		err(EXIT_FAILURE, "mkdir: '%s'", buf);
	if ((fd = open(buf, O_PATH)) == -1)
		err(EXIT_FAILURE, "open: '%s'", buf);

	if (mkdirat(fd,  DONEDIR, 0777) == -1 && errno != EEXIST)
		err(EXIT_FAILURE, "mkdir: '%s/"DONEDIR"'", buf);
	if (mkdirat(fd, NDONEDIR, 0777) == -1 && errno != EEXIST)
		err(EXIT_FAILURE, "mkdir: '%s/"NDONEDIR"'", buf);

	if ((fds[DONE]  = openat(fd,  DONEDIR, O_PATH)) == -1)
		err(EXIT_FAILURE, "openat: '%s/"DONEDIR"'", buf);
	if ((fds[NDONE] = openat(fd, NDONEDIR, O_PATH)) == -1)
		err(EXIT_FAILURE, "openat: '%s/"NDONEDIR"'", buf);

	close(fd);
}

void
usage(void)
{
	fprintf(stderr, "Usage: %s add [-e] [title]\n"
	                "       %s ls [-l] [id ...]\n"
	                "       %s done id ...\n", argv0, argv0, argv0);
	exit(EXIT_FAILURE);
}
