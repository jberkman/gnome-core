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
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <gnome.h>

#include "panel-include.h"
#include "gnome-panel.h"

#define PANEL_EVENT_MASK (GDK_BUTTON_PRESS_MASK |		\
			   GDK_BUTTON_RELEASE_MASK |		\
			   GDK_POINTER_MOTION_MASK |		\
			   GDK_POINTER_MOTION_HINT_MASK)

/*list of all panel widgets created*/
GSList *panel_list = NULL;

static int panel_dragged = FALSE;
static int panel_dragged_timeout = -1;
static int panel_been_moved = FALSE;

/*the number of base panels (corner/snapped) out there, never let it
  go below 1*/
int base_panels = 0;

extern int config_sync_timeout;
extern int applets_to_sync;
extern int panels_to_sync;
extern int need_complete_save;

extern GlobalConfig global_config;

extern GtkTooltips *panel_tooltips;

extern char *kde_menudir;
extern char *kde_icondir;
extern char *kde_mini_icondir;


/*the types of stuff we accept*/

enum {
	TARGET_URL,
	TARGET_NETSCAPE_URL,
	TARGET_DIRECTORY,
	TARGET_COLOR,
	TARGET_APPLET,
	TARGET_APPLET_INTERNAL
};

static GtkTargetEntry panel_drop_types[] = {
	{ "text/uri-list",       0, TARGET_URL },
	{ "x-url/http",          0, TARGET_NETSCAPE_URL },
	{ "x-url/ftp",           0, TARGET_NETSCAPE_URL },
	{ "_NETSCAPE_URL",       0, TARGET_NETSCAPE_URL },
	{ "application/x-panel-directory", 0, TARGET_DIRECTORY },
	{ "application/x-panel-applet", 0, TARGET_APPLET },
	{ "application/x-panel-applet-internal", 0, TARGET_APPLET_INTERNAL },
	{ "application/x-color", 0, TARGET_COLOR }
};

static gint n_panel_drop_types = 
   sizeof(panel_drop_types) / sizeof(panel_drop_types[0]);

static GtkTargetList *panel_target_list = NULL;


static void
change_window_cursor(GdkWindow *window, GdkCursorType cursor_type)
{
	GdkCursor *cursor = gdk_cursor_new(cursor_type);
	gdk_window_set_cursor(window, cursor);
	gdk_cursor_destroy(cursor);
}

static void
panel_realize(GtkWidget *widget, gpointer data)
{
	change_window_cursor(widget->window, GDK_LEFT_PTR);
	
	basep_widget_enable_buttons(BASEP_WIDGET(widget));
	/*FIXME: this seems to fix the panel size problems on startup
	  (from a report) but I don't think it's right*/
	gtk_widget_queue_resize(GTK_WIDGET(widget));
}

static void
freeze_changes(AppletInfo *info)
{
	if(info->type == APPLET_EXTERN) {
		Extern *ext = info->data;
		g_assert(ext);
		/*ingore this until we get an ior*/
		if(ext->applet) {
			CORBA_Environment ev;
			CORBA_exception_init(&ev);
			GNOME_Applet_freeze_changes(ext->applet, &ev);
			if(ev._major)
				panel_clean_applet(ext->info);
			CORBA_exception_free(&ev);
		}
	}
}

static void
thaw_changes(AppletInfo *info)
{
	if(info->type == APPLET_EXTERN) {
		Extern *ext = info->data;
		g_assert(ext);
		/*ingore this until we get an ior*/
		if(ext->applet) {
			CORBA_Environment ev;
			CORBA_exception_init(&ev);
			GNOME_Applet_thaw_changes(ext->applet, &ev);
			if(ev._major)
				panel_clean_applet(ext->info);
			CORBA_exception_free(&ev);
		}
	}
}

static void
freeze_changes_foreach(GtkWidget *w, gpointer data)
{
	AppletInfo *info = gtk_object_get_data(GTK_OBJECT(w), "applet_info");
	freeze_changes(info);
}

void
panel_freeze_changes(PanelWidget *panel)
{
	gtk_container_foreach(GTK_CONTAINER(panel),
			      freeze_changes_foreach, NULL);
}

static void
thaw_changes_foreach(GtkWidget *w, gpointer data)
{
	AppletInfo *info = gtk_object_get_data(GTK_OBJECT(w), "applet_info");
	thaw_changes(info);
}

void
panel_thaw_changes(PanelWidget *panel)
{
	gtk_container_foreach(GTK_CONTAINER(panel),
			      thaw_changes_foreach, NULL);
}

PanelOrientType
get_applet_orient (PanelWidget *panel)
{
	GtkWidget *panelw;
	g_return_val_if_fail(panel,ORIENT_UP);
	g_return_val_if_fail(IS_PANEL_WIDGET(panel),ORIENT_UP);
	g_return_val_if_fail(panel->panel_parent,ORIENT_UP);
	panelw = panel->panel_parent;
	g_assert (IS_BASEP_WIDGET (panelw));

	if (IS_BASEP_WIDGET(panelw))
		return basep_widget_get_applet_orient (BASEP_WIDGET(panelw));
	else
		g_assert_not_reached ();
	return ORIENT_UP;
}

/*we call this recursively*/
static void orient_change_foreach(GtkWidget *w, gpointer data);

