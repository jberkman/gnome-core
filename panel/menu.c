/*
 * GNOME panel menu module.
 * (C) 1997 The Free Software Foundation
 *
 * Authors: Miguel de Icaza
 *          Federico Mena
 */

#include <config.h>
#include <stdio.h>
#include <sys/stat.h>
#include <dlfcn.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include "gnome.h"
#include "panel_cmds.h"
#include "panel-widget.h"
#include "applet_cmds.h"
#include "panel.h"

#include "libgnomeui/gnome-session.h"


#define MENU_PATH "menu_path"

static char *gnome_folder;

static GList *small_icons = NULL;
int show_small_icons = TRUE;

static PanelCmdFunc panel_cmd_func;

typedef struct {
	char *translated;
	char *original_id;
} AppletItem;


void
activate_app_def (GtkWidget *widget, void *data)
{
	GnomeDesktopEntry *item = data;

	gnome_desktop_entry_launch (item);
}

void
setup_menuitem (GtkWidget *menuitem, GtkWidget *pixmap, char *title)
{
	GtkWidget *label, *hbox, *align;

	label = gtk_label_new (title);
	gtk_misc_set_alignment (GTK_MISC(label), 0.0, 0.5);
	gtk_widget_show (label);
	
	hbox = gtk_hbox_new (FALSE, 0);
	
	align = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
	gtk_container_border_width (GTK_CONTAINER (align), 1);

	gtk_box_pack_start (GTK_BOX (hbox), align, FALSE, FALSE, 0);

	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 4);
	gtk_container_add (GTK_CONTAINER (menuitem), hbox);

	if (pixmap) {
		gtk_container_add (GTK_CONTAINER (align), pixmap);
		gtk_widget_set_usize (align, 22, 16);
	} else
		gtk_widget_set_usize (align, 22, 16);

	*small_icons = g_list_prepend (*small_icons, align);

	gtk_widget_show (align);
	gtk_widget_show (hbox);
	gtk_widget_show (menuitem);
}

void
free_app_def (GtkWidget *widget, void *data)
{
	GnomeDesktopEntry *item = data;

	gnome_desktop_entry_free (item);
}

void
add_menu_separator (GtkWidget *menu)
{
	GtkWidget *menuitem;
	
	menuitem = gtk_menu_item_new ();
	gtk_widget_show (menuitem);
	gtk_container_add (GTK_CONTAINER (menu), menuitem);
}

void
free_string (GtkWidget *widget, void *data)
{
	g_free (data);
}

void
add_to_panel (char *applet, char *arg)
{
	PanelCommand cmd;
	
	cmd.cmd = PANEL_CMD_CREATE_APPLET;
	cmd.params.create_applet.id     = applet;
	cmd.params.create_applet.params = arg;
	cmd.params.create_applet.pos    = PANEL_UNKNOWN_APPLET_POSITION;
	cmd.params.create_applet.panel  = 0; /*FIXME: maybe add to some
						default*/

	(*panel_cmd_func) (&cmd);
}

void
add_app_to_panel (GtkWidget *widget, void *data)
{
	GnomeDesktopEntry *ii = data;

	add_to_panel ("Launcher", ii->location);
}

void
add_dir_to_panel (GtkWidget *widget, void *data)
{
	add_to_panel (APPLET_ID, data);
}

void
add_drawer (GtkWidget *widget, void *data)
{
	PanelCommand cmd;
	
	cmd.cmd = PANEL_CMD_CREATE_DRAWER;
	cmd.params.create_drawer.name       = "Drawer";
	cmd.params.create_drawer.iconopen   = "???";
	cmd.params.create_drawer.iconclosed = "???";
	cmd.params.create_drawer.step_size  = PANEL_UNKNOWN_STEP_SIZE;
	cmd.params.create_drawer.pos        = PANEL_UNKNOWN_APPLET_POSITION;
	cmd.params.create_drawer.panel      = 0;

	(*panel_cmd_func) (&cmd);
}


