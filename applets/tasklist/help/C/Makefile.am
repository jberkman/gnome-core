## Process this file with automake to produce Makefile.in

figs = \
	tasklist-fig.png              tasklist-properties.png	\
	tasklist-properties-size.png  tasklist-rightclick-fig.png


docdir = $(datadir)/gnome/help/tasklist_applet/C
doc_DATA =	\
	index.html	\
	topic.dat	\
	$(figs)

# automake does not know anything about .sgml files yet -> EXTRA_DIST
EXTRA_DIST = tasklist-applet.sgml tasklist.sgml topic.dat $(figs)

# this is taken from gnome-core/gnome-terminal/C/Makefile.am
all: index.html

index.html: tasklist-applet/index.html
	-cp tasklist-applet/index.html .

tasklist-applet/index.html: $(srcdir)/tasklist.sgml $(srcdir)/tasklist-applet.sgml
	-srcdir=`cd $(srcdir) && pwd`; \
	db2html $$srcdir/tasklist-applet.sgml

dist-hook: index.html
	mkdir $(distdir)/tasklist-applet
	-cp $(srcdir)/tasklist-applet/*.html $(distdir)/tasklist-applet
	-cp $(srcdir)/tasklist-applet/*.css  $(distdir)/tasklist-applet

install-data-local: index.html
	-$(mkinstalldirs) $(DESTDIR)$(docdir)/fig
	-for file in tasklist-applet/*.html tasklist-applet/*.css; do \
	  basefile=`echo $$file | sed -e 's,^.*/,,'`; \
	  $(INSTALL_DATA) $$file $(DESTDIR)$(docdir)/$$basefile; \
	done
	-for file in $(srcdir)/*.png; do \
	  basefile=`echo $$file | sed -e 's,^.*/,,'`; \
	  $(INSTALL_DATA) $$file $(DESTDIR)$(docdir)/fig/$$basefile; \
	done

tasklist-applet.ps: tasklist-applet.sgml tasklist.sgml
	-db2ps $<

tasklist-applet.rtf: tasklist-applet.sgml tasklist.sgml
	-db2rtf $<
