pixmapdir = $(datadir)/pixmaps/mailcheck

INCLUDES = 			\
	-I../../.. 		\
	-I$(srcdir)/../../.. 	\
	-I$(includedir)

lib_LTLIBRARIES = libpanel_mailcheck.la

libpanel_mailcheck_la_SOURCES = 	\
	mailcheck.c


pixmap_DATA = email.xpm email-e.xpm
EXTRA_DIST = $(pixmap_DATA)

