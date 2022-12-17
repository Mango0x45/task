#ifndef TASK_H
#define TASK_H

#define DONEDIR "done"
#define TODODIR "todo"

#define TASK_NAME_MAX 255
#define TASK_PATH_MAX 4096

enum fd_type {
	DONE,
	TODO,
	/* Number of enum members */
	FD_COUNT
};

struct task {
	uintmax_t id;
	const char *title, *filename;
};

extern int dfds[FD_COUNT];
extern const char *argv0;

void subcmdadd(int, char **);
void subcmdlist(int, char **);
void subcmddone(int, char **);

#endif /* !TASK_H */
