PREFIX	= /usr/local
DESTDIR	=
MKDIR	= mkdir -m 0755 -p
INSTALL	= install
RM	= rm -f


all:

clean:

distclean: clean

install:
	$(MKDIR) $(DESTDIR)$(PREFIX)/share/Coder/Simulator/models
	$(INSTALL) -m 0644 -- nokia-n800.conf $(DESTDIR)$(PREFIX)/share/Coder/Simulator/models/nokia-n800.conf
	$(MKDIR) $(DESTDIR)$(PREFIX)/share/Coder/Simulator/models
	$(INSTALL) -m 0644 -- nokia-n9.conf $(DESTDIR)$(PREFIX)/share/Coder/Simulator/models/nokia-n9.conf
	$(MKDIR) $(DESTDIR)$(PREFIX)/share/Coder/Simulator/models
	$(INSTALL) -m 0644 -- nokia-n900.conf $(DESTDIR)$(PREFIX)/share/Coder/Simulator/models/nokia-n900.conf
	$(MKDIR) $(DESTDIR)$(PREFIX)/share/Coder/Simulator/models
	$(INSTALL) -m 0644 -- openmoko-gta01.conf $(DESTDIR)$(PREFIX)/share/Coder/Simulator/models/openmoko-gta01.conf
	$(MKDIR) $(DESTDIR)$(PREFIX)/share/Coder/Simulator/models
	$(INSTALL) -m 0644 -- openmoko-gta02.conf $(DESTDIR)$(PREFIX)/share/Coder/Simulator/models/openmoko-gta02.conf

uninstall:
	$(RM) -- $(DESTDIR)$(PREFIX)/share/Coder/Simulator/models/nokia-n800.conf
	$(RM) -- $(DESTDIR)$(PREFIX)/share/Coder/Simulator/models/nokia-n9.conf
	$(RM) -- $(DESTDIR)$(PREFIX)/share/Coder/Simulator/models/nokia-n900.conf
	$(RM) -- $(DESTDIR)$(PREFIX)/share/Coder/Simulator/models/openmoko-gta01.conf
	$(RM) -- $(DESTDIR)$(PREFIX)/share/Coder/Simulator/models/openmoko-gta02.conf

.PHONY: all clean distclean install uninstall
