/* Gnome panel: general applet functionality
 * (C) 1997 the Free Software Foundation
 *
 * Authors:  George Lebl
 *           Federico Mena
 *           Miguel de Icaza
 */

#include <config.h>
#include <string.h>
#include <signal.h>
#include <libgnome/libgnome.h>
#include <libbonobo.h>
#include <gdk/gdkx.h>

#include "basep-widget.h"
#include "button-widget.h"
#include "drawer.h"
#include "extern.h"
#include "launcher.h"
#include "menu-properties.h"
#include "menu-util.h"
#include "panel_config.h"
#include "panel_config_global.h"
#include "session.h"
#include "status.h"
#include "swallow.h"

#define SMALL_ICON_SIZE 20

extern GSList *panels;

GSList *applets = NULL;
GSList *applets_last = NULL;
int applet_count = 0;

/*config sync stuff*/
extern int applets_to_sync;
extern int need_complete_save;

extern gboolean commie_mode;
extern GlobalConfig global_config;

static void
move_applet_callback (GtkWidget *widget, AppletInfo *info)
{
	PanelWidget *panel;

	g_return_if_fail (info != NULL);
	g_return_if_fail (info->widget != NULL);
	g_return_if_fail (info->widget->parent != NULL);
	g_return_if_fail (IS_PANEL_WIDGET (info->widget->parent));

	panel = PANEL_WIDGET (info->widget->parent);

	panel_widget_applet_drag_start (panel, info->widget,
					PW_DRAG_OFF_CENTER);
}

/*destroy widgets and call the above cleanup function*/
void
panel_clean_applet (AppletInfo *info)
{
	g_return_if_fail (info != NULL);

	if (info->widget != NULL) {
		if(info->type == APPLET_STATUS) {
			status_applet_put_offscreen (info->data);
		}
		/* destroy will remove it from the panel */
		gtk_widget_destroy (info->widget);
		info->widget = NULL;
	}

	info->data = NULL;
}

static gboolean
kill_applet_in_idle(gpointer data)
{
	AppletInfo *info = data;

	if (info->type == APPLET_EXTERN)
		extern_save_last_position ((Extern)info->data, TRUE);

	else if (info->type == APPLET_SWALLOW) {
		Swallow *swallow = info->data;

		swallow->clean_remove = TRUE;
	}

	panel_clean_applet (info);

	return FALSE;
}

static void
remove_applet_callback(GtkWidget *widget, AppletInfo *info)
{
	gtk_idle_add (kill_applet_in_idle, info);
}

