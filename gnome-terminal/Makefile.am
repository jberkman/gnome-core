## Process this file with automake to produce Makefile.in.

INCLUDES = 			\
	-I$(includedir)		\
	-I$(top_srcdir)/intl	\
	$(GNOME_INCLUDEDIR)	\
	-DGNOMELOCALEDIR=\""$(datadir)/locale\""

LDADD = \
	$(GNOME_LIBDIR)					\
	$(GNOMEUI_LIBS)					\
	-lzvt -lutil
	$(INTLLIBS)


bin_PROGRAMS = gnome-terminal

EXTRA_DIST = gnome-terminal.desktop

gnome_terminal_SOURCES = 	\
	gnome-terminal.c	\
	menu.c
gnome_terminal_LDADD = $(LDADD)

CFLAGS += -Wall

utildir = $(datadir)/apps/Utilities
util_DATA = gnome-terminal.desktop

