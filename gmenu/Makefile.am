INCLUDES = 			\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	-I$(top_srcdir)/intl -I$(top_builddir)/intl	\
	-I$(includedir) $(GNOME_INCLUDEDIR)

bin_PROGRAMS = gmenu

gmenu_SOURCES =	\
	gmenu.c			\
	gmenu.h			\
	tree.c			\
	edit.c			\
	order.c			\
	dialogs.c

gmenu_LDADD = \
	$(GNOME_LIBDIR)					\
	$(GNOMEUI_LIBS)					\
	$(INTLLIBS)

EXTRA_DIST = gmenu.desktop top.xpm folder.xpm unknown.xpm

sysdir = $(datadir)/apps/System
sys_DATA = gmenu.desktop
