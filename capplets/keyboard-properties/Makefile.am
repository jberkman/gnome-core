INCLUDES = -I. -I$(srcdir) 				\
	-I$(top_srcdir)/intl -I$(top_builddir)/intl	\
	-I$(srcdir)/../../control-center		\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"	\
	-I$(includedir) $(GNOME_INCLUDEDIR)

bin_PROGRAMS = keyboard-properties

keyboard_properties_SOURCES = keyboard-properties.c

keyboard_properties_LDADD = ../../control-center/libcapplet.la \
		     $(GNOME_LIBDIR) $(ORB_LIBS) \
		     $(GNOMEUI_LIBS) $(INTLLIBS) @XF86MISC_LIBS@

EXTRA_DIST = \
	keyboard-properties.desktop

sysdir = $(datadir)/control-center/Inputdevs
sys_DATA = keyboard-properties.desktop
