INCLUDES = 			\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	-I$(includedir) $(GNOME_INCLUDEDIR)

bin_PROGRAMS = gnomecc

gnomecc_SOURCES =	\
	main.c		\
	tree.c		\
	callbacks.c	\
	$(CORBA_SRCLIST)


CORBA_SRCLIST = control-center-skels.c control-center-common.c control-center.h

gnomecc_LDADD = \
	$(GNOME_LIBDIR)					\
	$(GNOMEUI_LIBS)					\
	$(INTLLIBS)					\
	$(ORB_LIBS)

EXTRA_DIST = gnomecc.desktop

sysdir = $(datadir)/apps/System
sys_DATA = gnomecc.desktop

MAINTAINERCLEANFILES = $(CORBA_SRCLIST)

$(CORBA_SRCLIST): control-center.idl
	orbit-idl $(srcdir)/control-center.idl
main.c: $(CORBA_SRCLIST)
