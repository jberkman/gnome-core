## Process this file with automake to produce Makefile.in.

SUBDIRS = gnome-man2html gnome-info2html

INCLUDES = 							\
	-I$(includedir)						\
	-I$(top_srcdir)/intl -I$(top_builddir)/intl		\
	$(GNOME_INCLUDEDIR)					\
	-DGNOMEHELPPATH=\""$(datadir)/gnome/help\"" 	\
	-DGNOMELOCALEDIR=\""$(datadir)/locale\""	\
	-DVERSION=\""$(VERSION)"\"

LDADD = \
	$(GNOME_LIBDIR)					\
	$(GNOMEUI_LIBS)					\
	-lgnorba                                        \
	$(ORBIT_LIBS)                                   \
        $(INTLLIBS)


bin_PROGRAMS = gnome-help-browser  gnome-help-caller

gnome_help_caller_SOURCES = \
				$(ORBIT_HELP_CLIENT_SOURCES)   \
				help-caller.c

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
			     check-running.c check-running.h \
			     help-browser-impl.c \
			     $(ORBIT_HELP_BROWSER_SOURCES)


ORBIT_HELP_COMMON_SOURCES   = help-browser.h \
			      help-browser-common.c

ORBIT_HELP_BROWSER_SOURCES  = $(ORBIT_HELP_COMMON_SOURCES) \
			      help-browser-skels.c

ORBIT_HELP_CLIENT_SOURCES   = $(ORBIT_HELP_COMMON_SOURCES) \
			      help-browser-stubs.c

ORBIT_GENERATED_SOURCES      = $(ORBIT_HELP_COMMON_SOURCES) \
		               help-browser-skels.c help-browser-stubs.c

MAINTAINERCLEANFILES = $(ORBIT_GENERATED_SOURCES)
CLEANFILES = $(ORBIT_GENERATED_SOURCES)

$(ORBIT_GENERATED_SOURCES): $(srcdir)/idl/help-browser.idl 

BUILT_SOURCES = $(ORBIT_GENERATED_SOURCES)

$(ORBIT_GENERATED_SOURCES): $(srcdir)/idl/help-browser.idl $(ORBIT_IDL)
	$(ORBIT_IDL) $<


gnome_help_browser_LDADD = $(LDADD) $(RES_LIBS) $(GTKXMHTML_LIBS)

PIXMAPS = close.xpm contents.xpm help.xpm left_arrow.xpm right_arrow.xpm 

EXTRA_DIST = gnome-help-browser-help.html \
	     gnome-help-browser-topic.dat \
	     default-page.html bg.png gnome-logo2.png \
	     toc.c toc-man.c toc-ghelp.c toc-info.c \
	     toc.h toc-man.h toc-ghelp.h toc-info.h \
	     $(PIXMAPS) gnome-help.desktop

help_browser_helpdir = $(datadir)/gnome/help/help-browser/C
install-data-local:
	$(mkinstalldirs) $(help_browser_helpdir)
	$(mkinstalldirs) $(prefix)/etc/CORBA/servers
	$(INSTALL_DATA)  $(srcdir)/help-browser.gnorba \
			 $(prefix)/etc/CORBA/servers/help-browser.gnorba
	$(INSTALL_DATA)  $(srcdir)/gnome-help-browser-help.html \
			 $(help_browser_helpdir)/help-browser.html
	$(INSTALL_DATA)  $(srcdir)/gnome-help-browser-topic.dat \
			 $(help_browser_helpdir)/topic.dat
	$(INSTALL_DATA)  $(srcdir)/default-page.html $(help_browser_helpdir)/
	$(INSTALL_DATA)  $(srcdir)/bg.png $(help_browser_helpdir)/
	$(INSTALL_DATA)  $(srcdir)/gnome-logo2.png $(help_browser_helpdir)/
	cd $(help_browser_helpdir); \
	     rm -f index.html; ln -s help-browser.html index.html

sysdir = $(datadir)/apps
sys_DATA = gnome-help.desktop
