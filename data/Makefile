PREFIX	= /usr/local
DESTDIR	=
MKDIR	= mkdir -m 0755 -p
INSTALL	= install
RM	= rm -f


all:

clean:

distclean: clean

install:
	$(MKDIR) $(DESTDIR)$(PREFIX)/share/applications
	$(INSTALL) -m 0644 coder.desktop $(DESTDIR)$(PREFIX)/share/applications/coder.desktop
	$(MKDIR) $(DESTDIR)$(PREFIX)/share/applications
	$(INSTALL) -m 0644 sequel.desktop $(DESTDIR)$(PREFIX)/share/applications/sequel.desktop
	$(MKDIR) $(DESTDIR)$(PREFIX)/share/applications
	$(INSTALL) -m 0644 simulator.desktop $(DESTDIR)$(PREFIX)/share/applications/simulator.desktop

uninstall:
	$(RM) -- $(DESTDIR)$(PREFIX)/share/applications/coder.desktop
	$(RM) -- $(DESTDIR)$(PREFIX)/share/applications/sequel.desktop
	$(RM) -- $(DESTDIR)$(PREFIX)/share/applications/simulator.desktop

.PHONY: all clean distclean install uninstall
