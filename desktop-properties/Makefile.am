INCLUDES = 				\
	-I../.. 			\
	-I$(srcdir)/../.. 		\
	-I$(includedir)			\
	-I$(top_srcdir)/intl		\
	$(X_CFLAGS)

bin_PROGRAMS = desktop_properties

desktop_properties_SOURCES =	\
	main.c			\
	property-background.c	\
	property-screensaver.cc	\
	property-screensaver.h	\
	xlockmore.cc		\
	xlockmore.h		\
	xlockparse.ll		\
        property-keyboard.c	\
	property-mouse.c	\
	gnome-desktop.h		\
	screensaver.h		\
	ghash.c			\
	ghash.h			\
	gdkx.h

desktop_properties_LDADD = \
	$(top_builddir)/libgnome/libgnome.la		\
	$(top_builddir)/libgnomeui/libgnomeui.la 	\
	$(GTK_LIBS)					\
        $(X_LIBS)					\
        @XF86MISC_LIBS@					\
	@LEXLIB@					\
	@INTLLIBS@

EXTRA_DIST = desktop_properties.desktop

install-data-local:
	cp $(srcdir)/desktop_properties.desktop $(datadir)/apps/System/"Desktop Properties.desktop"
