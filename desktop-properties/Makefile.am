INCLUDES = 			\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	-I$(top_srcdir)/intl -I$(top_builddir)/intl	\
	-I$(includedir) $(X_CFLAGS) $(GNOME_INCLUDEDIR)

bin_PROGRAMS = bell-properties ui-properties

bell_properties_SOURCES =	\
	main.c			\
	args.c			\
        property-bell.c		\
	gnome-desktop.h		\
	app-bell.c

ui_properties_SOURCES =	\
	main.c			\
	args.c			\
        property-ui.c		\
	gnome-desktop.h		\
	app-ui.c
bell_properties_LDADD = \
	$(GNOME_LIBDIR)					\
	$(GNOMEUI_LIBS)					\
        @XF86MISC_LIBS@					\
	$(INTLLIBS)

ui_properties_LDADD = \
	$(GNOME_LIBDIR)					\
	$(GNOMEUI_LIBS)					\
	$(INTLLIBS)

EXTRA_DIST = bell.desktop ui.desktop

sysdir = $(datadir)/apps/System
sys_DATA = bell.desktop ui.desktop

