INCLUDES = -I. -I$(srcdir) 				\
	-I$(top_srcdir)/intl -I$(top_builddir)/intl	\
	-I$(srcdir)/../../control-center		\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"	\
	-I$(includedir) $(GNOME_INCLUDEDIR)

bin_PROGRAMS = mouse-properties-capplet

mouse_properties_capplet_SOURCES = mouse-properties.c

mouse_properties_capplet_LDADD = ../../control-center/libcapplet.la \
		     $(GNOME_LIBDIR) $(ORB_LIBS) \
		     $(GNOMEUI_LIBS) $(INTLLIBS) -lgnorba
install-exec-hook:
	ln -s $(bindir)/mouse-properties-capplet $(bindir)/mouse-properties-init

EXTRA_DIST = \
	mouse-properties.desktop

sysdir = $(datadir)/control-center/Inputdevs
sys_DATA = mouse-properties.desktop
