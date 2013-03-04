PACKAGE	= Coder
VERSION	= 0.0.0
SUBDIRS	= data doc po src tools
RM	= rm -f
LN	= ln -f
TAR	= tar -czvf


all: subdirs

subdirs:
	@for i in $(SUBDIRS); do (cd $$i && $(MAKE)) || exit; done

clean:
	@for i in $(SUBDIRS); do (cd $$i && $(MAKE) clean) || exit; done

distclean:
	@for i in $(SUBDIRS); do (cd $$i && $(MAKE) distclean) || exit; done

dist:
	$(RM) -r -- $(PACKAGE)-$(VERSION)
	$(LN) -s -- . $(PACKAGE)-$(VERSION)
	@$(TAR) $(PACKAGE)-$(VERSION).tar.gz -- \
		$(PACKAGE)-$(VERSION)/data/Makefile \
		$(PACKAGE)-$(VERSION)/data/coder.desktop \
		$(PACKAGE)-$(VERSION)/data/sequel.desktop \
		$(PACKAGE)-$(VERSION)/data/simulator.desktop \
		$(PACKAGE)-$(VERSION)/data/project.conf \
		$(PACKAGE)-$(VERSION)/doc/Makefile \
		$(PACKAGE)-$(VERSION)/doc/docbook.sh \
		$(PACKAGE)-$(VERSION)/doc/coder.xml \
		$(PACKAGE)-$(VERSION)/doc/sequel.xml \
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
		$(PACKAGE)-$(VERSION)/tools/sequel.c \
		$(PACKAGE)-$(VERSION)/tools/sequel-main.c \
		$(PACKAGE)-$(VERSION)/tools/simulator.c \
		$(PACKAGE)-$(VERSION)/tools/simulator-main.c \
		$(PACKAGE)-$(VERSION)/tools/Makefile \
		$(PACKAGE)-$(VERSION)/tools/sequel.h \
		$(PACKAGE)-$(VERSION)/tools/simulator.h \
		$(PACKAGE)-$(VERSION)/tools/project.conf \
		$(PACKAGE)-$(VERSION)/tools/models/Makefile \
		$(PACKAGE)-$(VERSION)/tools/models/nokia-n800.conf \
		$(PACKAGE)-$(VERSION)/tools/models/nokia-n9.conf \
		$(PACKAGE)-$(VERSION)/tools/models/nokia-n900.conf \
		$(PACKAGE)-$(VERSION)/tools/models/openmoko-gta01.conf \
		$(PACKAGE)-$(VERSION)/tools/models/openmoko-gta02.conf \
		$(PACKAGE)-$(VERSION)/tools/models/project.conf \
		$(PACKAGE)-$(VERSION)/Makefile \
		$(PACKAGE)-$(VERSION)/COPYING \
		$(PACKAGE)-$(VERSION)/config.h \
		$(PACKAGE)-$(VERSION)/config.sh \
		$(PACKAGE)-$(VERSION)/project.conf
	$(RM) -- $(PACKAGE)-$(VERSION)

install:
	@for i in $(SUBDIRS); do (cd $$i && $(MAKE) install) || exit; done

uninstall:
	@for i in $(SUBDIRS); do (cd $$i && $(MAKE) uninstall) || exit; done

.PHONY: all subdirs clean distclean dist install uninstall
