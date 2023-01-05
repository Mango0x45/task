#include <sys/stat.h>
#include <sys/types.h>

#include <dirent.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <geset.h>

#include "common.h"
#include "tagset.h"
#include "task.h"
#include "umaxset.h"

bool rflag;

static void listtags(void);
static void usage(void);

void
subcmdtag(int argc, char **argv)
{
	int opt;
	umaxset_t ids = {0};
	tagset_t tags;
	static struct option longopts[] = {
		{"remove", no_argument, NULL, 'r'},
		{ NULL,    no_argument, NULL,  0 }
	};

	if (argc == 1) {
		listtags();
		return;
	}

	while ((opt = getopt_long(argc, argv, "r", longopts, NULL)) != -1) {
		switch (opt) {
		case 'r':
			rflag = true;
			break;
		default:
			usage();
		}
	}

	argc -= optind, argv += optind;
	if (argc < 2)
		usage();

	if (tagset_new(&tags, 0, 0) == -1)
		die("tagvec_new");
	parsetags(&tags, *argv);
	if (tagset_has(&tags, "")) {
		ewarnx("Empty tag provided");
		tagset_remove(&tags, "");
	}

	argv++, argc--;
	if (umaxset_new(&ids, argc, 0) == -1)
		die("umaxset_new");
	parseids(&ids, argv, argc);
	//(rflag ? removetags : addtags)(&tags, &ids);
	umaxset_free(&ids);
	tagset_free(&tags);
}

void
listtags(void)
{
}

void
usage(void)
{
	fprintf(stderr, "Usage: %s tag [[-r] tags id ...]\n",
		argv0);
	exit(EXIT_FAILURE);
}