static void
applet_callback_callback(GtkWidget *widget, gpointer data)
{
	AppletUserMenu *menu = data;

	g_return_if_fail(menu->info != NULL);

	switch(menu->info->type) {
	case APPLET_EXTERN:
		extern_handle_do_callback ((Extern)menu->info->data,
					   menu->name);
		break;
	case APPLET_LAUNCHER:
		if (strcmp (menu->name, "properties") == 0) {
			launcher_properties (menu->info->data);
		} else if (strcmp (menu->name, "help") == 0) {
			panel_show_help ("launchers", NULL);
		} else if (strcmp (menu->name, "help_on_app") == 0) {
			Launcher * launcher = menu->info->data;
			if (launcher->ditem != NULL) {
				const char *docpath =
					gnome_desktop_item_get_string
					    (launcher->ditem, "DocPath");
				char *path =
					panel_gnome_kde_help_path (docpath);
				if (path != NULL) {
					gnome_url_show (path, NULL);
					/* FIXME: handle errors */
					g_free (path);
				}
			}
		}
		break;
	case APPLET_DRAWER: 
		if (strcmp (menu->name, "properties")==0) {
			Drawer *drawer = menu->info->data;
			g_assert(drawer);
			panel_config(drawer->drawer);
		} else if (strcmp (menu->name, "help") == 0) {
			panel_show_help ("drawers", NULL);
		}
		break;
	case APPLET_SWALLOW:
 		if (strcmp (menu->name, "help") == 0)
			panel_show_help ("specialobjects", "SWALLOWEDAPP");
#if 0
		if(strcmp(menu->name,"properties")==0) {
			Swallow *swallow = info->data;
			g_assert(swallow);
			swallow_properties(swallow); /* doesn't exist yet*/
		}
#endif
		break; 
	case APPLET_MENU:
		if(strcmp(menu->name,"properties")==0)
			menu_properties(menu->info->data);
		else if(strcmp(menu->name,"edit_menus")==0) {
			char *tmp;
			if((tmp = gnome_is_program_in_path("gmenu")))  {
				gnome_execute_async (g_get_home_dir (), 1, &tmp);
				g_free(tmp);
			}
		} else if (strcmp (menu->name, "help") == 0) {
			Menu *menu2 = menu->info->data;
			char *page;
			page = (menu2->path && strcmp (menu2->path,"."))
				? "menus" : "mainmenu";
			panel_show_help (page, NULL);
		}
		break;
	case APPLET_LOCK: {
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
		char *command = NULL;
		gboolean freeit = FALSE;
		if (strcmp (menu->name, "help") == 0)
			panel_show_help ("specialobjects", "LOCKBUTTON");
		else if (!strcmp (menu->name, "restart")) {
			command = "xscreensaver-command -exit ; xscreensaver &";
		} else if (!strcmp (menu->name, "prefs")) {
			command = "xscreensaver-demo";
		} else {
			command = g_strdup_printf ("xscreensaver-command -%s",
						   menu->name);
			freeit = TRUE;
		}
		if (command)
			gnome_execute_shell (g_get_home_dir (), command);
		if (freeit)
			g_free (command);
		break;
	}
	case APPLET_LOGOUT:
		if (strcmp (menu->name, "help") == 0)
			panel_show_help ("specialobjects", "LOGOUTBUTTON");
		break;
	case APPLET_STATUS:
		if (strcmp (menu->name, "help") == 0)
			panel_show_help ("specialobjects", "STATUSDOC");
		break;
	case APPLET_RUN:
		if (strcmp (menu->name, "help") == 0)
			panel_show_help ("specialobjects", "RUNBUTTON");
		break;
	default: break;
	}
}

static void
applet_menu_deactivate(GtkWidget *w, AppletInfo *info)
{
	GtkWidget *panel = get_panel_parent(info->widget);
	info->menu_age = 0;
	
	if(BASEP_IS_WIDGET(panel))
		BASEP_WIDGET(panel)->autohide_inhibit = FALSE;
}

AppletUserMenu *
applet_get_callback (GList      *user_menu,
		     const char *name)
{
	GList *l;

	for (l = user_menu; l; l = l->next) {
		AppletUserMenu *menu = l->data;

		if (!strcmp (menu->name, name))
			return menu;
	}

	return NULL;	
}

void
applet_add_callback(AppletInfo *info,
		    const char *callback_name,
		    const char *stock_item,
		    const char *menuitem_text)
{
	AppletUserMenu *menu;

	g_return_if_fail (info != NULL);
	
	menu = applet_get_callback (info->user_menu, callback_name);
	if (menu == NULL) {
		menu = g_new0 (AppletUserMenu, 1);
		menu->name = g_strdup (callback_name);
		menu->stock_item = g_strdup (stock_item);
		menu->text = g_strdup (menuitem_text);
		menu->sensitive = TRUE;
		menu->info = info;
		menu->menuitem = NULL;
		menu->submenu = NULL;
		info->user_menu = g_list_append (info->user_menu, menu);
	} else {
		g_free (menu->stock_item);
		menu->stock_item = NULL;
		g_free (menu->text);
		menu->text = NULL;
		menu->text = g_strdup (menuitem_text);
		menu->stock_item = g_strdup (stock_item);
	}

	/*make sure the menu is rebuilt*/
	if(info->menu) {
		GList *list;
		for(list=info->user_menu;list!=NULL;list=g_list_next(list)) {
			AppletUserMenu *menu = list->data;
			menu->menuitem=NULL;
			menu->submenu=NULL;
		}
		gtk_widget_unref(info->menu);
		info->menu = NULL;
		info->menu_age = 0;
	}
}