GtkWidget *
create_menu_at (GtkWidget *window, char *menudir, int create_app_menu);
{	
	GnomeDesktopEntry *item_info;
	GtkWidget *menu;
	struct dirent *dent;
	struct stat s;
	char *filename;
	DIR *dir;
	int items = 0;
	
	dir = opendir (menudir);
	if (dir == NULL)
		return NULL;

	menu = gtk_menu_new ();
	
	while ((dent = readdir (dir)) != NULL) {
		GtkWidget     *menuitem, *sub, *pixmap;
		GtkSignalFunc  activate_func;
		char          *thisfile, *pixmap_name;
		char          *p;
		char          *menuitem_name;

		thisfile = dent->d_name;
		/* Skip over . and .. */
		if ((thisfile [0] == '.' && thisfile [1] == 0) ||
		    (thisfile [0] == '.' && thisfile [1] == '.' && thisfile [2] == 0))
			continue;

		filename = g_concat_dir_and_file (menudir, thisfile);
		if (stat (filename, &s) == -1) {
			g_free (filename);
			continue;
		}

		sub = 0;
		item_info = 0;
		if (S_ISDIR (s.st_mode)) {
			char *dentry_name;

			sub = create_menu_at (window, filename,
					      create_app_menu, small_icons);
			if (!sub) {
				g_free (filename);
				continue;
			}

			dentry_name = g_concat_dir_and_file (filename, ".directory");
			item_info = gnome_desktop_entry_load (dentry_name);
			g_free (dentry_name);

			if (item_info)
				menuitem_name = item_info->name;
			else
				menuitem_name = thisfile;

			/* just for now */
			pixmap_name = NULL;

			if (create_app_menu) {
				GtkWidget *pixmap = NULL;
				char *text;
				char *dirname;

				/* create separator */

				menuitem = gtk_menu_item_new ();
				gtk_menu_prepend (GTK_MENU (sub), menuitem);
				gtk_widget_show (menuitem);

				/* create menu item */

				menuitem = gtk_menu_item_new ();
				if (gnome_folder) {
					pixmap = gnome_create_pixmap_widget (window, menuitem, gnome_folder);
					gtk_widget_show (pixmap);
				}

				text = g_copy_strings ("Menu: ", menuitem_name, NULL);
				setup_menuitem (menuitem, pixmap, text, small_icons);
				g_free (text);
				
				dirname = g_strdup (filename);
				gtk_menu_prepend (GTK_MENU (sub), menuitem);
				gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
						    (GtkSignalFunc) add_dir_to_panel,
						    dirname);
				gtk_signal_connect (GTK_OBJECT (menuitem), "destroy",
						    (GtkSignalFunc) free_string,
						    dirname);
			}
		} else {
			if (strstr (filename, ".desktop") == 0) {
				g_free (filename);
				continue;
			}
			item_info = gnome_desktop_entry_load (filename);
			if (!item_info) {
				g_free (filename);
				continue;
			}
			menuitem_name = item_info->name;
			pixmap_name = item_info->small_icon;
		}
		
		items++;
		
		menuitem = gtk_menu_item_new ();
		if (sub)
			gtk_menu_item_set_submenu (GTK_MENU_ITEM(menuitem), sub);

		pixmap = NULL;
		if (pixmap_name && g_file_exists (pixmap_name)) {
			pixmap = gnome_create_pixmap_widget (window, menuitem,
							     pixmap_name);
			if (pixmap)
				gtk_widget_show (pixmap);
		}

 		setup_menuitem (menuitem, pixmap, menuitem_name, small_icons);
		gtk_menu_append (GTK_MENU (menu), menuitem);

		gtk_signal_connect (GTK_OBJECT (menuitem), "destroy",
				    (GtkSignalFunc) free_app_def, item_info);

		activate_func = create_app_menu ? (GtkSignalFunc) add_app_to_panel : (GtkSignalFunc) activate_app_def;
		gtk_signal_connect (GTK_OBJECT (menuitem), "activate", activate_func, item_info);

		g_free (filename);
	}
	closedir (dir);

	if (items == 0) {
		gtk_widget_destroy (menu);
		menu = NULL;
	}
	
	return menu;
}

