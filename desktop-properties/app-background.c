/* app-background.c - Background/Screensaver configuration application.  */

#include <config.h>
#include "gnome-desktop.h"

/* If true, we need to call gnome_config_sync () */
int need_sync = 0;

extern void background_register (GnomePropertyConfigurator *c);
extern void screensaver_register (GnomePropertyConfigurator *c);

char *
application_title (void)
{
  return _("Background Properties");
}

char *
application_property (void)
{
  return "GNOME_BACKGROUND_PROPERTY";
}

void
application_help (void)
{
}

void
application_register (GnomePropertyConfigurator *pconf)
{
  background_register (pconf);
  screensaver_register (pconf);
}

int
main (int argc, char *argv[])
{
	int v;
	
	v = property_main ("background_properties", argc, argv);
	if (need_sync)
		gnome_config_sync ();
	return v;
}