void
applet_remove_callback(AppletInfo *info, const char *callback_name)
{
	AppletUserMenu *menu;

	g_return_if_fail (info != NULL);
	
	menu = applet_get_callback (info->user_menu, callback_name);
	if (menu != NULL) {
		info->user_menu = g_list_remove (info->user_menu, menu);
		g_free(menu->name);
		menu->name = NULL;
		g_free(menu->stock_item);
		menu->stock_item = NULL;
		g_free(menu->text);
		menu->text = NULL;
		g_free(menu);
	} else
		return; /*it just isn't there*/

	/*make sure the menu is rebuilt*/
	if (info->menu != NULL) {
		GList *list;
		for (list = info->user_menu; list != NULL; list = list->next) {
			AppletUserMenu *menu = list->data;
			menu->menuitem = NULL;
			menu->submenu = NULL;
		}
		gtk_widget_unref (info->menu);
		info->menu = NULL;
		info->menu_age = 0;
	}
}

void
applet_callback_set_sensitive(AppletInfo *info, const char *callback_name, int sensitive)
{
	AppletUserMenu *menu;

	g_return_if_fail(info != NULL);
	
	menu = applet_get_callback(info->user_menu, callback_name);
	if (menu != NULL)
		menu->sensitive = sensitive;
	else
		return; /*it just isn't there*/

	/*make sure the menu is rebuilt*/
	if(info->menu) {
		GList *list;
		for(list=info->user_menu;list!=NULL;list=g_list_next(list)) {
			AppletUserMenu *menu = list->data;
			menu->menuitem=NULL;
			menu->submenu=NULL;
		}
		gtk_widget_unref(info->menu);
		info->menu = NULL;
		info->menu_age = 0;
	}
}

static void
setup_an_item(AppletUserMenu *menu,
	      GtkWidget *submenu,
	      int is_submenu)
{
	menu->menuitem = gtk_menu_item_new ();
	gtk_signal_connect (GTK_OBJECT (menu->menuitem), "destroy",
			    GTK_SIGNAL_FUNC (gtk_widget_destroyed),
			    &menu->menuitem);
	if(menu->stock_item && *(menu->stock_item))
		setup_menuitem (menu->menuitem,
				gtk_image_new_from_stock (menu->stock_item,
							  GTK_ICON_SIZE_MENU),
				menu->text);
	else
		setup_menuitem (menu->menuitem,
				NULL,
				menu->text);

	if(submenu)
		gtk_menu_shell_append (GTK_MENU_SHELL (submenu), menu->menuitem);

	/*if an item not a submenu*/
	if(!is_submenu) {
		gtk_signal_connect(GTK_OBJECT(menu->menuitem), "activate",
				   (GtkSignalFunc) applet_callback_callback,
				   menu);
		gtk_signal_connect(GTK_OBJECT (submenu), "destroy",
				   GTK_SIGNAL_FUNC (gtk_widget_destroyed),
				   &menu->submenu);
	/* if the item is a submenu and doesn't have it's menu
	   created yet*/
	} else if(!menu->submenu) {
		menu->submenu = gtk_menu_new ();
	}

	if(menu->submenu) {
		gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu->menuitem),
					  menu->submenu);
		gtk_signal_connect (GTK_OBJECT (menu->submenu), "destroy",
				    GTK_SIGNAL_FUNC (gtk_widget_destroyed),
				    &menu->submenu);
	}
	
	gtk_widget_set_sensitive(menu->menuitem,menu->sensitive);
}

