#ifndef TASK_H
#define TASK_H

#define DONEDIR "done"
#define TODODIR "todo"

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
