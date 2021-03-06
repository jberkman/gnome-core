## We require new-style dependency handling.
AUTOMAKE_OPTIONS = 1.3

NULL =

#SUBDIRS =

EXTRA_SUBDIRS = help

INCLUDES =							\
	-I.							\
	-I$(srcdir)						\
	-I$(top_srcdir)/gnome-desktop				\
	-I$(top_builddir)/panel					\
	-DPANEL_DEBUG						\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"		\
	-DGLADEDIR=\""$(datadir)/gnome/panel"\"			\
	$(DISABLE_DEPRECATED_CFLAGS)				\
	$(STANDARD_PROPERTIES_CFLAGS)				\
	$(ALLOW_CRACKPOT_MENU_CODE)				\
	$(PANEL_CFLAGS)						\
	$(PANELCONFIG_CFLAGS)					\
	$(WARN_CFLAGS)						\
	$(NULL)

# FIXME:
# this define is to allow menu.c to continue
# using deprecated gtk+ internals
ALLOW_CRACKPOT_MENU_CODE=-DGTK_MENU_INTERNALS

# Flags to make sure we're not using any deprecated functions
DISABLE_DEPRECATED_CFLAGS = 					\
	-DG_DISABLE_DEPRECATED					\
	-DGDK_DISABLE_DEPRECATED				\
	-DGTK_DISABLE_DEPRECATED				\
	-DGDK_PIXBUF_DISABLE_DEPRECATED				\
	-DGNOME_DISABLE_DEPRECATED				\
	-DGNOME_CORE_INTERNAL					\
	$(NULL)

# Used by the GNOME_PROGRAM_STANDARD_PROPERTIES macros
STANDARD_PROPERTIES_CFLAGS = 					\
	-DPREFIX=\""$(prefix)"\"				\
	-DSYSCONFDIR=\""$(sysconfdir)"\"			\
	-DLIBDIR=\""$(libdir)"\"				\
	-DDATADIR=\""$(datadir)"\"				\
	$(NULL)

bin_PROGRAMS = \
	gnome-panel-2 			\
	gnome-panel-properties		\
	gnome-panel-screenshot

$(gnome_panel_2_OBJECTS) $(gnome_panel_properties_OBJECTS) $(gnome_panel_screenshot_OBJECTS): $(BUILT_SOURCES)

CORBA_SRCLIST =			\
	GNOME_Panel-stubs.c	\
	GNOME_Panel-skels.c	\
	GNOME_Panel-common.c	\
	GNOME_Panel.h		\
	$(NULL)

$(CORBA_SRCLIST): $(top_srcdir)/idl/GNOME_Panel.idl $(ORBIT_IDL)
	$(ORBIT_IDL) -I $(BONOBO_IDLDIR) -I $(BONOBO_ACT_IDLDIR) $(top_srcdir)/idl/GNOME_Panel.idl

BUILT_SOURCES = \
	$(CORBA_SRCLIST)	\
	panel-typebuiltins.c	\
	panel-typebuiltins.h	\
	panel-marshal.c		\
	panel-marshal.h

panel_sources =			\
	panel-typebuiltins.c	\
	panel-typebuiltins.h	\
	panel-marshal.c		\
	panel-marshal.h		\
	main.c			\
	panel-widget.c  	\
	basep-widget.c  	\
	button-widget.c  	\
	multiscreen-stuff.c	\
	xstuff.c		\
	session.c		\
	foobar-widget.c		\
	border-widget.c		\
	edge-widget.c		\
	aligned-widget.c	\
	drawer-widget.c		\
	sliding-widget.c	\
	floating-widget.c	\
	rgb-stuff.c		\
	panel.c			\
	applet.c		\
	logout.c		\
	drawer.c  		\
	panel-util.c		\
	panel-gconf.c		\
	global-keys.c		\
	panel-config.c  	\
	status.c  		\
	distribution.c		\
	gnome-run.c  		\
	menu.c  		\
	menu-util.c		\
	menu-fentry.c  		\
	swallow.c  		\
	menu-properties.c	\
	launcher.c		\
	panel-applet-frame.c	\
	panel-shell.c		\
	quick-desktop-reader.c	\
	$(NULL)

panel_headers =			\
	panel-types.h		\
	panel-widget.h		\
	panel-main.h		\
	basep-widget.h		\
	button-widget.h		\
	multiscreen-stuff.h	\
	xstuff.h		\
	session.h		\
	foobar-widget.h		\
	border-widget.h		\
	edge-widget.h		\
	aligned-widget.h	\
	drawer-widget.h		\
	sliding-widget.h	\
	floating-widget.h	\
	rgb-stuff.h		\
	panel.h         	\
	applet.h         	\
	logout.h		\
	drawer.h  		\
	panel-util.h		\
	global-keys.h		\
	panel-config.h		\
	panel-config-global.h  	\
	panel-gconf.h		\
	status.h  		\
	distribution.h		\
	gnome-run.h  		\
	menu.h  		\
	menu-util.h		\
	menu-fentry.h  		\
	swallow.h  		\
	menu-properties.h	\
	launcher.h		\
	panel-applet-frame.h	\
	quick-desktop-reader.h	\
	panel-shell.h		\
	$(NULL)

gnome_panel_2_SOURCES =		\
	$(CORBA_SRCLIST)	\
	$(panel_sources)	\
	$(panel_headers)	\
	$(NULL)

