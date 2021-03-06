/* Gnome panel: status applet
 * (C) 1999-2000 the Free Software Foundation
 *
 * Authors:  George Lebl
 */

#include <config.h>
#include <string.h>
#include <signal.h>

#include <gdk/gdkx.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <libgnome/libgnome.h>
#include <libbonobo.h>

#include "status.h"

#include "applet.h"
#include "xstuff.h"

static StatusApplet *the_status = NULL; /*"there can only be one" status
					  applet*/
static GtkWidget *offscreen = NULL; /*offscreen window for putting status
				      spots if there is no status applet*/
static GtkWidget *fixed = NULL; /*the fixed container in which the docklets reside*/
static GSList *spots = NULL;
static int nspots = 0;

gboolean status_inhibit = FALSE; /*inhibit adding and updating for the purpose
				   of quitting*/

#define DOCKLET_SPOT 22

#define MINIMUM_WIDTH 10

/*this will show debug output and put the offscreen window on 10 10 to
  view it*/
#undef STATUS_DEBUG

#ifdef STATUS_DEBUG
#define DPUTS(x) puts(x)
#define DPRINTD(d) printf("%s: %d\n",#d,d)
#else
#define DPUTS(x)
#define DPRINTD(d)
#endif

StatusSpot *
status_applet_get_ss(guint32 winid)
{
	GSList *li;
	for(li = spots; li; li = li->next) {
		StatusSpot *ss = li->data;
		GtkSocket *s = GTK_SOCKET(ss->socket);
		if(s->plug_window &&
		   GDK_WINDOW_XWINDOW(s->plug_window) == winid)
			return ss;
	}
	return NULL;
}

static void
ensure_fixed_and_offscreen(void)
{
	if(!offscreen) {
		/* this is not supposed to happen so give a warning */
		g_warning("offscreen not created yet, we must be on crack");
		status_applet_create_offscreen();
	}
	if(!fixed) {
		/* this is even weirder */
		g_warning("fixed not created yet, we must really be on crack");
		status_applet_create_offscreen();
		fixed = gtk_fixed_new();
		gtk_container_add(GTK_CONTAINER(offscreen), fixed);
		gtk_widget_show_now(fixed);
	}
}

void
status_applet_update(StatusApplet *s)
{
	GSList *li;
	int w,h;
	int sz;
	int rows;
	int i,j;
	
	if(status_inhibit)
		return;
	
	DPUTS("STATUS_APPLET_UPDATE");
	DPRINTD(nspots);

	if(s->orient == GTK_ORIENTATION_HORIZONTAL)
		GTK_HANDLE_BOX(s->handle)->handle_position = GTK_POS_LEFT;
	else
		GTK_HANDLE_BOX(s->handle)->handle_position = GTK_POS_TOP;
	
	sz = s->size;
	
	rows = s->size / DOCKLET_SPOT;

	if (rows <= 0)
		rows = 1;

	if(nspots%rows == 0)
		w = DOCKLET_SPOT*(nspots/rows);
	else
		w = DOCKLET_SPOT*((nspots/rows)+1);
	
	/*make minimum size*/
	if(w < MINIMUM_WIDTH) w = MINIMUM_WIDTH;	

	h = DOCKLET_SPOT*rows;

	/*if we are vertical just switch stuff around*/
	if(s->orient == GTK_ORIENTATION_VERTICAL) {
		int t = w;
		w = h;
		h = t;
	}
	
	gtk_widget_set_size_request (fixed, w, h);

	DPRINTD(w);
	DPRINTD(h);
	
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
	if(s->handle && s->handle->parent)
		gtk_widget_queue_resize(s->handle->parent);
}

static void
status_socket_destroyed(GtkWidget *w, StatusSpot *ss)
{
	/*so that we don't get called recursively, we set the ->socket to
	  null inside status_spot_remove*/
	if(ss->socket)
		status_spot_remove(ss, FALSE);
}

StatusSpot *
new_status_spot(void)
{
	StatusSpot *ss;
	
	if(status_inhibit)
		return NULL;
	
	DPUTS("NEW_STATUS_SPOT");

	ss = g_new0(StatusSpot,1);
	ss->wid = 0;
	ss->sspot = CORBA_OBJECT_NIL;

	spots = g_slist_prepend(spots,ss);
	nspots++;

	ss->socket = gtk_socket_new ();
	gtk_widget_set_size_request (ss->socket, DOCKLET_SPOT, DOCKLET_SPOT);

	/* ensures that fixed and offscreen exist, even though they
	 * should have been created long ago, but I guess it's better
	 * then failing an assert and disappearing */
	ensure_fixed_and_offscreen();

	gtk_fixed_put(GTK_FIXED(fixed),ss->socket,0,0);
	if(the_status)
		status_applet_update(the_status);

	gtk_widget_show_now(ss->socket);
	g_signal_connect (G_OBJECT(ss->socket),"destroy",
			  G_CALLBACK(status_socket_destroyed),
			  ss);
	
	ss->wid = GDK_WINDOW_XWINDOW(ss->socket->window);
	return ss;
}

