INCLUDES = 			\
	-I../.. 		\
	-I$(srcdir)/../.. 	\
	-I$(includedir)		\
	-I$(top_builddir)/intl  \
	-I$(top_srcdir)/intl	\
	$(X_CFLAGS)

bin_PROGRAMS = mouse-properties keyboard-properties background-properties

mouse_properties_SOURCES =	\
	main.c			\
	property-mouse.c	\
	gnome-desktop.h		\
	gdkx.h			\
	app-mouse.c

keyboard_properties_SOURCES =	\
	main.c			\
        property-keyboard.c	\
	gnome-desktop.h		\
	gdkx.h			\
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
	gdkx.h			\
	app-background.c

LDADD = \
	$(top_builddir)/libgnome/libgnome.la		\
	$(top_builddir)/libgnomeui/libgnomeui.la 	\
	$(GTK_LIBS)					\
        $(X_LIBS) $(x_libs)				\
	$(DL_LIB)					\
        @XF86MISC_LIBS@					\
	@LEXLIB@					\
	$(INTLLIBS)

EXTRA_DIST = background.desktop keyboard.desktop mouse.desktop

sysdir = $(datadir)/apps/System
install-data-local:
	$(INSTALL_DATA) $(srcdir)/background.desktop $(sysdir)/background.desktop
	$(INSTALL_DATA) $(srcdir)/mouse.desktop $(sysdir)/mouse.desktop
	$(INSTALL_DATA) $(srcdir)/keyboard.desktop $(sysdir)/keyboard.desktop