void
menu_position (GtkMenu *menu, gint *x, gint *y, gpointer data)
{
	Menu * menu = data;
	GtkWidget *widget = menu->button;
	int wx, wy;
	
	gdk_window_get_origin (widget->window, &wx, &wy);

	switch(menu->oerientation) {
		case MENU_DOWN:
			*x = wx;
			*y = wy + widget->allocation.height;
			break;
		case MENU_UP:
			*x = wx;
			*y = wy - GTK_WIDGET (menu)->allocation.height;
			break;
		case MENU_RIGHT:
			*x = wx + widget->allocation.width;
			*y = wy;
			break;
		case MENU_LEFT:
			*x = wx - GTK_WIDGET (menu)->allocation.width;
			*y = wy;
			break;
	}
	if(*x + GTK_WIDGET (menu)->allocation.width > gdk_screen_width())
		*x=gdk_screen_width() - GTK_WIDGET (menu)->allocation.width;
	if(*y + GTK_WIDGET (menu)->allocation.height > gdk_screen_height())
		*y=gdk_screen_height() - GTK_WIDGET (menu)->allocation.height;
}

void
activate_menu (GtkWidget *widget, gpointer data)
{
	Menu *menu = data;

	gtk_menu_popup (GTK_MENU (menu->menu), 0, 0, menu_position, data,
			1, 0);
}

void
panel_configure (GtkWidget *widget, void *data)
{
	PanelCommand cmd;

	cmd.cmd = PANEL_CMD_PROPERTIES;

	(*panel_cmd_func) (&cmd);
}

/* FIXME: panel is dynamicly configured! so we shouldn't need this*/
/*
void
panel_reload (GtkWidget *widget, void *data)
{
	fprintf(stderr, "Panel reload not yet implemented\n");
}*/

static AppletItem *
applet_item_new(char *translated, char *original_id)
{
	AppletItem *ai;

	ai = g_new(AppletItem, 1);
	ai->translated = translated;
	ai->original_id = original_id;

	return ai;
}

static void
applet_item_destroy(AppletItem *ai)
{
	g_free(ai->translated);
	g_free(ai->original_id);
	g_free(ai);
}

static void
add_applet_to_panel(GtkWidget *widget, gpointer data)
{
	add_to_panel(gtk_object_get_user_data(GTK_OBJECT(widget)),
		     NULL); /* NULL means request default params */
}

static void
munge_applet_item(gpointer untrans, gpointer user_data)
{
	GList      **list;
	GList       *node;
	int          pos;
	char        *trans;

	list = user_data;

	/* Insert applet id in alphabetical order */
	
	node = *list;
	pos  = 0;

	trans = _(untrans);
	
	for (pos = 0; node; node = node->next, pos++)
		if (strcmp(trans, _(node->data)) < 0)
			break;

	*list = g_list_insert(*list,
			      applet_item_new(g_strdup(trans), untrans),
			      pos);
}

static void
append_applet_item_to_menu(gpointer data, gpointer user_data)
{
	GtkMenu    *menu;
	GtkWidget  *menuitem;
	AppletItem *ai;
	char       *oid;

	ai = data;
	menu = GTK_MENU(user_data);

	oid = g_strdup(ai->original_id);

	menuitem = gtk_menu_item_new();
	setup_menuitem(menuitem, NULL, ai->translated);
	gtk_object_set_user_data(GTK_OBJECT(menuitem), oid);
	gtk_signal_connect(GTK_OBJECT(menuitem), "activate",
			   (GtkSignalFunc) add_applet_to_panel,
			   NULL);
	gtk_signal_connect(GTK_OBJECT(menuitem), "destroy",
			   (GtkSignalFunc) free_string,
			   oid);
	
	gtk_menu_append(menu, menuitem);

	/* Free applet item */

	applet_item_destroy(ai);
}

static GtkWidget *
create_applets_menu(void)
{
	GtkWidget    *menu;
	GList        *list;
	GList        *applets_list;
	PanelCommand  cmd;

	/* Get list of applet types */

	cmd.cmd = PANEL_CMD_GET_APPLET_TYPES;
	list = (*panel_cmd_func) (&cmd);

	/* Now translate and sort them */

	applets_list = NULL;
	g_list_foreach(list, munge_applet_item, &applets_list);

	/* Create a menu of the translated and sorted ones */

	g_list_free(list);

	menu = gtk_menu_new();

	g_list_foreach(applets_list, append_applet_item_to_menu, menu);

	/* Destroy the list (the list items have already been freed by
	 * append_applet_item_to_menu()), and return the finished menu.
	 */

	g_list_free(applets_list);
	return menu;
}

