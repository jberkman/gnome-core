/* logout.c - Panel applet to end current session.  */
/* Original author unknown. CORBAized by Elliot Lee */
/* uncorbized by George Lebl */

#include <config.h>
#include <gnome.h>

#include "panel-include.h"

extern GSList *applets_last;

extern GtkTooltips *panel_tooltips;

extern GlobalConfig global_config;

static void
logout(GtkWidget *widget)
{
	if(global_config.drawer_auto_close) {
		GtkWidget *parent = PANEL_WIDGET(widget->parent)->panel_parent;
		g_return_if_fail(parent!=NULL);
		if(IS_DRAWER_WIDGET(parent)) {
			BasePWidget *basep = BASEP_WIDGET(parent);
			GtkWidget *grandparent = PANEL_WIDGET(basep->panel)->master_widget->parent;
			GtkWidget *grandparentw =
				PANEL_WIDGET(grandparent)->panel_parent;
			drawer_widget_close_drawer (DRAWER_WIDGET (parent),
						    BASEP_WIDGET (grandparentw));
		}
	}

	panel_quit();
}

static GtkWidget *
create_logout_widget(void)
{
	GtkWidget *button;
	char *pixmap_name;

	pixmap_name = gnome_pixmap_file("gnome-term-night.png");

	button = button_widget_new(pixmap_name,-1,
				   MISC_TILE,
				   FALSE,
				   ORIENT_UP,
				   _("Log out"));
	g_free(pixmap_name);
	gtk_tooltips_set_tip (panel_tooltips,button,_("Log out of GNOME"),NULL);

	gtk_signal_connect(GTK_OBJECT(button), "clicked",
			   GTK_SIGNAL_FUNC(logout), NULL);

	return button;
}

void
load_logout_applet(PanelWidget *panel, int pos, gboolean exactpos)
{
	GtkWidget *logout;

	logout = create_logout_widget();
	if(!logout)
		return;

	if (!register_toy(logout, NULL, NULL, panel,
			  pos, exactpos, APPLET_LOGOUT))
		return;

	applet_add_callback(applets_last->data, "help",
			    GNOME_STOCK_PIXMAP_HELP,
			    _("Help"));
}

static GtkWidget *
create_lock_widget(void)
{
	GtkWidget *button;
	char *pixmap_name;

	pixmap_name = gnome_pixmap_file("gnome-lockscreen.png");

	button = button_widget_new(pixmap_name,-1,
				   MISC_TILE,
				   FALSE,
				   ORIENT_UP,
				   _("Lock screen"));
	g_free(pixmap_name);
	gtk_tooltips_set_tip (panel_tooltips,button,_("Lock screen"),NULL);

	gtk_signal_connect(GTK_OBJECT(button), "clicked",
			   GTK_SIGNAL_FUNC(panel_lock), NULL);

	return button;
}

void
load_lock_applet(PanelWidget *panel, int pos, gboolean exactpos)
{
	GtkWidget *lock;
	lock = create_lock_widget();

	if(!lock)
		return;
	if (!register_toy(lock, NULL, NULL, panel, pos, 
			  exactpos, APPLET_LOCK))
		return;

        /*
	  <jwz> Blank Screen Now
	  <jwz> Lock Screen Now
	  <jwz> Kill Daemon
	  <jwz> Restart Daemon
	  <jwz> Preferences
	  <jwz> (or "configuration" instead?  whatever word you use)
	  <jwz> those should do xscreensaver-command -activate, -lock, -exit...
	  <jwz> and "xscreensaver-command -exit ; xscreensaver &"
	  <jwz> and "xscreensaver-demo"
	*/

	applet_add_callback(applets_last->data, "activate",
			    NULL, _("Blank Screen Now"));
	applet_add_callback(applets_last->data, "lock",
			    NULL, _("Lock Screen Now"));
	applet_add_callback(applets_last->data, "exit",
			    NULL, _("Kill Daemon"));
	applet_add_callback(applets_last->data, "restart",
			    NULL, _("Restart Daemon"));
	applet_add_callback(applets_last->data, "prefs",
			    NULL, _("Preferences"));
	applet_add_callback(applets_last->data, "help",
			    GNOME_STOCK_PIXMAP_HELP,
			    _("Help"));
}


