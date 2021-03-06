#ifndef SESSION_H
#define SESSION_H

#include <glib.h>
#include <libgnomeui/gnome-client.h>
#include "panel-widget.h"

G_BEGIN_DECLS

#define DEFAULT_HIDE_SPEED 50

/* amount of time in ms. to wait before lowering panel */
#define DEFAULT_HIDE_DELAY 300
#define DEFAULT_SHOW_DELAY 0

/* number of pixels it'll stick up from the bottom when using
   PANEL_AUTO_HIDE */
#define DEFAULT_MINIMIZED_SIZE 6

#define DEFAULT_PANEL_NUM 0

#define PANEL_CONFIG_PATH "panel2.d/default/"

int panel_session_save (GnomeClient *client,
			int phase,
			GnomeSaveStyle save_style,
			int shutdown,
			GnomeInteractStyle interact_style,
			int fast,
			gpointer client_data);

void save_next_applet(void);


int panel_session_die (GnomeClient *client,
			gpointer client_data);

void panel_quit(void);

void panel_config_sync(void);
void panel_config_sync_schedule (void);

void load_system_wide (void);

G_CONST_RETURN gchar *session_get_current_profile (void);
void                  session_set_current_profile (const gchar *profile_name);

void session_load (void);

void session_add_dead_launcher (const gchar *location);

void panel_session_setup_config_sync (void);

G_END_DECLS

#endif
