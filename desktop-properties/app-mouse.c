/* app-mouse.c - Mouse configuration application.  */

#include <config.h>
#include "gnome-desktop.h"

extern void mouse_register (GnomePropertyConfigurator *c);

char *
application_title (void)
{
  return _("Mouse Properties");
}

char *
application_property (void)
{
  return "GNOME_MOUSE_PROPERTY";
}

void
application_help (void)
{
}

void
application_register (GnomePropertyConfigurator *pconf)
{
  mouse_register (pconf);
}

int
main (int argc, char *argv[])
{
  return (property_main ("mouse_properties", argc, argv));
}
