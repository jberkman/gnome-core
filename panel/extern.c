/* Gnome panel: extern applet functions
 * (C) 1997 the Free Software Foundation
 *
 * Authors:  George Lebl
 *           Federico Mena
 *           Miguel de Icaza
 */

#include <gdk/gdkx.h>
#include <config.h>
#include <string.h>
#include <signal.h>
#include <gnome.h>

#include "panel-include.h"

#define APPLET_EVENT_MASK (GDK_BUTTON_PRESS_MASK |		\
			   GDK_BUTTON_RELEASE_MASK |		\
			   GDK_POINTER_MOTION_MASK |		\
			   GDK_POINTER_MOTION_HINT_MASK)

extern GArray *applets;
extern int applet_count;

extern GtkTooltips *panel_tooltips;

extern GlobalConfig global_config;

extern char *panel_cfg_path;
extern char *old_panel_cfg_path;

void
extern_clean(Extern *ext)
{
        CORBA_Environment ev;
	CORBA_exception_init(&ev);
        CORBA_Object_release(ext->obj, &ev);
	CORBA_exception_free(&ev);

	g_free(ext->path);
	g_free(ext->params);
	g_free(ext->cfg);
	g_free(ext);
}


void
applet_show_menu(int applet_id)
{
	AppletInfo *info = get_applet_info(applet_id);
	GtkWidget *panel;

	g_return_if_fail(info != NULL);

	if (!info->menu)
		create_applet_menu(info);

	panel = get_panel_parent(info->widget);
	if(IS_SNAPPED_WIDGET(panel)) {
		SNAPPED_WIDGET(panel)->autohide_inhibit = TRUE;
		snapped_widget_queue_pop_down(SNAPPED_WIDGET(panel));
	}

	gtk_menu_popup(GTK_MENU(info->menu), NULL, NULL, applet_menu_position,
		       GINT_TO_POINTER(applet_id), 3, GDK_CURRENT_TIME);
}

PanelOrientType
applet_get_panel_orient(int applet_id)
{
	AppletInfo *info = get_applet_info(applet_id);
	PanelWidget *panel;

	g_return_val_if_fail(info != NULL,ORIENT_UP);

	panel = PANEL_WIDGET(info->widget->parent);

	g_return_val_if_fail(panel != NULL,ORIENT_UP);

	return get_applet_orient(panel);
}


int
applet_get_panel(int applet_id)
{
	int panel;
	GList *list;
	AppletInfo *info = get_applet_info(applet_id);
	gpointer p;

	g_return_val_if_fail(info != NULL,-1);

	p = PANEL_WIDGET(info->widget->parent);

	for(panel=0,list=panels;list!=NULL;list=g_list_next(list),panel++)
		if(list->data == p)
			return panel;
	return -1;
}

void
applet_abort_id(int applet_id)
{
	AppletInfo *info = get_applet_info(applet_id);

	g_return_if_fail(info != NULL);

	/*only reserved spots can be canceled, if an applet
	  wants to chance a pending applet it needs to first
	  user reserve spot to obtain id and make it EXTERN_RESERVED*/
	if(info->type != APPLET_EXTERN_RESERVED)
		return;

	panel_clean_applet(applet_id);
}


int
applet_get_pos(int applet_id)
{
	AppletInfo *info = get_applet_info(applet_id);
	AppletData *ad;

	g_return_val_if_fail(info != NULL,-1);

	ad = gtk_object_get_data(GTK_OBJECT(info->widget),
				 PANEL_APPLET_DATA);
	if(!ad)
		return -1;
	return ad->pos;
}

void
applet_drag_start(int applet_id)
{
	PanelWidget *panel;
	AppletInfo *info = get_applet_info(applet_id);

	g_return_if_fail(info != NULL);

	panel = PANEL_WIDGET(info->widget->parent);

	g_return_if_fail(panel!=NULL);

	/*panel_widget_applet_drag_start(panel,info->widget);
	panel_widget_applet_drag_end(panel);*/
	panel_widget_applet_drag_start_no_grab(panel,info->widget);
	panel_widget_applet_move_use_idle(panel);
}

