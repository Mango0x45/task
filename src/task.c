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

#define D_FLAGS (O_RDONLY | O_DIRECTORY)

int dfds[FD_COUNT];
const char *argv0;

static void mkdatadirs(void);
static void usage(void);

int
main(int argc, char *argv[])
{
	argv0 = getprogname();

	if (argc == 1)
		usage();
	mkdatadirs();

	argc--, argv++;
	if (streq(argv[0], "add"))
		subcmdadd(argc, argv);
	else if (streq(argv[0], "ls"))
		subcmdlist(argc, argv);
	else if (streq(argv[0], "done"))
		subcmddone(argc, argv);
	else {
		warnx("invalid subcommand -- '%s'", argv[0]);
		usage();
	}

	close(dfds[DONE]);
	close(dfds[TODO]);
	return EXIT_SUCCESS;
}

void
mkdatadirs(void)
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
		die("mkdir: '%s'", buf);
	if ((fd = open(buf, D_FLAGS)) == -1)
		die("open: '%s'", buf);

	if (mkdirat(fd, DONEDIR, 0777) == -1 && errno != EEXIST)
		die("mkdir: '%s/"DONEDIR"'", buf);
	if (mkdirat(fd, TODODIR, 0777) == -1 && errno != EEXIST)
		die("mkdir: '%s/"TODODIR"'", buf);

	if ((dfds[DONE] = openat(fd, DONEDIR, D_FLAGS)) == -1)
		die("openat: '%s/"DONEDIR"'", buf);
	if ((dfds[TODO] = openat(fd, TODODIR, D_FLAGS)) == -1)
		die("openat: '%s/"TODODIR"'", buf);

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
