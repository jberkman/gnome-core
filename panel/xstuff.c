/*
 * GNOME panel x stuff
 * Copyright 2000 Eazel, Inc.
 *
 * Authors: George Lebl
 */
#include <config.h>
#include <gnome.h>
#include <gdk/gdkx.h>

#include <X11/Xmd.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include "panel-include.h"
#include "gnome-panel.h"
#include "global-keys.h"
#include "gwmh.h"

#include "xstuff.h"

static GdkAtom KWM_MODULE = 0;
static GdkAtom KWM_MODULE_DOCKWIN_ADD = 0;
static GdkAtom KWM_MODULE_DOCKWIN_REMOVE = 0;
static GdkAtom KWM_DOCKWINDOW = 0;
static GdkAtom NAUTILUS_DESKTOP_WINDOW_ID = 0;
static GdkAtom _NET_WM_DESKTOP = 0;

extern GList *check_swallows;

/*list of all panel widgets created*/
extern GSList *panel_list;

static void
steal_statusspot(StatusSpot *ss, Window winid)
{
	GdkDragProtocol protocol;

	gtk_socket_steal(GTK_SOCKET(ss->socket), winid);
	if (gdk_drag_get_protocol (winid, &protocol))
		gtk_drag_dest_set_proxy (GTK_WIDGET (ss->socket),
					 GTK_SOCKET(ss->socket)->plug_window,
					 protocol, TRUE);
}

static void
try_adding_status(guint32 winid)
{
	guint32 *data;
	int size;

	if (status_applet_get_ss (winid))
		return;

	data = get_typed_property_data (GDK_DISPLAY (),
					winid,
					KWM_DOCKWINDOW,
					KWM_DOCKWINDOW,
					&size, 32);

	if(data && *data) {
		StatusSpot *ss;
		ss = new_status_spot ();
		if (ss != NULL)
			steal_statusspot (ss, winid);
	}
	g_free(data);
}

static void
try_checking_swallows (guint32 winid)
{
	char *tit = NULL;
	if (XFetchName (GDK_DISPLAY (), winid, &tit) &&
	    tit != NULL) {
		GList *li;
		for (li = check_swallows; li;
		     li = li->next) {
			Swallow *swallow = li->data;
			if (strstr (tit, swallow->title) != NULL) {
				swallow->wid = winid;
				gtk_socket_steal (GTK_SOCKET (swallow->socket),
						  swallow->wid);
				check_swallows = 
					g_list_remove (check_swallows, swallow);
				break;
			}
		}
		XFree (tit);
	}
}

void
xstuff_go_through_client_list (void)
{
	GList *li;

	gdk_error_trap_push ();
	/* just for status dock stuff for now */
	for (li = gwmh_task_list_get (); li != NULL; li = li->next) {
		GwmhTask *task = li->data;
		if (check_swallows != NULL)
			try_checking_swallows (task->xwin);
		try_adding_status (task->xwin);
	}
	gdk_flush();
	gdk_error_trap_pop ();
}

static void
redo_interface (void)
{
	GSList *li;
	for (li = panel_list; li != NULL; li = li->next) {
		PanelData *pd = li->data;
		if (IS_BASEP_WIDGET (pd->panel))
			basep_widget_redo_window (BASEP_WIDGET (pd->panel));
		else if (IS_FOOBAR_WIDGET (pd->panel))
			foobar_widget_redo_window (FOOBAR_WIDGET (pd->panel));
	}
}

/* some deskguide code borrowed */
static gboolean
desk_notifier (gpointer func_data,
	       GwmhDesk *desk,
	       GwmhDeskInfoMask change_mask)
{
	if (change_mask & GWMH_DESK_INFO_BOOTUP)
		redo_interface ();

	/* we should maybe notice desk changes here */

	return TRUE;
}

static gboolean
task_notifier (gpointer func_data,
	       GwmhTask *task,
	       GwmhTaskNotifyType ntype,
	       GwmhTaskInfoMask imask)
{
	switch (ntype) {
	case GWMH_NOTIFY_INFO_CHANGED:
	case GWMH_NOTIFY_NEW:
		if (check_swallows != NULL)
			try_checking_swallows (task->xwin);
		try_adding_status (task->xwin);
		break;
	default:
		break;
	}

	return TRUE;
}

