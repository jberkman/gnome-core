INCLUDES = -I. -I$(srcdir) 				\
	-I$(top_srcdir)/intl -I$(top_builddir)/intl	\
	-I$(srcdir)/../../panel				\
	-I$(srcdir)/../desk-guide			\
	-I$(top_builddir)/panel \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"	\
	-I$(includedir) $(GNOME_INCLUDEDIR)		\
	-DVERSION=\""$(VERSION)"\"

bin_PROGRAMS = tasklist_applet

tasklist_applet_SOURCES = tasklist_applet.c \
			  tasklist_config.c \
			  tasklist_menu.c \
			  tasklist_properties.c

tasklist_applet_LDADD = ../../panel/libpanel_applet.la \
		     $(GNOME_LIBDIR) $(ORB_LIBS)         \
		     $(GNOMEUI_LIBS) $(GNORBA_LIBS)      \
		     $(INTLLIBS)

EXTRA_DIST = \
	pixmaps.h \
	tasklist_applet.h \
	tasklist_applet.gnorba \
	tasklist_applet.desktop \
	unknown.xpm

pixmapdir = $(datadir)/pixmaps
pixmap_DATA = gnome-tasklist.png

sysdir = $(datadir)/applets/Utility
sys_DATA = tasklist_applet.desktop 

gnorbadir  = $(sysconfdir)/CORBA/servers
gnorba_DATA = tasklist_applet.gnorba 

