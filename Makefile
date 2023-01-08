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
objs = src/common.o      \
       src/compat.o      \
       src/tagset.o      \
       src/task.o        \
       src/task-add.o    \
       src/task-done.o   \
       src/task-ls.o     \
       src/task-parser.o \
       src/task-tag.o    \
       src/umaxset.o

all: ${prog}
${prog}: ${objs}
	${CC} ${CFLAGS} ${LDFLAGS} ${LDLIBS} -o $@ ${objs}

src/common.o: src/common.c
	${CC} ${CFLAGS} ${CPPFLAGS} -c -o $@ $<
src/tagset.o: src/tagset.c
	${CC} ${CFLAGS} ${CPPFLAGS} -c -o $@ $<
src/task.o: src/task.c
	${CC} ${CFLAGS} ${CPPFLAGS} -c -o $@ $<
src/task-add.o: src/task-add.c
	${CC} ${CFLAGS} ${CPPFLAGS} -c -o $@ $<
src/task-done.o: src/task-done.c
	${CC} ${CFLAGS} ${CPPFLAGS} -c -o $@ $<
src/task-ls.o: src/task-ls.c
	${CC} ${CFLAGS} ${CPPFLAGS} -c -o $@ $<
src/task-parser.o: src/task-parser.c
	${CC} ${CFLAGS} ${CPPFLAGS} -c -o $@ $<
src/task-tag.o: src/task-tag.c
	${CC} ${CFLAGS} ${CPPFLAGS} -c -o $@ $<
src/umaxset.o: src/umaxset.c
	${CC} ${CFLAGS} ${CPPFLAGS} -c -o $@ $<

clean:
	rm -f ${prog} src/*.o

install:
	mkdir -p ${DPREFIX}/bin
	cp ${prog} ${DPREFIX}/bin
