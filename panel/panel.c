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
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <libgnome/libgnome.h>
#include <libbonobo.h>

#include "panel.h"

#include "applet.h"
#include "button-widget.h"
#include "distribution.h"
#include "drawer-widget.h"
#include "edge-widget.h"
#include "extern.h"
#include "floating-widget.h"
#include "foobar-widget.h"
#include "gnome-run.h"
#include "launcher.h"
#include "logout.h"
#include "menu-fentry.h"
#include "menu-util.h"
#include "menu.h"
#include "panel-util.h"
#include "panel_config.h"
#include "session.h"
#include "status.h"
#include "swallow.h"
#include "panel-applet-frame.h"

/* nautilus uses this UTTER HACK to reset backgrounds, ugly ugly ugly,
 * broken, ugly ugly, but whatever */
#define RESET_IMAGE_NAME "/nautilus/backgrounds/reset.png"

#define PANEL_EVENT_MASK (GDK_BUTTON_PRESS_MASK |		\
			   GDK_BUTTON_RELEASE_MASK |		\
			   GDK_POINTER_MOTION_MASK |		\
			   GDK_POINTER_MOTION_HINT_MASK)

/*list of all panel widgets created*/
GSList *panel_list = NULL;

static gboolean panel_dragged = FALSE;
static int panel_dragged_timeout = -1;
static gboolean panel_been_moved = FALSE;

/*the number of base panels (corner/snapped) out there, never let it
  go below 1*/
int base_panels = 0;

extern GSList *applets;

extern int applets_to_sync;
extern int panels_to_sync;

extern gboolean commie_mode;

extern GtkTooltips *panel_tooltips;

extern char *kde_menudir;

/*the types of stuff we accept*/

enum {
	TARGET_URL,
	TARGET_NETSCAPE_URL,
	TARGET_DIRECTORY,
	TARGET_COLOR,
	TARGET_APPLET,
	TARGET_APPLET_INTERNAL,
	TARGET_ICON_INTERNAL,
	TARGET_BGIMAGE
};

static void
change_window_cursor(GdkWindow *window, GdkCursorType cursor_type)
{
	GdkCursor *cursor = gdk_cursor_new(cursor_type);
	gdk_window_set_cursor(window, cursor);
	gdk_cursor_destroy(cursor);
}

static void
panel_realize (GtkWidget *widget, gpointer data)
{
	change_window_cursor (widget->window, GDK_LEFT_PTR);
	
	if (BASEP_IS_WIDGET (widget))
		basep_widget_enable_buttons(BASEP_WIDGET(widget));
	else if (FOOBAR_IS_WIDGET (widget))
		foobar_widget_update_winhints (FOOBAR_WIDGET(widget));

	/*FIXME: this seems to fix the panel size problems on startup
	  (from a report) but I don't think it's right*/
	gtk_widget_queue_resize (GTK_WIDGET (widget));
}

void
freeze_changes (AppletInfo *info)
{
	if (info->type == APPLET_EXTERN)
		extern_handle_freeze_changes ((Extern)info->data);
}

