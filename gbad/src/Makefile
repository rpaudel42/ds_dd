# Makefile
#
# GBAD 3.3
#
CC =		gcc
CFLAGS =	-Wall -O3
LDFLAGS =	-O3
# debug CFLAGS and LDFLAGS
#CFLAGS =	-g -pg -Wall -O3
#LDFLAGS =	-g -pg -O3

LDLIBS =	-lm
OBJS = 		compress.o discover.o dot.o evaluate.o extend.o graphmatch.o\
                graphops.o labels.o sgiso.o subops.o utility.o \
                gbad.o actions.o lex.yy.o y.tab.o  
TARGETS =	gbad graph2dot

all: $(TARGETS)

graph2dot: graph2dot_main.o $(OBJS)
	$(CC) $(LDFLAGS) -o graph2dot graph2dot_main.o $(OBJS) $(LDLIBS)

gbad: main.o $(OBJS)
	 $(CC) $(LDFLAGS) -o gbad main.o $(OBJS) $(LDLIBS)

%.o: %.c gbad.h
	$(CC) -c $(CFLAGS) $< -o $@

install:
	cp $(TARGETS) ../bin

clean:
	/bin/rm -f *.o $(TARGETS)

