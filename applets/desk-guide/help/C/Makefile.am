## Process this file with automake to produce Makefile.in

figs = \
	deskguide-fig.png			\
	deskguide-properties-tasks-fig.png	\
	deskguide-properties-advanced-fig.png	\
	deskguide-properties.png		\
	deskguide-properties-geometry-fig.png	\
	deskguide-tasklist-fig.png

docdir = $(datadir)/gnome/help/desk-guide_applet/C
doc_DATA =	\
	index.html	\
	topic.dat	\
	$(figs)

# automake does not know anything about .sgml files yet -> EXTRA_DIST
EXTRA_DIST = desk-guide-applet.sgml desk-guide.sgml topic.dat $(figs)

# this is taken from gnome-core/gnome-terminal/C/Makefile.am
all: index.html

index.html: desk-guide-applet/index.html
	-cp desk-guide-applet/index.html .

desk-guide-applet/index.html: $(srcdir)/desk-guide.sgml $(srcdir)/desk-guide-applet.sgml
	-srcdir=`cd $(srcdir) && pwd`; \
	db2html $$srcdir/desk-guide-applet.sgml

dist-hook: index.html
	mkdir $(distdir)/desk-guide-applet
	-cp $(srcdir)/desk-guide-applet/*.html $(distdir)/desk-guide-applet
	-cp $(srcdir)/desk-guide-applet/*.css  $(distdir)/desk-guide-applet

install-data-local: index.html
	-$(mkinstalldirs) $(DESTDIR)$(docdir)/fig
	-for file in desk-guide-applet/*.html desk-guide-applet/*.css; do \
	  basefile=`echo $$file | sed -e 's,^.*/,,'`; \
	  $(INSTALL_DATA) $$file $(DESTDIR)$(docdir)/$$basefile; \
	done
	-for file in $(srcdir)/*.png; do \
	  basefile=`echo $$file | sed -e 's,^.*/,,'`; \
	  $(INSTALL_DATA) $$file $(DESTDIR)$(docdir)/fig/$$basefile; \
	done

desk-guide-applet.ps: desk-guide-applet.sgml desk-guide.sgml
	-db2ps $<

desk-guide-applet.rtf: desk-guide-applet.sgml desk-guide.sgml
	-db2rtf $<