void
thaw_changes(AppletInfo *info)
{
	if (info->type == APPLET_EXTERN)
		extern_handle_thaw_changes ((Extern)info->data);
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

PanelOrient
get_applet_orient (PanelWidget *panel)
{
	GtkWidget *panelw;
	g_return_val_if_fail(panel,PANEL_ORIENT_UP);
	g_return_val_if_fail(PANEL_IS_WIDGET(panel),PANEL_ORIENT_UP);
	g_return_val_if_fail(panel->panel_parent,PANEL_ORIENT_UP);
	panelw = panel->panel_parent;

	if (BASEP_IS_WIDGET(panelw))
		return basep_widget_get_applet_orient (BASEP_WIDGET(panelw));
	else
		return PANEL_ORIENT_DOWN;
}

/*we call this recursively*/
static void orient_change_foreach(GtkWidget *w, gpointer data);

void
orientation_change (AppletInfo  *info,
		    PanelWidget *panel)
{

	switch (info->type) {
	case APPLET_EXTERN:
		extern_handle_change_orient ((Extern)info->data,
					     get_applet_orient (panel));
		break;
	case APPLET_BONOBO:
		panel_applet_frame_change_orient ((PanelAppletFrame *) info->data,
						  get_applet_orient (panel));
		break;
	case APPLET_MENU:
		set_menu_applet_orient ((Menu *)info->data,
					get_applet_orient (panel));
		break;
	case APPLET_DRAWER: {
		Drawer      *drawer = info->data;
		BasePWidget *basep = BASEP_WIDGET (drawer->drawer);

		set_drawer_applet_orient (drawer,
					  get_applet_orient (panel));
		gtk_widget_queue_resize (drawer->drawer);
		gtk_container_foreach (GTK_CONTAINER (basep->panel),
				       orient_change_foreach,
				       (gpointer)basep->panel);
		}
		break;
	case APPLET_SWALLOW: {
		Swallow *swallow = info->data;

		if (panel->orient == PANEL_VERTICAL)
			set_swallow_applet_orient (swallow,
						   SWALLOW_VERTICAL);
		else
			set_swallow_applet_orient (swallow,
						   SWALLOW_HORIZONTAL);
		}
		break;
	case APPLET_STATUS: {
		StatusApplet *status = info->data;

		if (status->orient != panel->orient) {
			status->orient = panel->orient;
			status_applet_update (status);
		}
		}
		break;
	default:
		break;
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

	if (FLOATING_IS_WIDGET (PANEL_WIDGET (widget)->panel_parent))
		update_config_floating_orient (FLOATING_WIDGET (PANEL_WIDGET (widget)->panel_parent));

	panels_to_sync = TRUE;
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

/*we call this recursively*/
static void size_change_foreach(GtkWidget *w, gpointer data);

void
size_change (AppletInfo  *info,
	     PanelWidget *panel)
{
	PanelSize size = panel->sz;
	
	switch (info->type) {
	case APPLET_BONOBO:
		panel_applet_frame_change_size (
			PANEL_APPLET_FRAME (info->data), size);
		break;
	case APPLET_EXTERN:
		extern_handle_change_size ((Extern)info->data, size);
		break;
	case APPLET_STATUS: {
		StatusApplet *status = info->data;

		status->size = size;

		status_applet_update (status);
		}
		break;
	default:
		break;
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
	update_config_size (PANEL_WIDGET (widget)->panel_parent);
}

void
back_change (AppletInfo *info,
	     PanelWidget *panel)
{
	switch (info->type) {
	case APPLET_EXTERN:
		extern_handle_back_change ((Extern)info->data, panel);
		break;
	case APPLET_BONOBO: {
		PanelAppletFrame *frame = PANEL_APPLET_FRAME (info->data);

		switch (panel->back_type) {
		case PANEL_BACK_PIXMAP:
			panel_applet_frame_change_background_pixmap (frame,
								     panel->back_pixmap);
			break;
		case PANEL_BACK_COLOR:
			panel_applet_frame_change_background_color (frame,
								    panel->back_color.red,
								    panel->back_color.green,
								    panel->back_color.blue);
			break;
		case PANEL_BACK_NONE:
			panel_applet_frame_clear_background (frame);
			break;
		default:
			g_assert_not_reached ();
			break;
		}
		}
		
		break;
	default:
		break;
	}
}

static void
back_change_foreach (GtkWidget   *widget,
		     PanelWidget *panel)
{
	AppletInfo *info;

	info = gtk_object_get_data (GTK_OBJECT (widget), "applet_info");

	back_change (info, panel);
}

static void
panel_back_change(GtkWidget *widget,
		  PanelBackType type,
		  char *pixmap,
		  GdkColor *color)
{
	gtk_container_foreach (GTK_CONTAINER (widget),
			       (GtkCallback) back_change_foreach,
			       widget);

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
		GtkWidget *widget = GTK_WIDGET(basep);

		DRAWER_POS (basep->pos)->temp_hidden = TRUE;
		gtk_container_foreach(GTK_CONTAINER(basep->panel),
				      state_hide_foreach,
				      NULL);

		gtk_widget_queue_resize (widget);

		/* quickly hide the window from sight, the allocation
		   and all that will get updated in the main loop */
		if(widget->window) {
			gdk_window_move(widget->window,
					-widget->allocation.width - 1,
					-widget->allocation.height - 1);
		}
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
	
	/*
	 * on a real add the info will be NULL as the 
	 * only adding is done in panel_applet_register 
	 * and that doesn't add the info to the array until 
	 * after the add, so we can be sure this was 
	 * generated on a reparent.
	 */
	if((BASEP_IS_WIDGET(panelw) &&
	    !DRAWER_IS_WIDGET(panelw)) &&
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
	if(BASEP_IS_WIDGET(panelw)) {
		basep_widget_autoshow(BASEP_WIDGET(panelw));
		/*try to pop down though if the mouse is out*/
		basep_widget_queue_autohide(BASEP_WIDGET(panelw));
	}

	freeze_changes(info);
	orientation_change(info,PANEL_WIDGET(widget));
	size_change(info,PANEL_WIDGET(widget));
	back_change(info,PANEL_WIDGET(widget));
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
			if(BASEP_IS_WIDGET(parentw)) {
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
	if(BASEP_IS_WIDGET(pd->panel))
		BASEP_WIDGET(pd->panel)->autohide_inhibit = FALSE;
}

static void
move_panel_to_cursor(GtkWidget *w)
{
	int x,y;
	gdk_window_get_pointer(NULL,&x,&y,NULL);
	if(BASEP_IS_WIDGET(w))
		basep_widget_set_pos(BASEP_WIDGET(w),x,y);
}

static gboolean
panel_move_timeout(gpointer data)
{
	if(panel_dragged && panel_been_moved)
		move_panel_to_cursor(data);
	
	panel_been_moved = FALSE;
	panel_dragged_timeout = -1;

	return FALSE;
}

static void
clean_kill_applets (PanelWidget *panel)
{
	GList *li;

	for (li = panel->applet_list; li != NULL; li = li->next) {
		AppletData *ad = li->data;
		AppletInfo *info =
			gtk_object_get_data (GTK_OBJECT (ad->applet),
					     "applet_info");

		if (info->type == APPLET_EXTERN)
			extern_save_last_position ((Extern)info->data, FALSE);

		else if (info->type == APPLET_SWALLOW) {
			Swallow *swallow = info->data;

			swallow->clean_remove = TRUE;
		}
	}

	gnome_config_sync ();
}

static void
panel_destroy (GtkWidget *widget, gpointer data)
{
	PanelData *pd = gtk_object_get_user_data (GTK_OBJECT (widget));
	PanelWidget *panel = NULL;

	if (BASEP_IS_WIDGET (widget))
		panel = PANEL_WIDGET(BASEP_WIDGET(widget)->panel);
	else if (FOOBAR_IS_WIDGET (widget))
		panel = PANEL_WIDGET (FOOBAR_WIDGET (widget)->panel);

	clean_kill_applets (panel);
		
	kill_config_dialog(widget);

	if (DRAWER_IS_WIDGET(widget)) {
		GtkWidget *master_widget = panel->master_widget;

		if (master_widget != NULL) {
			AppletInfo *info =
				gtk_object_get_data(GTK_OBJECT(master_widget),
						    "applet_info");
			Drawer *drawer = info->data;
			drawer->drawer = NULL;
			panel_applet_clean (info);
			gtk_object_remove_data (GTK_OBJECT (master_widget),
						"applet_info");
		}
	} else if ((BASEP_IS_WIDGET(widget)
		    && !DRAWER_IS_WIDGET(widget))
		   || FOOBAR_IS_WIDGET (widget)) {
		/*this is a base panel and we just lost it*/
		base_panels--;
	}

	if (pd->menu != NULL)
		gtk_widget_unref (pd->menu);
	pd->menu = NULL;

	pd->panel = NULL;

	panel_list = g_slist_remove (panel_list, pd);
	g_free (pd);
}

static void
panel_applet_move(PanelWidget *panel, GtkWidget *widget, gpointer data)
{
	applets_to_sync = TRUE;
}

static void
panel_applet_draw(GtkWidget *panel, GtkWidget *widget, gpointer data)
{
	AppletInfo *info = gtk_object_get_data(GTK_OBJECT(widget),
					       "applet_info");

	g_return_if_fail(info!=NULL);

	if(info->type == APPLET_EXTERN)
		extern_send_draw(info->data);
}

static void
panel_applet_about_to_die (GtkWidget *panel, GtkWidget *widget, gpointer data)
{
	AppletInfo *info = gtk_object_get_data (GTK_OBJECT (widget),
						"applet_info");

	g_return_if_fail (info != NULL);

	/* this needs to be befor we null the widget field */
	if (info->type == APPLET_EXTERN &&
	    info->data != NULL) {
		extern_before_remove (info->data);
	}
}

static GtkWidget *
panel_menu_get (PanelWidget *panel, PanelData *pd)
{
	if (pd->menu != NULL)
		return pd->menu;
	
	pd->menu = create_panel_root_menu (panel, TRUE);
	g_signal_connect (G_OBJECT (pd->menu), "deactivate",
			  G_CALLBACK (menu_deactivate), pd);
	return pd->menu;
}

GtkWidget *
make_popup_panel_menu (PanelWidget *panel)
{
	GtkWidget *panelw;
	PanelData *pd;
	GtkWidget *menu;

	if (!panel) {
		panelw = ((PanelData *)panel_list->data)->panel;
		if (BASEP_IS_WIDGET (panelw))
			panel = PANEL_WIDGET (BASEP_WIDGET (panelw)->panel);
		else if (FOOBAR_IS_WIDGET (panelw))
			panel = PANEL_WIDGET (FOOBAR_WIDGET (panelw)->panel);
	} else
		panelw = panel->panel_parent;

	pd = gtk_object_get_user_data (GTK_OBJECT (panelw));
	menu = panel_menu_get (panel, pd);
	gtk_object_set_data (GTK_OBJECT (menu), "menu_panel", panel);

	pd->menu_age = 0;
	return menu;
}

static gboolean
panel_initiate_move (GtkWidget *widget, guint32 event_time)
{
	PanelWidget *panel = NULL;
	BasePWidget *basep = NULL;

	if (BASEP_IS_WIDGET (widget)) {
		basep = BASEP_WIDGET (widget);
		panel = PANEL_WIDGET (basep->panel);
	} else if (FOOBAR_IS_WIDGET (widget)) {
		panel = PANEL_WIDGET (FOOBAR_WIDGET (widget)->panel);
	}

	/*this should probably be in snapped widget*/
	if(!panel_dragged &&
	   !DRAWER_IS_WIDGET (widget) &&
	   !FOOBAR_IS_WIDGET (widget)) {
		GdkCursor *cursor = gdk_cursor_new (GDK_FLEUR);
		gtk_grab_add(widget);
		gdk_pointer_grab (widget->window,
				  FALSE,
				  PANEL_EVENT_MASK,
				  NULL,
				  cursor,
				  event_time);
		gdk_cursor_destroy (cursor);

		if (basep) {
			basep->autohide_inhibit = TRUE;
			basep_widget_init_offsets (basep);
		}

		panel_dragged = TRUE;
		return TRUE;
	} if(DRAWER_IS_WIDGET(widget) &&
	     !panel_applet_in_drag) {
		panel_widget_applet_drag_start (
						PANEL_WIDGET(panel->master_widget->parent),
						panel->master_widget,
						PW_DRAG_OFF_CURSOR);
		return TRUE;
	}

	return FALSE;
}
	
static gboolean
panel_event(GtkWidget *widget, GdkEvent *event, PanelData *pd)
{
	PanelWidget *panel = NULL;
	BasePWidget *basep = NULL;
	GdkEventButton *bevent;

	if (BASEP_IS_WIDGET (widget)) {
		basep = BASEP_WIDGET (widget);
		panel = PANEL_WIDGET (basep->panel);
	} else if (FOOBAR_IS_WIDGET (widget)) {
		panel = PANEL_WIDGET (FOOBAR_WIDGET (widget)->panel);
	}

	switch (event->type) {
	case GDK_BUTTON_PRESS:
		bevent = (GdkEventButton *) event;
		switch(bevent->button) {
		case 3:
			if(!panel_applet_in_drag) {
				GtkWidget *menu;

				menu = make_popup_panel_menu (panel);
				if (basep) {
					basep->autohide_inhibit = TRUE;
					basep_widget_autohide (basep);
				}

				gtk_menu_popup (GTK_MENU (menu),
						NULL,
						NULL, 
						panel_menu_position,
						widget,
						bevent->button,
						bevent->time);

				return TRUE;
			}
			break;
		case 2:
			if ( ! commie_mode)
				return panel_initiate_move (widget,
							    bevent->time);
			break;
		default: break;
		}
		break;

	case GDK_BUTTON_RELEASE:
		bevent = (GdkEventButton *) event;
		if(panel_dragged) {
			if (!basep)
				return TRUE;
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

static gboolean
panel_widget_event (GtkWidget *widget, GdkEvent *event, GtkWidget *panelw)
{
	if (event->type == GDK_BUTTON_PRESS) {
		GdkEventButton *bevent = (GdkEventButton *) event;

		if (bevent->button == 1 ||
		    bevent->button == 2)
			return panel_initiate_move (panelw, bevent->time);
	}

	return FALSE;
}

static gboolean
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
	if (strncmp (uri, "file://", strlen ("file://")) == 0) {
		char *hostname = g_strdup (&uri[strlen("file://")]);
		char *p = strchr (hostname, '/');
		char *path;
		char localhostname[1024];
		/* if we can't find the '/' this uri is bad */
		if(p == NULL) {
			g_free (hostname);
			return NULL;
		}
		/* if no hostname */
		if(p == hostname)
			return hostname;

		path = g_strdup (p);
		*p = '\0';

		/* if really local */
		if (g_ascii_strcasecmp (hostname, "localhost") == 0 ||
		    g_ascii_strcasecmp (hostname, "localhost.localdomain") == 0) {
			g_free (hostname);
			return path;
		}

		/* ok get the hostname */
		if (gethostname (localhostname,
				 sizeof (localhostname)) < 0) {
			strcpy (localhostname, "");
		}

		/* if really local */
		if (localhostname[0] &&
		    g_ascii_strcasecmp (hostname, localhostname) == 0) {
			g_free (hostname);
			return path;
		}
		
		g_free (hostname);
		g_free (path);
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
drop_url(PanelWidget *panel, int pos, const char *url)
{
	char *p;

	g_return_if_fail (url != NULL);

	p = g_strdup_printf (_("Open URL: %s"), url);
	load_launcher_applet_from_info_url (url, p, url, "gnome-globe.png",
					    panel, pos, TRUE);
	g_free (p);
}

static void
drop_menu (PanelWidget *panel, int pos, const char *dir)
{
	int flags = MAIN_MENU_SYSTEM | MAIN_MENU_USER;
	DistributionType distribution = get_distribution_type ();

	/*guess distribution menus*/
	if(distribution != DISTRIBUTION_UNKNOWN)
		flags |= MAIN_MENU_DISTRIBUTION_SUB;
	/* Guess KDE menus */
	if (g_file_test (kde_menudir, G_FILE_TEST_IS_DIR))
		flags |= MAIN_MENU_KDE_SUB;
	/* FIXME: checkout gnome-vfs stuff for drop, this should be
	 * a uri */
	load_menu_applet (dir, FALSE /* main_menu */, flags, TRUE, FALSE, NULL, panel, pos, TRUE);
}

static void
drop_directory (PanelWidget *panel, int pos, const char *dir)
{
	char *tmp;

	tmp = g_build_filename (dir, ".directory", NULL);
	if (g_file_test (tmp, G_FILE_TEST_EXISTS)) {
		g_free (tmp);
		drop_menu (panel, pos, dir);
		return;
	}
	g_free (tmp);

	tmp = g_build_filename (dir, ".order", NULL);
	if (g_file_test (tmp, G_FILE_TEST_EXISTS)) {
		g_free (tmp);
		drop_menu (panel, pos, dir);
		return;
	}
	g_free (tmp);

	tmp = g_find_program_in_path  ("nautilus");
	if (tmp != NULL) {
		/* nautilus */
		char *exec = g_strdup_printf ("nautilus %s",
					      panel_quote_string (dir));
		char *base;
		g_free (tmp);

		base = g_path_get_basename (dir);

		load_launcher_applet_from_info (base,
						dir,
						exec,
						"gnome-folder.png",
						panel,
						pos,
						TRUE);
		g_free (exec);
		g_free (base);
	} else {
		tmp = g_find_program_in_path  ("gmc-client");
		if (tmp != NULL) {
			/* gmc */
			char *exec = g_strdup_printf ("gmc-client "
						      "--create-window=%s",
						      panel_quote_string (dir));

			g_free (tmp);

			load_launcher_applet_from_info (g_basename (dir),
							dir,
							exec,
							"gnome-folder.png",
							panel,
							pos,
							TRUE);
			g_free (exec);
		} else {
			drop_menu (panel, pos, dir);
		}
	}
}

static void
drop_urilist (PanelWidget *panel, int pos, char *urilist,
	      gboolean background_drops)
{
  g_warning ("FIXME: drop_urilist() not implemented");
#ifdef FIXME
	GList *li, *files;
	struct stat s;

	files = gnome_uri_list_extract_uris(urilist);

	for(li = files; li; li = li->next) {
		char *uri;
		const char *mimetype;
		char *filename;

		uri = li->data;

		if (strncmp (uri, "http:", strlen ("http:")) == 0 ||
		    strncmp (uri, "https:", strlen ("https:")) == 0 ||
		    strncmp (uri, "ftp:", strlen ("ftp:")) == 0 ||
		    strncmp (uri, "gopher:", strlen ("gopher:")) == 0 ||
		    strncmp (uri, "ghelp:", strlen ("ghelp:")) == 0 ||
		    strncmp (uri, "man:", strlen ("man:")) == 0 ||
		    strncmp (uri, "info:", strlen ("info:")) == 0) {
			drop_url (panel, pos, uri);
			continue;
		}

		filename = extract_filename (li->data);
		if(filename == NULL)
			continue;

		if(stat(filename, &s) != 0) {
			g_free(filename);
			continue;
		}

		mimetype = gnome_mime_type(filename);

		if (background_drops &&
		    mimetype != NULL &&
		    strncmp(mimetype, "image", sizeof("image")-1) == 0) {
			panel_widget_set_back_pixmap (panel, filename);
		} else if (mimetype != NULL &&
			  (strcmp(mimetype, "application/x-gnome-app-info") == 0 ||
			   strcmp(mimetype, "application/x-kde-app-info") == 0)) {
			Launcher *launcher;

			launcher = load_launcher_applet (filename, panel, pos, TRUE);

			if (launcher != NULL)
				launcher_hoard (launcher);
		} else if (S_ISDIR(s.st_mode)) {
			drop_directory (panel, pos, filename);
		} else if (S_IEXEC & s.st_mode) /*executable?*/
			ask_about_launcher (filename, panel, pos, TRUE);
		g_free (filename);
	}

	gnome_uri_list_free_strings (files);
#endif
}

static void
drop_bgimage (PanelWidget *panel, const char *bgimage)
{
	char *filename;

	filename = extract_filename (bgimage);
	if (filename != NULL) {
		/* an incredible hack, no, worse, INCREDIBLE FUCKING HACK,
		 * whatever, we need to work with nautilus on this one */
		if (strstr (filename, RESET_IMAGE_NAME) != NULL) {
			panel_widget_change_params (panel,
						    panel->orient,
						    panel->sz,
						    PANEL_BACK_NONE,
						    panel->back_pixmap,
						    panel->fit_pixmap_bg,
						    panel->strech_pixmap_bg,
						    panel->rotate_pixmap_bg,
						    &panel->back_color);
		} else {
			panel_widget_set_back_pixmap (panel, filename);
		}

		g_free (filename);
	}
}

static void
drop_internal_icon (PanelWidget *panel, int pos, const char *icon_name,
		    int action)
{
	Launcher *old_launcher, *launcher;

	if (icon_name == NULL)
		return;

	if (action == GDK_ACTION_MOVE) {
		old_launcher = find_launcher (icon_name);
	} else {
		old_launcher = NULL;
	}

	launcher = load_launcher_applet (icon_name, panel, pos, TRUE);

	if (launcher != NULL) {
		launcher_hoard (launcher);

		if (old_launcher != NULL &&
		    old_launcher->button != NULL)
			gtk_widget_destroy (old_launcher->button);
	}
}

static void
move_applet (PanelWidget *panel, int pos, int applet_num)
{
	AppletInfo *info = g_slist_nth_data (applets, applet_num);

	if (pos < 0)
		pos = 0;

	if (info != NULL &&
	    info->widget != NULL &&
	    info->widget->parent != NULL &&
	    PANEL_IS_WIDGET (info->widget->parent)) {
		GSList *forb;
		forb = gtk_object_get_data (GTK_OBJECT (info->widget),
					    PANEL_APPLET_FORBIDDEN_PANELS);
		if ( ! g_slist_find (forb, panel))
			panel_widget_reparent (PANEL_WIDGET (info->widget->parent),
					       panel,
					       info->widget,
					       pos);
	}
}

static void
drop_internal_applet (PanelWidget *panel, int pos, const char *applet_type,
		      int action)
{
	int applet_num = -1;
	gboolean remove_applet = FALSE;

	if (applet_type == NULL)
		return;

	if (sscanf (applet_type, "MENU:%d", &applet_num) == 1 ||
	    sscanf (applet_type, "DRAWER:%d", &applet_num) == 1 ||
	    sscanf (applet_type, "SWALLOW:%d", &applet_num) == 1) {
		if (action != GDK_ACTION_MOVE)
			g_warning ("Only MOVE supported for menus/drawers/swallows");
		move_applet (panel, pos, applet_num);

	} else if (strncmp (applet_type, "MENU:", strlen("MENU:")) == 0) {
		const char *menu = &applet_type[strlen ("MENU:")];
		if (strcmp (menu, "MAIN") == 0)
			drop_menu (panel, pos, NULL);
		else
			drop_menu (panel, pos, menu);

	} else if (strcmp(applet_type,"DRAWER:NEW")==0) {
		load_drawer_applet(-1, NULL, NULL, panel, pos, TRUE);

	} else if (strcmp (applet_type, "LOGOUT:NEW") == 0) {
		load_logout_applet (panel, pos, TRUE);

	} else if (sscanf (applet_type, "LOGOUT:%d", &applet_num) == 1) {
		load_logout_applet (panel, pos, TRUE);
		remove_applet = TRUE;

	} else if (strcmp (applet_type, "LOCK:NEW") == 0) {
		load_lock_applet (panel, pos, TRUE);

	} else if (sscanf (applet_type, "LOCK:%d", &applet_num) == 1) {
		load_lock_applet (panel, pos, TRUE);
		remove_applet = TRUE;

	} else if (strcmp (applet_type, "SWALLOW:ASK") == 0) {
		ask_about_swallowing(panel, pos, TRUE);

	} else if(strcmp(applet_type,"LAUNCHER:ASK")==0) {
		ask_about_launcher(NULL, panel, pos, TRUE);

	} else if(strcmp(applet_type,"STATUS:TRY")==0) {
		load_status_applet(panel, pos, TRUE);

	} else if(strcmp(applet_type,"RUN:NEW")==0) {
		load_run_applet(panel, pos, TRUE);

	} else if (sscanf (applet_type, "RUN:%d", &applet_num) == 1) {
		load_run_applet(panel, pos, TRUE);
		remove_applet = TRUE;
	}

	if (remove_applet &&
	    action == GDK_ACTION_MOVE) {
		AppletInfo *info = g_slist_nth_data (applets, applet_num);

		if (info != NULL)
			panel_applet_clean (info);
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

static GtkTargetList *
get_target_list (void)
{
	static GtkTargetEntry drop_types [] = {
		{ "text/uri-list",                       0, TARGET_URL },
		{ "x-url/http",                          0, TARGET_NETSCAPE_URL },
		{ "x-url/ftp",                           0, TARGET_NETSCAPE_URL },
		{ "_NETSCAPE_URL",                       0, TARGET_NETSCAPE_URL },
		{ "application/x-panel-directory",       0, TARGET_DIRECTORY },
		{ "application/x-panel-applet",          0, TARGET_APPLET },
		{ "application/x-panel-applet-internal", 0, TARGET_APPLET_INTERNAL },
		{ "application/x-panel-icon-internal",   0, TARGET_ICON_INTERNAL },
		{ "application/x-color",                 0, TARGET_COLOR },
		{ "property/bgimage",                    0, TARGET_BGIMAGE }
	};
	static GtkTargetList *target_list = NULL;

	if (!target_list) {
		gint length = sizeof (drop_types) / sizeof (drop_types [0]);

		target_list = gtk_target_list_new (drop_types, length);
	}

	return target_list;
}

static gboolean
is_this_drop_ok (GtkWidget      *widget,
		 GdkDragContext *context,
		 guint          *ret_info,
		 GdkAtom        *ret_atom)
{
	GtkWidget *panel;
	GList     *l;

	g_return_val_if_fail (widget, FALSE);

	if (!BASEP_IS_WIDGET (widget) && !FOOBAR_IS_WIDGET (widget))
		return FALSE;

	if (!(context->actions & (GDK_ACTION_COPY|GDK_ACTION_MOVE)))
		return FALSE;

	if (BASEP_IS_WIDGET (widget))
		panel = BASEP_WIDGET (widget)->panel;
	else
		panel = FOOBAR_WIDGET (widget)->panel;

	for (l = context->targets; l; l = l->next) {
		GdkAtom atom;
		guint   info;

		atom = GDK_POINTER_TO_ATOM (l->data);

		if (gtk_target_list_find (get_target_list (), atom, &info)) {

			if (FOOBAR_IS_WIDGET (widget) &&
			    (info == TARGET_COLOR || info == TARGET_BGIMAGE))
				return FALSE;

			if (ret_info)
				*ret_info = info;

			if (ret_atom)
				*ret_atom = atom;
			break;
		}
	}

	return l ? TRUE : FALSE;
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
drag_motion_cb (GtkWidget	   *widget,
		GdkDragContext     *context,
		gint                x,
		gint                y,
		guint               time)
{
	guint info;

	if ( ! is_this_drop_ok (widget, context, &info, NULL))
		return FALSE;

	/* check forbiddenness */
	if (info == TARGET_APPLET_INTERNAL) {
		GtkWidget *source_widget;

		source_widget = gtk_drag_get_source_widget (context);
		if (source_widget != NULL &&
		    BUTTON_IS_WIDGET (source_widget)) {
			GSList *forb;
			PanelWidget *panel = NULL;

			if (BASEP_IS_WIDGET (widget)) {
				BasePWidget *basep =
					BASEP_WIDGET (widget);
				panel = PANEL_WIDGET (basep->panel);
			} else if (FOOBAR_IS_WIDGET (widget)) {
				panel = PANEL_WIDGET (FOOBAR_WIDGET (widget)->panel);
			}
			forb = gtk_object_get_data (GTK_OBJECT (source_widget),
						    PANEL_APPLET_FORBIDDEN_PANELS);
			if (panel != NULL &&
			    g_slist_find (forb, panel) != NULL)
				return FALSE;
		}
	}

	/* always prefer copy, except for internal icons/applets,
	 * where we prefer move */
	if (info == TARGET_ICON_INTERNAL ||
	    info == TARGET_APPLET_INTERNAL) {
		if (context->actions & GDK_ACTION_MOVE) {
			gdk_drag_status (context, GDK_ACTION_MOVE, time);
		} else {
			gdk_drag_status (context, context->suggested_action, time);
		}
	} else if (context->actions & GDK_ACTION_COPY) {
		gdk_drag_status (context, GDK_ACTION_COPY, time);
	} else {
		gdk_drag_status (context, context->suggested_action, time);
	}

	do_highlight (widget, TRUE);

	if (BASEP_IS_WIDGET (widget)) {
		basep_widget_autoshow (BASEP_WIDGET (widget));
		basep_widget_queue_autohide (BASEP_WIDGET (widget));
	}

	return TRUE;
}

static gboolean
drag_drop_cb (GtkWidget	        *widget,
	      GdkDragContext    *context,
	      gint               x,
	      gint               y,
	      guint              time,
	      Launcher          *launcher)
{
	GdkAtom ret_atom = 0;

	if ( ! is_this_drop_ok (widget, context, NULL, &ret_atom))
		return FALSE;

	gtk_drag_get_data(widget, context,
			  ret_atom, time);

	return TRUE;
}

static void  
drag_leave_cb (GtkWidget	*widget,
	       GdkDragContext   *context,
	       guint             time,
	       Launcher         *launcher)
{
	do_highlight (widget, FALSE);
}

static void
drag_data_recieved_cb (GtkWidget	*widget,
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
	g_return_if_fail (BASEP_IS_WIDGET (widget) ||
			  FOOBAR_IS_WIDGET (widget));

	/* we use this only to really find out the info, we already
	   know this is an ok drop site and the info that got passed
	   to us is bogus (it's always 0 in fact) */
	if ( ! is_this_drop_ok (widget, context, &info, NULL)) {
		gtk_drag_finish (context, FALSE, FALSE, time);
		return;
	}

	if (BASEP_IS_WIDGET (widget))
		panel = PANEL_WIDGET (BASEP_WIDGET (widget)->panel);
	else
		panel = PANEL_WIDGET (FOOBAR_WIDGET (widget)->panel);

	pos = panel_widget_get_cursorloc(panel);
	
	/* 
	 * -1 passed to panel_applet_register will turn on 
	 * the insert_at_pos flag for panel_widget_add_full,
	 * which will not place it after the first applet.
	 */
	if(pos < 0)
		pos = -1;
	else if(pos > panel->size)
		pos = panel->size;

	switch (info) {
	case TARGET_URL:
		drop_urilist (panel, pos, (char *)selection_data->data,
			      FOOBAR_IS_WIDGET(widget) ? FALSE : TRUE);
		break;
	case TARGET_NETSCAPE_URL:
		drop_url (panel, pos, (char *)selection_data->data);
		break;
	case TARGET_COLOR:
		drop_color (panel, pos, (guint16 *)selection_data->data);
		break;
	case TARGET_BGIMAGE:
		if ( ! FOOBAR_IS_WIDGET(widget))
			drop_bgimage (panel, (char *)selection_data->data);
		break;
	case TARGET_DIRECTORY:
		drop_directory (panel, pos, (char *)selection_data->data);
		break;
	case TARGET_APPLET:
		if ( ! selection_data->data) {
			gtk_drag_finish (context, FALSE, FALSE, time);
			return;
		}
#ifdef FIXME
		extern_load_applet ((char *)selection_data->data,
				    NULL, panel, pos, TRUE, FALSE);
#endif
		break;
	case TARGET_APPLET_INTERNAL:
		drop_internal_applet (panel, pos, (char *)selection_data->data,
				      context->action);
		break;
	case TARGET_ICON_INTERNAL:
		drop_internal_icon (panel, pos, (char *)selection_data->data,
				    context->action);
		break;
	default:
		gtk_drag_finish (context, FALSE, FALSE, time);
		return;
	}

	gtk_drag_finish (context, TRUE, FALSE, time);
}

static void
panel_widget_setup(PanelWidget *panel)
{
	g_signal_connect (G_OBJECT(panel),
			  "applet_added",
			  G_CALLBACK(panel_applet_added),
			  NULL);
	g_signal_connect (G_OBJECT(panel),
			  "applet_removed",
			  G_CALLBACK(panel_applet_removed),
			  NULL);
	g_signal_connect (G_OBJECT(panel),
			  "applet_move",
			  G_CALLBACK(panel_applet_move),
			  NULL);
	g_signal_connect (G_OBJECT(panel),
			  "applet_draw",
			  G_CALLBACK(panel_applet_draw),
			  NULL);
	g_signal_connect (G_OBJECT(panel),
			  "applet_about_to_die",
			  G_CALLBACK(panel_applet_about_to_die),
			  NULL);
	g_signal_connect (G_OBJECT(panel),
			  "back_change",
			  G_CALLBACK(panel_back_change),
			  NULL);
	g_signal_connect (G_OBJECT(panel),
			  "size_change",
			  G_CALLBACK(panel_size_change),
			  NULL);
	g_signal_connect (G_OBJECT (panel),
			  "orient_change",
			  G_CALLBACK (panel_orient_change),
			  NULL);
}

void
basep_pos_connect_signals (BasePWidget *basep)
{
	if (BORDER_IS_WIDGET (basep)) {
		g_signal_connect (G_OBJECT (basep->pos),
				  "edge_change",
				  G_CALLBACK (border_edge_change),
				  basep);
	}

	if (ALIGNED_IS_WIDGET (basep))
		gtk_signal_connect_object (GTK_OBJECT (basep->pos),
					   "align_change",
					   GTK_SIGNAL_FUNC (update_config_align),
					   GTK_OBJECT (basep));
	else if (FLOATING_IS_WIDGET (basep))
		gtk_signal_connect_object (GTK_OBJECT (basep->pos),
					   "floating_coords_change",
					   GTK_SIGNAL_FUNC (update_config_floating_pos),
					   GTK_OBJECT(basep));
	else if (SLIDING_IS_WIDGET (basep)) {
		gtk_signal_connect_object (GTK_OBJECT (basep->pos),
					   "anchor_change",
					   GTK_SIGNAL_FUNC (update_config_anchor),
					   GTK_OBJECT(basep));
		gtk_signal_connect_object (GTK_OBJECT (basep->pos),
					   "offset_change",
					   GTK_SIGNAL_FUNC (update_config_offset),
					   GTK_OBJECT (basep));
	}
}

static void
drawer_orient_change_foreach(GtkWidget *w, gpointer data)
{
	AppletInfo *info = gtk_object_get_data(GTK_OBJECT(w), "applet_info");
	PanelWidget *panel = data;
	
	if(info->type == APPLET_DRAWER)
		orientation_change(info, panel);
}

static void
panelw_size_alloc(BasePWidget *basep, GtkAllocation *alloc, gpointer data)
{
	if(!GTK_WIDGET_REALIZED(basep))
		return;

	if(DRAWER_IS_WIDGET(basep)) {
		gtk_container_foreach(GTK_CONTAINER(basep->panel),
				      orient_change_foreach,
				      basep->panel);
	} else if(FLOATING_IS_WIDGET(basep)) {
		gtk_container_foreach(GTK_CONTAINER(basep->panel),
				      orient_change_foreach,
				      basep->panel);
		update_config_floating_pos_limits(basep);
	} else if(ALIGNED_IS_WIDGET(basep)) {
		gtk_container_foreach(GTK_CONTAINER(basep->panel),
				      drawer_orient_change_foreach,
				      basep->panel);
	} else if(SLIDING_IS_WIDGET(basep)) {
		gtk_container_foreach(GTK_CONTAINER(basep->panel),
				      drawer_orient_change_foreach,
				      basep->panel);
		update_config_offset_limit(basep);
	}
}

void
panel_setup(GtkWidget *panelw)
{
	PanelData *pd;
	BasePWidget *basep = NULL; 
	PanelWidget *panel = NULL;

	g_return_if_fail(panelw);

	if (BASEP_IS_WIDGET (panelw)) {
		basep = BASEP_WIDGET(panelw);
		panel = PANEL_WIDGET(basep->panel);
	} else if (FOOBAR_IS_WIDGET (panelw)) {
		panel = PANEL_WIDGET (FOOBAR_WIDGET (panelw)->panel);
	}

	pd = g_new(PanelData,1);
	pd->menu = NULL;
	pd->menu_age = 0;
	pd->panel = panelw;

	if (FOOBAR_IS_WIDGET (panelw) || 
	    (BASEP_IS_WIDGET (panelw) &&
	     !DRAWER_IS_WIDGET (panelw)))
		base_panels++;
	
	if(EDGE_IS_WIDGET(panelw))
		pd->type = EDGE_PANEL;
	else if(DRAWER_IS_WIDGET(panelw))
		pd->type = DRAWER_PANEL;
	else if(ALIGNED_IS_WIDGET(panelw))
		pd->type = ALIGNED_PANEL;
	else if(SLIDING_IS_WIDGET(panelw))
		pd->type = SLIDING_PANEL;
	else if(FLOATING_IS_WIDGET(panelw))
		pd->type = FLOATING_PANEL;
	else if(FOOBAR_IS_WIDGET(panelw))
		pd->type = FOOBAR_PANEL;
	else
		g_warning("unknown panel type");
	
	panel_list = g_slist_append(panel_list,pd);
	
	gtk_object_set_user_data(GTK_OBJECT(panelw),pd);

	panel_widget_setup(panel);

	if (basep) {
		g_signal_connect (G_OBJECT(basep->hidebutton_e), "event",
				  G_CALLBACK (panel_sub_event_handler),
				  panelw);
		g_signal_connect (G_OBJECT(basep->hidebutton_w), "event",
				  G_CALLBACK (panel_sub_event_handler),
				  panelw);
		g_signal_connect (G_OBJECT(basep->hidebutton_n), "event",
				  G_CALLBACK (panel_sub_event_handler),
				  panelw);
		g_signal_connect (G_OBJECT(basep->hidebutton_s), "event",
				  G_CALLBACK (panel_sub_event_handler),
				  panelw);
		g_signal_connect (G_OBJECT (basep), "state_change",
				  G_CALLBACK (basep_state_change),
				  NULL);
		basep_pos_connect_signals (basep);
		basep_widget_disable_buttons(basep);

		g_signal_connect_after (G_OBJECT(panelw), "size_allocate",
					G_CALLBACK(panelw_size_alloc),
					NULL);
	}

	g_signal_connect (G_OBJECT(panelw), "drag_data_received",
			  G_CALLBACK(drag_data_recieved_cb),
			  NULL);
	g_signal_connect (G_OBJECT(panelw), "drag_motion",
			  G_CALLBACK(drag_motion_cb),
			  NULL);
	g_signal_connect (G_OBJECT(panelw), "drag_leave",
			  G_CALLBACK(drag_leave_cb),
			  NULL);
	g_signal_connect (G_OBJECT(panelw), "drag_drop",
			  G_CALLBACK(drag_drop_cb),
			  NULL);

	gtk_drag_dest_set (GTK_WIDGET (panelw),
			   0, NULL, 0, 0);

	g_signal_connect (G_OBJECT (panelw), "event",
			  G_CALLBACK (panel_event), pd);
	g_signal_connect (G_OBJECT (panel), "event",
			  G_CALLBACK (panel_widget_event), panelw);
	
	gtk_widget_set_events(panelw,
			      gtk_widget_get_events(panelw) |
			      PANEL_EVENT_MASK);
 
	g_signal_connect (G_OBJECT (panelw), "destroy",
			  G_CALLBACK (panel_destroy), NULL);


	if(GTK_WIDGET_REALIZED(GTK_WIDGET(panelw)))
		panel_realize(GTK_WIDGET(panelw),NULL);
	else
		g_signal_connect_after(G_OBJECT(panelw), "realize",
				       G_CALLBACK(panel_realize),
				       NULL);
}

/*send state change to all the panels*/
void
send_state_change(void)
{
	GSList *list;
	for(list = panel_list; list != NULL; list = g_slist_next(list)) {
		PanelData *pd = list->data;
		if(BASEP_IS_WIDGET (pd->panel) && !DRAWER_IS_WIDGET(pd->panel))
			basep_state_change(BASEP_WIDGET(pd->panel),
					   BASEP_WIDGET(pd->panel)->state,
					   NULL);
	}
}

PanelData *
panel_data_by_id (int id)
{
	GSList *list;
	for(list = panel_list; list != NULL; list = g_slist_next(list)) {
		PanelData *pd = list->data;
		int pd_id = -1;

		if (BASEP_IS_WIDGET (pd->panel))
		       pd_id = PANEL_WIDGET (BASEP_WIDGET (pd->panel)->panel)->unique_id;
		else if (FOOBAR_IS_WIDGET (pd->panel))
		       pd_id = PANEL_WIDGET (FOOBAR_WIDGET (pd->panel)->panel)->unique_id;

		if (id == pd_id)
			return pd;
	}
	return NULL;
}

void
panel_set_id (GtkWidget *widget, int id)
{
	if (BASEP_IS_WIDGET (widget))
		PANEL_WIDGET (BASEP_WIDGET (widget)->panel)->unique_id = id;
	else if (FOOBAR_IS_WIDGET (widget))
		PANEL_WIDGET (FOOBAR_WIDGET (widget)->panel)->unique_id = id;
}

void
status_unparent (GtkWidget *widget)
{
	GList *li;
	PanelWidget *panel = NULL;
	if (BASEP_IS_WIDGET (widget))
		panel = PANEL_WIDGET(BASEP_WIDGET(widget)->panel);
	else if (FOOBAR_IS_WIDGET (widget))
		panel = PANEL_WIDGET (FOOBAR_WIDGET (widget)->panel);
	for(li=panel->applet_list;li;li=li->next) {
		AppletData *ad = li->data;
		AppletInfo *info = gtk_object_get_data(GTK_OBJECT(ad->applet),
						       "applet_info");
		if(info->type == APPLET_STATUS) {
			status_applet_put_offscreen(info->data);
		} else if(info->type == APPLET_DRAWER) {
			Drawer *dr = info->data;
			status_unparent(dr->drawer);
		}
	}
}

