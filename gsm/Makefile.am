## Process this file with automake to produce Makefile.in.

#SUBDIRS = help

defaultdir = $(datadir)/gnome

INCLUDES = \
	$(GNOME_SESSION_CFLAGS)				\
	-DGNOMELOCALEDIR=\""$(datadir)/locale\"" \
	-DDEFAULTDIR="\"$(defaultdir)\""

LDADD = $(GNOME_SESSION_LIBS) $(LIBWRAP_LIBS)

sysdir = $(datadir)/applications

if SESSION
noinst_in_files = session.desktop.in
noinst_DATA = $(noinst_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

noinst_PROGRAMS = \
	splash-test

bin_PROGRAMS = \
	gnome-session		\
	save-session		\
	gnome-login-check	\
	session-properties	\
	gnome-libice-check

#	session-properties-capplet
endif

splash_test_SOURCES = \
	splash.c	\
	splash.h	\
	splash-test.c

gnome_session_SOURCES = \
	manager.c	\
	manager.h	\
	ice.c		\
	ice.h		\
	main.c		\
	prop.c		\
	prop.h		\
	save.c		\
	save.h		\
	command.c	\
	command.h	\
	session.h	\
	remote.c	\
	remote.h	\
	logout.c	\
	logout.h	\
	splash.c	\
	splash.h	\
	gsm-protocol.c	\
	gsm-protocol.h	\
	headers.h

save_session_SOURCES = \
	save-session.c	\
	gsm-protocol.c	\
	gsm-protocol.h

session_properties_SOURCES = \
	session-properties.c	\
	gsm-client-list.c	\
	gsm-client-list.h	\
	gsm-client-editor.c	\
	gsm-client-editor.h	\
	gsm-client-row.c	\
	gsm-client-row.h	\
	gsm-protocol.c		\
	gsm-protocol.h		\
	session.h

# session_properties_capplet_SOURCES = \
# 	session-properties-capplet.c	\
# 	session-properties-capplet.h	\
# 	session.h			\
# 	startup-programs.c		\
# 	session-names.c			\
# 	gsm-protocol.c			\
# 	gsm-protocol.h			\
# 	gsm-client-list.c		\
# 	gsm-client-list.h		\
# 	gsm-client-row.c		\
# 	gsm-client-row.h		\
# 	gsm-client-editor.c		\
# 	gsm-client-editor.h 

gnome_login_check_SOURCES = gnome-login-check.c

gnome_libice_check_SOURCES = gnome-libice-check.c

bin_SCRIPTS = gnome-wm

default_DATA = default.session default.wm

desktopdir = $(datadir)/control-center/capplets
desktop_DATA = save-session.desktop session.desktop

sysdir = $(datadir)/control-center/Session

if SESSION
sys_in_files = session-properties.desktop.in
sys_DATA = $(sys_in_files:.desktop.in=.desktop)
endif

settingsdir = $(datadir)/control-center/capplets

if SESSION
settings_in_files = session-properties.desktop.in
settings_DATA = $(settings_in_files:.desktop.in=.desktop)
endif

default.session: default.in ../config.status
	sed -e 's,\@WINDOW_MANAGER\@,$(WINDOW_MANAGER),g' \
		< $(srcdir)/default.in > default.tmp \
	  && mv default.tmp default.session

default.wm: default.wm.in ../config.status
	sed -e 's,\@WINDOW_MANAGER\@,$(WINDOW_MANAGER),g' \
		< $(srcdir)/default.wm.in > default.wm.tmp \
	  && mv default.wm.tmp default.wm

man_MANS = gnome-wm.1 save-session.1 default.session.5 gnome-session.1

manuals = gnome-wm.1 default.session.5.in gnome-session.1.in save-session.1

pixmapdir = $(datadir)/pixmaps/splash/
pixmap_DATA = gnome-splash.png gnome-splash-1.4.0.png

EXTRA_DIST =			\
	default.in		\
	default.wm.in		\
	gnome-wm 		\
	save-session.desktop.in 	\
	session.desktop.in		\
	$(manuals)		\
	$(pixmap_DATA)		\
	gsm-marshal.list	\
	session-properties.desktop.in

marshal_sources = gsm-marshal.c gsm-marshal.h

$(gnome_session_OBJECTS) $(save_session_OBJECTS) $(session_properties_OBJECTS): $(marshal_sources)

gsm-marshal.h: gsm-marshal.list $(GLIB_GENMARSHAL)
	$(GLIB_GENMARSHAL) $< --header --prefix=gsm_marshal > $@
gsm-marshal.c: gsm-marshal.list $(GLIB_GENMARSHAL)
	$(GLIB_GENMARSHAL) $< --body --prefix=gsm_marshal > $@

BUILT_SOURCES = $(marshal_sources)
CLEANFILES    = $(BUILT_SOURCES)

dist-hook:
	cd $(distdir) ; rm -f $(CLEANFILES)