void
orientation_change(AppletInfo *info, PanelWidget *panel)
{
	if(info->type == APPLET_EXTERN) {
		Extern *ext = info->data;
		int orient = get_applet_orient(panel);
		g_assert(ext);
		/*ingore this until we get an ior*/
		if(ext->applet && ext->orient != orient) {
			CORBA_Environment ev;

			CORBA_exception_init(&ev);
			GNOME_Applet_change_orient(ext->applet,
						   orient,
						   &ev);
			if(ev._major)
				panel_clean_applet(ext->info);
			CORBA_exception_free(&ev);

			/* we have now sent this orientation thus we
			   save it and don't send it again unless it
			   changes */
			ext->orient = orient;

		}
	} else if(info->type == APPLET_MENU) {
		Menu *menu = info->data;
		set_menu_applet_orient(menu,get_applet_orient(panel));
	} else if(info->type == APPLET_DRAWER) {
		Drawer *drawer = info->data;
		BasePWidget *basep = BASEP_WIDGET(drawer->drawer);
		set_drawer_applet_orient(drawer,get_applet_orient(panel));
		gtk_widget_queue_resize(drawer->drawer);
		gtk_container_foreach(GTK_CONTAINER(basep->panel),
				      orient_change_foreach,
				      (gpointer)basep->panel);
	} else if(info->type == APPLET_SWALLOW) {
		Swallow *swallow = info->data;

		if(panel->orient == PANEL_VERTICAL)
			set_swallow_applet_orient(swallow,SWALLOW_VERTICAL);
		else
			set_swallow_applet_orient(swallow,SWALLOW_HORIZONTAL);
	} else if(info->type == APPLET_STATUS) {
		StatusApplet *status = info->data;
		if(status->orient != panel->orient) {
			status->orient = panel->orient;
			status_applet_update(status);
		}
	}
}

static void
orient_change_foreach(GtkWidget *w, gpointer data)
{
	AppletInfo *info = gtk_object_get_data(GTK_OBJECT(w), "applet_info");
	PanelWidget *panel = data;
	
	orientation_change(info,panel);
}


static void
panel_orient_change(GtkWidget *widget,
		    PanelOrientation orient,
		    gpointer data)
{
	gtk_container_foreach(GTK_CONTAINER(widget),
			      orient_change_foreach,
			      widget);

	if (IS_FLOATING_WIDGET (PANEL_WIDGET (widget)->panel_parent))
		update_config_floating_orient (FLOATING_WIDGET (PANEL_WIDGET (widget)->panel_parent));

	panels_to_sync = TRUE;
}

static void
floating_pos_change (FloatingPos *pos,
		     gint x, gint y,
		     gpointer data)
{
	update_config_floating_pos (BASEP_WIDGET (data));

}

static void
border_edge_change (BorderPos *border,
		    BorderEdge edge,
		    gpointer data)
{
	BasePWidget *basep = BASEP_WIDGET (data);
	PanelWidget *panel = PANEL_WIDGET (basep->panel);
	gtk_container_foreach (GTK_CONTAINER (panel),
			       orient_change_foreach,
			       panel);
	panels_to_sync = TRUE;
	update_config_edge (basep);
}

static void
sliding_anchor_change (SlidingPos *pos,
		       SlidingAnchor anchor,
		       gpointer data)
{
	update_config_anchor (BASEP_WIDGET (data));
}

static void
sliding_offset_change (SlidingPos *pos,
		       gint16 offset,
		       gpointer data)
{
	update_config_offset (BASEP_WIDGET (data));
}

static void
aligned_align_change (AlignedPos *pos,
		      AlignedAlignment align,
		      gpointer data)
{
	update_config_align (BASEP_WIDGET (data));
}

/*we call this recursively*/
static void size_change_foreach(GtkWidget *w, gpointer data);

void
size_change(AppletInfo *info, PanelWidget *panel)
{
	if(info->type == APPLET_EXTERN) {
		Extern *ext = info->data;
		g_assert(ext);
		/*ingore this until we get an ior*/
		if(ext->applet) {
			CORBA_Environment ev;
			CORBA_exception_init(&ev);
			GNOME_Applet_change_size(ext->applet,
						 panel->sz,
						 &ev);
			if(ev._major)
				panel_clean_applet(ext->info);
			CORBA_exception_free(&ev);
		}
	} else if(info->type == APPLET_STATUS) {
		StatusApplet *status = info->data;
		status->size = panel->sz;
		status_applet_update(status);
	}
}

static void
size_change_foreach(GtkWidget *w, gpointer data)
{
	AppletInfo *info = gtk_object_get_data(GTK_OBJECT(w), "applet_info");
	PanelWidget *panel = data;
	
	size_change(info,panel);
}


static void
panel_size_change(GtkWidget *widget,
		  int sz,
		  gpointer data)
{
	gtk_container_foreach(GTK_CONTAINER(widget), size_change_foreach,
			      widget);
	panels_to_sync = TRUE;
	/*update the configuration box if it is displayed*/
	update_config_size(PANEL_WIDGET(widget)->panel_parent);
}

