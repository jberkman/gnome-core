## Process this file with automake to produce Makefile.in.

# Documentation
#SUBDIRS = C de da es it no

INCLUDES = 						\
	$(GNOME_TERMINAL_CFLAGS)			\
	-DGNOME_TERMINAL_PREFIX=\""$(prefix)"\"		\
	-DGNOME_TERMINAL_DATADIR=\""$(datadir)"\" 	\
	-DGNOME_TERMINAL_GLADEDIR=\""$(gladedir)"\"	\
	-DGNOMELOCALEDIR=\""$(datadir)/locale\""

LDADD = \
	$(GNOME_TERMINAL_LIBS)

bin_PROGRAMS = gnome-terminal

#CORBA_SOURCE = \
#	GNOME_Terminal.h	\
#	GNOME_Terminal-stubs.c	\
#	GNOME_Terminal-skels.c	\
#	GNOME_Terminal-common.c

#gnome_terminal_idl = \
#	$(top_srcdir)/idl/GNOME_Terminal.idl

#idl_flags = -I$(top_srcdir)/idl -I $(BONOBO_IDLDIR)

#$(CORBA_SOURCE) : $(gnome_terminal_idl)
#	$(ORBIT_IDL) $(idl_flags) $(gnome_terminal_idl)

gnome_terminal_src = \
	gnome-terminal.c	\
	gnome-terminal.h	\
	gnome-x-font-selector.c	\
	gnome-x-font-selector.h

#	corba.c			\

gnome_terminal_SOURCES = \
	$(gnome_terminal_src)

#	$(CORBA_SOURCE)

#$(gnome_terminal_OBJECTS): GNOME_Terminal.h

utildir = $(datadir)/applications
util_DATA = gnome-terminal.desktop

uidir = $(datadir)/gnome-2.0/ui
ui_DATA = gnome-terminal.xml

gladedir   = $(datadir)/gnome-terminal/glade
glade_DATA = gnome-terminal.glade2
glade_src = gnome-terminal.glade
glade_msgs = gnome-terminal.glade.h

gnome-terminal.glade2: gnome-terminal.glade $(LIBGLADE_CONVERT)
	$(LIBGLADE_CONVERT) $(srcdir)/gnome-terminal.glade > gnome-terminal.glade.tmp \
	&& mv gnome-terminal.glade.tmp gnome-terminal.glade2

man_MANS = gnome-terminal.1

EXTRA_DIST =				\
	$(util_DATA)			\
	$(glade_DATA)			\
	$(glade_src)			\
	$(glade_msgs)			\
	$(man_MANS)			\
	$(ui_DATA)

# BUILT_SOURCES = $(CORBA_SOURCE)
CLEANFILES    = $(BUILT_SOURCES)

dist-hook:
	cd $(distdir) ; rm -f $(CLEANFILES)
