deskguide_helpdir = $(datadir)/gnome/help/desk-guide_applet/C
deskguide_help_DATA = \
	index.html	\
	topic.dat	\
	deskguide-fig.png	\
	deskguide-properties-tasks-fig.png	\
	deskguide-properties-advanced-fig.png 	\
	deskguide-properties.png		\
	deskguide-properties-geometry-fig.png	\
	deskguide-tasklist-fig.png

EXTRA_DIST = desk-guide.sgml desk-guide-applet.sgml $(deskguide_help_DATA)

all: index.html

index.html: desk-guide/deskguide-applet.html
	-cp desk-guide/deskguide-applet.html index.html

# the wierd srcdir trick is because the db2html from the Cygnus RPMs
# cannot handle relative filenames
desk-guide/deskguide-applet.html: $(srcdir)/desk-guide.sgml
	-srcdir=`cd $(srcdir) && pwd`; \
	db2html $$srcdir/desk-guide.sgml

dist-hook: index.html
	mkdir $(distdir)/desk-guide
	-cp desk-guide/*.html $(distdir)/desk-guide
	-cp desk-guide/*.css $(distdir)/desk-guide

install-data-local: index.html
	-for file in $(srcdir)/desk-guide/*.html $(srcdir)/desk-guide/*.css; do \
	  basefile=`echo $$file | sed -e 's,^.*/,,'`; \
	  $(INSTALL_DATA) $$file $(DESTDIR)$(deskguide_helpdir)/$$basefile; \
	done

desk-guide.ps: desk-guide.sgml
	-db2ps $<

desk-guide.rtf: desk-guide.sgml
	-db2rtf $<

