INCLUDES = 			\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	-I$(includedir) $(GNOME_INCLUDEDIR)

bin_PROGRAMS = gmenu

gmenu_SOURCES =	\
	gmenu.c			\
	gmenu.h			\
	tree.c			\
	edit.c			\
	order.c			\
	dialogs.c		\
	wait-feet.h

gmenu_LDADD = \
	$(GNOME_LIBDIR)					\
	$(GNOMEUI_LIBS)					\
	$(INTLLIBS)

EXTRA_DIST = gmenu.desktop top.xpm folder.xpm unknown.xpm up.xpm down.xpm

sysdir = $(datadir)/apps/System
sys_DATA = gmenu.desktop
