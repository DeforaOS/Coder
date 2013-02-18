TARGETS	= gdeasm simulator
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


all: $(TARGETS)

gdeasm_OBJS = gdeasm.o
gdeasm_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS) `pkg-config --cflags Asm`
gdeasm_LDFLAGS = $(LDFLAGSF) $(LDFLAGS) `pkg-config --libs Asm`

gdeasm: $(gdeasm_OBJS)
	$(CC) -o gdeasm $(gdeasm_OBJS) $(gdeasm_LDFLAGS)

simulator_OBJS = simulator.o simulator-main.o
simulator_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
simulator_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

simulator: $(simulator_OBJS)
	$(CC) -o simulator $(simulator_OBJS) $(simulator_LDFLAGS)

gdeasm.o: gdeasm.c ../config.h
	$(CC) $(gdeasm_CFLAGS) -c gdeasm.c

simulator.o: simulator.c ../config.h
	$(CC) -D PREFIX=\"$(PREFIX)\" $(simulator_CFLAGS) -c simulator.c

simulator-main.o: simulator-main.c
	$(CC) $(simulator_CFLAGS) -c simulator-main.c

clean:
	$(RM) -- $(gdeasm_OBJS) $(simulator_OBJS)

distclean: clean
	$(RM) -- $(TARGETS)

install: $(TARGETS)
	$(MKDIR) $(DESTDIR)$(BINDIR)
	$(INSTALL) -m 0755 -- gdeasm $(DESTDIR)$(BINDIR)/gdeasm
	$(MKDIR) $(DESTDIR)$(BINDIR)
	$(INSTALL) -m 0755 -- simulator $(DESTDIR)$(BINDIR)/simulator

uninstall:
	$(RM) -- $(DESTDIR)$(BINDIR)/gdeasm
	$(RM) -- $(DESTDIR)$(BINDIR)/simulator

.PHONY: all clean distclean install uninstall
