#include "tasklist_applet.h"

/* The applet */
extern GtkWidget *applet;

/* The configuration */
TasklistConfig Config;

void write_config (void)
{
	gnome_config_push_prefix (APPLET_WIDGET (applet)->privcfgpath);
	
	gnome_config_set_int ("tasklist/horz_width", Config.horz_width);
	gnome_config_set_int ("tasklist/horz_rows", Config.horz_rows);
	
	gnome_config_set_int ("tasklist/vert_height", Config.vert_height);
	gnome_config_set_int ("tasklist/vert_width", Config.vert_width);
	
	gnome_config_set_bool ("tasklist/show_mini_icons", Config.show_mini_icons);
	gnome_config_set_int ("tasklist/tasks_to_show", Config.tasks_to_show);
	gnome_config_set_bool ("tasklist/confirm_before_kill", Config.confirm_before_kill);
	gnome_config_sync ();
	
	gnome_config_pop_prefix ();
}

void read_config (void)
{
  gnome_config_push_prefix (APPLET_WIDGET (applet)->privcfgpath);

  Config.horz_width = gnome_config_get_int ("tasklist/horz_width=450");
  Config.horz_rows = gnome_config_get_int ("tasklist/horz_rows=2");

  Config.vert_fixed = gnome_config_get_bool ("tasklist/vert_fixed=true");
  Config.vert_width = gnome_config_get_int ("tasklist/vert_width=48");
  Config.vert_height = gnome_config_get_int ("tasklist/vert_height=300");

  Config.confirm_before_kill = gnome_config_get_bool ("tasklist/confirm_before_kill=true");

  Config.show_mini_icons = gnome_config_get_bool ("tasklist/show_mini_icons=true");
  Config.tasks_to_show = gnome_config_get_int ("tasklist/tasks_to_show=0");

}

