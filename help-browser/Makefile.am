## Process this file with automake to produce Makefile.in.

INCLUDES = 			\
	-I../.. 		\
	-I$(srcdir)/../.. 	\
	-I$(includedir)		\
	-I$(top_srcdir)/intl


LDADD = \
	@LEXLIB@					\
	$(GNOME_LIBDIR)					\
	$(GNOMEUI_LIBS)					\
        @XF86MISC_LIBS@

bin_PROGRAMS = gnome-help-browser 

noinst_PROGRAMS = sample

gnome_help_browser_SOURCES = gnome-helpwin.c gnome-helpwin.h \
			     gnome-help-browser.c

gnome_help_browser_LDADD = $(LDADD) $(GTKXMHTML_LIBS)

help_DATA = sample-help.html sample-topic.dat

EXTRA_DIST = sample-hello.html

#
# shouldn't hard code locate here, how do I get it??
#
helpdir = $(datadir)/gnome/help/sample-help/C

install-data-local:
	$(INSTALL_DATA)	$(srcdir)/sample-help.html $(helpdir)/
	$(INSTALL_DATA)	$(srcdir)/sample-topic.dat $(helpdir)/topic.dat
