/* app-keyboard.c - Keyboard configuration application.  */

#include <config.h>
#include "gnome-desktop.h"

extern void keyboard_register(GnomePropertyConfigurator *c);

char *
application_title (void)
{
  return _("Keyboard Properties");
}

char *
application_property (void)
{
  return "GNOME_KEYBOARD_PROPERTY";
}

void
application_help (void)
{
}

void
application_register (GnomePropertyConfigurator *pconf)
{
  keyboard_register (pconf);
}

int
main (int argc, char *argv[])
{
  return (property_main ("keyboard_properties", argc, argv));
}