void
back_change(AppletInfo *info, PanelWidget *panel)
{
	if(info->type == APPLET_EXTERN) {
		Extern *ext = info->data;
		g_assert(ext);
		/*ignore until we have a valid IOR*/
		if(ext->applet) {
			GNOME_Panel_BackInfoType backing;
			CORBA_Environment ev;
			CORBA_exception_init(&ev);
			backing._d = panel->back_type;
			if(panel->back_type == PANEL_BACK_PIXMAP)
				backing._u.pmap = panel->back_pixmap;
			else if(panel->back_type == PANEL_BACK_COLOR) {
				backing._u.c.red = panel->back_color.red;
				backing._u.c.green = panel->back_color.green;
				backing._u.c.blue = panel->back_color.blue;
			}
			GNOME_Applet_back_change(ext->applet, &backing, &ev);
			if(ev._major)
				panel_clean_applet(ext->info);
			CORBA_exception_free(&ev);
		}
	} 
}


static void
back_change_foreach(GtkWidget *w, gpointer data)
{
	AppletInfo *info = gtk_object_get_data(GTK_OBJECT(w), "applet_info");
	PanelWidget *panel = data;

	back_change(info,panel);
}

static void
panel_back_change(GtkWidget *widget,
		  PanelBackType type,
		  char *pixmap,
		  GdkColor *color)
{
	gtk_container_foreach(GTK_CONTAINER(widget),back_change_foreach,widget);

	panels_to_sync = TRUE;
	/*update the configuration box if it is displayed*/
	update_config_back(PANEL_WIDGET(widget));
}

static void state_hide_foreach(GtkWidget *w, gpointer data);

static void
state_restore_foreach(GtkWidget *w, gpointer data)
{
	AppletInfo *info = gtk_object_get_data(GTK_OBJECT(w), "applet_info");
	
	if(info->type == APPLET_DRAWER) {
		Drawer *drawer = info->data;
		BasePWidget *basep = BASEP_WIDGET(drawer->drawer);

		DRAWER_POS (basep->pos)->temp_hidden = FALSE;
		gtk_widget_queue_resize (GTK_WIDGET (basep));

		gtk_container_foreach (GTK_CONTAINER (basep->panel),
				       (basep->state == BASEP_SHOWN)
				       ? state_restore_foreach
				       : state_hide_foreach,
				       NULL);
	}
}

static void
state_hide_foreach(GtkWidget *w, gpointer data)
{
	AppletInfo *info = gtk_object_get_data(GTK_OBJECT(w), "applet_info");
	if(info->type == APPLET_DRAWER) {
		Drawer *drawer = info->data;
		BasePWidget *basep = BASEP_WIDGET(drawer->drawer);

		DRAWER_POS (basep->pos)->temp_hidden = TRUE;
		gtk_container_foreach(GTK_CONTAINER(basep->panel),
				      state_hide_foreach,
				      NULL);

		gtk_widget_queue_resize (GTK_WIDGET (basep));
	}
}

static void
queue_resize_foreach(GtkWidget *w, gpointer data)
{
	AppletInfo *info = gtk_object_get_data(GTK_OBJECT(w), "applet_info");

	if(info->type == APPLET_DRAWER) {
		Drawer *drawer = info->data;
		BasePWidget *basep = BASEP_WIDGET(drawer->drawer);
		
		if(basep->state == BASEP_SHOWN) {
			gtk_widget_queue_resize(w);
			gtk_container_foreach(GTK_CONTAINER(basep->panel),
					       queue_resize_foreach,
					       NULL);
		}
	}
}

static void
basep_state_change(BasePWidget *basep,
		   BasePState state,
		   gpointer data)
{
	gtk_container_foreach (GTK_CONTAINER (basep->panel),
			       (state == BASEP_SHOWN)
			       ? state_restore_foreach
			       : state_hide_foreach,
			       (gpointer)basep);

	panels_to_sync = TRUE;
}

/*static void
basep_type_change(BasePWidget *basep,
		  PanelType type,
		  gpointer data)
{
	update_config_type(basep);
	panels_to_sync = TRUE;
}*/

static void
panel_applet_added(GtkWidget *widget, GtkWidget *applet, gpointer data)
{
	AppletInfo *info = gtk_object_get_data(GTK_OBJECT(applet),
					       "applet_info");
	GtkWidget *panelw = PANEL_WIDGET(widget)->panel_parent;
	
	/*on a real add the info will be NULL as the only adding
	  is done in register_toy and that doesn't add the info to the
	  array until after the add, so we can be sure this was
	  generated on a reparent*/
	if((IS_BASEP_WIDGET(panelw) &&
	    !IS_DRAWER_WIDGET(panelw)) &&
	   info && info->type == APPLET_DRAWER) {
	        Drawer *drawer = info->data;
		BasePWidget *basep = BASEP_WIDGET(drawer->drawer);
		if(basep->state == BASEP_SHOWN ||
		   basep->state == BASEP_AUTO_HIDDEN) {
			BASEP_WIDGET(panelw)->drawers_open++;
			basep_widget_autoshow(BASEP_WIDGET(panelw));
		}
	}
	
	/*pop the panel up on addition*/
	if(IS_BASEP_WIDGET(panelw)) {
		basep_widget_autoshow(BASEP_WIDGET(panelw));
		/*try to pop down though if the mouse is out*/
		basep_widget_queue_autohide(BASEP_WIDGET(panelw));
	}

	freeze_changes(info);
	orientation_change(info,PANEL_WIDGET(BASEP_WIDGET(panelw)->panel));
	size_change(info,PANEL_WIDGET(BASEP_WIDGET(panelw)->panel));
	back_change(info,PANEL_WIDGET(BASEP_WIDGET(panelw)->panel));
	thaw_changes(info);

	/*we will need to save this applet's config now*/
	applets_to_sync = TRUE;
}

