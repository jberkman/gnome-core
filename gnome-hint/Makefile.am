INCLUDES = 			\
	-I$(includedir)		\
	-I$(top_srcdir)/intl	\
	-I$(top_builddir)/intl	\
	$(GNOME_INCLUDEDIR)	\
	-DGNOMELOCALEDIR=\""$(datadir)/locale\""

LDADD = \
	$(GNOME_LIBDIR)					\
	$(GNOMEUI_LIBS)					\
	$(GNOME_XML_LIB)				\
	$(INTLLIBS)

bin_PROGRAMS = gnome-hint gnome-hint-properties-capplet

gnome_hint_SOURCES = 		\
	gnome-hint.c

gnome_hint_properties_capplet_SOURCES = gnome-hint-properties.c

gnome_hint_properties_capplet_LDADD = -lcapplet \
		     $(GNOME_LIBDIR) $(ORB_LIBS) \
		     $(GNOMEUI_LIBS) $(INTLLIBS) -lgnorba

# EXTRA_DIST = gnome-hint.desktop
# utildir = $(datadir)/gnome/apps/Utilities
# util_DATA = gnome-hint.desktop

EXTRA_DIST = \
	gnome-hint-properties.desktop

sysdir = $(datadir)/control-center/
sys_DATA = gnome-hint-properties.desktop

install-data-local:
	$(INSTALL_DATA) $(srcdir)/gnome-hint-properties.desktop $(DESTDIR)$(datadir)/gnome/apps/Settings/gnome-hint-properties.desktop

