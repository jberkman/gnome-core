/* app-mouse.c - Mouse configuration application.  */

#include "gnome-desktop.h"

char *
application_title (void)
{
  return _("Mouse Properties");
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
  return (property_main (argc, argv));
}
