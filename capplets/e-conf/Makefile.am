INCLUDES = -I. -I$(srcdir) 				\
	-I$(top_srcdir)/intl -I$(top_builddir)/intl	\
	-I$(srcdir)/../../control-center		\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"	\
	-I$(includedir) $(GNOME_INCLUDEDIR)

bin_PROGRAMS = e-conf

e_conf_SOURCES = e-conf.c

e_conf_LDADD = ../../control-center/libcapplet.la \
		     $(GNOME_LIBDIR) $(ORB_LIBS) \
		     $(GNOMEUI_LIBS) $(INTLLIBS) -lgnorba

EXTRA_DIST = \
	e-conf.desktop \
	e-desks.desktop \
	e-fonts.desktop \
	e-sound.desktop

sysdir = $(datadir)/control-center/Workspace/Enlightenment
sys_DATA = \
	e-conf.desktop \
	e-desks.desktop \
	e-fonts.desktop \
	e-sound.desktop \
	e-colors.desktop

install-data-local:
	$(mkinstalldirs) $(sysdir)
	$(INSTALL_DATA) $(srcdir)/Enlightenment.directory $(sysdir)/.directory
					 
