#ifndef TASK_H
#define TASK_H

#define DONEDIR "done"
#define TODODIR "todo"

extern const char *argv0;

void subcmdadd(int, char **, int *);
void subcmdlist(int, char **, int *);
void subcmddone(int, char **, int *);

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

#endif /* !TASK_H */