void
xstuff_init (void)
{
	KWM_MODULE = gdk_atom_intern ("KWM_MODULE", FALSE);
	KWM_MODULE_DOCKWIN_ADD =
		gdk_atom_intern ("KWM_MODULE_DOCKWIN_ADD", FALSE);
	KWM_MODULE_DOCKWIN_REMOVE =
		gdk_atom_intern ("KWM_MODULE_DOCKWIN_REMOVE", FALSE);
	KWM_DOCKWINDOW = gdk_atom_intern ("KWM_DOCKWINDOW", FALSE);
	NAUTILUS_DESKTOP_WINDOW_ID =
		gdk_atom_intern ("NAUTILUS_DESKTOP_WINDOW_ID", FALSE);
	_NET_WM_DESKTOP =
		gdk_atom_intern ("_NET_WM_DESKTOP", FALSE);

	gwmh_init ();

	gwmh_desk_notifier_add (desk_notifier, NULL);
	gwmh_task_notifier_add (task_notifier, NULL);

	/* setup the keys filter */
	gdk_window_add_filter (GDK_ROOT_PARENT(),
			       panel_global_keys_filter,
			       NULL);

	xstuff_go_through_client_list ();
}

gboolean
xstuff_nautilus_desktop_present (void)
{
	gboolean ret = FALSE;
	guint32 *data;
	int size;

	gdk_error_trap_push ();
	data = get_typed_property_data (GDK_DISPLAY (),
					GDK_ROOT_WINDOW (),
					NAUTILUS_DESKTOP_WINDOW_ID,
					XA_WINDOW,
					&size, 32);
	if (data != NULL &&
	    *data != 0) {
		guint32 *desktop;
		desktop = get_typed_property_data (GDK_DISPLAY (),
						   *data,
						   _NET_WM_DESKTOP,
						   XA_CARDINAL,
						   &size, 32);
		if (size > 0)
			ret = TRUE;
		g_free (desktop);
	}
	g_free (data);

	gdk_flush ();
	gdk_error_trap_pop ();

	return ret;
}

void
xstuff_set_simple_hint (GdkWindow *w, GdkAtom atom, int val)
{
	gdk_error_trap_push ();
	XChangeProperty (GDK_DISPLAY(), GDK_WINDOW_XWINDOW(w), atom, atom,
			 32, PropModeReplace, (unsigned char*)&val, 1);
	gdk_flush ();
	gdk_error_trap_pop ();
}

static GdkFilterReturn
status_event_filter (GdkXEvent *gdk_xevent, GdkEvent *event, gpointer data)
{
	XEvent *xevent;

	xevent = (XEvent *)gdk_xevent;

	if(xevent->type == ClientMessage) {
		if(xevent->xclient.message_type == KWM_MODULE_DOCKWIN_ADD &&
		   !status_applet_get_ss(xevent->xclient.data.l[0])) {
			Window w = xevent->xclient.data.l[0];
			StatusSpot *ss;
			ss = new_status_spot ();
			if (ss != NULL)
				steal_statusspot (ss, w);
		} else if(xevent->xclient.message_type ==
			  KWM_MODULE_DOCKWIN_REMOVE) {
			StatusSpot *ss;
			ss = status_applet_get_ss (xevent->xclient.data.l[0]);
			if (ss != NULL)
				status_spot_remove (ss, TRUE);
		}
	}

	return GDK_FILTER_CONTINUE;
}

void
xstuff_setup_kde_dock_thingie (GdkWindow *w)
{
	xstuff_set_simple_hint (w, KWM_MODULE, 2);
	gdk_window_add_filter (w, status_event_filter, NULL);
	send_client_message_1L (GDK_ROOT_WINDOW (), GDK_WINDOW_XWINDOW (w),
				KWM_MODULE, SubstructureNotifyMask,
				GDK_WINDOW_XWINDOW (w));
}

