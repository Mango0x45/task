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

#include <gevector.h>

#include "common.h"
#include "tag-vector.h"
#include "task.h"

bool rflag;

static void listtags(void);
static void addtags(struct tagvec *, uintmax_t *, int);
static void removetags(struct tagvec *, uintmax_t *, int);
static void tagtask(struct tagvec *, char *, int);
static void usage(void);

void
subcmdtag(int argc, char **argv)
{
	int opt;
	uintmax_t *ids;
	struct tagvec tags;
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

	if (tagvec_new(&tags, 0, 0) == -1)
		die("tagvec_new");
	parsetags(&tags, *argv);
	for (size_t i = 0; i < tags.length; i++) {
		if (*tags.items[i] == '\0') {
			ewarnx("Empty tag provided");
			tags.items[i] = NULL;
		}
	}

	ids = parseids(++argv, --argc);
	(rflag ? removetags : addtags)(&tags, ids, argc);
	free(ids);
	free(tags.items);
}

void
listtags(void)
{
	struct tagvec vec;

	if (tagvec_new(&vec, 0, 0) == -1)
		die("tagvec_new");
	iterdir(&vec, dfds[DONE]);
	qsort(vec.items, vec.length, sizeof(char *), voidcoll);
	for (size_t i = 0; i < vec.length; i++) {
		puts(vec.items[i]);
		free(vec.items[i]);
	}
	free(vec.items);
}

void
addtags(struct tagvec *vec, uintmax_t *ids, int idcnt)
{
	DIR *dp;
	char *base, *start, *end;
	uintmax_t id;
	struct dirent *ent;

	for (size_t i = 0; i < vec->length; i++) {
		base = start = vec->items[i];
		while ((end = strchr(start, '/')) != NULL) {
			*end = '\0';
			if (mkdirat(dfds[DONE], base, 0777) == -1
					&& errno != EEXIST)
				die("mkdir: '%s'", base);
			if (mkdirat(dfds[TODO], base, 0777) == -1
					&& errno != EEXIST)
				die("mkdir: '%s'", base);
			*end = '/';
			start = end + 1;
		}
		if (mkdirat(dfds[DONE], base, 0777) == -1 && errno != EEXIST)
			die("mkdir: '%s'", base);
		if (mkdirat(dfds[TODO], base, 0777) == -1 && errno != EEXIST)
			die("mkdir: '%s'", base);
	}

	for (int i = 0; i < FD_COUNT; i++) {
		if ((dp = fdopendir(dfds[i])) == NULL)
			die("fdopendir");

		while (errno = 0, (ent = readdir(dp)) != NULL) {
			if (ent->d_type == DT_DIR)
				continue;
			if (sscanf(ent->d_name, "%ju-", &id) != 1) {
				ewarnx("%s: Couldn't parse task ID",
				       ent->d_name);
				continue;
			}
			for (int j = 0; j < idcnt; j++) {
				if (ids[j] == id)
					tagtask(vec, ent->d_name, dfds[i]);
			}
		}
		if (errno != 0)
			die("readdir");

		closedir(dp);
	}
}

void
removetags(struct tagvec *vec, uintmax_t *ids, int idcnt)
{
	DIR *dp;
	char *pathbuf;
	long pathmax;
	uintmax_t id;
	struct dirent *ent;

	pathmax = getpathmax("/");
	pathbuf = xmalloc(pathmax + 1);
	for (int i = 0; i < FD_COUNT; i++) {
		fchdir(dfds[i]);

		for (size_t j = 0; j < vec->length; j++) {
			if ((dp = opendir(vec->items[j])) == NULL)
				die("opendir: '%s'", vec->items[j]);

			while (errno = 0, (ent = readdir(dp)) != NULL) {
				if (ent->d_type == DT_DIR)
					continue;
				if (sscanf(ent->d_name, "%ju-", &id) != 1) {
					ewarnx("%s: Couldn't parse task ID",
					       ent->d_name);
					continue;
				}
				for (int k = 0; k < idcnt; k++) {
					if (ids[k] != id)
						continue;
					sprintf(pathbuf, "%s/%s", vec->items[j],
					        ent->d_name);
					if (unlink(pathbuf) == -1)
						ewarn("unlink: '%s'", pathbuf);
				}
			}
			if (errno != 0)
				die("readdir");

			closedir(dp);
		}
	}

	free(pathbuf);
}

void
tagtask(struct tagvec *vec, char *filename, int ffd)
{
	int tfd;

	for (size_t i = 0; i < vec->length; i++) {
		if ((tfd = openat(ffd, vec->items[i], D_FLAGS)) == -1)
			die("openat: '%s'", vec->items[i]);

		if (linkat(ffd, filename, tfd, filename, 0) == -1
				&& errno != EEXIST)
			die("linkat: '%s'", filename);

		close(tfd);
	}
}

void
usage(void)
{
	fprintf(stderr, "Usage: %s tag [[-r] tags id ...]\n",
		argv0);
	exit(EXIT_FAILURE);
}