static void
add_to_submenus (AppletInfo *info,
		 const char *path,
		 const char *name,
		 AppletUserMenu *menu,
		 GtkWidget *submenu,
		 GList *user_menu)
{
	char *n = g_strdup (name);
	char *p = strchr (n, '/');
	char *t;
	AppletUserMenu *s_menu;

	/*this is the last one*/
	if (p == NULL) {
		g_free (n);
		setup_an_item (menu, submenu, FALSE);
		return;
	}
	
	/*this is the last one and we are a submenu, we have already been
	  set up*/
	if(p==(n + strlen(n) - 1)) {
		g_free(n);
		return;
	}
	
	*p = '\0';
	p++;
	
	t = g_strconcat (path, n, "/", NULL);
	s_menu = applet_get_callback (user_menu, t);
	/*the user did not give us this sub menu, whoops, will create an empty
	  one then*/
	if (s_menu == NULL) {
		s_menu = g_new0 (AppletUserMenu,1);
		s_menu->name = g_strdup (t);
		s_menu->stock_item = NULL;
		s_menu->text = g_strdup (_("???"));
		s_menu->sensitive = TRUE;
		s_menu->info = info;
		s_menu->menuitem = NULL;
		s_menu->submenu = NULL;
		info->user_menu = g_list_append (info->user_menu,s_menu);
		user_menu = info->user_menu;
	}
	
	if (s_menu->submenu == NULL) {
		s_menu->submenu = gtk_menu_new ();
		/*a more elegant way to do this should be done
		  when I don't want to go to sleep */
		if (s_menu->menuitem != NULL) {
			gtk_widget_destroy (s_menu->menuitem);
			s_menu->menuitem = NULL;
		}
	}
	if (s_menu->menuitem == NULL)
		setup_an_item (s_menu, submenu, TRUE);
	
	add_to_submenus (info, t, p, menu, s_menu->submenu, user_menu);
	
	g_free(t);
	g_free(n);
}

static GtkWidget *
applet_setup_panel_menu (gboolean is_basep)
{
	GtkWidget *menuitem;
	GtkWidget *panel_menu;
	gchar     *pixmap_path;

	menuitem = gtk_menu_item_new ();

	pixmap_path = gnome_program_locate_file (NULL, GNOME_FILE_DOMAIN_PIXMAP, 
						 "gnome-panel.png", TRUE, NULL);
	if (pixmap_path) {
		setup_menuitem (menuitem, 
				gtk_image_new_from_file (pixmap_path),
				_("Panel"));

		g_free (pixmap_path);
	} else {
		g_message (_("Cannot find pixmap file %s"), "gnome-panel.png");

		setup_menuitem (menuitem, NULL, _("Panel"));
	}

	panel_menu = gtk_menu_new ();

	make_panel_submenu (panel_menu, TRUE, is_basep);

	gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem), panel_menu);

	return menuitem;
}

void
create_applet_menu (AppletInfo *info,
		    gboolean    is_basep)
{
	GtkWidget *menuitem;
	GList     *l;

	info->menu = gtk_menu_new ();

	if (!commie_mode) {
		GtkWidget *image;

		menuitem = gtk_menu_item_new ();

		image = gtk_image_new_from_stock (GTK_STOCK_REMOVE,
						  GTK_ICON_SIZE_MENU);

		setup_menuitem (menuitem, image , _("Remove from panel"));

		gtk_signal_connect (GTK_OBJECT (menuitem),
				    "activate",
				    (GtkSignalFunc) remove_applet_callback,
				    info);

		gtk_menu_shell_append (GTK_MENU_SHELL (info->menu), menuitem);

		menuitem = gtk_menu_item_new();

		/*
		 * FIXME: should have a "Move" pixmap.
		 */
		setup_menuitem (menuitem, NULL, _("Move"));

		gtk_signal_connect (GTK_OBJECT (menuitem),
				    "activate",
				    (GtkSignalFunc) move_applet_callback,
				    info);

		gtk_menu_shell_append (GTK_MENU_SHELL (info->menu), menuitem);
	}

	menuitem = applet_setup_panel_menu (is_basep);

	gtk_menu_shell_append (GTK_MENU_SHELL (info->menu), menuitem);

	if (info->user_menu) {
		menuitem = gtk_menu_item_new ();

		gtk_menu_shell_append (GTK_MENU_SHELL (info->menu), menuitem);

		gtk_widget_set_sensitive (menuitem, FALSE);

		gtk_widget_show (menuitem);
	}

	for (l = info->user_menu; l; l = l->next) {
		AppletUserMenu *menu = (AppletUserMenu *)l->data;

		add_to_submenus (info, "", menu->name, menu, 
				 info->menu, info->user_menu);
	}

	/*
	 * connect the deactivate signal, so that we can "re-allow" 
	 * autohide when the menu is deactivated.
	 */
	gtk_signal_connect (GTK_OBJECT (info->menu),
			    "deactivate",
			     GTK_SIGNAL_FUNC (applet_menu_deactivate),
			     info);
}

