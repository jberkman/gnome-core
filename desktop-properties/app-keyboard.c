/* app-keyboard.c - Keyboard configuration application.  */

#include <config.h>
#include "gnome.h"
#include "gnome-desktop.h"

char *
application_title (void)
{
  return _("Keyboard Properties");
}

void
application_help (void)
{
}

void
application_register (GnomePropertyConfigurator *config)
{
  keyboard_register (config);
}

int
main (int argc, char *argv[])
{
  return (property_main (argc, argv));
}
