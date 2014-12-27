PACKAGE	= Coder
VERSION	= 0.0.0
SUBDIRS	= data doc po src tools
RM	= rm -f
LN	= ln -f
TAR	= tar
MKDIR	= mkdir -m 0755 -p


all: subdirs

subdirs:
	@for i in $(SUBDIRS); do (cd "$$i" && $(MAKE)) || exit; done

clean:
	@for i in $(SUBDIRS); do (cd "$$i" && $(MAKE) clean) || exit; done

distclean:
	@for i in $(SUBDIRS); do (cd "$$i" && $(MAKE) distclean) || exit; done

dist:
	$(RM) -r -- $(OBJDIR)$(PACKAGE)-$(VERSION)
	$(LN) -s -- "$$PWD" $(OBJDIR)$(PACKAGE)-$(VERSION)
	@cd $(OBJDIR). && $(TAR) -czvf $(OBJDIR)$(PACKAGE)-$(VERSION).tar.gz -- \
		$(PACKAGE)-$(VERSION)/data/Makefile \
		$(PACKAGE)-$(VERSION)/data/deforaos-coder.desktop \
		$(PACKAGE)-$(VERSION)/data/deforaos-gdeasm.desktop \
		$(PACKAGE)-$(VERSION)/data/deforaos-sequel.desktop \
		$(PACKAGE)-$(VERSION)/data/deforaos-simulator.desktop \
		$(PACKAGE)-$(VERSION)/data/project.conf \
		$(PACKAGE)-$(VERSION)/doc/Makefile \
		$(PACKAGE)-$(VERSION)/doc/docbook.sh \
		$(PACKAGE)-$(VERSION)/doc/coder.css.xml \
		$(PACKAGE)-$(VERSION)/doc/coder.xml \
		$(PACKAGE)-$(VERSION)/doc/manual.css.xml \
		$(PACKAGE)-$(VERSION)/doc/sequel.css.xml \
		$(PACKAGE)-$(VERSION)/doc/sequel.xml \
		$(PACKAGE)-$(VERSION)/doc/simulator.css.xml \
		$(PACKAGE)-$(VERSION)/doc/simulator.xml \
		$(PACKAGE)-$(VERSION)/doc/project.conf \
		$(PACKAGE)-$(VERSION)/po/Makefile \
		$(PACKAGE)-$(VERSION)/po/gettext.sh \
		$(PACKAGE)-$(VERSION)/po/POTFILES \
		$(PACKAGE)-$(VERSION)/po/fr.po \
		$(PACKAGE)-$(VERSION)/po/project.conf \
		$(PACKAGE)-$(VERSION)/src/callbacks.c \
		$(PACKAGE)-$(VERSION)/src/coder.c \
		$(PACKAGE)-$(VERSION)/src/main.c \
		$(PACKAGE)-$(VERSION)/src/project.c \
		$(PACKAGE)-$(VERSION)/src/Makefile \
		$(PACKAGE)-$(VERSION)/src/callbacks.h \
		$(PACKAGE)-$(VERSION)/src/coder.h \
		$(PACKAGE)-$(VERSION)/src/project.h \
		$(PACKAGE)-$(VERSION)/src/project.conf \
		$(PACKAGE)-$(VERSION)/tools/gdeasm.c \
		$(PACKAGE)-$(VERSION)/tools/gdeasm-main.c \
		$(PACKAGE)-$(VERSION)/tools/sequel.c \
		$(PACKAGE)-$(VERSION)/tools/sequel-main.c \
		$(PACKAGE)-$(VERSION)/tools/simulator.c \
		$(PACKAGE)-$(VERSION)/tools/simulator-main.c \
		$(PACKAGE)-$(VERSION)/tools/Makefile \
		$(PACKAGE)-$(VERSION)/tools/gdeasm.h \
		$(PACKAGE)-$(VERSION)/tools/sequel.h \
		$(PACKAGE)-$(VERSION)/tools/simulator.h \
		$(PACKAGE)-$(VERSION)/tools/project.conf \
		$(PACKAGE)-$(VERSION)/tools/models/Makefile \
		$(PACKAGE)-$(VERSION)/tools/models/htc-touchpro.conf \
		$(PACKAGE)-$(VERSION)/tools/models/nokia-n800.conf \
		$(PACKAGE)-$(VERSION)/tools/models/nokia-n9.conf \
		$(PACKAGE)-$(VERSION)/tools/models/nokia-n900.conf \
		$(PACKAGE)-$(VERSION)/tools/models/openmoko-gta01.conf \
		$(PACKAGE)-$(VERSION)/tools/models/openmoko-gta02.conf \
		$(PACKAGE)-$(VERSION)/tools/models/project.conf \
		$(PACKAGE)-$(VERSION)/tools/models/16x16/Makefile \
		$(PACKAGE)-$(VERSION)/tools/models/16x16/simulator-nokia-n9.png \
		$(PACKAGE)-$(VERSION)/tools/models/16x16/simulator-nokia-n900.png \
		$(PACKAGE)-$(VERSION)/tools/models/16x16/simulator-openmoko-freerunner.png \
		$(PACKAGE)-$(VERSION)/tools/models/16x16/project.conf \
		$(PACKAGE)-$(VERSION)/tools/models/24x24/Makefile \
		$(PACKAGE)-$(VERSION)/tools/models/24x24/simulator-nokia-n9.png \
		$(PACKAGE)-$(VERSION)/tools/models/24x24/simulator-nokia-n900.png \
		$(PACKAGE)-$(VERSION)/tools/models/24x24/simulator-openmoko-freerunner.png \
		$(PACKAGE)-$(VERSION)/tools/models/24x24/project.conf \
		$(PACKAGE)-$(VERSION)/tools/models/32x32/Makefile \
		$(PACKAGE)-$(VERSION)/tools/models/32x32/simulator-nokia-n9.png \
		$(PACKAGE)-$(VERSION)/tools/models/32x32/simulator-nokia-n900.png \
		$(PACKAGE)-$(VERSION)/tools/models/32x32/simulator-openmoko-freerunner.png \
		$(PACKAGE)-$(VERSION)/tools/models/32x32/project.conf \
		$(PACKAGE)-$(VERSION)/tools/models/48x48/Makefile \
		$(PACKAGE)-$(VERSION)/tools/models/48x48/simulator-nokia-n9.png \
		$(PACKAGE)-$(VERSION)/tools/models/48x48/simulator-nokia-n900.png \
		$(PACKAGE)-$(VERSION)/tools/models/48x48/simulator-openmoko-freerunner.png \
		$(PACKAGE)-$(VERSION)/tools/models/48x48/project.conf \
		$(PACKAGE)-$(VERSION)/Makefile \
		$(PACKAGE)-$(VERSION)/COPYING \
		$(PACKAGE)-$(VERSION)/config.h \
		$(PACKAGE)-$(VERSION)/config.sh \
		$(PACKAGE)-$(VERSION)/project.conf
	$(RM) -- $(OBJDIR)$(PACKAGE)-$(VERSION)

