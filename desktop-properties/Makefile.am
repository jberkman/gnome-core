INCLUDES = 			\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	-I$(top_srcdir)/intl -I$(top_builddir)/intl	\
	-I$(includedir) $(X_CFLAGS) $(GNOME_INCLUDEDIR)

bin_PROGRAMS = ui-properties

ui_properties_SOURCES =	\
	main.c			\
	args.c			\
        property-ui.c		\
	gnome-desktop.h		\
	app-ui.c

ui_properties_LDADD = \
	$(GNOME_LIBDIR)					\
	$(GNOMEUI_LIBS)					\
	$(INTLLIBS)

EXTRA_DIST = ui.desktop

sysdir = $(datadir)/apps/System
sys_DATA = ui.desktop