void
status_spot_remove(StatusSpot *ss, gboolean destroy_socket)
{
	GtkWidget *w;

	spots = g_slist_remove(spots,ss);
	nspots--;
	
	/*set socket to NULL, as to indicate that we have taken
	  care of destruction here*/
	if(destroy_socket) {
		w = ss->socket;
		ss->socket = NULL;
		gtk_widget_destroy(w);
	}

	DPUTS("STATUS_SPOT_REMOVE");
	DPRINTD(nspots);
	DPRINTD(g_slist_length(spots));

	if (ss->sspot != CORBA_OBJECT_NIL) {
		PortableServer_POA       poa;
		PortableServer_ObjectId *id;
		CORBA_Environment        env;

		CORBA_exception_init (&env);

		poa = bonobo_poa ();

		CORBA_Object_release (ss->sspot, &env);

		id = PortableServer_POA_servant_to_id (poa, ss, &env);

		PortableServer_POA_deactivate_object (poa, id, &env);

		CORBA_free (id);

		POA_GNOME_Vertigo_StatusSpot__fini ((PortableServer_Servant)ss, &env);

		CORBA_exception_free (&env);
	}

	g_free(ss);
	if(the_status) status_applet_update(the_status);
}

/*kill all status spots*/
void
status_spot_remove_all(void)
{
	DPUTS("STATUS_SPOT_REMOVE_ALL");

	while(spots)
		status_spot_remove(spots->data, TRUE);
	
	DPUTS("DONE REMOVE_ALL");
}

void
status_applet_put_offscreen(StatusApplet *s)
{
	DPUTS("PUT_OFFSCREEN");

	/* ensures that fixed and offscreen exist, even though they
	 * should have been created long ago, but I guess it's better
	 * then failing an assert and disappearing */
	ensure_fixed_and_offscreen();

	if(fixed->parent != offscreen) {
		DPUTS("REPARENT");
		gtk_widget_reparent(fixed, offscreen);
		DPUTS("REPARENT DONE");
	}
}

void
status_applet_create_offscreen(void)
{
	DPUTS("CREATE OFFSCREEN");
	offscreen = gtk_window_new (GTK_WINDOW_POPUP);
	gtk_window_move (GTK_WINDOW (offscreen),
			 gdk_screen_width () + 10,
			 gdk_screen_height () + 10);
	if (fixed == NULL) {
		fixed = gtk_fixed_new ();
		gtk_widget_show (fixed);

		gtk_container_add (GTK_CONTAINER (offscreen), fixed);
	}

	gtk_widget_show_now(offscreen);

	/* if this fails we are seriously in trouble */
	g_assert(offscreen->window);

	xstuff_setup_kde_dock_thingie(offscreen->window);
}

static int
ignore_1st_click(GtkWidget *widget, GdkEvent *event)
{
	GdkEventButton *buttonevent = (GdkEventButton *)event;

	if((event->type == GDK_BUTTON_PRESS &&
	    buttonevent->button == 1) ||
	   (event->type == GDK_BUTTON_RELEASE &&
	    buttonevent->button == 1)) {
		buttonevent->button = 2;
	}
	 
	return FALSE;
}

static void
free_status (gpointer data)
{
	StatusApplet *s = data;
	g_free(s);

	the_status = NULL;
}

static void
reparent_fixed(GtkWidget *frame)
{
	if(fixed->parent != frame) {
		DPUTS("REPARENT");
		gtk_widget_reparent(fixed, frame);
		DPUTS("REPARENT DONE");
	}
}

gboolean
load_status_applet(PanelWidget *panel, int pos, gboolean exactpos, gboolean use_default)
{
	GtkWidget  *ebox;

	if (the_status != NULL)
		return FALSE;

	DPUTS("LOAD_STATUS_APPLET");
	
	the_status = g_new0(StatusApplet,1);
	the_status->orient = panel->orient;
	the_status->size = panel->sz;
	the_status->handle = gtk_handle_box_new();
	gtk_handle_box_set_shadow_type(GTK_HANDLE_BOX(the_status->handle),
				       GTK_SHADOW_IN);
	gtk_container_set_border_width(GTK_CONTAINER(the_status->handle),
				       1);
	g_signal_connect (G_OBJECT(the_status->handle), "event",
			  G_CALLBACK(ignore_1st_click), NULL);

	if(!fixed) {
		DPUTS("NO FIXED");
		fixed = gtk_fixed_new();
		gtk_container_set_border_width(GTK_CONTAINER(fixed),
					       0);
		gtk_widget_show(fixed);

		gtk_container_add(GTK_CONTAINER(the_status->handle), fixed);
	} else {
		g_signal_connect_after (G_OBJECT(the_status->handle), "realize",
				        G_CALLBACK(reparent_fixed),
					NULL);
	}
	
	status_applet_update(the_status);
	
	ebox = gtk_event_box_new();
	gtk_container_set_border_width(GTK_CONTAINER(ebox),
				       0);
	gtk_widget_show(ebox);
	gtk_container_add(GTK_CONTAINER(ebox),the_status->handle);

	the_status->info = panel_applet_register (ebox, the_status, free_status, panel,
						  pos, exactpos, APPLET_STATUS, NULL);
	if (the_status->info == NULL)
		return TRUE;

	panel_applet_add_callback (the_status->info, "help", GTK_STOCK_HELP, _("Help"));

	return TRUE;
}
