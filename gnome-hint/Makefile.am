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

bin_PROGRAMS = gnome-hint

# EXTRA_DIST = gnome-hint.desktop

gnome_hint_SOURCES = 		\
	gnome-hint.c


# utildir = $(datadir)/gnome/apps/Utilities
# util_DATA = gnome-hint.desktop
