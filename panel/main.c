/* Gnome panel: Initialization routines
 * (C) 1997,1998,1999,2000 the Free Software Foundation
 * (C) 2000 Eazel, Inc.
 *
 * Authors: Federico Mena
 *          Miguel de Icaza
 *          George Lebl
 */

#include <config.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>

#include <libgnome/libgnome.h>
#include <libgnomeui/libgnomeui.h>

#include <libbonoboui.h>
#include <bonobo-activation/bonobo-activation.h>

#include "main.h"

#include "conditional.h"
#include "drawer-widget.h"
#include "menu-fentry.h"
#include "menu.h"
#include "multiscreen-stuff.h"
#include "panel-util.h"
#include "panel-gconf.h"
#include "panel_config_global.h"
#include "session.h"
#include "status.h"
#include "xstuff.h"

/*#include "panel-unique-factory.h"*/
#include "panel-shell.h"

extern int config_sync_timeout;

extern GSList *panels;

extern GSList *applets;

extern gboolean commie_mode;
extern GlobalConfig global_config;
extern char *old_panel_cfg_path;

/*list of all panel widgets created*/
extern GSList *panel_list;

GtkTooltips *panel_tooltips = NULL;

GnomeClient *client = NULL;

char *kde_menudir = NULL;
char *kde_icondir = NULL;
char *kde_mini_icondir = NULL;

static gboolean
menu_age_timeout(gpointer data)
{
	GSList *li;
	for(li=applets;li!=NULL;li=g_slist_next(li)) {
		AppletInfo *info = li->data;
		if(info->menu && info->menu_age++>=6 &&
		   !GTK_WIDGET_VISIBLE(info->menu)) {
			gtk_widget_unref(info->menu);
			info->menu = NULL;
			info->menu_age = 0;
		}
		/*if we are allowed to, don't destroy applet menus*/
		if(!global_config.keep_menus_in_memory &&
		   info->type == APPLET_MENU) {
			Menu *menu = info->data;
			if(menu->menu && menu->age++>=6 &&
			   !GTK_WIDGET_VISIBLE(menu->menu)) {
				gtk_widget_unref(menu->menu);
				menu->menu = NULL;
				menu->age = 0;
			}
		}
	}
	
	/*skip panel menus if we are memory hungry*/
	if(global_config.keep_menus_in_memory)
		return TRUE;
	
	for(li = panel_list; li != NULL; li = g_slist_next(li)) {
		PanelData *pd = li->data;
		if(pd->menu && pd->menu_age++>=6 &&
		   !GTK_WIDGET_VISIBLE(pd->menu)) {
			gtk_widget_unref(pd->menu);
			pd->menu = NULL;
			pd->menu_age = 0;
		}
	}

	return TRUE;
}

/* Some important code copied from PonG */
typedef struct _AppletContainer AppletContainer;
struct _AppletContainer {
	GdkWindow *win;
	gboolean hide_mode;
	int state;
	int x, y, xs, ys;
	int handler;
	GdkPixmap *phsh[4];
	GdkBitmap *phsh_mask[4];
};
AppletContainer phsh = {0};

static void
phsh_kill (void)
{
	int i;
	for (i = 0; i < 4; i++) {
		gdk_pixmap_unref (phsh.phsh[i]);
		gdk_bitmap_unref (phsh.phsh_mask[i]);
	}
	gdk_window_destroy (phsh.win);
	gtk_timeout_remove (phsh.handler);
	memset (&phsh, 0, sizeof (AppletContainer));
}

static gboolean
phsh_move (gpointer data)
{
	int orient, state;
	gboolean change = TRUE;

	phsh.x += phsh.xs;
	phsh.y += phsh.ys;
	if (phsh.x <= -60 ||
	    phsh.x >= gdk_screen_width ()) {
		phsh_kill ();
		return FALSE;
	}
	if (phsh.y <= 0 ||
	    phsh.y >= gdk_screen_height () - 40 ||
	    rand() % (phsh.hide_mode?10:50) == 0)
		phsh.ys = -phsh.ys;

	phsh.state ++;
	if (phsh.state % (phsh.hide_mode?2:4) == 0)
		change = TRUE;
	if (phsh.state >= (phsh.hide_mode?4:8))
		phsh.state = 0;

	state = phsh.state >= (phsh.hide_mode?2:4) ? 1 : 0;
	orient = phsh.xs >= 0 ? 0 : 2;

	if (change) {
		gdk_window_set_back_pixmap (phsh.win, phsh.phsh[orient + state], FALSE);
		gdk_window_shape_combine_mask (phsh.win, phsh.phsh_mask[orient + state], 0, 0);
		gdk_window_clear (phsh.win);
	}

	gdk_window_move (phsh.win, phsh.x, phsh.y);
	gdk_window_raise (phsh.win);

	return TRUE;
}

