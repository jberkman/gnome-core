INCLUDES = 			\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	-I$(includedir) $(X_CFLAGS) $(GNOME_INCLUDEDIR)

bin_PROGRAMS = mouse-properties keyboard-properties background-properties

mouse_properties_SOURCES =	\
	main.c			\
	property-mouse.c	\
	gnome-desktop.h		\
	app-mouse.c

keyboard_properties_SOURCES =	\
	main.c			\
        property-keyboard.c	\
	gnome-desktop.h		\
	app-keyboard.c

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

EXTRA_DIST = background.desktop keyboard.desktop mouse.desktop

sysdir = $(datadir)/apps/System
sys_DATA = background.desktop mouse.desktop keyboard.desktop
