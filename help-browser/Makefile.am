## Process this file with automake to produce Makefile.in.

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
			     gnome-help-browser.c

gnome_help_browser_LDADD = $(LDADD) $(GTKXMHTML_LIBS)

EXTRA_DIST = sample-hello.html

#
# shouldn't hard code locate here, how do I get it??
#
helpdir = $(datadir)/gnome/help/sample-help/C

help_DATA = sample-help.html topic.dat

