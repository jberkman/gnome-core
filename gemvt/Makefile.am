bin_PROGRAMS = gemvt

INCLUDE_always = -I$(includedir) $(GTK_CFLAGS) 

INCLUDES = $(INCLUDE_always) $(GNOME_INCLUDEDIR)

gemvt_SOURCES = $(gmevt_c_files) 
gemvt_LDADD = $(GEMVT_LD_EXTRA) $(GTK_LIBS)

gemvt_DEPENDENCIES = $(GETOPT_OBJS)

gmevt_h_files = \
	gvtvt.h \
	gvtconf.h

gmevt_c_files = \
	gvtvt.c \
	gvtmain.c \
	gvtconf.c

EXTRA_DIST = \
	TODO \
	$(gmevt_h_files) \
	$(gmevt_c_files) \
	getopt.h \
	getopt.c \
	getopt1.c \
	gemvt.xpm \
	gem-r.xpm \
	gem-g.xpm \
	gem-b.xpm \
	turkey.vt