static void
panel_applet_removed(GtkWidget *widget, GtkWidget *applet, gpointer data)
{
	GtkWidget *parentw = PANEL_WIDGET(widget)->panel_parent;
	AppletInfo *info = gtk_object_get_data(GTK_OBJECT(applet),
					       "applet_info");

	/*we will need to save this applet's config now*/
	applets_to_sync = TRUE;

	if(info->type == APPLET_DRAWER) {
		Drawer *drawer = info->data;
		if((drawer->drawer) && (
			(BASEP_WIDGET(drawer->drawer)->state == BASEP_SHOWN) ||
			(BASEP_WIDGET(drawer->drawer)->state == BASEP_AUTO_HIDDEN))) {
			if(IS_BASEP_WIDGET(parentw)) {
				BASEP_WIDGET(parentw)->drawers_open--;
				basep_widget_queue_autohide(BASEP_WIDGET(parentw));
			}
		}
		/*it was a drawer so we need to save panels as well*/
		panels_to_sync = TRUE;
	}
}

static void
menu_deactivate(GtkWidget *w, PanelData *pd)
{
	pd->menu_age = 0;
	if(IS_BASEP_WIDGET(pd->panel))
		BASEP_WIDGET(pd->panel)->autohide_inhibit = FALSE;
}

static void
move_panel_to_cursor(GtkWidget *w)
{
	int x,y;
	gdk_window_get_pointer(NULL,&x,&y,NULL);
	if(IS_BASEP_WIDGET(w))
		basep_widget_set_pos(BASEP_WIDGET(w),x,y);
}

static int
panel_move_timeout(gpointer data)
{
	if(panel_dragged && panel_been_moved)
		move_panel_to_cursor(data);
	
	panel_been_moved = FALSE;
	panel_dragged_timeout = -1;

	return FALSE;
}

static void
panel_destroy(GtkWidget *widget, gpointer data)
{
	PanelData *pd = gtk_object_get_user_data(GTK_OBJECT(widget));
	PanelWidget *panel = PANEL_WIDGET(BASEP_WIDGET(widget)->panel);
		
	kill_config_dialog(widget);

	if(IS_DRAWER_WIDGET(widget)) {
		if(panel->master_widget) {
			AppletInfo *info = gtk_object_get_data(GTK_OBJECT(panel->master_widget),
							       "applet_info");
			Drawer *drawer = info->data;
			drawer->drawer = NULL;
			panel_clean_applet(info);
		}
	} else if(IS_BASEP_WIDGET(widget) &&
		  !IS_DRAWER_WIDGET(widget)) {
		/*this is a base panel and we just lost it*/
		base_panels--;
	}

	if(pd->menu)
		gtk_widget_destroy(pd->menu);
	
	panel_list = g_slist_remove(panel_list,pd);
	g_free(pd);
}

static void
panel_applet_move(GtkWidget *panel,GtkWidget *widget, gpointer data)
{
	applets_to_sync = TRUE;
}

static void
panel_applet_draw(GtkWidget *panel, GtkWidget *widget, gpointer data)
{
	AppletInfo *info = gtk_object_get_data(GTK_OBJECT(widget), "applet_info");

	g_return_if_fail(info!=NULL);

	if(info->type == APPLET_EXTERN)
		extern_send_draw(info->data);
}

static GtkWidget *
panel_menu_get(PanelWidget *panel, PanelData *pd)
{
	if(pd->menu)
		return pd->menu;
	
	pd->menu = create_panel_root_menu(panel, TRUE);
	gtk_signal_connect(GTK_OBJECT(pd->menu), "deactivate",
			   GTK_SIGNAL_FUNC(menu_deactivate),pd);
	return pd->menu;
}

GtkWidget *
make_popup_panel_menu (PanelWidget *panel)
{
	BasePWidget *basep;
	PanelData *pd;
	GtkWidget *menu;

	if (!panel) {
	        basep = BASEP_WIDGET (((PanelData *)panel_list->data)->panel);			
		panel = PANEL_WIDGET (basep->panel);
	} else
		basep = BASEP_WIDGET (panel->panel_parent);
	
	pd = gtk_object_get_user_data (GTK_OBJECT (basep));
	menu = panel_menu_get (panel, pd);
	gtk_object_set_data (GTK_OBJECT (menu), "menu_panel", panel);
	pd->menu_age = 0;
	return menu;
}

