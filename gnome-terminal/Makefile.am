## Process this file with automake to produce Makefile.in.

# Documentation
SUBDIRS = C es no

INCLUDES = 			\
	-I$(includedir)		\
	-I$(top_srcdir)/intl	\
	-I$(top_builddir)/intl	\
	$(GNOME_INCLUDEDIR)	\
	-DGNOMELOCALEDIR=\""$(datadir)/locale\""

LDADD = \
	$(GNOME_LIBDIR)					\
	$(GNOMEUI_LIBS)					\
	$(ZVT_LIBS)					\
	$(INTLLIBS)


bin_PROGRAMS = gnome-terminal

EXTRA_DIST = gnome-terminal.desktop

gnome_terminal_SOURCES = 	\
	gnome-terminal.c	\
	gnome-terminal.h


gnome_terminal_LDADD = $(LDADD)

utildir = $(datadir)/gnome/apps/Utilities
util_DATA = gnome-terminal.desktop
