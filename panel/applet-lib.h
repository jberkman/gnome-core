#ifndef APPLET_LIB_H
#define APPLET_LIB_H

#include <applet-widget.h>

BEGIN_GNOME_DECLS

#ifndef PANEL_UTIL_H
/*from panel-util.h*/
char *get_full_path(char *argv0);
#endif

/*all the cfgpaths in this interface are load paths (might be an old
  session different from current) ... except the save_session which gets
  the current session stuff (not in this file, but implemented in any
  applet)*/

char *gnome_panel_applet_request_id (const char *path, const char *param,
				     int dorestart,
				     int *applet_id, char **cfgpath,
				     char **globcfgpath, guint32 *winid);
char *gnome_panel_applet_register (GtkWidget *widget, int applet_id);
char *gnome_panel_applet_abort_id (int applet_id);
char *gnome_panel_applet_remove_from_panel (int applet_id);
char *gnome_panel_applet_request_glob_cfg (char **globcfgpath);
int gnome_panel_applet_get_panel_orient (int applet_id);
char *gnome_panel_quit (void);
char *gnome_panel_sync_config (int applet_id);
int gnome_panel_applet_init_corba (void);
int gnome_panel_applet_reinit_corba (void);
void gnome_panel_applet_register_callback (int applet_id,
					   char *name,
					   char *stock_item,
					   char *menutext,
					   AppletCallbackFunc func,
					   gpointer data);
void gnome_panel_applet_unregister_callback(int applet_id, char *name);
void gnome_panel_applet_register_callback_dir (int applet_id,
					       char *name,
					       char *stock_item,
					       char *menutext);
void gnome_panel_applet_unregister_callback_dir(int applet_id, char *name);


void applet_corba_gtk_main (char *str);
void applet_corba_gtk_main_quit (void);

void gnome_panel_applet_cleanup (int applet_id);

char * gnome_panel_applet_add_tooltip (int applet_id, char *tooltip);
char * gnome_panel_applet_remove_tooltip (int applet_id);

/*this is currently not used, it's an empty function for now, but it
  should register the orbit arguments*/
void panel_corba_register_arguments (void);

END_GNOME_DECLS

#endif
