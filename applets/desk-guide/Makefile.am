SUBDIRS = help

INCLUDES = -I. -I$(srcdir) 				\
	-I$(srcdir)/../../panel				\
	-I$(top_builddir)/panel \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"	\
	-I$(includedir) $(GNOME_INCLUDEDIR)		\
	-DVERSION=\""$(VERSION)"\"			\
	@PIXBUF_CFLAGS@

bin_PROGRAMS = deskguide_applet

deskguide_applet_SOURCES = gstc.c gwmh.c gwmthumbnail.c gwmdesktop.c gwmtaskview.c deskguide_applet.c

deskguide_applet_LDADD = ../../panel/libpanel_applet.la \
		     $(GNOME_LIBDIR) $(ORB_LIBS)        \
		     $(GNOMEUI_LIBS) $(GNORBA_LIBS)     \
		     @PIXBUF_LIBS@

EXTRA_DIST = \
	gstc.h \
	gwmh.h \
	gwmdesktop.h \
	gwmtaskview.h \
	gwmthumbnail.h \
	deskguide_applet.h \
	deskguide_applet.gnorba \
	deskguide_applet.desktop.in \
	gnome-deskguide-splash.png \
	gnome-deskguide.png

sysdir = $(datadir)/applets/Utility
sys_in_files = deskguide_applet.desktop.in
sys_DATA = $(sys_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

pixmapdir = $(datadir)/pixmaps
pixmap_DATA = gnome-deskguide-splash.png gnome-deskguide.png

gnorbadir  = $(sysconfdir)/CORBA/servers
gnorba_DATA = deskguide_applet.gnorba 