static GtkWidget *
create_panel_submenu (GtkWidget *app_menu)
{
	GtkWidget *menu, *menuitem;

	menu = gtk_menu_new ();
	
	menuitem = gtk_menu_item_new ();
	setup_menuitem (menuitem, 0, _("Add to panel"));
	gtk_menu_append (GTK_MENU (menu), menuitem);
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem), app_menu);

	menuitem = gtk_menu_item_new ();
	setup_menuitem (menuitem, 0, _("Add applet"));
	gtk_menu_append (GTK_MENU (menu), menuitem);
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem),
		create_applets_menu(small_icons));

	menuitem = gtk_menu_item_new ();
	setup_menuitem (menuitem, 0, _("Add Drawer"));
	gtk_menu_append (GTK_MENU (menu), menuitem);
        gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
			    (GtkSignalFunc) add_drawer, 0);

	add_menu_separator(menu);
	
	menuitem = gtk_menu_item_new ();
	setup_menuitem (menuitem, 0, _("Configure"));
	gtk_menu_append (GTK_MENU (menu), menuitem);
        gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
			    (GtkSignalFunc) panel_configure, 0);

	/*FIXME: this is not needed, or is it?, so take it out unless we
	  do need it!
	*/
	/*menuitem = gtk_menu_item_new ();
	setup_menuitem (menuitem, 0, _("Reload configuration"));
	gtk_menu_append (GTK_MENU (menu), menuitem);
        gtk_signal_connect (GTK_OBJECT (menuitem), "activate", (GtkSignalFunc) panel_reload, 0);*/

	return menu;
}

static void
panel_lock (GtkWidget *widget, void *data)
{
	system ("gnome-lock");
}

static void
panel_logout (GtkWidget *widget, void *data)
{
	PanelCommand cmd;

	cmd.cmd = PANEL_CMD_QUIT;
	(*panel_cmd_func) (&cmd);
}

static void
add_special_entries (GtkWidget *menu, GtkWidget *app_menu)
{
	GtkWidget *menuitem;
	
	/* Panel entry */

	add_menu_separator (menu);

	menuitem = gtk_menu_item_new ();
	setup_menuitem (menuitem, 0, _("Panel"));
	gtk_menu_append (GTK_MENU (menu), menuitem);
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem), create_panel_submenu (app_menu));

	add_menu_separator (menu);
	
	menuitem = gtk_menu_item_new ();
	setup_menuitem (menuitem, 0, _("Lock screen"));
	gtk_menu_append (GTK_MENU (menu), menuitem);
	gtk_signal_connect (GTK_OBJECT (menuitem), "activate", (GtkSignalFunc) panel_lock, 0);

	menuitem = gtk_menu_item_new ();
	setup_menuitem (menuitem, 0, _("Log out"));
	gtk_menu_append (GTK_MENU (menu), menuitem);
	gtk_signal_connect (GTK_OBJECT (menuitem), "activate", (GtkSignalFunc) panel_logout, 0);
}

static GtkWidget *
create_panel_menu (GtkWidget *window, char *menudir, int main_menu)
{
	GtkWidget *button;
	GtkWidget *pixmap;
	GtkWidget *menu;
	GtkWidget *app_menu;
	
	char *pixmap_name;

	if (main_menu)
		switch(panel_snapped) {
			case PANEL_TOP:
				pixmap_name = gnome_unconditional_pixmap_file ("gnome-menu-down.xpm");
				break;
			case PANEL_FREE:
			case PANEL_BOTTOM:
				pixmap_name = gnome_unconditional_pixmap_file ("gnome-menu-up.xpm");
				break;
			case PANEL_LEFT:
				pixmap_name = gnome_unconditional_pixmap_file ("gnome-menu-right.xpm");
				break;
			case PANEL_RIGHT:
				pixmap_name = gnome_unconditional_pixmap_file ("gnome-menu-left.xpm");
				break;
		}
	else
		/*FIXME: these guys need arrows as well*/
		pixmap_name = gnome_unconditional_pixmap_file ("panel-folder.xpm");
		
	/* main button */
	button = gtk_button_new ();
	
	/*make the pixmap*/
	pixmap = gnome_create_pixmap_widget (window, button, pixmap_name);
	gtk_widget_show(pixmap);
	gtk_widget_set_usize (button, pixmap->requisition.width,
			      pixmap->requisition.height);

	/* put pixmap in button */
	gtk_container_add (GTK_CONTAINER(button), pixmap);
	gtk_widget_show (button);

	menu = create_menu_at (window, menudir, 0);
	if (main_menu) {
		app_menu = create_menu_at (window, menudir, 1);
		add_special_entries (menu, app_menu);
	}
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
			    GTK_SIGNAL_FUNC (activate_menu), menu);

	g_free (pixmap_name);
	return button;
}

