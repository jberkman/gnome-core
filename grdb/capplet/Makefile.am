bin_PROGRAMS = grdb-capplet

grdb_capplet_SOURCES = \
	grdb-capplet.c \
	grdb-filtered-font-picker.c \
	grdb-filtered-font-picker.h

grdb_capplet_LDADD   = \
	$(GNOME_LIBDIR)		\
	$(GNOMEUI_LIBS)		\
	$(GNOME_CAPPLET_LIBS)	\
	$(INTLLIBS)

INCLUDES = \
	-I. \
	-I$(srcdir) \
	-I$(top_srcdir)/intl \
	-I$(top_builddir)/intl \
	-I$(includedir) \
	$(GNOME_INCLUDEDIR) \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	-DGNOME_DATADIR=\""$(datadir)"\"

ccdir = $(datadir)/control-center/Desktop
cc_DATA = grdb-capplet.desktop

paneldir = $(datadir)/gnome/apps/Settings/Desktop/
panel_DATA = $(cc_DATA)

icondir = $(datadir)/pixmaps
icon_DATA = grdb-capplet.png

EXTRA_DIST =  \
	grdb-capplet.desktop \
	grdb-capplet.png