static void
phsh_reverse (GdkPixbuf *gp)
{
	guchar *pixels = gdk_pixbuf_get_pixels (gp);
	int x, y;
	int rs = gdk_pixbuf_get_rowstride (gp);
#define DOSWAP(x,y) tmp = x; x = y; y = tmp;
	for (y = 0; y < 40; y++, pixels += rs) {
		guchar *p = pixels;
		guchar *p2 = pixels + 60*4 - 4;
		for (x = 0; x < 30; x++, p+=4, p2-=4) {
			guchar tmp;
			DOSWAP (p[0], p2[0]);
			DOSWAP (p[1], p2[1]);
			DOSWAP (p[2], p2[2]);
			DOSWAP (p[3], p2[3]);
		}
	}
#undef DOSWAP
}

/* This dered's the phsh */
static void
phsh_dered(GdkPixbuf *gp)
{
	guchar *pixels = gdk_pixbuf_get_pixels (gp);
	int x, y;
	int rs = gdk_pixbuf_get_rowstride (gp);
	for (y = 0; y < 40; y++, pixels += rs) {
		guchar *p = pixels;
		for (x = 0; x < 60; x++, p+=4) {
			if (p[0] < 55 && p[1] > 100)
			       p[3] = 0;	
		}
	}
}

static GdkFilterReturn
phsh_filter (GdkXEvent *gdk_xevent, GdkEvent *event, gpointer data)
{
	XEvent *xevent = (XEvent *)gdk_xevent;

	if (xevent->type == ButtonPress &&
	    ! phsh.hide_mode) {
		gtk_timeout_remove (phsh.handler);
		phsh.handler = gtk_timeout_add (90, phsh_move, NULL);
		phsh.xs *= 2.0;
		phsh.ys *= 2.5;
		phsh.hide_mode = TRUE;
		if (phsh.x < (gdk_screen_width () / 2))
			phsh.xs *= -1;
	}
	return GDK_FILTER_CONTINUE;
}

/* this checks the screen */
static void
check_screen (void)
{
	GdkWindowAttr attributes;
	char *phsh_file;
	char *name;
	GdkPixbuf *gp, *tmp;

	if (phsh.win != NULL)
		return;

	name = g_strdup_printf ("%cish/%cishanim.png",
				'f', 'f');

	phsh_file = gnome_program_locate_file (NULL, GNOME_FILE_DOMAIN_PIXMAP,
					       name, TRUE, NULL);

	g_free (name);

	if (!phsh_file)
		return;

	tmp = gdk_pixbuf_new_from_file (phsh_file, NULL);
	if (tmp == NULL)
		return;

	g_free (phsh_file);

	if (gdk_pixbuf_get_width (tmp) != 180 ||
	    gdk_pixbuf_get_height (tmp) != 40) {
		gdk_pixbuf_unref (tmp);
		return;
	}

	phsh.state = 0;
	phsh.hide_mode = FALSE;

	gp = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, 60, 40);
	gdk_pixbuf_copy_area (tmp, 60, 0, 60, 40, gp, 0, 0);

	phsh_dered (gp);
	gdk_pixbuf_render_pixmap_and_mask (gp, &phsh.phsh[2], &phsh.phsh_mask[2], 128);
	phsh_reverse (gp);
	gdk_pixbuf_render_pixmap_and_mask (gp, &phsh.phsh[0], &phsh.phsh_mask[0], 128);

	gdk_pixbuf_copy_area (tmp, 120, 0, 60, 40, gp, 0, 0);

	phsh_dered (gp);
	gdk_pixbuf_render_pixmap_and_mask (gp, &phsh.phsh[3], &phsh.phsh_mask[3], 128);
	phsh_reverse (gp);
	gdk_pixbuf_render_pixmap_and_mask (gp, &phsh.phsh[1], &phsh.phsh_mask[1], 128);
	gdk_pixbuf_unref (gp);

	gdk_pixbuf_unref (tmp);
	
	phsh.x = -60;
	phsh.y = (rand() % (gdk_screen_height () - 40 - 2)) + 1;
	phsh.xs = 8;
	phsh.ys = (rand() % 2) + 1;

	attributes.window_type = GDK_WINDOW_TEMP;
	attributes.x = phsh.x;
	attributes.y = phsh.y;
	attributes.width = 60;
	attributes.height = 40;
	attributes.wclass = GDK_INPUT_OUTPUT;
	attributes.visual = gdk_rgb_get_visual();
	attributes.colormap = gdk_rgb_get_cmap();
	attributes.event_mask = GDK_BUTTON_PRESS_MASK;

	phsh.win = gdk_window_new (NULL, &attributes,
				   GDK_WA_X | GDK_WA_Y |
				   GDK_WA_VISUAL | GDK_WA_COLORMAP);
	gdk_window_set_back_pixmap (phsh.win, phsh.phsh[0], FALSE);
	gdk_window_shape_combine_mask (phsh.win, phsh.phsh_mask[0], 0, 0);

	/* setup the keys filter */
	gdk_window_add_filter (phsh.win,
			       phsh_filter,
			       NULL);

	gdk_window_show (phsh.win);
	phsh.handler = gtk_timeout_add (150, phsh_move, NULL);
}