gnome_panel_2_LDADD =		\
	$(PANEL_LIBS)		\
	$(XINERAMA_LIBS)	\
	$(top_builddir)/gnome-desktop/libgnome-desktop-2.la

gnome_panel_2_LDFLAGS = -export-dynamic

gnome_panel_properties_SOURCES = gnome-panel-properties.c panel-util.c

gnome_panel_properties_LDADD = $(PANELCONFIG_LIBS)           

gnome_panel_properties_LDFLAGS = -export-dynamic


gnome_panel_screenshot_SOURCES = \
	gnome-panel-screenshot.c

gnome_panel_screenshot_LDFLAGS = -export-dynamic

gnome_panel_screenshot_LDADD = 	\
	$(PANELCONFIG_LIBS)	\
	$(PNG_LIBS)

gladedir = $(datadir)/gnome/panel
glade_DATA = \
	gnome-panel-properties.glade2		\
	gnome-panel-screenshot.glade2

glade_src = \
	gnome-panel-properties.glade		\
	gnome-panel-screenshot.glade

EXTRA_DIST =					\
	GNOME_Panel.server.in			\
	panel.hints				\
	gnome-panel-properties.desktop.in		\
	gnome-panel-properties			\
	$(glade_src)				\
	$(glade_DATA)				\
	nothing.cP				\
	nothing.h				\
	$(schema_DATA)				\
	panel-marshal.list

gnome-panel-properties.glade2: gnome-panel-properties.glade $(LIBGLADE_CONVERT)
	$(LIBGLADE_CONVERT) $(srcdir)/gnome-panel-properties.glade > gnome-panel-properties.glade.tmp \
	&& mv gnome-panel-properties.glade.tmp gnome-panel-properties.glade2

gnome-panel-screenshot.glade2: gnome-panel-screenshot.glade $(LIBGLADE_CONVERT)
	$(LIBGLADE_CONVERT) $(srcdir)/gnome-panel-screenshot.glade > gnome-panel-screenshot.glade.tmp \
	&& mv gnome-panel-screenshot.glade.tmp gnome-panel-screenshot.glade2

hintsdir = $(datadir)/gnome/hints
hints_DATA = panel.hints

sysdir = $(datadir)/gnome/capplets
sys_in_files = gnome-panel-properties.desktop.in
sys_DATA = $(sys_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

panel_enum_headers =				\
	$(top_srcdir)/panel/panel-types.h	\
	$(top_srcdir)/panel/panel-widget.h	\
	$(top_srcdir)/panel/basep-widget.h	\
	$(top_srcdir)/panel/aligned-widget.h	\
	$(top_srcdir)/panel/sliding-widget.h	\
	$(top_builddir)/panel/GNOME_Panel.h

panel-marshal.h: panel-marshal.list $(GLIB_GENMARSHAL)
	$(GLIB_GENMARSHAL) $< --header --prefix=panel_marshal > $@

panel-marshal.c: panel-marshal.list $(GLIB_GENMARSHAL)
	echo "#include \"panel-marshal.h\"" > $@ && \
	$(GLIB_GENMARSHAL) $< --body --prefix=panel_marshal >> $@

panel-typebuiltins.c: @REBUILD@ $(panel_enum_headers)
	glib-mkenums    --fhead "#include <glib-object.h>\n" \
			--fhead "#include \"panel-typebuiltins.h\"\n\n" \
			--fprod "\n/* enumerations from \"@filename@\" */" \
			--fprod "\n#include \"@filename@\"\n" \
			--vhead "static const G@Type@Value _@enum_name@_values[] = {" \
			--vprod "  { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
			--vtail "  { 0, NULL, NULL }\n};\n\n" \
			--vtail "GType\n@enum_name@_get_type (void)\n{\n" \
			--vtail "  static GType type = 0;\n\n" \
			--vtail "  if (!type)\n" \
			--vtail "    type = g_@type@_register_static (\"@EnumName@\", _@enum_name@_values);\n\n" \
			--vtail "  return type;\n}\n\n" \
		$(panel_enum_headers) > $@

panel-typebuiltins.h: @REBUILD@ $(panel_enum_headers)
	glib-mkenums    --fhead "#ifndef __PANEL_TYPEBUILTINS_H__\n" \
			--fhead "#define __PANEL_TYPEBUILTINS_H__ 1\n\n" \
			--fhead "G_BEGIN_DECLS\n\n" \
			--ftail "G_END_DECLS\n\n" \
			--ftail "#endif /* __PANEL_TYPEBUILTINS_H__ */\n" \
			--fprod "\n/* --- @filename@ --- */" \
			--eprod "#define PANEL_TYPE_@ENUMNAME@ @enum_name@_get_type()\n" \
			--eprod "GType @enum_name@_get_type (void);\n" \
		$(panel_enum_headers) >  $@

schemadir   = $(sysconfdir)/gconf/schemas
schema_DATA = panel-global-config.schemas panel-per-panel-config.schemas

serverdir       = $(libdir)/bonobo/servers
server_in_files = GNOME_Panel.server.in
server_DATA     = $(server_in_files:.server.in=.server)

@INTLTOOL_SERVER_RULE@

install-data-local:
	if test -z "$(DESTDIR)" ; then \
		for p in $(schema_DATA) ; do \
			GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(srcdir)/$$p ; \
		done \
	fi

CLEANFILES = $(BUILT_SOURCES)

dist-hook:
	cd $(distdir) ; rm -f $(CLEANFILES)
