/* app-ui.c - Gnome UI libs application.  */

#include <config.h>
#include "gnome-desktop.h"

extern void ui_register (GnomePropertyConfigurator *c);

char *
application_title (void)
{
  return _("Gnome Look and Feel Properties");
}

char *
application_property (void)
{
  return "GNOME_UI_PROPERTY";
}

void
application_help (void)
{
  g_warning("Help not implemented\n");
}

void
application_register (GnomePropertyConfigurator *pconf)
{
  ui_register (pconf);
}

int
main (int argc, char *argv[])
{
  return (property_main ("ui_properties", argc, argv));
}

