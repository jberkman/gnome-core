/* app-mouse.c - Mouse configuration application.  */

#include <config.h>
#include "gnome.h"
#include "gnome-desktop.h"

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
application_register (GnomePropertyConfigurator *config)
{
  mouse_register (config);
}

int
main (int argc, char *argv[])
{
  return (property_main ("mouse_properties", argc, argv));
}
