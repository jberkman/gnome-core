/* app-background.c - Background/Screensaver configuration application.  */

#include <config.h>
#include "gnome.h"
#include "gnome-desktop.h"

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
application_register (GnomePropertyConfigurator *config)
{
  background_register (config);
  screensaver_register (config);
}

int
main (int argc, char *argv[])
{
  return (property_main (argc, argv));
}
