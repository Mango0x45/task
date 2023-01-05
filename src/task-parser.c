#include <ctype.h>
#include <err.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

#include "common.h"
#include "task.h"
#include "tagset.h"

static char *strtrim(char *, size_t);

struct task
parsetask(const char *filename, bool parsebody)
{
	FILE *fp;
	char *s, *line = NULL;
	size_t len = 0;
	ssize_t nr;
	struct task task;

	if ((fp = fopen(filename, "r")) == NULL)
		die("fopen: '%s'", filename);
	if (sscanf(filename, "%ju", &task.id) != 1)
		die("Invalid task id: '%s'", filename);
	if (tagset_new(&task.tags, 0, 0) == -1)
		die("tagset_new");

	while ((nr = getline(&line, &len, fp)) != -1) {
		if (line[nr - 1] == '\n')
			line[--nr] = '\0';

		if (strncaseeq(line, "Title:", 6)) {
			if ((s = strtrim(line + 6, nr - 6)) == NULL)
				diex("Task has empty title");
			task.title = xstrdup(s);
		} else if (strncaseeq(line, "Status:", 7)) {          
			if ((s = strtrim(line + 7, nr - 7)) == NULL)
				diex("Task has empty status");

			if (streq(s, "done"))
				task.status = DONE;
			else if (streq(s, "todo"))
				task.status = TODO;
			else
				diex("Invalid value for task status field '%s'",
				     s);
		} else if (strncaseeq(line, "Tag:", 4)) {
			if ((s = strtrim(line + 4, nr - 4)) == NULL)
				die("Task has empty tag");
			if (tagset_add(&task.tags, xstrdup(s)) == -1)
				die("tagset_add");
		} else if (len == 0)
			break;
	}
	if (ferror(fp))
		die("getline");

	if (parsebody)
		task.body = xstrdup("hello world\n");

	free(line);
	fclose(fp);
	return task;
}

char *
strtrim(char *str, size_t len)
{
	char *s = str, *e = str + len - 1;

	while (isspace(*s))
		s++;
	while (e >= s && isspace(*e))
		e--;
	e[1] = '\0';

	return s < e ? s : NULL;
}
