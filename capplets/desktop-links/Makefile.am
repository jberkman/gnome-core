ccenterdir = $(datadir)/control-center

Rootdir         = $(ccenterdir)
Peripheralsdir  = $(ccenterdir)/Peripherals
Desktopdir   	= $(ccenterdir)/Desktop


Directory_files = \
	Peripherals.directory
	Desktop.directory

install-data-local:
	$(mkinstalldirs) $(Peripheralsdir)
	$(INSTALL_DATA) $(srcdir)/Gnome.directory $(ccenterdir)/.directory
	$(INSTALL_DATA) $(srcdir)/Peripherals.directory $(Peripheralsdir)/.directory
	$(INSTALL_DATA) $(srcdir)/Desktop.directory $(Desktopdir)/.directory