static int
panel_event(GtkWidget *widget, GdkEvent *event, PanelData *pd)
{
	BasePWidget *basep = BASEP_WIDGET(widget);
	GdkEventButton *bevent;
	switch (event->type) {
	case GDK_BUTTON_PRESS:
		bevent = (GdkEventButton *) event;
		switch(bevent->button) {
		case 3: /* fall through */
			if(!panel_applet_in_drag) {
				GtkWidget *menu;
				menu = make_popup_panel_menu (PANEL_WIDGET(basep->panel));
				BASEP_WIDGET(basep)->autohide_inhibit 
					= TRUE;
				basep_widget_autohide (
					BASEP_WIDGET (basep));
				gtk_menu_popup (GTK_MENU (menu), NULL, NULL, 
						global_config.off_panel_popups
						? panel_menu_position : NULL,
						widget, bevent->button,
						bevent->time);
				return TRUE;
			}
			break;
		case 2:
			/*this should probably be in snapped widget*/
			if(!panel_dragged &&
			   !IS_DRAWER_WIDGET (widget)) {
				GdkCursor *cursor = gdk_cursor_new (GDK_FLEUR);
				gtk_grab_add(widget);
				gdk_pointer_grab (widget->window,
						  FALSE,
						  PANEL_EVENT_MASK,
						  NULL,
						  cursor,
						  bevent->time);
				gdk_cursor_destroy (cursor);
				
				basep->autohide_inhibit = TRUE;

				panel_dragged = TRUE;
				return TRUE;
			}
			if(IS_DRAWER_WIDGET(widget) &&
			   !panel_applet_in_drag) {
				BasePWidget *basep = BASEP_WIDGET(widget);
				PanelWidget *pw = PANEL_WIDGET(basep->panel);
				panel_widget_applet_drag_start(PANEL_WIDGET(pw->master_widget->parent),
							       pw->master_widget);
				return TRUE;
			}
			break;
		default: break;
		}
		break;

	case GDK_BUTTON_RELEASE:
		bevent = (GdkEventButton *) event;
		if(panel_dragged) {

			basep_widget_set_pos(basep,
					     (gint16)bevent->x_root, 
					     (gint16)bevent->y_root);
			basep->autohide_inhibit = FALSE;
			basep_widget_queue_autohide(BASEP_WIDGET(widget));

			gdk_pointer_ungrab(bevent->time);
			gtk_grab_remove(widget);
			panel_dragged = FALSE;
			panel_dragged_timeout = -1;
			panel_been_moved = FALSE;
			return TRUE;
		}

		break;
	case GDK_MOTION_NOTIFY:
		if (panel_dragged) {
			if(panel_dragged_timeout==-1) {
				panel_been_moved = FALSE;
				move_panel_to_cursor(widget);
				panel_dragged_timeout = gtk_timeout_add (30,panel_move_timeout,widget);
			} else
				panel_been_moved = TRUE;
		}
		break;

	default:
		break;
	}

	return FALSE;
}

static int
panel_sub_event_handler(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	GdkEventButton *bevent;
	switch (event->type) {
		/*pass these to the parent!*/
		case GDK_BUTTON_PRESS:
	        case GDK_BUTTON_RELEASE:
	        case GDK_MOTION_NOTIFY:
			bevent = (GdkEventButton *) event;
			/*if the widget is a button we want to keep the
			  button 1 events*/
			if(!GTK_IS_BUTTON(widget) || bevent->button!=1)
				return gtk_widget_event(data, event);

			break;

		default:
			break;
	}

	return FALSE;
}

static gchar *
extract_filename (const gchar* uri)
{
	/* file uri with a hostname */
	if (strncmp(uri, "file://", strlen("file://"))==0) {
		char *hostname = g_strdup(&uri[strlen("file://")]);
		char *p = strchr(hostname,'/');
		char *path;
		char localhostname[1024];
		/* if we can't find the '/' this uri is bad */
		if(!p) {
			g_free(hostname);
			return NULL;
		}
		/* if no hostname */
		if(p==hostname)
			return hostname;

		path = g_strdup(p);
		*p = '\0';

		/* if really local */
		if(g_strcasecmp(hostname,"localhost")==0) {
			g_free(hostname);
			return path;
		}

		/* ok get the hostname */
		if(gethostname(localhostname,
			       sizeof(localhostname)) < 0) {
			strcpy(localhostname,"");
		}

		/* if really local */
		if(localhostname[0] &&
		   g_strcasecmp(hostname,localhostname)==0) {
			g_free(hostname);
			return path;
		}
		
		g_free(hostname);
		g_free(path);
		return NULL;

	/* if the file doesn't have the //, we take it containing 
	   a local path */
	} else if (strncmp(uri, "file:", strlen("file:"))==0) {
		const char *path = &uri[strlen("file:")];
		/* if empty bad */
		if(!*path) return NULL;
		return g_strdup(path);
	}
	return NULL;
}

static void
drop_url(PanelWidget *panel, int pos, char *url)
{
	char *p = g_strdup_printf("Open URL: %s",url);
	load_launcher_applet_from_info_url(url, p, url, "netscape.png",
					   panel, pos, TRUE);
	g_free(p);
}

static void
drop_menu(PanelWidget *panel, int pos, char *dir)
{
	int flags = MAIN_MENU_SYSTEM|MAIN_MENU_USER;

	/*guess redhat menus*/
	if(g_file_exists("/etc/X11/wmconfig"))
		flags |= MAIN_MENU_REDHAT_SUB;
	/* Guess KDE menus */
	if(g_file_exists(kde_menudir))
		flags |= MAIN_MENU_KDE_SUB;
	load_menu_applet(dir, flags, panel, pos, TRUE);
}

