SUBDIRS	= 16x16 24x24 32x32 48x48
OBJDIR	=
PREFIX	= /usr/local
DESTDIR	=
MKDIR	= mkdir -m 0755 -p
INSTALL	= install
RM	= rm -f


all: subdirs

subdirs:
	@for i in $(SUBDIRS); do (cd "$$i" && $(MAKE)) || exit; done

clean:
	@for i in $(SUBDIRS); do (cd "$$i" && $(MAKE) clean) || exit; done

distclean:
	@for i in $(SUBDIRS); do (cd "$$i" && $(MAKE) distclean) || exit; done

install:
	@for i in $(SUBDIRS); do (cd "$$i" && $(MAKE) install) || exit; done
	$(MKDIR) $(DESTDIR)$(PREFIX)/share/Coder/Simulator/models
	$(INSTALL) -m 0644 htc-touchpro.conf $(DESTDIR)$(PREFIX)/share/Coder/Simulator/models/htc-touchpro.conf
	$(MKDIR) $(DESTDIR)$(PREFIX)/share/Coder/Simulator/models
	$(INSTALL) -m 0644 nokia-n800.conf $(DESTDIR)$(PREFIX)/share/Coder/Simulator/models/nokia-n800.conf
	$(MKDIR) $(DESTDIR)$(PREFIX)/share/Coder/Simulator/models
	$(INSTALL) -m 0644 nokia-n9.conf $(DESTDIR)$(PREFIX)/share/Coder/Simulator/models/nokia-n9.conf
	$(MKDIR) $(DESTDIR)$(PREFIX)/share/Coder/Simulator/models
	$(INSTALL) -m 0644 nokia-n900.conf $(DESTDIR)$(PREFIX)/share/Coder/Simulator/models/nokia-n900.conf
	$(MKDIR) $(DESTDIR)$(PREFIX)/share/Coder/Simulator/models
	$(INSTALL) -m 0644 openmoko-gta01.conf $(DESTDIR)$(PREFIX)/share/Coder/Simulator/models/openmoko-gta01.conf
	$(MKDIR) $(DESTDIR)$(PREFIX)/share/Coder/Simulator/models
	$(INSTALL) -m 0644 openmoko-gta02.conf $(DESTDIR)$(PREFIX)/share/Coder/Simulator/models/openmoko-gta02.conf

uninstall:
	@for i in $(SUBDIRS); do (cd "$$i" && $(MAKE) uninstall) || exit; done
	$(RM) -- $(DESTDIR)$(PREFIX)/share/Coder/Simulator/models/htc-touchpro.conf
	$(RM) -- $(DESTDIR)$(PREFIX)/share/Coder/Simulator/models/nokia-n800.conf
	$(RM) -- $(DESTDIR)$(PREFIX)/share/Coder/Simulator/models/nokia-n9.conf
	$(RM) -- $(DESTDIR)$(PREFIX)/share/Coder/Simulator/models/nokia-n900.conf
	$(RM) -- $(DESTDIR)$(PREFIX)/share/Coder/Simulator/models/openmoko-gta01.conf
	$(RM) -- $(DESTDIR)$(PREFIX)/share/Coder/Simulator/models/openmoko-gta02.conf

.PHONY: all subdirs clean distclean install uninstall
