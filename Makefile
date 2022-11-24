.POSIX:

CC       = cc
CFLAGS   = -Wall -Wextra -Wpedantic -Werror      \
           -march=native -mtune=native -O3 -flto \
           -pipe `./genflags --cflags`
CPPFLAGS = -DLIBBSD_NETBSD_VIS
LDLIBS   = `./genflags --libs`

prog = task
objs = src/common.o    \
       src/task-add.o  \
       src/task-done.o \
       src/task-ls.o   \
       src/task.o

all: ${prog}
${prog}: ${objs}
	${CC} ${CFLAGS} ${LDFLAGS} ${LDLIBS} -o $@ ${objs}

src/common.o: src/common.c
	${CC} ${CFLAGS} ${CPPFLAGS} -c -o $@ $<
src/task-add.o: src/task-add.c
	${CC} ${CFLAGS} ${CPPFLAGS} -c -o $@ $<
src/task-done.o: src/task-done.c
	${CC} ${CFLAGS} ${CPPFLAGS} -c -o $@ $<
src/task-ls.o: src/task-ls.c
	${CC} ${CFLAGS} ${CPPFLAGS} -c -o $@ $<
src/task.o: src/task.c
	${CC} ${CFLAGS} ${CPPFLAGS} -c -o $@ $<

clean:
	rm -f ${prog} src/*.o
