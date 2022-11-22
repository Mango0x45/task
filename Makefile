.POSIX:

CC      = cc
CFLAGS  = -Wall -Wextra -Wpedantic -Werror \
          -march=native -mtune=native -O3  \
          -pipe `./genflags --cflags`      \
          -DLIBBSD_NETBSD_VIS
LDFLAGS = -flto `./genflags --libs`

prog = task
objs = src/common.o    \
       src/task-add.o  \
       src/task-done.o \
       src/task-ls.o   \
       src/task.o

all: ${prog}
${prog}: ${objs}
	${CC} ${CFLAGS} ${LDFLAGS} -o $@ ${objs}

src/common.o: src/common.c
	${CC} ${CFLAGS} -c -o $@ $<
src/task-add.o: src/task-add.c
	${CC} ${CFLAGS} -c -o $@ $<
src/task-done.o: src/task-done.c
	${CC} ${CFLAGS} -c -o $@ $<
src/task-ls.o: src/task-ls.c
	${CC} ${CFLAGS} -c -o $@ $<
src/task.o: src/task.c
	${CC} ${CFLAGS} -c -o $@ $<

clean:
	rm -f ${prog} src/*.o
