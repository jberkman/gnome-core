bin_PROGRAMS = gemvt

LINK     = $(LIBTOOL) --mode=link $(CC) $(CFLAGS) $(LDFLAGS) $(X_LIBS) -o $@
INCLUDE_always = -I$(includedir) $(X_CFLAGS)

INCLUDES = $(INCLUDE_always)

gemvt_SOURCES = $(gmevt_c_files) 
gemvt_LDADD = $(GETOPT_OBJS) $(GEMVT_LD_EXTRA)


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