static guint screen_check_id = 0;

static gboolean
check_screen_timeout (gpointer data)
{
	screen_check_id = 0;

	check_screen ();

	screen_check_id = gtk_timeout_add (rand()%120*1000,
					   check_screen_timeout, NULL);
	return FALSE;
}

void
start_screen_check (void)
{
	if (screen_check_id > 0)
		gtk_timeout_remove (screen_check_id);

	screen_check_id = 0;
	check_screen ();

	screen_check_id = gtk_timeout_add (rand()%120*1000, check_screen_timeout, NULL);
}

static int
try_config_sync(gpointer data)
{
	panel_config_sync();
	return TRUE;
}

/* Note: similar function is in gnome-desktop-item !!! */

static void
find_kde_directory (void)
{
	int i;
	const char *kdedir = g_getenv ("KDEDIR");
	char *try_prefixes[] = {
		"/usr",
		"/opt/kde",
		"/usr/local",
		"/kde",
		NULL
	};
	if (kdedir != NULL) {
		kde_menudir = g_build_filename (kdedir, "share", "applnk", NULL);
		kde_icondir = g_build_filename (kdedir, "share", "icons", NULL);
		kde_mini_icondir = g_build_filename (kdedir, "share", "icons", "mini", NULL);
		return;
	}

	/* if what configure gave us works use that */
	if (g_file_test (KDE_MENUDIR, G_FILE_TEST_IS_DIR)) {
		kde_menudir = g_strdup (KDE_MENUDIR);
		kde_icondir = g_strdup (KDE_ICONDIR);
		kde_mini_icondir = g_strdup (KDE_MINI_ICONDIR);
		return;
	}

	for (i = 0; try_prefixes[i] != NULL; i++) {
		char *try;
		try = g_build_filename (try_prefixes[i], "share", "applnk", NULL);
		if (g_file_test (try, G_FILE_TEST_IS_DIR)) {
			kde_menudir = try;
			kde_icondir = g_build_filename (try_prefixes[i], "share", "icons", NULL);
			kde_mini_icondir = g_build_filename (try_prefixes[i], "share", "icons", "mini", NULL);
			return;
		}
		g_free(try);
	}

	/* absolute fallback, these don't exist, but we're out of options
	   here */
	kde_menudir = g_strdup (KDE_MENUDIR);
	kde_icondir = g_strdup (KDE_ICONDIR);
	kde_mini_icondir = g_strdup (KDE_MINI_ICONDIR);
}

static void
setup_visuals (void)
{
	gdk_rgb_init ();
}

static void
kill_free_drawers (void)
{
	GSList *li;
	GSList *to_destroy = NULL;
	
	for (li = panel_list; li != NULL; li = li->next) {
		PanelData *pd = li->data;
		if (DRAWER_IS_WIDGET (pd->panel) &&
		    PANEL_WIDGET (BASEP_WIDGET (pd->panel)->panel)->master_widget == NULL) {
			status_unparent (pd->panel);
			to_destroy = g_slist_prepend (to_destroy, pd->panel);
		}
	}

	g_slist_foreach (to_destroy, (GFunc)gtk_widget_destroy, NULL);
	g_slist_free (to_destroy);
}

