
CC	= gcc
#CFLAGS	= 
#CFLAGS	= -DLATEX
CFLAGS	= -DTOKEN_HTML -g
LFLAGS	=

OBJS	= codegen.o \
	  compile.o \
	  getSource.o \
	  main.o \
	  table.o

.SUFFIXES	: .o .c
.PHONY: clean tags

.c.o	: 
	$(CC) $(CFLAGS) -c $<

pl0d	: ${OBJS}
	$(CC) -o $@ ${OBJS}

clean:
	\rm -rf *~ *.o

tags:
	ctags *.c *.h
