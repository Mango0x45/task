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
#include "task-parser.h"
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
	size_t i;
	DIR *dp;
	FILE *fp;
	tagset_t set;
	bool cache = true;
	char *line = NULL;
	char **tags;
	size_t len = 0;
	ssize_t nr;
	struct task task;
	struct dirent *ent;

	if (tagset_new(&set, 0, 0) == -1)
		die("tagset_new");

	if ((fp = fopen("tags", "r+")) == NULL && errno != ENOENT)
		die("fopen: 'tags'");
	if (errno == ENOENT) {
		cache = false;
		if ((fp = fopen("tags", "w")) == NULL)
			die("fopen: 'tags'");
	}

	if (cache) {
		while ((nr = getline(&line, &len, fp)) != -1) {
			if (line[nr - 1] == '\n')
				line[nr - 1] = '\0';
			if (tagset_add(&set, xstrdup(line)) == -1)
				die("tagset_add");
		}
		if (ferror(fp))
			die("getline: 'tags'");

		free(line);
	} else {
		if ((dp = opendir(".")) == NULL)
			die("opendir");
		while (errno = 0, (ent = readdir(dp)) != NULL) {
			if (ent->d_type == DT_DIR || streq(ent->d_name, "tags"))
				continue;
			task = parsetask(ent->d_name, false);
			GESET_FOREACH(tagset, char *, tag, task.tags) {
				if (tagset_has(&set, tag))
					free(tag);
				else if (tagset_add(&set, tag) == -1)
					die("tagset_add");
			}
			free(task.title);
			tagset_free(&task.tags);
		}
		if (errno != 0)
			die("readdir");
		closedir(dp);
	}

	i = 0;
	tags = xmalloc(sizeof(char *) * tagset_size(&set));
	GESET_FOREACH(tagset, char *, tag, set)
		tags[i++] = tag;

	qsort(tags, i, sizeof(char *), voidcoll);
	rewind(fp);

	for (size_t j = 0; j < i; j++) {
		fprintf(fp, "%s\n", tags[j]);
		puts(tags[j]);
	}
	if (ftruncate(fileno(fp), ftell(fp)) == -1)
		die("ftruncate: 'tags'");

	free(tags);

	tagset_deep_free(&set);
	fclose(fp);
}

void
usage(void)
{
	fprintf(stderr, "Usage: %s tag [[-r] tags id ...]\n",
		argv0);
	exit(EXIT_FAILURE);
}
