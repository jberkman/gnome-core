bin_PROGRAMS = grdb

grdb_SOURCES = \
	grdb.h \
	main.c   \
	fonts.c  \
	colors.c

grdb_LDADD = \
	$(GNOME_LIBDIR)		\
	$(GNOME_LIBS)		\
	$(GTK_LIBS)		\
	$(INTLLIBS)

INCLUDES = \
	-I. \
	-I$(srcdir) \
	-I$(top_srcdir)/intl \
	-I$(top_builddir)/intl \
	-I$(includedir) \
	$(GNOME_INCLUDEDIR) \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	-DGNOME_DATADIR=\""$(datadir)"\" \
	-DPATH_TO_XRDB=\"$(PATH_TO_XRDB)\"

