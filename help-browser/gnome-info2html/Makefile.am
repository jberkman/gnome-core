## Process this file with automake to produce Makefile.in.

INCLUDES = 			\
	-I$(includedir)		\
	$(GNOME_INCLUDEDIR)	

LDADD = \
	$(GNOME_LIBDIR)					\
	$(GNOMEUI_LIBS)

bin_PROGRAMS = gnome-info2html

gnome_info2html_SOURCES = main.c parse.c utils.c html.c data.h html.h parse.h utils.h version.h