distcheck: dist
	$(TAR) -xzvf $(OBJDIR)$(PACKAGE)-$(VERSION).tar.gz
	$(MKDIR) -- $(PACKAGE)-$(VERSION)/objdir
	$(MKDIR) -- $(PACKAGE)-$(VERSION)/destdir
	(cd "$(PACKAGE)-$(VERSION)" && $(MAKE) OBJDIR="$$PWD/objdir/")
	(cd "$(PACKAGE)-$(VERSION)" && $(MAKE) OBJDIR="$$PWD/objdir/" DESTDIR="$$PWD/destdir" install)
	(cd "$(PACKAGE)-$(VERSION)" && $(MAKE) OBJDIR="$$PWD/objdir/" DESTDIR="$$PWD/destdir" uninstall)
	(cd "$(PACKAGE)-$(VERSION)" && $(MAKE) OBJDIR="$$PWD/objdir/" distclean)
	(cd "$(PACKAGE)-$(VERSION)" && $(MAKE) dist)
	$(RM) -r -- $(PACKAGE)-$(VERSION)

install:
	@for i in $(SUBDIRS); do (cd "$$i" && $(MAKE) install) || exit; done

uninstall:
	@for i in $(SUBDIRS); do (cd "$$i" && $(MAKE) uninstall) || exit; done

.PHONY: all subdirs clean distclean dist distcheck install uninstall
