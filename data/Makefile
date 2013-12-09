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
	$(INSTALL) -m 0644 deforaos-coder.desktop $(DESTDIR)$(PREFIX)/share/applications/deforaos-coder.desktop
	$(MKDIR) $(DESTDIR)$(PREFIX)/share/applications
	$(INSTALL) -m 0644 deforaos-gdeasm.desktop $(DESTDIR)$(PREFIX)/share/applications/deforaos-gdeasm.desktop
	$(MKDIR) $(DESTDIR)$(PREFIX)/share/applications
	$(INSTALL) -m 0644 deforaos-sequel.desktop $(DESTDIR)$(PREFIX)/share/applications/deforaos-sequel.desktop
	$(MKDIR) $(DESTDIR)$(PREFIX)/share/applications
	$(INSTALL) -m 0644 deforaos-simulator.desktop $(DESTDIR)$(PREFIX)/share/applications/deforaos-simulator.desktop

uninstall:
	$(RM) -- $(DESTDIR)$(PREFIX)/share/applications/deforaos-coder.desktop
	$(RM) -- $(DESTDIR)$(PREFIX)/share/applications/deforaos-gdeasm.desktop
	$(RM) -- $(DESTDIR)$(PREFIX)/share/applications/deforaos-sequel.desktop
	$(RM) -- $(DESTDIR)$(PREFIX)/share/applications/deforaos-simulator.desktop

.PHONY: all clean distclean install uninstall
