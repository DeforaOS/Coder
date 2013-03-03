SUBDIRS	= models
TARGETS	= gdeasm sequel simulator
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
	@for i in $(SUBDIRS); do (cd $$i && $(MAKE)) || exit; done

gdeasm_OBJS = gdeasm.o
gdeasm_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS) `pkg-config --cflags Asm`
gdeasm_LDFLAGS = $(LDFLAGSF) $(LDFLAGS) `pkg-config --libs Asm`

gdeasm: $(gdeasm_OBJS)
	$(CC) -o gdeasm $(gdeasm_OBJS) $(gdeasm_LDFLAGS)

sequel_OBJS = sequel.o sequel-main.o
sequel_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS) `pkg-config --cflags libDatabase`
sequel_LDFLAGS = $(LDFLAGSF) $(LDFLAGS) `pkg-config --libs libDatabase`

sequel: $(sequel_OBJS)
	$(CC) -o sequel $(sequel_OBJS) $(sequel_LDFLAGS)

simulator_OBJS = simulator.o simulator-main.o
simulator_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
simulator_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

simulator: $(simulator_OBJS)
	$(CC) -o simulator $(simulator_OBJS) $(simulator_LDFLAGS)

gdeasm.o: gdeasm.c ../config.h
	$(CC) $(gdeasm_CFLAGS) -c gdeasm.c

sequel.o: sequel.c sequel.h
	$(CC) $(sequel_CFLAGS) -c sequel.c

sequel-main.o: sequel-main.c sequel.h
	$(CC) $(sequel_CFLAGS) -c sequel-main.c

simulator.o: simulator.c simulator.h ../config.h
	$(CC) -D PREFIX=\"$(PREFIX)\" $(simulator_CFLAGS) -c simulator.c

simulator-main.o: simulator-main.c simulator.h
	$(CC) $(simulator_CFLAGS) -c simulator-main.c

clean:
	@for i in $(SUBDIRS); do (cd $$i && $(MAKE) clean) || exit; done
	$(RM) -- $(gdeasm_OBJS) $(sequel_OBJS) $(simulator_OBJS)

distclean:
	@for i in $(SUBDIRS); do (cd $$i && $(MAKE) distclean) || exit; done
	$(RM) -- $(gdeasm_OBJS) $(sequel_OBJS) $(simulator_OBJS)
	$(RM) -- $(TARGETS)

install: $(TARGETS)
	@for i in $(SUBDIRS); do (cd $$i && $(MAKE) install) || exit; done
	$(MKDIR) $(DESTDIR)$(BINDIR)
	$(INSTALL) -m 0755 -- gdeasm $(DESTDIR)$(BINDIR)/gdeasm
	$(MKDIR) $(DESTDIR)$(BINDIR)
	$(INSTALL) -m 0755 -- sequel $(DESTDIR)$(BINDIR)/sequel
	$(MKDIR) $(DESTDIR)$(BINDIR)
	$(INSTALL) -m 0755 -- simulator $(DESTDIR)$(BINDIR)/simulator

uninstall:
	@for i in $(SUBDIRS); do (cd $$i && $(MAKE) uninstall) || exit; done
	$(RM) -- $(DESTDIR)$(BINDIR)/gdeasm
	$(RM) -- $(DESTDIR)$(BINDIR)/sequel
	$(RM) -- $(DESTDIR)$(BINDIR)/simulator

.PHONY: all subdirs clean distclean install uninstall