static void
tell_user_Im_on_crack (void)
{
	GtkWidget *dialog;

	dialog = gtk_message_dialog_new
	    (NULL /* parent */,
	     0 /* flags */,
	     GTK_MESSAGE_WARNING,
	     GTK_BUTTONS_YES_NO,
	     _("This is the GNOME2 panel, it is likely that it will crash,\n"
	       "destroy configuration, cause another world war, and most\n"
	       "likely just plain not work.  Use at your own risk.\n\n"
	       "Do you really want to run it?"));

	if (gtk_dialog_run (GTK_DIALOG (dialog)) != GTK_RESPONSE_YES)
		exit (0);

	gtk_widget_destroy (dialog);
}
static void
session_notify_global_changes (GConfClient *client, guint cnxn_id, GConfEntry *entry, gpointer user_data) {
	session_read_global_config ();
}

int
main(int argc, char **argv)
{
	gchar *real_global_path;
	
	bindtextdomain (GETTEXT_PACKAGE, GNOMELOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	gnome_program_init ("panel", VERSION,
			    LIBGNOMEUI_MODULE,
			    argc, argv,
			    GNOME_PROGRAM_STANDARD_PROPERTIES,
			    NULL);

	bonobo_activate ();
	
	if (g_getenv ("I_LOVE_PANEL_CRACK") == NULL)
		tell_user_Im_on_crack ();

	/*
	 * Let applets spew.
	 */
	putenv ("BONOBO_ACTIVATION_DEBUG_OUTPUT=1");

	if (!panel_shell_register ())
		return -1;

	setup_visuals ();

	find_kde_directory();

	client = gnome_master_client ();

	if (g_getenv ("I_LOVE_PANEL_CRACK") == NULL)
		gnome_client_set_restart_style (client, GNOME_RESTART_IMMEDIATELY);

	gnome_client_set_priority (client, 40);


	if (gnome_client_get_flags (client) & GNOME_CLIENT_RESTORED)
		old_panel_cfg_path = g_strdup (gnome_client_get_config_prefix (client));
	else
		old_panel_cfg_path = g_strdup (PANEL_CONFIG_PATH);

#ifndef PER_SESSION_CONFIGURATION
	real_global_path = gnome_config_get_real_path (old_panel_cfg_path);
	if ( ! g_file_test (real_global_path, G_FILE_TEST_EXISTS)) {
		g_free (old_panel_cfg_path);
		old_panel_cfg_path = g_strdup (PANEL_CONFIG_PATH);
	}
	g_free (real_global_path);
#endif /* !PER_SESSION_CONFIGURATION */

	gnome_client_set_global_config_prefix (client, PANEL_CONFIG_PATH);
	
	g_signal_connect (G_OBJECT (client), "save_yourself",
			  G_CALLBACK (panel_session_save), NULL);
	g_signal_connect (G_OBJECT (client), "die",
			  G_CALLBACK (panel_session_die), NULL);

	panel_tooltips = gtk_tooltips_new ();

	xstuff_init ();
	multiscreen_init ();

	load_system_wide ();

	panel_gconf_add_dir ("/apps/panel/global");

	/* set the globals, it is important this is before
	 * init_user_applets */
	session_read_global_config ();

	/* Do the notification stuff */
	panel_gconf_notify_add ("/apps/panel/global", session_notify_global_changes, NULL);

	/* this is so the capplet gets the right defaults */
	if ( ! commie_mode)
		session_write_global_config ();

	init_menus ();

	session_load ();	

	kill_free_drawers ();

	load_tornoff ();

	/*add forbidden lists to ALL panels*/
	g_slist_foreach (panels,
			 (GFunc)panel_widget_add_forbidden,
			 NULL);

	/*this will make the drawers be hidden for closed panels etc ...*/
	send_state_change ();

	/*attempt to sync the config every 10 seconds, only if a change was
	  indicated though*/
	config_sync_timeout = gtk_timeout_add (10*1000, try_config_sync, NULL);

	/* add some timeouts */
	gtk_timeout_add (10*1000, menu_age_timeout, NULL);

	status_applet_create_offscreen ();

	gtk_main ();

	return 0;
}