void
applet_drag_stop(int applet_id)
{
	PanelWidget *panel;
	AppletInfo *info = get_applet_info(applet_id);

	g_return_if_fail(info != NULL);

	panel = PANEL_WIDGET(info->widget->parent);

	g_return_if_fail(panel!=NULL);

	panel_widget_applet_drag_end_no_grab(panel);
}

static int
compare_params(const char *p1,const char *p2)
{
	if(!p1) {
		if(!p2 || *p2=='\0')
			return TRUE;
		else
			return FALSE;
	} else if(!p2) {
		if(!p1 || *p1=='\0')
			return TRUE;
		else
			return FALSE;
	}
	return (strcmp(p1,p2)==0);
}

int
applet_request_id (const char *path, const char *param,
		   int dorestart, char **cfgpath,
		   char **globcfgpath, guint32 * winid)
{
	AppletInfo *info;
	int i;
	Extern *ext;
	
	for(info=(AppletInfo *)applets->data,i=0;i<applet_count;i++,info++) {
		if(info && info->type == APPLET_EXTERN_PENDING) {
			Extern *ext = info->data;
			g_assert(ext);
			if(strcmp(ext->path,path)==0 &&
			   compare_params(param,ext->params)) {
				/*we started this and already reserved a spot
				  for it, including the socket widget*/
				GtkWidget *socket =
					GTK_BIN(info->widget)->child;
				g_return_val_if_fail(GTK_IS_SOCKET(socket),-1);

				*cfgpath = ext->cfg;
				ext->cfg = NULL;
				*globcfgpath = g_strdup(old_panel_cfg_path);
				info->type = APPLET_EXTERN_RESERVED;
				*winid=GDK_WINDOW_XWINDOW(socket->window);
				if(!dorestart && !mulapp_is_in_list(path))
					mulapp_add_to_list(path);

				return i;
			}
		}
	}
	
	/*this is an applet that was started from outside, otherwise we would
	  have already reserved a spot for it*/
	ext = g_new(Extern,1);
	ext->obj = CORBA_OBJECT_NIL;
	ext->path = g_strdup(path);
	ext->params = g_strdup(param);
	ext->cfg = NULL;

	*winid = reserve_applet_spot (ext, panels->data, 0,
				      APPLET_EXTERN_RESERVED);
	if(*winid == 0) {
		*globcfgpath = NULL;
		*cfgpath = NULL;
		return -1;
	}
	*cfgpath = g_copy_strings(old_panel_cfg_path,"Applet_Dummy/",NULL);
	*globcfgpath = g_strdup(old_panel_cfg_path);

	info = get_applet_info(applet_count-1);
	if(!dorestart && !mulapp_is_in_list(path))
		mulapp_add_to_list(path);

	return i;
}

void
applet_register (CORBA_Object obj, int applet_id, const char *goad_id)
{
	AppletInfo *info = get_applet_info(applet_id);
	PanelWidget *panel;
	Extern *ext;
	CORBA_Environment ev;

	/*start the next applet in queue*/
	exec_queue_done(applet_id);

	g_return_if_fail(info != NULL);
	
	ext = info->data;
	g_assert(ext);

	panel = PANEL_WIDGET(info->widget->parent);
	g_return_if_fail(panel!=NULL);

	/*no longer pending*/
	info->type = APPLET_EXTERN;

	/*set the obj*/
	CORBA_exception_init(&ev);
	CORBA_Object_release(ext->obj, &ev);
	ext->obj = CORBA_Object_duplicate(obj, &ev);
	CORBA_exception_free(&ev);

	mulapp_add_obj_and_free_queue(ext->path, ext->obj);

	orientation_change(applet_id,panel);
	back_change(applet_id,panel);
	send_applet_tooltips_state(ext->obj, applet_id,
				   global_config.tooltips_enabled);
}

