INCLUDES = 			\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	-I$(includedir) $(GNOME_INCLUDEDIR)

bin_PROGRAMS = gmenu

gmenu_SOURCES =	\
	gmenu.c			\
	gmenu.h			\
	fileops.c		\
	iconsel.c		\
	iconsel.h		\
	top.xpm			\
	folder.xpm		\
	unknown.xpm

gmenu_LDADD = \
	$(GNOME_LIBDIR)					\
	$(GNOMEUI_LIBS)					\
	$(INTLLIBS)

EXTRA_DIST = gmenu.desktop

sysdir = $(datadir)/apps/System
sys_DATA = gmenu.desktop

