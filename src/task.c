#include <sys/stat.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "task.h"

int rv = EXIT_SUCCESS;
const char *argv0;

static void mkdirs(void);
static void usage(void);

int
main(int argc, char *argv[])
{
	argv0 = getprogname();

	if (argc == 1)
		usage();
	mkdirs();

	argc--, argv++;
	if (streq(argv[0], "add"))
		subcmdadd(argc, argv);
	else if (streq(argv[0], "done"))
		subcmddone(argc, argv);
	else if (streq(argv[0], "ls"))
		subcmdlist(argc, argv);
	else if (streq(argv[0], "tag"))
		subcmdtag(argc, argv);
	else {
		warnx("invalid subcommand -- '%s'", argv[0]);
		usage();
	}

	return rv;
}

void
mkdirs(void)
{
	bool home = false;
	char *base, *buf;
	size_t n;

	if ((base = getenv("XDG_DATA_HOME")) == NULL) {
		if ((base = getenv("HOME")) == NULL)
			errx(EXIT_FAILURE,
			     "The HOME environment variable is not set");
		home = true;
	}

	n = getpathmax(base) + 1;
	buf = xmalloc(n);

	if (strlcpy(buf, base, n) >= n)
		errtoolong("%s", base);
	if (home && strlcat(buf, "/.local/share", n) >= n)
		errtoolong("%s/.local/share", buf);
	if (strlcat(buf, "/task", n) >= n)
		errtoolong("%s/task", buf);

	if (mkdir(buf, 0777) == -1 && errno != EEXIST)
		die("mkdir: '%s'", buf);
	if (chdir(buf) == -1)
		die("chdir: '%s'", buf);

	free(buf);
}

void
usage(void)
{
	fprintf(stderr, "Usage: %s add [-e] [title]\n"
	                "       %s done id ...\n"
	                "       %s ls [-adls] [id ...]\n"
	                "       %s tag [[-r] tags id ...]\n",
	        argv0, argv0, argv0, argv0);
	exit(EXIT_FAILURE);
}