static int
extern_socket_destroy(GtkWidget *w, gpointer data)
{
	Extern *ext = data;
	gtk_widget_destroy(ext->ebox);
	extern_clean(ext);
	return FALSE;
}

/*note that type should be APPLET_EXTERN_RESERVED or APPLET_EXTERN_PENDING
  only*/
guint32
reserve_applet_spot (Extern *ext, PanelWidget *panel, int pos,
		     AppletType type)
{
	GtkWidget *socket;

	ext->ebox = gtk_event_box_new();
	gtk_widget_set_events(ext->ebox, (gtk_widget_get_events(ext->ebox) |
					  APPLET_EVENT_MASK) &
			      ~( GDK_POINTER_MOTION_MASK |
				 GDK_POINTER_MOTION_HINT_MASK));

	socket = gtk_socket_new();

	g_return_val_if_fail(socket!=NULL,0);

	gtk_container_add(GTK_CONTAINER(ext->ebox),socket);

	gtk_signal_connect(GTK_OBJECT(socket),"destroy",
			   GTK_SIGNAL_FUNC(extern_socket_destroy),
			   ext);

	gtk_widget_show_all (ext->ebox);
	
	/*we save the obj in the id field of the appletinfo and the 
	  path in the path field */
	if(!register_toy(ext->ebox,ext,panel,pos,type)) {
		g_warning("Couldn't add applet");
		return 0;
	}
	
	if(!GTK_WIDGET_REALIZED(socket))
		gtk_widget_realize(socket);

	return GDK_WINDOW_XWINDOW(socket->window);
}

void
applet_set_tooltip(int applet_id, const char *tooltip)
{
	AppletInfo *info = get_applet_info(applet_id);
	g_return_if_fail(info != NULL);

	gtk_tooltips_set_tip (panel_tooltips,info->widget,tooltip,NULL);
}

void
load_extern_applet(char *path, char *params, char *cfgpath,
		   PanelWidget *panel, int pos)
{
	char *fullpath;
	char *param;
	char *goad_id = NULL, *ctmp1, *ctmp2;
	Extern *ext;

	/*start nothing, applet is taking care of everything*/
	if(path == NULL ||
	   path[0] == '\0')
		return;

	if(!params)
		param = "";
	else
		param = params;

	ctmp1 = strstr(params, "--activate-goad-server=");
	if(ctmp1) {
	  ctmp2 = strchr(ctmp1, ' ');
	  if(ctmp2) {
	    goad_id = g_strndup(ctmp1 + strlen("--activate-goad-server="),
				ctmp2 - ctmp1 + strlen("--activate-goad-server="));
	  } else
	    goad_id = g_strdup(ctmp1 + strlen("--activate-goad-server="));
	}

	if(!cfgpath || !*cfgpath)
		cfgpath = g_copy_strings(old_panel_cfg_path,
					 "Applet_Dummy/",NULL);
	else
		/*we will free this lateer*/
		cfgpath = g_strdup(cfgpath);

	/*make it an absolute path, same as the applets will
	  interpret it and the applets will sign themselves as
	  this, so it has to be exactly the same*/
	if(path[0]!='#')
		fullpath = get_full_path(path);
	else
		fullpath = g_strdup(path);
	
	ext = g_new(Extern,1);
	ext->obj = CORBA_OBJECT_NIL;
	ext->path = fullpath;
	ext->params = g_strdup(params);
	ext->cfg = cfgpath;

	if(reserve_applet_spot (ext, panel, pos, APPLET_EXTERN_PENDING)==0) {
		g_warning("Whoops! for some reason we can't add "
			  "to the panel");
		extern_clean(ext);
		return;
	}

	/*'#' marks an applet that will take care of starting
	  itself but wants us to reserve a spot for it*/
	if(path[0]!='#') {
	  if(goad_id)
	    CORBA_Object_release(goad_server_activate_with_id(NULL, goad_id, 0), NULL);
	  else
	    exec_prog(applet_count-1,fullpath,param);
	}
	g_free(goad_id);
}