static void
applet_menu_set_data_recursively (GtkMenu     *menu,
				  const gchar *key,
				  gpointer     data)
{
	GList *children;
	GList *l;

	gtk_object_set_data (GTK_OBJECT (menu), key, data);

	children = gtk_container_children (GTK_CONTAINER (menu));

	for (l = children; l; l = l->next) {
		GtkWidget *submenu = GTK_MENU_ITEM (l->data)->submenu;

		if (submenu)
			applet_menu_set_data_recursively (GTK_MENU (submenu), key, data);
	}

	g_list_free (children);
}

static void
applet_show_menu (AppletInfo     *info,
		  GdkEventButton *event)
{
	GtkWidget *panel;

	g_return_if_fail (info);

	panel = get_panel_parent (info->widget);

	if (!info->menu)
		create_applet_menu (info, BASEP_IS_WIDGET (panel));

	g_assert (info->menu);

	if (BASEP_IS_WIDGET (panel)) {
		BASEP_WIDGET (panel)->autohide_inhibit = TRUE;
		basep_widget_queue_autohide (BASEP_WIDGET (panel));
	}

	info->menu_age = 0;

	applet_menu_set_data_recursively (GTK_MENU (info->menu),
					  "menu_panel",
					  info->widget->parent);

	if (!GTK_WIDGET_REALIZED (info->menu))
		gtk_widget_show (info->menu);

	/* FIXME - off panel popups, should be automatic with new gtk-menu? */
	gtk_menu_popup (GTK_MENU (info->menu),
			NULL,
			NULL,
			applet_menu_position,
			info,
			event->button,
			event->time);
}

static gboolean
applet_button_press (GtkWidget      *widget,
		     GdkEventButton *event,
		     AppletInfo     *info)
{
	if (event->button == 3) {
		if (panel_applet_in_drag)
			return FALSE;

		if (info->type != APPLET_SWALLOW) 
			applet_show_menu (info, event);

		else {
			GtkHandleBox *handle_box;

			handle_box = GTK_HANDLE_BOX (((Swallow *)info->data)->handle_box);
				
			if (handle_box->child_detached)
				applet_show_menu (info, event);
		}

		return TRUE;
	}

	if (IS_BUTTON_WIDGET (widget))
		return FALSE;
	else
		return TRUE;
}

static GList *launchers_to_kill = NULL;

void
remove_unused_launchers (void)
{
	GList *li;

	for (li = launchers_to_kill; li != NULL; li = li->next) {
		char *file = li->data;
		li->data = NULL;

		unlink (file);

		g_free (file);
	}

	g_list_free (launchers_to_kill);
	launchers_to_kill = NULL;
}

static void
applet_destroy (GtkWidget *w, AppletInfo *info)
{
	GList *li;
	
	g_return_if_fail (info != NULL);

	info->widget = NULL;

	if (info->type == APPLET_DRAWER) {
		Drawer *drawer = info->data;
		g_assert(drawer);
		if(drawer->drawer) {
			GtkWidget *dw = drawer->drawer;
			drawer->drawer = NULL;
			PANEL_WIDGET(BASEP_WIDGET(dw)->panel)->master_widget = NULL;
			gtk_widget_destroy(dw);
		}
	} else if (info->type == APPLET_LAUNCHER) {
		Launcher *launcher = info->data;
		const char *location;

		/* we CAN'T unlink the file here as we may just
		 * be killing stuff before exit, basically we
		 * just want to schedule removals until session
		 * saving */

		location = gnome_desktop_item_get_location (launcher->ditem);
		if (location != NULL)
			launchers_to_kill = 
				g_list_prepend (launchers_to_kill,
						g_strdup (location));
	}


	if (info->menu != NULL) {
		gtk_widget_unref(info->menu);
		info->menu = NULL;
		info->menu_age = 0;
	}

	info->type = APPLET_EMPTY;

	if (info->data_destroy)
		info->data_destroy (info->data);
	info->data_destroy = NULL;
	info->data = NULL;

	/*free the user menu*/
	for(li = info->user_menu; li != NULL; li = g_list_next(li)) {
		AppletUserMenu *umenu = li->data;

		li->data = NULL;

		g_free(umenu->name);
		umenu->name = NULL;
		g_free(umenu->stock_item);
		umenu->stock_item = NULL;
		g_free(umenu->text);
		umenu->text = NULL;

		g_free(umenu);
	}
	g_list_free (info->user_menu);
	info->user_menu = NULL;
}

