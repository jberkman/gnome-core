INCLUDES = 			\
	-I$(includedir)		\
	$(GNOME_INCLUDEDIR)	\
	-DGNOMELOCALEDIR=\""$(datadir)/locale\""

LDADD = \
	$(GNOME_LIBDIR)					\
	$(GNOMEUI_LIBS)					\
	$(GNOME_XML_LIB)

bin_PROGRAMS = gnome-hint gnome-hint-properties-capplet

gnome_hint_SOURCES = 		\
	gnome-hint.c

gnome_hint_properties_capplet_SOURCES = gnome-hint-properties.c

gnome_hint_properties_capplet_LDADD = \
	$(GNOME_LIBDIR)		\
	$(GNOME_CAPPLET_LIBS)

utildir = $(datadir)/gnome/apps/Utilities
util_in_files = gnome-hint.desktop.in
util_DATA = $(util_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = \
	gnome-hint-properties.desktop.in \
	gnome-hint.png \
	gnome-hint.desktop.in

sysdir = $(datadir)/control-center/capplets
sys_in_files = gnome-hint-properties.desktop.in
sys_DATA = $(sys_in_files:.desktop.in=.desktop)

pixmapsdir = $(datadir)/pixmaps
pixmaps_DATA = gnome-hint.png