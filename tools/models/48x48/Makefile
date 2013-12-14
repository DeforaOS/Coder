PREFIX	= /usr/local
DESTDIR	=
MKDIR	= mkdir -m 0755 -p
INSTALL	= install
RM	= rm -f


all:

clean:

distclean: clean

install:
	$(MKDIR) $(DESTDIR)$(PREFIX)/share/icons/hicolor/48x48/apps
	$(INSTALL) -m 0644 simulator-nokia-n9.png $(DESTDIR)$(PREFIX)/share/icons/hicolor/48x48/apps/simulator-nokia-n9.png
	$(MKDIR) $(DESTDIR)$(PREFIX)/share/icons/hicolor/48x48/apps
	$(INSTALL) -m 0644 simulator-nokia-n900.png $(DESTDIR)$(PREFIX)/share/icons/hicolor/48x48/apps/simulator-nokia-n900.png

uninstall:
	$(RM) -- $(DESTDIR)$(PREFIX)/share/icons/hicolor/48x48/apps/simulator-nokia-n9.png
	$(RM) -- $(DESTDIR)$(PREFIX)/share/icons/hicolor/48x48/apps/simulator-nokia-n900.png

.PHONY: all clean distclean install uninstall
