## Process this file with automake to produce Makefile.in.

SUBDIRS = gnome-man2html gnome-info2html

INCLUDES = 							\
	-I$(includedir)						\
	-I$(top_srcdir)/intl					\
	$(GNOME_INCLUDEDIR)					\
	-DGNOMEHELPPATH=\""$(datadir)/gnome/help\"" 	\
	-DGNOMELOCALEDIR=\""$(datadir)/locale\""

LDADD = \
	$(GNOME_LIBDIR)					\
	$(GNOMEUI_LIBS)					\
	$(INTLLIBS)


bin_PROGRAMS = gnome-help-browser 

gnome_help_browser_SOURCES = gnome-helpwin.c gnome-helpwin.h \
			     gnome-help-browser.c \
			     toc2.c toc2-man.c toc2-ghelp.c toc2-info.c \
			     toc2.h toc2-man.h toc2-ghelp.h toc2-info.h \
	  		     docobj.c docobj.h mime.c mime.h \
			     transport.c transport.h \
			     HTParse.c HTParse.h cache.c cache.h \
			     parseUrl.c parseUrl.h \
			     queue.c queue.h history.c history.h \
                             misc.c misc.h window.c window.h \
			     visit.c visit.h magic.c magic.h \
			     bookmarks.c bookmarks.h \
			     check-running.c check-running.h

gnome_help_browser_LDADD = $(LDADD) $(RES_LIBS) $(GTKXMHTML_LIBS)

CFLAGS += -Wall

PIXMAPS = close.xpm contents.xpm help.xpm left_arrow.xpm right_arrow.xpm 

EXTRA_DIST = gnome-help-browser-help.html \
	     gnome-help-browser-topic.dat \
	     default-page.html bg.xpm gnome-logo2.xpm \
	     $(PIXMAPS) gnome-help.desktop

help_browser_helpdir = $(datadir)/gnome/help/help-browser/C
install-data-local:
	$(mkinstalldirs) $(help_browser_helpdir)
	$(INSTALL_DATA)  $(srcdir)/gnome-help-browser-help.html \
			 $(help_browser_helpdir)/help-browser.html
	$(INSTALL_DATA)  $(srcdir)/gnome-help-browser-topic.dat \
			 $(help_browser_helpdir)/topic.dat
	$(INSTALL_DATA)  $(srcdir)/default-page.html $(help_browser_helpdir)/
	$(INSTALL_DATA)  $(srcdir)/bg.xpm $(help_browser_helpdir)/
	$(INSTALL_DATA)  $(srcdir)/gnome-logo2.xpm $(help_browser_helpdir)/
	cd $(help_browser_helpdir); \
	     rm -f index.html; ln -s help-browser.html index.html

sysdir = $(datadir)/apps
sys_DATA = gnome-help.desktop
