TARGETS	= coder
PREFIX	= /usr/local
DESTDIR	=
BINDIR	= $(PREFIX)/bin
SBINDIR	= $(PREFIX)/sbin
CC	= cc
CPPFLAGSF=
CPPFLAGS=
CFLAGSF	= -W `pkg-config --cflags libDesktop`
CFLAGS	= -Wall -g -O2 -pedantic
LDFLAGSF= `pkg-config --libs libDesktop` -lintl
LDFLAGS	= 
RM	= rm -f
LN	= ln -f
MKDIR	= mkdir -m 0755 -p
INSTALL	= install


all: $(TARGETS)

coder_OBJS = callbacks.o coder.o main.o project.o
coder_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
coder_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

coder: $(coder_OBJS)
	$(CC) -o coder $(coder_OBJS) $(coder_LDFLAGS)

callbacks.o: callbacks.c callbacks.h coder.h project.h ../config.h
	$(CC) -D PREFIX=\"$(PREFIX)\" $(coder_CFLAGS) -c callbacks.c

coder.o: coder.c callbacks.h coder.h project.h ../config.h
	$(CC) -D PREFIX=\"$(PREFIX)\" $(coder_CFLAGS) -c coder.c

main.o: main.c coder.h
	$(CC) $(coder_CFLAGS) -c main.c

project.o: project.c project.h
	$(CC) $(coder_CFLAGS) -c project.c

clean:
	$(RM) -- $(coder_OBJS)

distclean: clean
	$(RM) -- $(TARGETS)

install: $(TARGETS)
	$(MKDIR) $(DESTDIR)$(BINDIR)
	$(INSTALL) -m 0755 coder $(DESTDIR)$(BINDIR)/coder

uninstall:
	$(RM) -- $(DESTDIR)$(BINDIR)/coder

.PHONY: all clean distclean install uninstall
