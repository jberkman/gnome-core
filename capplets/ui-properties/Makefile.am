INCLUDES = -I. -I$(srcdir) 				\
	-I$(top_srcdir)/intl -I$(top_builddir)/intl	\
	-I$(srcdir)/../../control-center		\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"	\
	-I$(includedir) $(GNOME_INCLUDEDIR)		\
	-DGNOMESYSCONFDIR=\""$(sysconfdir)"\"

bin_PROGRAMS = ui-properties

ui_properties_SOURCES = ui-properties.c

ui_properties_LDADD = ../../control-center/libcapplet.la \
		     $(GNOME_LIBDIR) $(ORB_LIBS) \
		     $(GNOMEUI_LIBS) $(INTLLIBS) -lgnorba


EXTRA_DIST = \
	ui-properties.desktop

sysdir = $(datadir)/control-center/Appearances
sys_DATA = ui-properties.desktop
