/* app-bell.c - Bell configuration application.  */

#include <config.h>
#include "gnome-desktop.h"

extern void bell_register(GnomePropertyConfigurator *c);

char *
application_title (void)
{
  return _("Bell Properties");
}

char *
application_property (void)
{
  return "GNOME_BELL_PROPERTY";
}

void
application_help (void)
{
}

void
application_register (GnomePropertyConfigurator *pconf)
{
  bell_register (pconf);
}

int
main (int argc, char *argv[])
{
  return (property_main ("bell_properties", argc, argv));
}
