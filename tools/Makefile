SUBDIRS	= models
TARGETS	= $(OBJDIR)gdeasm $(OBJDIR)sequel $(OBJDIR)simulator
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
LDFLAGSF= `pkg-config --libs libDesktop`
RM	= rm -f
LN	= ln -f
MKDIR	= mkdir -m 0755 -p
INSTALL	= install


all: subdirs $(TARGETS)

subdirs:
	@for i in $(SUBDIRS); do (cd "$$i" && $(MAKE)) || exit; done

gdeasm_OBJS = $(OBJDIR)gdeasm.o $(OBJDIR)gdeasm-main.o
gdeasm_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS) `pkg-config --cflags Asm`
gdeasm_LDFLAGS = $(LDFLAGSF) $(LDFLAGS) `pkg-config --libs Asm`

$(OBJDIR)gdeasm: $(gdeasm_OBJS)
	$(CC) -o $(OBJDIR)gdeasm $(gdeasm_OBJS) $(gdeasm_LDFLAGS)

sequel_OBJS = $(OBJDIR)sequel.o $(OBJDIR)sequel-main.o
sequel_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS) `pkg-config --cflags libDatabase`
sequel_LDFLAGS = $(LDFLAGSF) $(LDFLAGS) `pkg-config --libs libDatabase`

$(OBJDIR)sequel: $(sequel_OBJS)
	$(CC) -o $(OBJDIR)sequel $(sequel_OBJS) $(sequel_LDFLAGS)

simulator_OBJS = $(OBJDIR)simulator.o $(OBJDIR)simulator-main.o
simulator_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
simulator_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

$(OBJDIR)simulator: $(simulator_OBJS)
	$(CC) -o $(OBJDIR)simulator $(simulator_OBJS) $(simulator_LDFLAGS)

$(OBJDIR)gdeasm.o: gdeasm.c ../config.h
	$(CC) $(gdeasm_CFLAGS) -o $(OBJDIR)gdeasm.o -c gdeasm.c

$(OBJDIR)gdeasm-main.o: gdeasm-main.c
	$(CC) $(gdeasm_CFLAGS) -o $(OBJDIR)gdeasm-main.o -c gdeasm-main.c

$(OBJDIR)sequel.o: sequel.c sequel.h
	$(CC) $(sequel_CFLAGS) -o $(OBJDIR)sequel.o -c sequel.c

$(OBJDIR)sequel-main.o: sequel-main.c sequel.h
	$(CC) $(sequel_CFLAGS) -o $(OBJDIR)sequel-main.o -c sequel-main.c

$(OBJDIR)simulator.o: simulator.c simulator.h ../config.h
	$(CC) -D PREFIX=\"$(PREFIX)\" $(simulator_CFLAGS) -o $(OBJDIR)simulator.o -c simulator.c

$(OBJDIR)simulator-main.o: simulator-main.c simulator.h
	$(CC) $(simulator_CFLAGS) -o $(OBJDIR)simulator-main.o -c simulator-main.c

clean:
	@for i in $(SUBDIRS); do (cd "$$i" && $(MAKE) clean) || exit; done
	$(RM) -- $(gdeasm_OBJS) $(sequel_OBJS) $(simulator_OBJS)

distclean:
	@for i in $(SUBDIRS); do (cd "$$i" && $(MAKE) distclean) || exit; done
	$(RM) -- $(gdeasm_OBJS) $(sequel_OBJS) $(simulator_OBJS)
	$(RM) -- $(TARGETS)

install: $(TARGETS)
	@for i in $(SUBDIRS); do (cd "$$i" && $(MAKE) install) || exit; done
	$(MKDIR) $(DESTDIR)$(BINDIR)
	$(INSTALL) -m 0755 $(OBJDIR)gdeasm $(DESTDIR)$(BINDIR)/gdeasm
	$(MKDIR) $(DESTDIR)$(BINDIR)
	$(INSTALL) -m 0755 $(OBJDIR)sequel $(DESTDIR)$(BINDIR)/sequel
	$(MKDIR) $(DESTDIR)$(BINDIR)
	$(INSTALL) -m 0755 $(OBJDIR)simulator $(DESTDIR)$(BINDIR)/simulator

uninstall:
	@for i in $(SUBDIRS); do (cd "$$i" && $(MAKE) uninstall) || exit; done
	$(RM) -- $(DESTDIR)$(BINDIR)/gdeasm
	$(RM) -- $(DESTDIR)$(BINDIR)/sequel
	$(RM) -- $(DESTDIR)$(BINDIR)/simulator

.PHONY: all subdirs clean distclean install uninstall
