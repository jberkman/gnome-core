INCLUDES = 			\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	-I$(top_srcdir)/intl -I$(top_builddir)/intl	\
	-I$(includedir) $(GNOME_INCLUDEDIR)

bin_PROGRAMS = gnomecc

gnomecc_SOURCES =	\
	main.c		\
	tree.c		\
	tree.h		\
	callbacks.c	\
	corba-glue.c	\
	caplet-manager.c\
	callbacks.h	\
	$(CORBA_SRCLIST)

BUILT_SOURCES = $(CORBA_SRCLIST)

CORBA_SRCLIST = control-center-skels.c control-center-stubs.c control-center-common.c control-center.h

gnomecc_LDADD = \
	$(GNOME_LIBDIR)					\
	$(GNOMEUI_LIBS)					\
	$(INTLLIBS)					\
	$(ORB_LIBS)

EXTRA_DIST = gnomecc.desktop control-center.idl

sysdir = $(datadir)/apps/System
sys_DATA = gnomecc.desktop

libcaplet_la_SOURCES=caplet-widget.c caplet-widget-libs.c
libcaplet_la_LDFLAGS = -version-info 0:0:0 -rpath $(libdir)
libcaplet_la_LIBADD = $(ORB_LIBS) -lm $(DL_LIB)


lib_LTLIBRARIES = libcaplet.la

test: $(test_SOURCES)
MAINTAINERCLEANFILES = $(CORBA_SRCLIST)
CLEANFILES = $(CORBA_SRCLIST)
$(CORBA_SRCLIST): control-center.idl
	orbit-idl $(srcdir)/control-center.idl
main.c: $(CORBA_SRCLIST)

