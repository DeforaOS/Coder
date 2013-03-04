TARGETS	= coder.1 coder.html sequel.1 sequel.html simulator.1 simulator.html
PREFIX	= /usr/local
DESTDIR	=
RM	= rm -f
LN	= ln -f
MKDIR	= mkdir -m 0755 -p
INSTALL	= install


all: $(TARGETS)

coder.1: coder.xml
	./docbook.sh -P "$(PREFIX)" -- "coder.1"

coder.html: coder.xml
	./docbook.sh -P "$(PREFIX)" -- "coder.html"

sequel.1: sequel.xml
	./docbook.sh -P "$(PREFIX)" -- "sequel.1"

sequel.html: sequel.xml
	./docbook.sh -P "$(PREFIX)" -- "sequel.html"

simulator.1: simulator.xml
	./docbook.sh -P "$(PREFIX)" -- "simulator.1"

simulator.html: simulator.xml
	./docbook.sh -P "$(PREFIX)" -- "simulator.html"

clean:
	$(RM) -- $(coder.1_OBJS) $(coder.html_OBJS) $(sequel.1_OBJS) $(sequel.html_OBJS) $(simulator.1_OBJS) $(simulator.html_OBJS)
	./docbook.sh -c -P "$(PREFIX)" -- "simulator.html"

distclean: clean
	$(RM) -- $(TARGETS)

install: $(TARGETS)
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -i -- "coder.1"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -i -- "coder.html"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -i -- "sequel.1"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -i -- "sequel.html"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -i -- "simulator.1"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -i -- "simulator.html"

uninstall:
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -u -- "coder.1"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -u -- "coder.html"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -u -- "sequel.1"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -u -- "sequel.html"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -u -- "simulator.1"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -u -- "simulator.html"

.PHONY: all clean distclean install uninstall
