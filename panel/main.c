/* Gnome panel: Initialization routines
 * (C) 1997 the Free Software Foundation
 *
 * Authors: Federico Mena
 *          Miguel de Icaza
 *          George Lebl
 */

#include <config.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <gnome.h>
#include <libgnorba/gnorba.h>

#include "panel-include.h"

#include "xstuff.h"

extern int config_sync_timeout;
extern int applets_to_sync;
extern int panels_to_sync;
extern int need_complete_save;

extern GSList *panels;

extern GSList *applets;
extern GSList *applets_last;
extern int applet_count;

extern GlobalConfig global_config;
extern char *panel_cfg_path;
extern char *old_panel_cfg_path;

/*list of all panel widgets created*/
extern GSList *panel_list;

GtkTooltips *panel_tooltips = NULL;

GnomeClient *client = NULL;

/*a list of started extern applet child processes*/
extern GList * children;

static int
menu_age_timeout(gpointer data)
{
	GSList *li;
	for(li=applets;li!=NULL;li=g_slist_next(li)) {
		AppletInfo *info = li->data;
		if(info->menu && info->menu_age++>=6 &&
		   !GTK_WIDGET_VISIBLE(info->menu)) {
			gtk_widget_destroy(info->menu);
			info->menu = NULL;
			info->menu_age = 0;
		}
		/*if we are allowed to, don't destroy applet menus*/
		if(!global_config.hungry_menus &&
		   info->type == APPLET_MENU) {
			Menu *menu = info->data;
			if(menu->menu && menu->age++>=6 &&
			   !GTK_WIDGET_VISIBLE(menu->menu)) {
				gtk_widget_destroy(menu->menu);
				menu->menu = NULL;
				menu->age = 0;
			}
		}
	}
	
	/*skip panel menus if we are memory hungry*/
	if(global_config.hungry_menus)
		return TRUE;
	
	for(li = panel_list; li != NULL; li = g_slist_next(li)) {
		PanelData *pd = li->data;
		if(pd->menu && pd->menu_age++>=6 &&
		   !GTK_WIDGET_VISIBLE(pd->menu)) {
			gtk_widget_destroy(pd->menu);
			pd->menu = NULL;
			pd->menu_age = 0;
		}
	}

	return TRUE;
}

static int
try_config_sync(gpointer data)
{
	panel_config_sync();
	return TRUE;
}



int
main(int argc, char **argv)
{
	CORBA_ORB orb;
	CORBA_Environment ev;
	gint duplicate;
	gchar *real_global_path;
	
	bindtextdomain(PACKAGE, GNOMELOCALEDIR);
	textdomain(PACKAGE);

	CORBA_exception_init(&ev);
	orb = gnome_CORBA_init("panel", VERSION,
			       &argc, argv,
			       GNORBA_INIT_SERVER_FUNC, &ev);
	CORBA_exception_free(&ev);

	switch (panel_corba_gtk_init (orb)) {
	case 0: 
		duplicate = 0;
		break; /* success */
	case -4: {
		GtkWidget* box = gnome_question_dialog
			(_("I've detected a panel already running.\n"
			   "Start another panel as well?\n" 
			   "(The new panel will not be restarted.)"), NULL, NULL);
		if (gnome_dialog_run_and_close (GNOME_DIALOG (box)))
			return 0;
		duplicate = 1;
		break;
	}
	default: {
		GtkWidget *box = gnome_error_dialog
			(_("There was a problem registering the panel "
			   "with the GOAD server.\n"
			   "The panel will now exit."));
		gnome_dialog_run_and_close (GNOME_DIALOG (box));
		return 0;
		break;
	}
	}

	client = gnome_master_client ();

	gnome_client_set_restart_style (client, duplicate 
					? GNOME_RESTART_NEVER 
					: GNOME_RESTART_IMMEDIATELY);

	gnome_client_set_priority(client,40);


	if (gnome_client_get_flags(client) & GNOME_CLIENT_RESTORED)
		old_panel_cfg_path = g_strdup (gnome_client_get_config_prefix (client));
	else
		old_panel_cfg_path = g_strdup ("/panel.d/default/");

#ifndef PER_SESSION_CONFIGURATION
	real_global_path = gnome_config_get_real_path (old_panel_cfg_path);
	if (!g_file_exists (real_global_path)) {
		g_free (old_panel_cfg_path);
		old_panel_cfg_path = g_strdup ("/panel.d/default/");
	}
	g_free (real_global_path);
#endif /* !PER_SESSION_CONFIGURATION */

	gnome_client_set_global_config_prefix (client, PANEL_CONFIG_PATH);
	
	gtk_signal_connect (GTK_OBJECT (client), "save_yourself",
			    GTK_SIGNAL_FUNC (panel_session_save), NULL);
	gtk_signal_connect (GTK_OBJECT (client), "die",
			    GTK_SIGNAL_FUNC (panel_session_die), NULL);

	panel_tooltips = gtk_tooltips_new();

	gnome_win_hints_init ();

	/* read, convert and remove old config */
	convert_old_config();

	/*set the globals*/
	load_up_globals();

	init_fr_chunks ();
	
	init_menus();
	
	init_user_panels();
	init_user_applets();
	init_user_tearoffs();

	gnome_triggers_do("Session startup", NULL, "gnome", "login", NULL);

	/*add forbidden lists to ALL panels*/
	g_slist_foreach(panels,(GFunc)panel_widget_add_forbidden,NULL);

	/*this will make the drawers be hidden for closed panels etc ...*/
	send_state_change();

	/*attempt to sync the config every 10 seconds, only if a change was
	  indicated though*/
	config_sync_timeout = gtk_timeout_add(10*1000,try_config_sync,NULL);

	gtk_timeout_add(10*1000,menu_age_timeout,NULL);
	
	/*load these as the last thing to prevent some races any races from
	  starting multiple goad_id's at once are libgnorba's problem*/
	load_queued_externs();

	xstuff_init();

	status_applet_create_offscreen();
	
	/* I use the glue code to avoid making this a C++ file */
	gtk_main ();

	return 0;
}
