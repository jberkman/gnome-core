## Process this file with automake to produce Makefile.in.

SUBDIRS = gnome-man2html gnome-info2html

INCLUDES = 			\
	-I$(includedir)		\
	-I$(top_srcdir)/intl

LDADD = \
	$(GNOME_LIBDIR)					\
	$(GNOMEUI_LIBS)					\
	$(INTLLIBS)


bin_PROGRAMS = gnome-help-browser 

noinst_PROGRAMS = sample

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
			     visit.c visit.h magic.c magic.h 

gnome_help_browser_LDADD = $(LDADD) $(GTKXMHTML_LIBS)

CFLAGS += -Wall

EXTRA_DIST = sample-help.html gnome-help-browser-help.html \
	     sample-topic.dat gnome-help-browser-topic.dat

#
# shouldn't hard code locate here, how do I get it??
#
#sample_DATA = sample-help.html sample-topic.dat
#gnome_help_browser_DATA = gnome-help-browser-help.html \
#	    	         gnome-help-browser-topic.dat

sample_helpdir = $(datadir)/gnome/help/sample-help/C
help_browser_helpdir = $(datadir)/gnome/help/help-browser/C
install-data-local:
	$(mkinstalldirs) $(sample_helpdir)
	$(mkinstalldirs) $(help_browser_helpdir)
	$(INSTALL_DATA)  $(srcdir)/sample-help.html $(sample_helpdir)/
	$(INSTALL_DATA)  $(srcdir)/sample-topic.dat $(sample_helpdir)/topic.dat
	$(INSTALL_DATA)  $(srcdir)/gnome-help-browser-help.html \
			 $(help_browser_helpdir)/help-browser.html
	$(INSTALL_DATA)  $(srcdir)/gnome-help-browser-topic.dat \
			 $(help_browser_helpdir)/topic.dat

helpdir = $(datadir)/apps
help_DATA = gnome-help.desktop
