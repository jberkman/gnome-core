## Process this file with automake to produce Makefile.in.

INCLUDES = 			\
	-I$(includedir)		\
	-I$(top_srcdir)/intl	\
	-I$(top_builddir)/intl	\
	$(GNOME_INCLUDEDIR)	\
	-DGNOMELOCALEDIR=\""$(datadir)/locale\""

LDADD = \
	$(GNOME_LIBDIR)					\
	$(GNOMEUI_LIBS)					\
	-lzvt 						\
	$(INTLLIBS)


bin_PROGRAMS = gnome-terminal

EXTRA_DIST = gnome-terminal.desktop

gnome_terminal_SOURCES = 	\
	gnome-terminal.c	\
	gnome-terminal.h


gnome_terminal_LDADD = $(LDADD)

CFLAGS += -Wall

utildir = $(datadir)/apps/Utilities
util_DATA = gnome-terminal.desktop
