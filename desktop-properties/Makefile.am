INCLUDES = 			\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	-I$(includedir) $(X_CFLAGS)

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
	property-screensaver.cc	\
	property-screensaver.h	\
	xlockmore.cc		\
	xlockmore.h		\
	xlockparse.ll		\
	gnome-desktop.h		\
	screensaver.h		\
	ghash.c			\
	ghash.h			\
	app-background.c

LDADD = \
	$(GNOME_LIBDIR)					\
	$(GNOMEUI_LIBS)					\
        @XF86MISC_LIBS@					\
	$(INTLLIBS)

EXTRA_DIST = background.desktop keyboard.desktop mouse.desktop

sysdir = $(datadir)/apps/System
sys_DATA = background.desktop mouse.desktop keyboard.desktop
