TARGETS	= $(OBJDIR)coder
OBJDIR	=
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

coder_OBJS = $(OBJDIR)callbacks.o $(OBJDIR)coder.o $(OBJDIR)main.o $(OBJDIR)project.o
coder_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
coder_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

$(OBJDIR)coder: $(coder_OBJS)
	$(CC) -o $(OBJDIR)coder $(coder_OBJS) $(coder_LDFLAGS)

$(OBJDIR)callbacks.o: callbacks.c callbacks.h coder.h project.h ../config.h
	$(CC) -D PREFIX=\"$(PREFIX)\" $(coder_CFLAGS) -o $(OBJDIR)callbacks.o -c callbacks.c

$(OBJDIR)coder.o: coder.c callbacks.h coder.h project.h ../config.h
	$(CC) -D PREFIX=\"$(PREFIX)\" $(coder_CFLAGS) -o $(OBJDIR)coder.o -c coder.c

$(OBJDIR)main.o: main.c coder.h
	$(CC) $(coder_CFLAGS) -o $(OBJDIR)main.o -c main.c

$(OBJDIR)project.o: project.c project.h
	$(CC) $(coder_CFLAGS) -o $(OBJDIR)project.o -c project.c

clean:
	$(RM) -- $(coder_OBJS)

distclean: clean
	$(RM) -- $(TARGETS)

install: $(TARGETS)
	$(MKDIR) $(DESTDIR)$(BINDIR)
	$(INSTALL) -m 0755 $(OBJDIR)coder $(DESTDIR)$(BINDIR)/coder

uninstall:
	$(RM) -- $(DESTDIR)$(BINDIR)/coder

.PHONY: all clean distclean install uninstall
