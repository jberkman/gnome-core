INCLUDES = 			\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	-I$(top_srcdir)/intl -I$(top_builddir)/intl	\
	-I$(includedir) $(X_CFLAGS) $(GNOME_INCLUDEDIR)

bin_PROGRAMS = mouse-properties keyboard-properties background-properties \
	bell-properties ui-properties

mouse_properties_SOURCES =	\
	main.c			\
	args.c			\
	property-mouse.c	\
	gnome-desktop.h		\
	app-mouse.c

keyboard_properties_SOURCES =	\
	main.c			\
	args.c			\
        property-keyboard.c	\
	gnome-desktop.h		\
	app-keyboard.c

bell_properties_SOURCES =	\
	main.c			\
	args.c			\
        property-bell.c		\
	gnome-desktop.h		\
	app-bell.c

background_properties_SOURCES =	\
	main.c			\
	property-background.c	\
	gnome-desktop.h		\
	screensaver.h		\
	app-background.c	\
	imlib-misc.c		\
	imlib-misc.h		\
	property-screensaver.cc	\
	property-screensaver.h	\
	xlockmore.cc		\
	xlockmore.h		\
	xlockparse.ll

ui_properties_SOURCES =	\
	main.c			\
	args.c			\
        property-ui.c		\
	gnome-desktop.h		\
	app-ui.c

background_properties_LDADD = \
	$(GNOME_LIBDIR)					\
	-lImlib						\
	$(GNOMEUI_LIBS)					\
        @XF86MISC_LIBS@					\
	$(INTLLIBS)

mouse_properties_LDADD = \
	$(GNOME_LIBDIR)					\
	$(GNOMEUI_LIBS)					\
        @XF86MISC_LIBS@					\
	$(INTLLIBS)

keyboard_properties_LDADD = \
	$(GNOME_LIBDIR)					\
	$(GNOMEUI_LIBS)					\
        @XF86MISC_LIBS@					\
	$(INTLLIBS)

bell_properties_LDADD = \
	$(GNOME_LIBDIR)					\
	$(GNOMEUI_LIBS)					\
        @XF86MISC_LIBS@					\
	$(INTLLIBS)

ui_properties_LDADD = \
	$(GNOME_LIBDIR)					\
	$(GNOMEUI_LIBS)					\
	$(INTLLIBS)

EXTRA_DIST = background.desktop keyboard.desktop mouse.desktop bell.desktop \
	ui.desktop

sysdir = $(datadir)/apps/System
sys_DATA = background.desktop mouse.desktop keyboard.desktop bell.desktop \
	ui.desktop

