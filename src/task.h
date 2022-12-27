#ifndef TASK_H
#define TASK_H

#define D_FLAGS (O_RDONLY | O_DIRECTORY)

#define DONEDIR "done"
#define TODODIR "todo"

enum fd_type {
	DONE,
	TODO,
	/* Number of enum members */
	FD_COUNT
};

struct task {
	int dfd;
	uintmax_t id;
	char *filename;
	const char *title;
};

extern int rv;
extern int dfds[FD_COUNT];
extern const char *argv0;

void subcmdadd(int, char **);
void subcmddone(int, char **);
void subcmdlist(int, char **);
void subcmdtag(int, char **);

#endif /* !TASK_H */
