tasklist_helpdir = $(datadir)/gnome/help/tasklist_applet/C
tasklist_help_DATA = \
	index.html	\
	topic.dat	\
	tasklist-fig.png              tasklist-properties.png	\
	tasklist-properties-size.png  tasklist-rightclick-fig.png

EXTRA_DIST = tasklist-applet.sgml tasklist.sgml topic.dat index.html	\
	tasklist-fig.png              tasklist-properties.png	\
	tasklist-properties-size.png  tasklist-rightclick-fig.png

all: index.html

index.html: tasklist/tasklist-applet.html
	-cp tasklist/tasklist-applet.html index.html

# the wierd srcdir trick is because the db2html from the Cygnus RPMs
# cannot handle relative filenames
tasklist/tasklist-applet.html: $(srcdir)/tasklist.sgml
	-srcdir=`cd $(srcdir) && pwd`; \
	db2html $$srcdir/tasklist.sgml

dist-hook: index.html
	mkdir $(distdir)/tasklist
	-cp tasklist/*.html $(distdir)/tasklist
	-cp tasklist/*.css $(distdir)/tasklist

install-data-local: index.html
	-for file in $(srcdir)/tasklist/*.html $(srcdir)/tasklist/*.css; do \
	  basefile=`echo $$file | sed -e 's,^.*/,,'`; \
	  $(INSTALL_DATA) $$file $(DESTDIR)$(tasklist_helpdir)/$$basefile; \
	done

tasklist.ps: tasklist.sgml
	-db2ps $<

tasklist.rtf: tasklist.sgml
	-db2rtf $<