static GtkWidget *
create_menu_widget (GtkWidget *window, char *arguments, char *menudir)
{
	GtkWidget *menu;
	int main_menu;
	
	main_menu = (strcmp (arguments, ".") == 0);
	menu = create_panel_menu (window, menudir, main_menu);
	return menu;
}

static void
set_show_small_icons(gpointer data, gpointer user_data)
{
	GtkWidget *w = data;
	if (!w) {
		g_warning("Internal error in set_show_small_icons (!w)");
		return;
	}
	if (*(int *)user_data)
		gtk_widget_show(w);
	else
		gtk_widget_hide(w);
}

static void
create_instance (PanelWidget *panel, char *params, int pos, int panelnum)
{
	char *menu_base = gnome_unconditional_datadir_file ("apps");
	char *this_menu;
	char *p;
	Menu *menu;
	PanelCommand cmd;
	int show_small_icons;

	if (!getenv ("PATH"))
		return;

	if(!params)
		return;

	/*parse up the params*/
	p = strchr(params,':');
	show_small_icons = TRUE;
	if (p) {
		*(p++)='\0';
		if(*(p++)=='0')
			show_small_icons = FALSE;
	}

	if (*params == '/')
		this_menu = strdup (params);
	else 
		this_menu = g_concat_dir_and_file (menu_base, params);

	if (!g_file_exists (this_menu)) {
		g_free (menu_base);
		g_free (this_menu);
		return;
	}

	gnome_folder = gnome_unconditional_pixmap_file ("gnome-folder-small.xpm");
	if (!g_file_exists (gnome_folder)) {
		free (gnome_folder);
		gnome_folder = NULL;
	}

	menu = g_new(Menu,1);
	menu->button = create_menu_widget (GTK_WIDGET(panel), params,
					   this_menu);
	menu->path = g_strdup(params);

	g_list_foreach(small_icons,set_show_small_icons, &show_small_icons);
	
	gtk_object_set_user_data(GTK_OBJECT(menu->button),menu);
	
	cmd.cmd = PANEL_CMD_REGISTER_TOY;
	cmd.params.register_toy.applet = menu->button;
	cmd.params.register_toy.id     = APPLET_ID;
	cmd.params.register_toy.pos    = pos;
	cmd.params.register_toy.panel  = panelnum;
	cmd.params.register_toy.flags  = APPLET_HAS_PROPERTIES;

	(*panel_cmd_func) (&cmd);
}

static void
set_orientation(GtkWidget *applet, PanelWidget *panel, PanelSnapped snapped)
{
	GtkWidget *pixmap;
	char *pixmap_name;
	Menu *menu;

	panel_snapped = snapped; /*FIXME: this should probably be in the structure*/

	menu = gtk_object_get_user_data(GTK_OBJECT(applet));
	if(!menu || !menu->path)
		return;

	if (strcmp (menu->path, ".") == 0)
		switch (panel_snapped) {
			case PANEL_TOP:
				pixmap_name = gnome_unconditional_pixmap_file(
					"gnome-menu-down.xpm");
				break;
			case PANEL_FREE:
			case PANEL_BOTTOM:
				pixmap_name = gnome_unconditional_pixmap_file(
					"gnome-menu-up.xpm");
				break;
			case PANEL_LEFT:
				pixmap_name = gnome_unconditional_pixmap_file(
					"gnome-menu-right.xpm");
				break;
			case PANEL_RIGHT:
				pixmap_name = gnome_unconditional_pixmap_file(
					"gnome-menu-left.xpm");
				break;
		}
	else
		/*FIXME: these guys need arrows as well*/
		pixmap_name = gnome_unconditional_pixmap_file ("panel-folder.xpm");
		
	pixmap=GTK_BUTTON(applet)->child;
	gtk_container_remove(GTK_CONTAINER(applet),pixmap);
	gtk_widget_destroy(pixmap);

	/*make the pixmap*/
	pixmap = gnome_create_pixmap_widget (GTK_WIDGET(panel),applet,pixmap_name);

	gtk_container_add (GTK_CONTAINER(applet), pixmap);
	gtk_widget_show (pixmap);
	
	g_free(pixmap_name);
}
