TARG=getdrv
INCLUDES=-I.
CFLAGS=-pipe -g2 -O2 -std=c99 -Wfatal-errors -Wextra -Wall -Werror -Wunused-macros -Wunreachable-code -Wformat=2 -fdata-sections -ffunction-sections -fno-show-column ${INCLUDES}
LDFLAGS=-Wl,--gc-sections
SRCS=main.c stack.c

${TARG}: ${SRCS} main.h Makefile
	@${CC} ${LDFLAGS} ${CFLAGS} -o $@ ${SRCS}
	@echo ${TARG} is built

debug: main.c backtrace.c main.h Makefile
	${CC} ${CFLAGS} -Wl,--gc-sections -o ${TARG} ${SRCS} backtrace.c -lbfd

check:
	@cppcheck -q -f --enable=all ${SRCS} ${INCLUDES}

lint:
	@splint +checks +quiet +linelen 110 +trytorecover +matchanyintegral +posixlib -statictrans -nullpass -compdestroy -onlytrans -branchstate -compdef -type -nullassign -nullret ${SRCS} ${INCLUDES}

clean:
	${RM} ${TARG}