static void
drop_urilist(PanelWidget *panel, int pos, char *urilist)
{
	GList *li, *files;
	struct stat s;

	files = gnome_uri_list_extract_uris(urilist);

	for(li = files; li; li = g_list_next(li)) {
		const char *mimetype;
		char *filename;

		if(strncmp(li->data,"http:",strlen("http:"))==0 ||
		   strncmp(li->data,"https:",strlen("https:"))==0 ||
		   strncmp(li->data,"ftp:",strlen("ftp:"))==0 ||
		   strncmp(li->data,"gopher:",strlen("gopher:"))==0) {
			drop_url(panel,pos,li->data);
			continue;
		}

		filename = extract_filename(li->data);
		if(!filename)
			continue;

		if(stat(filename, &s) != 0) {
			g_free(filename);
			continue;
		}

		mimetype = gnome_mime_type(filename);

		if(mimetype && !strncmp(mimetype, "image", sizeof("image")-1))
			panel_widget_set_back_pixmap (panel, filename);
		else if(mimetype
			&& !strcmp(mimetype, "application/x-gnome-app-info"))
			load_launcher_applet(filename, panel, pos, TRUE);
		else if(S_ISDIR(s.st_mode)) {
			drop_menu(panel, pos, filename);
		} else if(S_IEXEC & s.st_mode) /*executable?*/
			ask_about_launcher(filename,panel,pos,TRUE);
		g_free(filename);
	}

	gnome_uri_list_free_strings (files);
}

static void
drop_internal_applet(PanelWidget *panel, int pos, char *applet_type)
{
	if(!applet_type)
		return;
	if(strcmp(applet_type,"MENU:MAIN")==0) {
		drop_menu(panel, pos, NULL);
	} else if(strcmp(applet_type,"DRAWER:NEW")==0) {
		load_drawer_applet(-1, NULL, NULL, panel, pos, TRUE);
	} else if(strcmp(applet_type,"LOGOUT:NEW")==0) {
		load_logout_applet(panel, pos, TRUE);
	} else if(strcmp(applet_type,"LOCK:NEW")==0) {
		load_lock_applet(panel, pos, TRUE);
	} else if(strcmp(applet_type,"SWALLOW:ASK")==0) {
		ask_about_swallowing(panel, pos, TRUE);
	} else if(strcmp(applet_type,"LAUNCHER:ASK")==0) {
		ask_about_launcher(NULL, panel, pos, TRUE);
	} else if(strcmp(applet_type,"STATUS:TRY")==0) {
		load_status_applet(panel, pos, TRUE);
	}
}

static void
drop_color(PanelWidget *panel, int pos, guint16 *dropped)
{
	GdkColor c;

	if(!dropped) return;

	c.red = dropped[0];
	c.green = dropped[1];
	c.blue = dropped[2];
	c.pixel = 0;

	panel_widget_set_back_color(panel, &c);
}

static gboolean
is_this_drop_ok(GtkWidget *widget, GdkDragContext *context, guint *info,
		GdkAtom *ret_atom)
{
	GtkWidget *source_widget;
	GtkWidget *panel; /*PanelWidget*/
	GList *li;

	g_return_val_if_fail(widget!=NULL, FALSE);
	g_return_val_if_fail(IS_BASEP_WIDGET(widget), FALSE);

	if(!(context->actions & GDK_ACTION_COPY))
		return FALSE;

	panel = BASEP_WIDGET (widget)->panel;

	source_widget = gtk_drag_get_source_widget (context);
	/* if this is a drag from this panel to this panel of a launcher,
	   then ignore it */
	if(source_widget && IS_BUTTON_WIDGET(source_widget) &&
	   source_widget->parent == panel) {
		return FALSE;
	}

	if(!panel_target_list)
		panel_target_list = gtk_target_list_new(panel_drop_types,
							n_panel_drop_types);
	for(li = context->targets; li; li = li->next) {
		guint temp_info;
		if(gtk_target_list_find(panel_target_list, 
					GPOINTER_TO_INT(li->data),
					&temp_info)) {
			if(info) *info = temp_info;
			if(ret_atom) *ret_atom = GPOINTER_TO_INT(li->data);
			break;
		}
	}
	/* if we haven't found it */
	if(!li)
		return FALSE;
	return TRUE;
}

static void
do_highlight (GtkWidget *widget, gboolean highlight)
{
	gboolean have_drag;
	have_drag = GPOINTER_TO_INT(gtk_object_get_data (GTK_OBJECT (widget),
							 "have-drag"));
	if(highlight) {
		if(!have_drag) {
			gtk_object_set_data (GTK_OBJECT (widget), "have-drag",
					     GINT_TO_POINTER (TRUE));
			gtk_drag_highlight (widget);
		}
	} else {
		if(have_drag) {
			gtk_object_remove_data (GTK_OBJECT (widget),
						"have-drag");
			gtk_drag_unhighlight (widget);
		}
	}
}


static gboolean
drag_motion_cb(GtkWidget	  *widget,
	       GdkDragContext     *context,
	       gint                x,
	       gint                y,
	       guint               time)
{
	gdk_drag_status (context, GDK_ACTION_COPY, time);

	if(!is_this_drop_ok(widget, context, NULL, NULL))
		return FALSE;

	do_highlight(widget, TRUE);

	basep_widget_autoshow(BASEP_WIDGET(widget));
	basep_widget_queue_autohide(BASEP_WIDGET(widget));

	return TRUE;
}

