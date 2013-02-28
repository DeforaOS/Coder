TARGETS	= simulator.1 simulator.html
PREFIX	= /usr/local
DESTDIR	=
RM	= rm -f
LN	= ln -f
MKDIR	= mkdir -m 0755 -p
INSTALL	= install


all: $(TARGETS)

simulator.1: simulator.xml
	./docbook.sh -P "$(PREFIX)" -- "simulator.1"

simulator.html: simulator.xml
	./docbook.sh -P "$(PREFIX)" -- "simulator.html"

clean:
	$(RM) -- $(simulator.1_OBJS) $(simulator.html_OBJS)
	./docbook.sh -c -P "$(PREFIX)" -- "simulator.html"

distclean: clean
	$(RM) -- $(TARGETS)

install: $(TARGETS)
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -i -- "simulator.1"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -i -- "simulator.html"

uninstall:
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -u -- "simulator.1"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -u -- "simulator.html"

.PHONY: all clean distclean install uninstall
