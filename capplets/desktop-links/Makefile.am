ccenterdir = $(datadir)/control-center

Rootdir         = $(ccenterdir)
Inputdevsdir	= $(ccenterdir)/Inputdevs
Appearancesdir	= $(ccenterdir)/Appearances
Workspacedir	= $(ccenterdir)/Workspace
WindowManagerdir= $(ccenterdir)/WindowManager


Directory_files = \
	Gnome.directory		\
	Desktop.directory	\
	Input.directory		\
	Appearances.directory	\
	Workspace.directory	\
	WindowManager.directory

EXTRA_DIST = \
	Gnome.directory		\
	Desktop.directory	\
	Input.directory		\
	Appearances.directory	\
	Workspace.directory	\
	WindowManager.directory

install-data-local:
	$(mkinstalldirs) $(Inputdevsdir)
	$(mkinstalldirs) $(WindowManagerdir)
	$(mkinstalldirs) $(Workspacedir)
	$(mkinstalldirs) $(Appearancesdir)
	$(INSTALL_DATA) $(srcdir)/Gnome.directory $(ccenterdir)/.directory
	$(INSTALL_DATA) $(srcdir)/Input.directory $(Inputdevsdir)/.directory
	$(INSTALL_DATA) $(srcdir)/Appearances.directory $(Appearancesdir)/.directory
	$(INSTALL_DATA) $(srcdir)/Workspace.directory $(Workspacedir)/.directory
	$(INSTALL_DATA) $(srcdir)/WindowManager.directory $(WindowManagerdir)/.directory

