TARGETS	= $(OBJDIR)coder.1 $(OBJDIR)coder.html $(OBJDIR)sequel.1 $(OBJDIR)sequel.html $(OBJDIR)simulator.1 $(OBJDIR)simulator.html
OBJDIR	=
PREFIX	= /usr/local
DESTDIR	=
RM	= rm -f
LN	= ln -f
MKDIR	= mkdir -m 0755 -p
INSTALL	= install


all: $(TARGETS)

$(OBJDIR)coder.1: coder.xml
	./docbook.sh -P "$(PREFIX)" -- "$(OBJDIR)coder.1"

$(OBJDIR)coder.html: coder.css.xml coder.xml manual.css.xml
	./docbook.sh -P "$(PREFIX)" -- "$(OBJDIR)coder.html"

$(OBJDIR)sequel.1: sequel.xml
	./docbook.sh -P "$(PREFIX)" -- "$(OBJDIR)sequel.1"

$(OBJDIR)sequel.html: manual.css.xml sequel.css.xml sequel.xml
	./docbook.sh -P "$(PREFIX)" -- "$(OBJDIR)sequel.html"

$(OBJDIR)simulator.1: simulator.xml
	./docbook.sh -P "$(PREFIX)" -- "$(OBJDIR)simulator.1"

$(OBJDIR)simulator.html: manual.css.xml simulator.css.xml simulator.xml
	./docbook.sh -P "$(PREFIX)" -- "$(OBJDIR)simulator.html"

clean:
	$(RM) -- $(coder.1_OBJS) $(coder.html_OBJS) $(sequel.1_OBJS) $(sequel.html_OBJS) $(simulator.1_OBJS) $(simulator.html_OBJS)
	./docbook.sh -c -P "$(PREFIX)" -- "coder.1"
	./docbook.sh -c -P "$(PREFIX)" -- "coder.html"
	./docbook.sh -c -P "$(PREFIX)" -- "sequel.1"
	./docbook.sh -c -P "$(PREFIX)" -- "sequel.html"
	./docbook.sh -c -P "$(PREFIX)" -- "simulator.1"
	./docbook.sh -c -P "$(PREFIX)" -- "simulator.html"

distclean: clean
	$(RM) -- $(TARGETS)

install: $(TARGETS)
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -i -- "$(OBJDIR)coder.1"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -i -- "$(OBJDIR)coder.html"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -i -- "$(OBJDIR)sequel.1"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -i -- "$(OBJDIR)sequel.html"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -i -- "$(OBJDIR)simulator.1"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -i -- "$(OBJDIR)simulator.html"

uninstall:
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -u -- "coder.1"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -u -- "coder.html"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -u -- "sequel.1"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -u -- "sequel.html"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -u -- "simulator.1"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -u -- "simulator.html"

.PHONY: all clean distclean install uninstall