static gboolean
drag_drop_cb(GtkWidget	       *widget,
	     GdkDragContext    *context,
	     gint               x,
	     gint               y,
	     guint              time,
	     Launcher *launcher)
{
	GdkAtom ret_atom = 0;

	if(!is_this_drop_ok(widget, context, NULL, &ret_atom))
		return FALSE;

	gtk_drag_get_data(widget, context,
			  ret_atom, time);

	return TRUE;
}

static void  
drag_leave_cb(GtkWidget	       *widget,
	      GdkDragContext   *context,
	      guint             time,
	      Launcher *launcher)
{
	do_highlight (widget, FALSE);
}

static void
drag_data_recieved_cb (GtkWidget	 *widget,
		       GdkDragContext   *context,
		       gint              x,
		       gint              y,
		       GtkSelectionData *selection_data,
		       guint             info,
		       guint             time)
{
	PanelWidget *panel;
	int pos;

	g_return_if_fail(widget!=NULL);
	g_return_if_fail(IS_BASEP_WIDGET(widget));

	/* we use this only to really find out the info, we already
	   know this is an ok drop site and the info that got passed
	   to us is bogus (it's always 0 in fact) */
	if(!is_this_drop_ok(widget, context, &info, NULL)) {
		gtk_drag_finish(context,FALSE,FALSE,time);
		return;
	}

	panel = PANEL_WIDGET (BASEP_WIDGET (widget)->panel);

	pos = panel_widget_get_cursorloc(panel);
	
	/* -1 passed to register_toy will turn on the insert_at_pos
	   flag for panel_widget_add_full, which will not place it
	   after the first applet */
	if(pos < 0)
		pos = -1;
	else if(pos > panel->size)
		pos = panel->size;

	switch (info) {
	case TARGET_URL:
		drop_urilist(panel, pos, (char *)selection_data->data);
		break;
	case TARGET_NETSCAPE_URL:
		drop_url(panel, pos, (char *)selection_data->data);
		break;
	case TARGET_COLOR:
		drop_color(panel, pos, (guint16 *)selection_data->data);
		break;
	case TARGET_DIRECTORY:
		drop_menu(panel, pos, (char *)selection_data->data);
		break;
	case TARGET_APPLET:
		if(!selection_data->data) {
			gtk_drag_finish(context,FALSE,FALSE,time);
			return;
		}
		load_extern_applet((char *)selection_data->data, NULL,
				   panel, pos, TRUE, FALSE);
		break;
	case TARGET_APPLET_INTERNAL:
		drop_internal_applet(panel, pos, (char *)selection_data->data);
		break;
	}

	gtk_drag_finish(context,TRUE,FALSE,time);
}

static void
panel_widget_setup(PanelWidget *panel)
{
	gtk_signal_connect(GTK_OBJECT(panel),
			   "applet_added",
			   GTK_SIGNAL_FUNC(panel_applet_added),
			   NULL);
	gtk_signal_connect(GTK_OBJECT(panel),
			   "applet_removed",
			   GTK_SIGNAL_FUNC(panel_applet_removed),
			   NULL);
	gtk_signal_connect(GTK_OBJECT(panel),
			   "applet_move",
			   GTK_SIGNAL_FUNC(panel_applet_move),
			   NULL);
	gtk_signal_connect(GTK_OBJECT(panel),
			   "applet_draw",
			   GTK_SIGNAL_FUNC(panel_applet_draw),
			   NULL);
	gtk_signal_connect(GTK_OBJECT(panel),
			   "back_change",
			   GTK_SIGNAL_FUNC(panel_back_change),
			   NULL);
	gtk_signal_connect(GTK_OBJECT(panel),
			   "size_change",
			   GTK_SIGNAL_FUNC(panel_size_change),
			   NULL);
	gtk_signal_connect (GTK_OBJECT (panel),
			    "orient_change",
			    GTK_SIGNAL_FUNC (panel_orient_change),
			    NULL);
}

void
basep_pos_connect_signals (BasePWidget *basep)
{
	if (IS_BORDER_WIDGET (basep)) {
		gtk_signal_connect (GTK_OBJECT (basep->pos),
				    "edge_change",
				    GTK_SIGNAL_FUNC (border_edge_change),
				    basep);
	}

	if (IS_ALIGNED_WIDGET (basep))
		gtk_signal_connect (GTK_OBJECT (basep->pos),
				    "align_change",
				    GTK_SIGNAL_FUNC (aligned_align_change),
				    basep);
	else if (IS_FLOATING_WIDGET (basep))
		gtk_signal_connect (GTK_OBJECT (basep->pos),
				    "floating_coords_change",
				    GTK_SIGNAL_FUNC (floating_pos_change),
				    basep);
	else if (IS_SLIDING_WIDGET (basep)) {
		gtk_signal_connect (GTK_OBJECT (basep->pos),
				    "anchor_change",
				    GTK_SIGNAL_FUNC (sliding_anchor_change),
				    basep);
		gtk_signal_connect (GTK_OBJECT (basep->pos),
				    "offset_change",
				    GTK_SIGNAL_FUNC (sliding_offset_change),
				    basep);
	}
}

