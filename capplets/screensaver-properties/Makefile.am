INCLUDES = -I. -I$(srcdir) 				\
	-I$(top_srcdir)/intl -I$(top_builddir)/intl	\
	-I$(srcdir)/../../control-center		\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"	\
	-I$(includedir) $(GNOME_INCLUDEDIR)

bin_PROGRAMS = screensaver-properties-capplet

screensaver_properties_capplet_SOURCES = \
	screensaver-properties-capplet.c	\
	callbacks.c				\
	callbacks.h				\
	screensaver-dialog.c			\
	screensaver-dialog.h			\
	parser.c				\
	parser.h

screensaver_properties_capplet_LDADD = ../../control-center/libcapplet.la \
	$(GNOME_LIBDIR) $(ORB_LIBS) \
	$(GNOMEUI_LIBS) $(INTLLIBS) \
	-lImlib

EXTRA_DIST =
	#screensaver-properties.desktop 

SUBDIRS=screensaver-desktops

sysdir = $(datadir)/control-center/Workspace
sys_DATA = #screensaver-properties.desktop
