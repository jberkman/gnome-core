INCLUDES = 			\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	-I$(top_srcdir)/intl -I$(top_builddir)/intl	\
	-I$(includedir) $(GNOME_INCLUDEDIR)

bin_PROGRAMS = gnomecc

gnomecc_SOURCES = \
	main.c			\
	tree.c			\
	tree.h			\
	callbacks.c		\
	callbacks.h		\
	corba-glue.c		\
	corba-glue.h		\
	capplet-manager.c	\
	capplet-manager.h	\
	$(CORBA_SRCLIST)

BUILT_SOURCES = $(CORBA_SRCLIST)


CORBA_SRCLIST = control-center-skels.c control-center-stubs.c control-center-common.c control-center.h

gnomecc_LDADD = \
	$(GNOME_LIBDIR)					\
	$(GNOMEUI_LIBS)					\
	$(INTLLIBS)					\
	$(ORB_LIBS)

appicondir = $(datadir)/pixmaps
appicon_DATA = control-center.png
 
EXTRA_DIST = gnomecc.desktop cappletConf.sh.in $(appicon_DATA)

sysdir = $(datadir)/apps/System
sys_DATA = gnomecc.desktop

## Put `exec' in the name because this should be installed by
## `install-exec', not `install-data'.
confexecdir = $(libdir)
confexec_DATA = cappletConf.sh

libcapplet_la_SOURCES = \
	capplet-widget.c	\
	capplet-widget-libs.c	\
	capplet-widget-libs.h	\
	$(CORBA_SRCLIST)

libcapplet_la_LDFLAGS = -version-info 0:0:0 -rpath $(libdir)
libcapplet_la_LIBADD = $(ORB_LIBS) -lm $(DL_LIB)

include_HEADERS = capplet-widget.h

lib_LTLIBRARIES = libcapplet.la

test: $(test_SOURCES)
MAINTAINERCLEANFILES = $(CORBA_SRCLIST)
CLEANFILES = $(CORBA_SRCLIST)
$(CORBA_SRCLIST): $(top_srcdir)/idl/control-center.idl
	orbit-idl $(top_srcdir)/idl/control-center.idl

main.c: $(CORBA_SRCLIST)

capplet-widget.c: $(CORBA_SRCLIST)

#
# Create the cappletConf.sh file from the sh.in file
#
cappletConf.sh: cappletConf.sh.in Makefile
	sed -e 's?\@GNOME_LIBDIR\@?$(GNOME_LIBDIR)?g' \
	    -e 's?\@GNOMEUI_LIBS\@?$(GNOMEUI_LIBS)?g' \
	    -e 's?\@GNOME_INCLUDEDIR\@?$(GNOME_INCLUDEDIR)?g' \
	    -e 's?\@ORB_LIBS\@?$(ORB_LIBS)?g' \
	    < $(srcdir)/cappletConf.sh.in > cappletConf.tmp \
	  && mv cappletConf.tmp cappletConf.sh
