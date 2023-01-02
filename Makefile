.POSIX:

CC       = cc
CFLAGS   = -Wall -Wextra -Wpedantic -Werror      \
           -march=native -mtune=native -O3 -flto \
           -pipe
CPPFLAGS = `./genflags --cflags` -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200809L \
	   -DLIBBSD_NETBSD_VIS
LDLIBS   = `./genflags --libs`

PREFIX  = /usr/local
DPREFIX = ${DESTDIR}${PREFIX}

prog = task
objs = src/common.o     \
       src/tag-vector.o \
       src/task-add.o   \
       src/task-done.o  \
       src/task-ls.o    \
       src/task-tag.o   \
       src/task.o       \
       src/umax-set.o

all: ${prog}
${prog}: ${objs}
	${CC} ${CFLAGS} ${LDFLAGS} ${LDLIBS} -o $@ ${objs}

src/common.o: src/common.c
	${CC} ${CFLAGS} ${CPPFLAGS} -c -o $@ $<
src/tag-vector.o: src/tag-vector.c
	${CC} ${CFLAGS} ${CPPFLAGS} -c -o $@ $<
src/task-add.o: src/task-add.c
	${CC} ${CFLAGS} ${CPPFLAGS} -c -o $@ $<
src/task-done.o: src/task-done.c
	${CC} ${CFLAGS} ${CPPFLAGS} -c -o $@ $<
src/task-ls.o: src/task-ls.c
	${CC} ${CFLAGS} ${CPPFLAGS} -c -o $@ $<
src/task-tag.o: src/task-tag.c
	${CC} ${CFLAGS} ${CPPFLAGS} -c -o $@ $<
src/task.o: src/task.c
	${CC} ${CFLAGS} ${CPPFLAGS} -c -o $@ $<
src/umax-set.o: src/umax-set.c
	${CC} ${CFLAGS} ${CPPFLAGS} -c -o $@ $<

clean:
	rm -f ${prog} src/*.o

install:
	mkdir -p ${DPREFIX}/bin
	cp ${prog} ${DPREFIX}/bin