static void
floating_size_alloc(BasePWidget *basep, GtkAllocation *alloc, gpointer data)
{
	if(!GTK_WIDGET_REALIZED(basep))
		return;

	gtk_container_foreach(GTK_CONTAINER(basep->panel),
			      orient_change_foreach,
			      basep->panel);
}

void
panel_setup(GtkWidget *panelw)
{
	PanelData *pd;
	BasePWidget *basep; 
	PanelWidget *panel;

	g_return_if_fail(panelw);

	basep = BASEP_WIDGET(panelw);
	panel = PANEL_WIDGET(basep->panel);

	pd = g_new(PanelData,1);
	pd->menu = NULL;
	pd->menu_age = 0;
	pd->panel = panelw;

	if (IS_BASEP_WIDGET (panelw) &&
	    !IS_DRAWER_WIDGET (panelw))
		base_panels++;
	
	if(IS_EDGE_WIDGET(panelw))
		pd->type = EDGE_PANEL;
	else if(IS_DRAWER_WIDGET(panelw))
		pd->type = DRAWER_PANEL;
	else if(IS_ALIGNED_WIDGET(panelw))
		pd->type = ALIGNED_PANEL;
	else if(IS_SLIDING_WIDGET(panelw))
		pd->type = SLIDING_PANEL;
	else if(IS_FLOATING_WIDGET(panelw))
		pd->type = FLOATING_PANEL;
	else
		g_warning("unknown panel type");
	
	panel_list = g_slist_append(panel_list,pd);
	
	gtk_object_set_user_data(GTK_OBJECT(panelw),pd);

	gtk_signal_connect(GTK_OBJECT(basep->hidebutton_e), "event",
			   (GtkSignalFunc) panel_sub_event_handler,
			   panelw);
	gtk_signal_connect(GTK_OBJECT(basep->hidebutton_w), "event",
			   (GtkSignalFunc) panel_sub_event_handler,
			   panelw);
	gtk_signal_connect(GTK_OBJECT(basep->hidebutton_n), "event",
			   (GtkSignalFunc) panel_sub_event_handler,
			   panelw);
	gtk_signal_connect(GTK_OBJECT(basep->hidebutton_s), "event",
			   (GtkSignalFunc) panel_sub_event_handler,
			   panelw);

	panel_widget_setup(panel);

	gtk_signal_connect(GTK_OBJECT(basep),
			   "drag_data_received",
			   GTK_SIGNAL_FUNC(drag_data_recieved_cb),
			   NULL);
	gtk_signal_connect(GTK_OBJECT(basep),
			   "drag_motion",
			   GTK_SIGNAL_FUNC(drag_motion_cb),
			   NULL);
	gtk_signal_connect(GTK_OBJECT(basep),
			   "drag_leave",
			   GTK_SIGNAL_FUNC(drag_leave_cb),
			   NULL);
	gtk_signal_connect(GTK_OBJECT(basep),
			   "drag_drop",
			   GTK_SIGNAL_FUNC(drag_drop_cb),
			   NULL);

	/*gtk_drag_dest_set (GTK_WIDGET (basep),
			   GTK_DEST_DEFAULT_MOTION |
			   GTK_DEST_DEFAULT_HIGHLIGHT |
			   GTK_DEST_DEFAULT_DROP,
			   panel_drop_types, n_panel_drop_types,
			   GDK_ACTION_COPY);*/
	gtk_drag_dest_set (GTK_WIDGET (basep),
			   0, NULL, 0, 0);

	gtk_signal_connect (GTK_OBJECT (basep),
			    "state_change",
			    GTK_SIGNAL_FUNC (basep_state_change),
			    NULL);
	/*gtk_signal_connect (GTK_OBJECT (basep),
			    "type_change",
			    GTK_SIGNAL_FUNC (basep_type_change),
			    NULL);*/

	basep_pos_connect_signals (basep);

	basep_widget_disable_buttons(basep);
	gtk_signal_connect(GTK_OBJECT(panelw), "event",
			   GTK_SIGNAL_FUNC(panel_event),pd);
	
	gtk_widget_set_events(panelw,
			      gtk_widget_get_events(panelw) |
			      PANEL_EVENT_MASK);
 
	gtk_signal_connect(GTK_OBJECT(panelw), "destroy",
			   GTK_SIGNAL_FUNC(panel_destroy),NULL);


	if(GTK_WIDGET_REALIZED(GTK_WIDGET(panelw)))
		panel_realize(GTK_WIDGET(panelw),NULL);
	else
		gtk_signal_connect_after(GTK_OBJECT(panelw), "realize",
					 GTK_SIGNAL_FUNC(panel_realize),
					 NULL);

	if(IS_FLOATING_WIDGET(panelw))
		gtk_signal_connect_after(GTK_OBJECT(panelw), "size_allocate",
					 GTK_SIGNAL_FUNC(floating_size_alloc),
					 NULL);
}

/*send state change to all the panels*/
void
send_state_change(void)
{
	GSList *list;
	for(list = panel_list; list != NULL; list = g_slist_next(list)) {
		PanelData *pd = list->data;
		if(!IS_DRAWER_WIDGET(pd->panel))
			basep_state_change(BASEP_WIDGET(pd->panel),
					   BASEP_WIDGET(pd->panel)->state,
					   NULL);
	}
}

