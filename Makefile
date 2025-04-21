#
# Makefile for icom
#
PROGRAM= icom
COMPILER= gcc
COPTS= -g -O 
BINDIR= /usr/local/bin
INSTALL= install
DEFS=
#
INCL= -I../include
CFLAGS= $(COPTS) $(DEFS) $(INCL)
CC= $(COMPILER)
LIB= /lib/libm.so
#
SOURCE= icom.c radio.c packet.c tables.c
OBJS= icom.o radio.o packet.o tables.o
EXEC= icom

all:	$(PROGRAM)

icom:	$(OBJS)
	$(CC) $(COPTS) -o $@ $(OBJS) $(LIB)

install: $(BINDIR)/$(PROGRAM)

$(BINDIR)/$(PROGRAM): $(PROGRAM)
	$(INSTALL) -c -m 0755 $(PROGRAM) $(BINDIR)

tags:
	ctags *.c *.h

depend:
	mkdep $(CFLAGS) $(SOURCE)

clean:
	-@rm -f $(PROGRAM) $(EXEC) $(OBJS)
