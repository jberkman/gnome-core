INCLUDES = 			\
	-I$(top_srcdir)/intl -I$(top_builddir)/intl     \
	-I$(srcdir)/../../control-center                \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"        \
	-I$(includedir) $(GNOME_INCLUDEDIR)

bin_PROGRAMS =background-properties-capplet

background_properties_capplet_SOURCES =	\
	property-background.c	\
	gnome-desktop.h		\
	app-background.c	\
	imlib-misc.c		\
	imlib-misc.h		

background_properties_capplet_LDADD = ../../control-center/libcapplet.la \
		$(GNOME_LIBDIR) $(ORB_LIBS) -lImlib\
		$(GNOMEUI_LIBS) $(INTLLIBS) 

EXTRA_DIST = background-properties.desktop

sysdir = $(datadir)/apps/System
sys_DATA = background-properties.desktop 