/* Stolen from deskguide */
gpointer
get_typed_property_data (Display *xdisplay,
			 Window   xwindow,
			 Atom     property,
			 Atom     requested_type,
			 gint    *size_p,
			 guint    expected_format)
{
  static const guint prop_buffer_lengh = 1024 * 1024;
  unsigned char *prop_data = NULL;
  Atom type_returned = 0;
  unsigned long nitems_return = 0, bytes_after_return = 0;
  int format_returned = 0;
  gpointer data = NULL;
  gboolean abort = FALSE;

  g_return_val_if_fail (size_p != NULL, NULL);
  *size_p = 0;

  gdk_error_trap_push ();

  abort = XGetWindowProperty (xdisplay,
			      xwindow,
			      property,
			      0, prop_buffer_lengh,
			      False,
			      requested_type,
			      &type_returned, &format_returned,
			      &nitems_return,
			      &bytes_after_return,
			      &prop_data) != Success;
  if (gdk_error_trap_pop () ||
      type_returned == None)
    abort++;
  if (!abort &&
      requested_type != AnyPropertyType &&
      requested_type != type_returned)
    {
      g_warning (G_GNUC_PRETTY_FUNCTION "(): Property has wrong type, probably on crack");
      abort++;
    }
  if (!abort && bytes_after_return)
    {
      g_warning (G_GNUC_PRETTY_FUNCTION "(): Eeek, property has more than %u bytes, stored on harddisk?",
		 prop_buffer_lengh);
      abort++;
    }
  if (!abort && expected_format && expected_format != format_returned)
    {
      g_warning (G_GNUC_PRETTY_FUNCTION "(): Expected format (%u) unmatched (%d), programmer was drunk?",
		 expected_format, format_returned);
      abort++;
    }
  if (!abort && prop_data && nitems_return && format_returned)
    {
      switch (format_returned)
	{
	case 32:
	  *size_p = nitems_return * 4;
	  if (sizeof (gulong) == 8)
	    {
	      guint32 i, *mem = g_malloc0 (*size_p + 1);
	      gulong *prop_longs = (gulong*) prop_data;

	      for (i = 0; i < *size_p / 4; i++)
		mem[i] = prop_longs[i];
	      data = mem;
	    }
	  break;
	case 16:
	  *size_p = nitems_return * 2;
	  break;
	case 8:
	  *size_p = nitems_return;
	  break;
	default:
	  g_warning ("Unknown property data format with %d bits (extraterrestrial?)",
		     format_returned);
	  break;
	}
      if (!data && *size_p)
	{
	  guint8 *mem = g_malloc (*size_p + 1);

	  memcpy (mem, prop_data, *size_p);
	  mem[*size_p] = 0;
	  data = mem;
	}
    }

  if (prop_data)
    XFree (prop_data);
  
  return data;
}

/* sorta stolen from deskguide */
gboolean
send_client_message_1L (Window recipient,
			Window event_window,
			Atom   message_type,
			long   event_mask,
			glong  long1)
{
  XEvent xevent = { 0 };

  xevent.type = ClientMessage;
  xevent.xclient.window = event_window;
  xevent.xclient.message_type = message_type;
  xevent.xclient.format = 32;
  xevent.xclient.data.l[0] = long1;

  gdk_error_trap_push ();

  XSendEvent (GDK_DISPLAY (), recipient, False, event_mask, &xevent);
  gdk_flush ();

  return !gdk_error_trap_pop ();
}

gboolean
xstuff_is_compliant_wm (void)
{
	GwmhDesk *desk;

	desk = gwmh_desk_get_config ();

	return desk->detected_gnome_wm;
}

void
xstuff_set_no_group_and_no_input (GdkWindow *win)
{
	XWMHints *old_wmhints;
	XWMHints wmhints = {0};
	static GdkAtom wm_client_leader_atom = GDK_NONE;

	if (wm_client_leader_atom == GDK_NONE)
		wm_client_leader_atom = gdk_atom_intern ("WM_CLIENT_LEADER",
							 FALSE);

	gdk_property_delete (win, wm_client_leader_atom);

	old_wmhints = XGetWMHints (GDK_DISPLAY (), GDK_WINDOW_XWINDOW (win));
	/* General paranoia */
	if (old_wmhints != NULL) {
		memcpy (&wmhints, old_wmhints, sizeof (XWMHints));
		XFree (old_wmhints);

		wmhints.flags &= ~WindowGroupHint;
		wmhints.flags |= InputHint;
		wmhints.input = False;
		wmhints.window_group = 0;
	} else {
		/* General paranoia */
		wmhints.flags = InputHint | StateHint;
		wmhints.window_group = 0;
		wmhints.input = False;
		wmhints.initial_state = NormalState;
	}
	XSetWMHints (GDK_DISPLAY (),
		     GDK_WINDOW_XWINDOW (win),
		     &wmhints);
}