gboolean
register_toy (GtkWidget *applet,
	      gpointer data,
	      GDestroyNotify data_destroy,
	      PanelWidget *panel,
	      int pos,
	      gboolean exactpos,
	      AppletType type)
{
	AppletInfo *info;
	int newpos;
	gboolean insert_at_pos;
	
	g_return_val_if_fail (applet != NULL, FALSE);
	g_return_val_if_fail (panel != NULL, FALSE);

	if ( ! GTK_WIDGET_NO_WINDOW (applet))
		gtk_widget_set_events (applet, (gtk_widget_get_events (applet) |
						APPLET_EVENT_MASK) &
				       ~( GDK_POINTER_MOTION_MASK |
					  GDK_POINTER_MOTION_HINT_MASK));

	info = g_new0 (AppletInfo, 1);
	info->applet_id = applet_count;
	info->type = type;
	info->widget = applet;
	info->menu = NULL;
	info->menu_age = 0;
	info->data = data;
	info->data_destroy = data_destroy;
	info->user_menu = NULL;

	gtk_object_set_data (GTK_OBJECT (applet), "applet_info", info);

	if (type == APPLET_DRAWER) {
		Drawer *drawer = data;
		PanelWidget *assoc_panel =
			PANEL_WIDGET (BASEP_WIDGET (drawer->drawer)->panel);

		gtk_object_set_data (GTK_OBJECT (applet),
				     PANEL_APPLET_ASSOC_PANEL_KEY, assoc_panel);
		assoc_panel->master_widget = applet;
	}

	gtk_object_set_data (GTK_OBJECT (applet),
			     PANEL_APPLET_FORBIDDEN_PANELS, NULL);
		
	if (applets == NULL) {
		applets_last = applets = g_slist_append (NULL, info);
	} else {
		applets_last = g_slist_append (applets_last, info);
		applets_last = applets_last->next;
	}
	applet_count++;

	/*we will need to save this applet's config now*/
	applets_to_sync = TRUE;

	/*add at the beginning if pos == -1*/
	if (pos >= 0) {
		newpos = pos;
		insert_at_pos = FALSE;
	} else {
		newpos = 0;
		insert_at_pos = TRUE;
	}
	/* if exact pos is on then insert at that precise location */
	if (exactpos)
		insert_at_pos = TRUE;

	if (panel_widget_add_full (panel, applet, newpos, TRUE,
				   insert_at_pos)==-1) {
		GSList *list;
		for(list = panels; list != NULL; list = g_slist_next(list))
			if (panel_widget_add_full (panel, applet, 0,
						   TRUE, TRUE)!=-1)
				break;
		if(!list) {
			/*can't put it anywhere, clean up*/
			gtk_widget_destroy(applet);
			info->widget = NULL;
			panel_clean_applet(info);
			g_warning(_("Can't find an empty spot"));
			return FALSE;
		}
		panel = PANEL_WIDGET(list->data);
	}

	if(IS_BUTTON_WIDGET (applet) || !GTK_WIDGET_NO_WINDOW(applet))
		gtk_signal_connect(GTK_OBJECT(applet),
				   "button_press_event",
				   GTK_SIGNAL_FUNC(applet_button_press),
				   info);

	gtk_signal_connect(GTK_OBJECT(applet), "destroy",
			   GTK_SIGNAL_FUNC(applet_destroy),
			   info);

	gtk_widget_show_all(applet);

	/*gtk_signal_connect (GTK_OBJECT (applet), 
			    "drag_request_event",
			    GTK_SIGNAL_FUNC(panel_dnd_drag_request),
			    info);

	gtk_widget_dnd_drag_set (GTK_WIDGET(applet), TRUE,
				 applet_drag_types, 1);*/

	orientation_change (info, panel);
	size_change (info, panel);
	back_change (info, panel);

	return TRUE;
}
