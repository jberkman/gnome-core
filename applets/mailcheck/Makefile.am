INCLUDES = -I. -I$(srcdir) 				\
	-I$(srcdir)/../../panel				\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"	\
	-I$(includedir) $(GNOME_INCLUDEDIR)

BUILT_SOURCES = dummy.cc

dummy.cc:
	@touch dummy.cc

bin_PROGRAMS = mailcheck_applet

mailcheck_applet_SOURCES = mailcheck.c


mailcheck_applet_LDADD = ../../panel/libpanel_applet.la \
		     $(GNOME_LIBDIR) $(MICO_LIBS)      \
		     $(GNOMEUI_LIBS) $(INTLLIBS)

mailcheck_applet_LINK = $(CXXLINK)

BITMAPS = email.xpm email-e.xpm

EXTRA_DIST = mailcheck_applet.desktop $BITMAPS

sysdir = $(datadir)/applets/Network
sys_DATA = mailcheck_applet.desktop

pixmapdir = $(datadir)/pixmaps/mailcheck
pixmap_DATA = $(BITMAPS)

