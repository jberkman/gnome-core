#include "tasklist_applet.h"

/* The applet */
extern GtkWidget *applet;

/* The configuration */
TasklistConfig Config;

void write_config (void)
{
  gnome_config_push_prefix (APPLET_WIDGET (applet)->privcfgpath);
  
  gnome_config_set_int ("tasklist/width", Config.width);
  gnome_config_set_int ("tasklist/height", Config.height);
  gnome_config_set_int ("tasklist/rows", Config.rows);
  gnome_config_set_bool ("tasklist/confirm_before_kill", Config.confirm_before_kill);
  gnome_config_sync ();

  gnome_config_pop_prefix ();
}

void read_config (void)
{
  gnome_config_push_prefix (APPLET_WIDGET (applet)->privcfgpath);

  Config.width = gnome_config_get_int ("tasklist/width=450");
  Config.height = gnome_config_get_int ("tasklist/height=300");
  Config.rows = gnome_config_get_int ("tasklist/rows=2");
  Config.confirm_before_kill = gnome_config_get_bool ("tasklist/confirm_before_kill=true");
  /* This should be fixed */
  Config.show_pixmaps = TRUE;
}
