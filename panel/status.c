/* Gnome panel: status applet
 * (C) 1999 the Free Software Foundation
 *
 * Authors:  George Lebl
 */

#include <gdk/gdkx.h>
#include <config.h>
#include <string.h>
#include <signal.h>
#include <gnome.h>

#include "panel-include.h"
#include "gnome-panel.h"

static StatusApplet *the_status = NULL; /*"there can only be one" status applet*/
static GtkWidget *offscreen = NULL; /*offscreen window for putting status
				      spots if there is no status applet*/
static GtkWidget *fixed = NULL; /*the fixed container in which the docklets reside*/
static GSList *spots = NULL;
static int nspots = 0;

extern GSList *applets;
extern GSList *applets_last;
extern int applet_count;

#define DOCKLET_SPOT 22

void
status_applet_update(StatusApplet *s)
{
	GSList *li;
	int w,h;
	int sz;
	int rows;
	int i,j;

	if(s->orient == PANEL_HORIZONTAL)
		GTK_HANDLE_BOX(s->handle)->handle_position = GTK_POS_LEFT;
	else
		GTK_HANDLE_BOX(s->handle)->handle_position = GTK_POS_TOP;
	
	switch(s->size) {
	case SIZE_TINY: sz = 24; break;
	case SIZE_STANDARD: sz = 48; break;
	case SIZE_LARGE: sz = 64; break;
	case SIZE_HUGE: sz = 80; break;
	default: sz = 48; break;
	}
	
	rows = sz/DOCKLET_SPOT;

	if(nspots%rows == 0)
		w = DOCKLET_SPOT*(nspots/rows);
	else
		w = DOCKLET_SPOT*(nspots/rows)+1;
	
	/*make minimum size*/
	if(w==0) w = 10;	

	h = DOCKLET_SPOT*rows;

	/*if we are vertical just switch stuff around*/
	if(s->orient == PANEL_VERTICAL) {
		int t = w;
		w = h;
		h = t;
	}
	
	gtk_widget_set_usize(fixed,w,h);
	
	i = j = 0;
	for(li = spots; li; li = li->next) {
		StatusSpot *ss = li->data;
		gtk_fixed_move(GTK_FIXED(fixed),ss->socket,i,j);
		i+=DOCKLET_SPOT;
		if(i>=w) {
			i = 0;
			j+=DOCKLET_SPOT;
		}
	}
	gtk_widget_queue_resize(s->handle);
}

static void
status_socket_destroyed(GtkWidget *w, StatusSpot *ss)
{
	status_spot_remove(ss);
}

StatusSpot *
new_status_spot(void)
{
	StatusSpot *ss = g_new0(StatusSpot,1);
	ss->wid = 0;

	spots = g_slist_prepend(spots,ss);
	nspots++;

	ss->socket = gtk_socket_new();
	gtk_widget_set_usize(ss->socket,DOCKLET_SPOT,DOCKLET_SPOT);
	if(!the_status && !offscreen) {
		offscreen = gtk_window_new(GTK_WINDOW_POPUP);
		gtk_widget_set_uposition(offscreen,gdk_screen_width()+10,
					 gdk_screen_height()+10);

		/*it should be null at this point*/
		g_assert(!fixed);
		
		fixed = gtk_fixed_new();
		gtk_widget_show(fixed);
		
		gtk_container_add(GTK_CONTAINER(offscreen),fixed);

		gtk_fixed_put(GTK_FIXED(fixed),ss->socket,0,0);
		gtk_widget_show_now(offscreen);
	} else {
		gtk_fixed_put(GTK_FIXED(fixed),ss->socket,0,0);
		status_applet_update(the_status);
	}
	gtk_widget_show_now(ss->socket);
	gtk_signal_connect(GTK_OBJECT(ss->socket),"destroy",
			   GTK_SIGNAL_FUNC(status_socket_destroyed),
			   ss);
	
	if(GTK_WIDGET_REALIZED(ss->socket))
		g_warning("DEBUG: BLAHBLAHBLAH");

	ss->wid = GDK_WINDOW_XWINDOW(ss->socket->window);
	return ss;
}

void
status_spot_remove(StatusSpot *ss)
{
	CORBA_Environment ev;
	spots = g_slist_remove(spots,ss);
	nspots--;
	gtk_widget_destroy(ss->socket);

	CORBA_exception_init(&ev);
	CORBA_Object_release(ss->sspot, &ev);
	POA_GNOME_StatusSpot__fini((PortableServer_Servant) ss, &ev);
	CORBA_exception_free(&ev);

	g_free(ss);
	if(the_status) status_applet_update(the_status);
}

static int
ignore_1st_click(GtkWidget *widget, GdkEvent *event)
{
	GdkEventButton *buttonevent = (GdkEventButton *)event;

	if (event->type == GDK_BUTTON_PRESS &&
	    buttonevent->button == 1) {
		return TRUE;
	}
	if (event->type == GDK_BUTTON_RELEASE &&
	    buttonevent->button == 1) {
		return TRUE;
	}
	 
	return FALSE;
}

static void
applet_destroy(GtkWidget *w, StatusApplet *s)
{
	if(!offscreen) {
		offscreen = gtk_window_new(GTK_WINDOW_POPUP);
		gtk_widget_set_uposition(offscreen,gdk_screen_width()+10,
					 gdk_screen_height()+10);
		gtk_widget_show_now(offscreen);
	}
	gtk_widget_reparent(fixed,offscreen);
	g_free(s);
	the_status = NULL;
}

int
load_status_applet(PanelWidget *panel, int pos)
{
	if(the_status)
		return FALSE;
	
	the_status = g_new0(StatusApplet,1);
	the_status->frame = gtk_frame_new(NULL);
	the_status->orient = panel->orient;
	the_status->size = panel->sz;
	gtk_frame_set_shadow_type(GTK_FRAME(the_status->frame),
				  GTK_SHADOW_IN);
	the_status->handle = gtk_handle_box_new();
	gtk_signal_connect(GTK_OBJECT(the_status->handle), "event",
			   GTK_SIGNAL_FUNC(ignore_1st_click), NULL);
	gtk_container_add(GTK_CONTAINER(the_status->handle),
			  the_status->frame);
	gtk_signal_connect(GTK_OBJECT(the_status->handle), "destroy",
			   GTK_SIGNAL_FUNC(applet_destroy), the_status);
	
	if(!fixed) {
		fixed = gtk_fixed_new();
		gtk_container_add(GTK_CONTAINER(the_status->frame),fixed);
	} else {
		gtk_widget_reparent(fixed,the_status->frame);
	}
	
	status_applet_update(the_status);

	register_toy(the_status->handle,the_status, panel, pos, APPLET_STATUS);
	the_status->info = applets_last->data;

	return TRUE;
}
