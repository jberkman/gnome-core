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
	callbacks.h	\
	control-center-widget.c \
	control-center-widget.h \
	$(CORBA_SRCLIST)


CORBA_SRCLIST = control-center-skels.c control-center-common.c control-center.h

gnomecc_LDADD = \
	$(GNOME_LIBDIR)					\
	$(GNOMEUI_LIBS)					\
	$(INTLLIBS)					\
	$(ORB_LIBS)

EXTRA_DIST = gnomecc.desktop control-center.idl

sysdir = $(datadir)/apps/System
sys_DATA = gnomecc.desktop

MAINTAINERCLEANFILES = $(CORBA_SRCLIST)

$(CORBA_SRCLIST): control-center.idl
	orbit-idl $(srcdir)/control-center.idl
main.c: $(CORBA_SRCLIST)
