/* fvwm-pager
 *
 * Copyright (C) 1998 Michael Lausch
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <stdio.h>
#include <signal.h>

#include "module.h"
#include "fvwm.h"
#include "fvwmlib.h"

#include <gnome.h>
#include <gdk/gdkx.h>
#include <X11/Xatom.h>

#include "properties.h"
#include "gtkpager.h"

#include <applet-widget.h>
#include "fvwm-pager.h"


void        DeadPipe             (int);
static void ParseOptions         (void);
static void switch_to_desktop    (GtkFvwmPager* unused, int desktop_offset, gpointer sendit);
void        set_focus(GtkFvwmPager* pager, unsigned long* body);

#if 0
static void move_window          (GtkFvwmPager* pager, unsigned long xid, int desktop, int x, int y);
#endif

PagerProps pager_props;
int  ndesks;

gint   pager_width  = 170;
gint   pager_height = 70;

static int noapplet = 0;
static GList* desktop_names = 0;

static Atom _XA_WIN_WORKSPACE;
static Atom _XA_WIN_WORKSPACE_NAMES;
static Atom _XA_WIN_WORKSPACE_COUNT;
static Atom _XA_WIN_LAYER;
static Atom _XA_WIN_SUPPORTING_WM_CHECK;
static Atom _XA_WIN_CLIENT_LIST;
static Atom _XA_WIN_STATE;

static int        desk1;
static int        desk2;
static int        fd[2];
static char*      pgm_name;

unsigned long xprop_long_data[20];
unsigned char xprop_char_data[128];


void process_message    (GtkFvwmPager* pager, unsigned long, unsigned long*);
void destroy            (GtkWidget* w, gpointer data);
void about_cb           (AppletWidget* widget, gpointer data);
void properties_dialog  (AppletWidget *widget, gpointer data);
void configure_window   (GtkFvwmPager* pager, unsigned long* body);
void configure_icon     (GtkFvwmPager* pager, unsigned long* body);
void destroy_window     (GtkFvwmPager* pager, unsigned long* body);
void deiconify_window   (GtkFvwmPager* pager, unsigned long* body);
void add_window         (GtkFvwmPager* pager, unsigned long* body);


GtkWidget* root_widget = 0;

void destroy(GtkWidget* w, gpointer data)
{
  gtk_main_quit();
}

/* UTILITY functions */

static void *
util_get_atom(Window win, gchar *atom, Atom type, gint *size)
{
  unsigned char      *retval;
  Atom                to_get, type_ret;
  unsigned long       bytes_after, num_ret;
  int                 format_ret;
  long                length;
  void               *data;
  
  to_get = XInternAtom(GDK_DISPLAY(), atom, False);
  retval = NULL;
  length = 0x7fffffff;
  XGetWindowProperty(GDK_DISPLAY(), win, to_get, 0,
		     length,
		     False, type,
		     &type_ret,
		     &format_ret,
		     &num_ret,
		     &bytes_after,
		     &retval);
  if ((retval) && (num_ret > 0) && (format_ret > 0))
    {
      data = g_malloc(num_ret * (format_ret >> 3));
      if (data)
	memcpy(data, retval, num_ret * (format_ret >> 3));
      XFree(retval);
      *size = num_ret * (format_ret >> 3);
      return data;
    }
  return NULL;
}


static void
fvwm_command_received(gpointer data,
		      gint     source,
		      GdkInputCondition cond)
{
  int count;
  unsigned long header[HEADER_SIZE];
  unsigned long* body;
  GtkFvwmPager*  pager = data ? GTK_FVWMPAGER(data) : 0;

  if ((count = ReadFvwmPacket(source, header, &body)) > 0)
    {
      if (body) {
	process_message(pager, header[1], body);
	free(body);
      }
      else
	fprintf(stderr,"NULL body message received\n");
      fprintf(stderr,"Message process, waiting for new message\n");
    }
}

void
about_cb(AppletWidget* widget, gpointer data)
{
  GtkWidget* about;
  gchar* authors[] ={"M. Lausch", NULL};

  about = gnome_about_new( _("Fvwm Pager Applet"),
			   "0.4",
			   _("Copyright (C) 1998 M. Lausch"),
			   (const gchar**)authors,
			   "Pager for Fvwm2 window manager",
			   0);
  gtk_widget_show(about);
}

static gint
save_session (GtkWidget* widget, char* privcfgpath,
	      char* globcfgpath, gpointer data)
{
  save_fvwmpager_properties ("fvwmpager", &pager_props);
  return FALSE;
}

volatile int wfd = 1;

static void
substruct_notify(GtkWidget* rw, GdkEvent* ev)
{
  GdkEventClient*  cev;
  gint             desktop;
  
  if (ev->type != GDK_CLIENT_EVENT)
    return ;
  cev = (GdkEventClient*)ev;
  if (cev->message_type != _XA_WIN_WORKSPACE)
    return;
  desktop = cev->data.l[0];
  switch_to_desktop(0, desktop, 0);
  XChangeProperty(GDK_DISPLAY(), GDK_ROOT_WINDOW(),
		  _XA_WIN_WORKSPACE, XA_CARDINAL,
		  32, PropModeReplace, (unsigned char*)&desktop,
		  1);
}

GdkFilterReturn root_widget_filter(GdkXEvent* xevent,
				   GdkEvent*  event,
				   gpointer   data);


GdkFilterReturn
root_widget_filter(GdkXEvent* xevent, GdkEvent*  event, gpointer  data)
{
  XEvent* xev = (XEvent*) xevent;
  if (xev->type == ClientMessage)
    {
      return GDK_FILTER_CONTINUE;
    }
  return GDK_FILTER_REMOVE;
}
    

int
main(int argc, char* argv[])
{
  GtkWidget* window = 0;
  char bfr[128];
  int  idx;
  char* tmp;
  static GtkWidget* pager = 0;
  GList* desktops;
  Window root_window;
  gint   desktop_idx = 6;

  
  
  while (!wfd)
    ;
  tmp = strrchr(argv[0], '/');
  if (tmp)
    tmp++;
  else
    tmp = argv[0];
  pgm_name = strdup(tmp);
  
  signal(SIGPIPE, DeadPipe);

  fd[0] = atoi(argv[1]);
  fd[1] = atoi(argv[2]);
  
  if (strcmp(argv[6],"HeadLess") == 0)
    {
      desktop_idx = 7;
      noapplet = 1;
    }
  desk1 = atoi(argv[desktop_idx]);
  desk2 = atoi(argv[desktop_idx+1]);

  if (noapplet)
    gnome_init("fvwmpager", VERSION, argc, argv);
  else
    applet_widget_init("#fvwmpager", VERSION, argc, argv,
				NULL, 0, NULL);

  _XA_WIN_WORKSPACE           = XInternAtom(GDK_DISPLAY(), XA_WIN_WORKSPACE, False);
  _XA_WIN_WORKSPACE_NAMES     = XInternAtom(GDK_DISPLAY(), XA_WIN_WORKSPACE_NAMES, False);
  _XA_WIN_WORKSPACE_COUNT     = XInternAtom(GDK_DISPLAY(), XA_WIN_WORKSPACE_COUNT, False);
  _XA_WIN_LAYER               = XInternAtom(GDK_DISPLAY(), XA_WIN_LAYER, False);
  _XA_WIN_SUPPORTING_WM_CHECK = XInternAtom(GDK_DISPLAY(), XA_WIN_SUPPORTING_WM_CHECK, False);
  _XA_WIN_CLIENT_LIST         = XInternAtom(GDK_DISPLAY(), XA_WIN_CLIENT_LIST, False);
  _XA_WIN_STATE               = XInternAtom(GDK_DISPLAY(), XA_WIN_STATE, False);

  root_window = GDK_ROOT_WINDOW();
#if 1
  root_widget = gtk_window_new(GTK_WINDOW_POPUP);
  fprintf(stderr,"root_widget is at %p\n", root_widget);
  gtk_widget_realize(root_widget);
  fprintf(stderr,"root_widget realized\n");
  gdk_window_set_user_data(GDK_ROOT_PARENT(), root_widget);
  gdk_window_ref(GDK_ROOT_PARENT());
  gdk_xid_table_insert(&(GDK_WINDOW_XWINDOW(GDK_ROOT_PARENT())), 
		       GDK_ROOT_PARENT());
  gdk_window_add_filter(GDK_ROOT_PARENT(), root_widget_filter, 0);
#else
  root_widget = gnome_rootwin_new();
  if (!root_widget)
    fprintf(stderr," * * * NO ROOT WIDGET HERE * * *\n");
  gtk_widget_realize(root_widget);
#endif  
  gtk_signal_connect(GTK_OBJECT(root_widget), "event",
		     GTK_SIGNAL_FUNC(substruct_notify), 0);
  fprintf(stderr,"root_widget signal connected\n");

  XSelectInput(GDK_DISPLAY(), GDK_WINDOW_XWINDOW(GDK_ROOT_PARENT()), 
	       SubstructureNotifyMask | KeyPressMask);
  fprintf(stderr,"root_widget event mask added\n");

  XChangeProperty(GDK_DISPLAY(), GDK_ROOT_WINDOW(),
		  _XA_WIN_SUPPORTING_WM_CHECK, XA_CARDINAL,
		  32, PropModeReplace, (unsigned char*)&root_window, 1);

  /* Clear the client list */
  XDeleteProperty(GDK_DISPLAY(), GDK_ROOT_WINDOW(), _XA_WIN_CLIENT_LIST);
  
  XChangeProperty(GDK_DISPLAY(), GDK_ROOT_WINDOW(),
		  _XA_WIN_SUPPORTING_WM_CHECK, XA_CARDINAL,
		  32, PropModeReplace, (unsigned char*)&root_window, 1);
  if (desk1 > desk2)
    {
      int itemp;
      itemp = desk1;
      desk1 = desk2;
      desk2 = itemp;
    }
  ndesks = desk2 - desk1 + 1;

  xprop_long_data[0] = ndesks;
  
  XChangeProperty(GDK_DISPLAY(), GDK_ROOT_WINDOW(), _XA_WIN_WORKSPACE_COUNT, XA_CARDINAL,
		  32, PropModeReplace, (unsigned char*) xprop_long_data, 1);
  fprintf(stderr,"Got %d desktops\n", ndesks);
  
  desktops = 0;
  for (idx = 0; idx < ndesks; idx++)
    {
      Desktop* new_desk;
      
      g_snprintf(bfr, sizeof(bfr), _("Desk %d"), idx);
      new_desk = g_new0(Desktop, 1);
      new_desk->title = g_strdup(bfr);
      desktops = g_list_append(desktops, new_desk);
    }
  SetMessageMask(fd,
	     M_ADD_WINDOW       |
	     M_CONFIGURE_WINDOW |
	     M_DESTROY_WINDOW   |
	     M_FOCUS_CHANGE     |
	     M_NEW_PAGE         |
	     M_NEW_DESK         |
	     M_RAISE_WINDOW     |
	     M_LOWER_WINDOW     |
	     M_ICONIFY          |
	     M_ICON_LOCATION    |
	     M_DEICONIFY        |
	     M_ICON_NAME        |
	     M_CONFIG_INFO      |
	     M_END_CONFIG_INFO  |
	     M_MINI_ICON        |
	     M_END_WINDOWLIST);
  fprintf(stderr,"Parsing options\n");
  ParseOptions();
  fprintf(stderr,"Options parsed\n");
  {
    GList* elem;
    XTextProperty tp;
    char*  desk_names[ndesks];
    int i;

    elem = desktops;
    
    for (i = 0; i < ndesks; i++)
      {
	Desktop* desk;
	desk = elem->data;
	
	desk_names[i] = desk->title;
	elem = g_list_next(elem);
      }
    if (!XStringListToTextProperty(desk_names, ndesks, &tp))
      {
	g_log("fvwm-pager", G_LOG_LEVEL_ERROR, "Cannot get textproperty for workspace names\n");
      }
    else
      {
	XSetTextProperty(GDK_DISPLAY(), GDK_ROOT_WINDOW(), &tp, _XA_WIN_WORKSPACE_NAMES);
      }
  }

  fprintf(stderr,"Loading options\n");
  load_fvwmpager_properties("fvwmpager", &pager_props); 
  fprintf(stderr,"Options loaded\n");
      
      
  pager = gtk_fvwmpager_new(fd, pager_props.width, pager_props.height);
  
  if (!noapplet)
    {
      gchar* name;
      GList* desktop_ptr;
      gint   desk = 0;
      
      
      window = applet_widget_new("fvwmpager_applet"); 
      gtk_widget_realize(window); 
      gtk_widget_set_usize(GTK_WIDGET(window), pager_props.width, pager_props.height); 
      gtk_signal_connect(GTK_OBJECT(pager), "switch_desktop",
			 GTK_SIGNAL_FUNC(switch_to_desktop), (gpointer)1);
      
      gtk_signal_connect(GTK_OBJECT(window), "delete_event",
			 GTK_SIGNAL_FUNC(destroy), NULL);
      
      gtk_signal_connect(GTK_OBJECT(window), "save_session",
			 GTK_SIGNAL_FUNC(save_session),
			 NULL);
      desktop_ptr = desktop_names;
      while (desktop_ptr) {
	name = desktop_ptr->data;
	gtk_fvwmpager_label_desk(GTK_FVWMPAGER(pager), desk, name);
	desk++;
	desktop_ptr = g_list_next(desktop_ptr);
      }
      applet_widget_add(APPLET_WIDGET(window), pager);
      
      applet_widget_register_stock_callback(APPLET_WIDGET(window),
					    "about",
					    GNOME_STOCK_MENU_ABOUT,
					    _("About	"),
					    about_cb,
					    NULL);
      applet_widget_register_stock_callback(APPLET_WIDGET(window),
					    "properties",
					    GNOME_STOCK_MENU_PROP,
					    _("Properties"),
					    pager_properties_dialog,
					    pager);
    }
  gtk_fvwmpager_set_desktops(GTK_FVWMPAGER(pager), desktops);
  if (!noapplet) {
    gtk_fvwmpager_display_desks(GTK_FVWMPAGER(pager));
    gtk_widget_show(window);
    gtk_widget_show(pager);
  }
  gdk_input_add(fd[1],
		GDK_INPUT_READ,
		fvwm_command_received,
		pager);
  
  SendInfo(fd, "Send_WindowList", 0);
  applet_widget_gtk_main();
  fprintf(stderr,"applet_wiget_gtk_main returned\n");
  if (!noapplet)
    save_fvwmpager_properties ("fvwmpager", &pager_props);
  return 0;
}

void
switch_to_desktop(GtkFvwmPager* unused, int offset, gpointer data)
{
  char    command[256];
  XEvent  xev;
  
  snprintf(command, sizeof(command), "Desk 0 %d\n", offset + desk1);
  SendInfo(fd,command, 0);
  if (data)
    {
      xev.type = ClientMessage;
      xev.xclient.type = ClientMessage;
      xev.xclient.window = GDK_ROOT_WINDOW();
      xev.xclient.message_type = _XA_WIN_WORKSPACE;
      xev.xclient.format = 32;
      xev.xclient.data.l[0] = offset;
      xev.xclient.data.l[1] = gdk_time_get();
      XSendEvent(GDK_DISPLAY(), GDK_ROOT_WINDOW(), False,
		 SubstructureNotifyMask, &xev);
    }
}

void
set_window_desktop(Window xid, gint desktop)
{
  XChangeProperty(GDK_DISPLAY(), xid,
		  _XA_WIN_WORKSPACE, XA_CARDINAL,
		  32, PropModeReplace, (unsigned char*)&desktop, 1);
}		  

void
configure_window(GtkFvwmPager* pager, unsigned long* body)
{
  PagerWindow* win;
  gint         xid = body[0];
  PagerWindow  old;
  gint         new_window = 0;

  win = g_hash_table_lookup(pager->windows, (gconstpointer)xid);
  fprintf(stderr,"configure_window: Found window ptr %p in window hash table\n", win);
  if (!win)
    {
      win = g_new0(PagerWindow, 1);
      win->xid    = xid;
      win->ix     = -1000;
      win->iy     = -1000;
      win->iw     = 0;
      win->ih     = 0;
      win->flags  = 0;
      if (xid == 0)
	{
	  fprintf(stderr,"Inserting window with xid 0\n");
	  return;
	}
      g_hash_table_insert(pager->windows, (gpointer)xid, win);
      new_window = 1;
      XChangeProperty(GDK_DISPLAY(), GDK_ROOT_WINDOW(),
		      _XA_WIN_CLIENT_LIST, XA_CARDINAL,
		      32,PropModeAppend, (unsigned char*)&xid, 1);
      fprintf(stderr,"Added new window\n");
    }
  old           = *win;
  win->x        = body[3];
  win->y        = body[4];
  win->w        = body[5];
  win->h        = body[6];
  win->th       = body[9];
  win->bw       = body[10];
  win->desk     = g_list_nth(pager->desktops, body[7])->data;
  win->flags    = 0;
  win->ixid     = body[20];
  XChangeProperty(GDK_DISPLAY(), xid,
		  _XA_WIN_WORKSPACE, XA_CARDINAL,
		  32, PropModeReplace, (unsigned char*)&body[7], 1);

  if (body[8] & ICONIFIED)
    {
      win->flags |= GTKPAGER_WINDOW_ICONIFIED;
    }
  if (body[8] & STICKY)
    {
      gint           size;
      guint          new_state = 0;
      
      GnomeWinState* win_state = util_get_atom(xid, "_WIN_STATE",
					       XA_CARDINAL, &size);
      win->flags |= GTKPAGER_WINDOW_STICKY;
      if (win_state)
	new_state = *win_state;
      new_state  = WIN_STATE_STICKY;
      
      XChangeProperty(GDK_DISPLAY(), xid,
		      _XA_WIN_STATE, XA_CARDINAL,
		      32, PropModeReplace, (char*)&new_state, 1);
    }
  if (!noapplet)
    gtk_fvwmpager_display_window(GTK_FVWMPAGER(pager), new_window ? 0 : &old, xid);
}

void configure_icon(GtkFvwmPager* pager, unsigned long* body)
{
  PagerWindow* win;
  PagerWindow  old;
  
  gint         xid = body[0];
  
  win = g_hash_table_lookup(pager->windows, (gconstpointer)xid);
  if (!win)
    {
      return;
    }
  old = *win;
  win->flags  |= GTKPAGER_WINDOW_ICONIFIED;
  win->ix      = body[3];
  win->iy      = body[4];
  win->iw      = body[5];
  win->ih      = body[6];
  
  if (!noapplet)
    gtk_fvwmpager_display_window(GTK_FVWMPAGER(pager), &old, xid);
}

    
void
destroy_window(GtkFvwmPager* pager, unsigned long* body)
{
  unsigned long xid;
  Window*       winarray;
  Window*       ptr;
  gint          size;
  gint          nelems;
  gint          current = 0;
  PagerWindow*  win;
  
  xid = body[0];
  if (!noapplet)
    gtk_fvwmpager_destroy_window(GTK_FVWMPAGER(pager), xid);
  win = g_hash_table_lookup(pager->windows, (gconstpointer)xid);
  if (!win)
    {
      fprintf(stderr,"Warning: Deleting unknown window (not in hash table) 0x%08lx\n", xid);
    }
  else
    {
      g_hash_table_remove(pager->windows, (gconstpointer)xid);
      g_free(win);
    }
  winarray = util_get_atom(GDK_ROOT_WINDOW(), "_WIN_CLIENT_LIST", XA_CARDINAL, &size);
  nelems = size / sizeof(Window);
  ptr = winarray;
  while (current < nelems) {
    if (*ptr == xid)
      break;
    current++;
    ptr++;
  }
  if (*ptr != xid)
    return;
  memcpy(ptr, ptr+1, (nelems - current) * sizeof (Window));
  XChangeProperty(GDK_DISPLAY(), GDK_ROOT_WINDOW(),
		  _XA_WIN_CLIENT_LIST, XA_CARDINAL,
		  32, PropModeReplace, (unsigned char*) winarray,
		  nelems - 1);
}

void
set_focus(GtkFvwmPager* pager, unsigned long* body)
{
  unsigned long xid;

  xid = body[0];
  if (!noapplet)
    gtk_fvwmpager_set_current_window(GTK_FVWMPAGER(pager), xid);
}

void
deiconify_window(GtkFvwmPager* pager, unsigned long* body)
{
  unsigned long xid;
  PagerWindow   old;
  PagerWindow*  win;

  xid = body[0];
  win = g_hash_table_lookup(pager->windows, (gconstpointer)xid);
  if (!win)
    return;
  old = *win;
  win->flags &= ~GTKPAGER_WINDOW_ICONIFIED;
  if (!noapplet)
    gtk_fvwmpager_display_window(GTK_FVWMPAGER(pager), &old, xid);
}

void
add_window(GtkFvwmPager* pager, unsigned long* body)
{
  unsigned long xid;
  PagerWindow*  win;

  xid = body[0];
  win = g_hash_table_lookup(pager->windows, (gconstpointer)xid);
  if (!win)
    {
      return;
    }
  if (!noapplet)
    gtk_fvwmpager_display_window(GTK_FVWMPAGER(pager), 0, xid);
}


void process_message(GtkFvwmPager* pager, unsigned long type,unsigned long *body)
{
  switch(type)
    {
    case M_ADD_WINDOW:
      g_log("fvwm-pager", G_LOG_LEVEL_DEBUG, "test-message: M_ADD_WINDOW received\n");
      add_window(pager, body);
      break;
    case M_CONFIGURE_WINDOW:
      g_log("fvwm-pager", G_LOG_LEVEL_DEBUG, "message: M_CONFIGURE_WINDOW received\n");
      configure_window(pager, body);
      break;
    case M_DESTROY_WINDOW:
      g_log("fvwm-pager", G_LOG_LEVEL_DEBUG, "message: M_DESTROY_WINDOW received\n");
      destroy_window(pager, body);
      break;
    case M_FOCUS_CHANGE:
#if 0
      g_log("fvwm-pager", G_LOG_LEVEL_DEBUG, "message: M_FOCUS_CHANGE received\n");
#endif
      set_focus(pager, body);
      break;
    case M_NEW_PAGE:
      g_log("fvwm-pager", G_LOG_LEVEL_DEBUG, "message: unhandled M_ADD_WINDOW received\n");
      break;
    case M_NEW_DESK:
      {
	long desktop = (long)body[0];

	g_log("fvwm-pager", G_LOG_LEVEL_DEBUG, "message: M_NEW_DESK received, setting workspace prop to %ld\n", desktop);
	if (desktop >= ndesks)
	  {
	    fprintf(stderr,"Desktop %ld out of range, not switching\n", desktop);
	    return;
	  }
	if (!noapplet)
	  gtk_fvwmpager_set_current_desk(GTK_FVWMPAGER(pager), desktop);

	switch_to_desktop(0, desktop, (gpointer)1);
	XChangeProperty(GDK_DISPLAY(), GDK_ROOT_WINDOW(),
			_XA_WIN_WORKSPACE, XA_CARDINAL,
			32, PropModeReplace, (unsigned char*)&desktop,
			1);
	g_log("fvwm-pager", G_LOG_LEVEL_DEBUG, "message: property changed\n");
	  
      }
      break;
    case M_RAISE_WINDOW:
      g_log("fvwm-pager", G_LOG_LEVEL_DEBUG, "message: M_RAISE_WINDOW received\n");
      if (!noapplet)
	gtk_fvwmpager_raise_window(GTK_FVWMPAGER(pager), body[0]);
      break;
    case M_LOWER_WINDOW:
      g_log("fvwm-pager", G_LOG_LEVEL_DEBUG, "message: M_LOWER_WINDOW received\n");
      if (!noapplet)
	gtk_fvwmpager_lower_window(GTK_FVWMPAGER(pager), body[0]);
      break;
    case M_ICONIFY:
    case M_ICON_LOCATION:
      g_log("fvwm-pager", G_LOG_LEVEL_DEBUG, "message: M_ICONIFY || M_ICON_LOCATION received\n");
      configure_icon(pager, body);
      break;
    case M_DEICONIFY:
      g_log("fvwm-pager", G_LOG_LEVEL_DEBUG, "message: M_DEICONIFY received\n");
      deiconify_window(pager, body); 
      break;
    case M_ICON_NAME:
      g_log("fvwm-pager", G_LOG_LEVEL_DEBUG, "message: unhandled M_AICON_NAME received\n");
      break;
    case M_MINI_ICON:
      g_log("fvwm-pager", G_LOG_LEVEL_DEBUG, "message: unhandled M_MINI_ICON received\n");
      break;
    case M_END_WINDOWLIST:
      g_log("fvwm-pager", G_LOG_LEVEL_DEBUG, "message: unhandled M_END_WINDOWLIST received\n");
      break;
    default:
      g_log("fvwm-pager", G_LOG_LEVEL_ERROR, "unknown FVWM2 test-message\n");
      break;
    }
}



void
DeadPipe(int signo)
{
  exit(0);
}

void ParseOptions()
{
  char* tline;
  int   len = strlen(pgm_name);
    
  GetConfigLine(fd, &tline);

  while(tline != NULL)
    {
      char* ptr;
      int   desk;

      if ((strlen(&tline[0]) > 1) && mystrncasecmp(tline, CatString3("*", pgm_name, "Label"), len + 6) == 0)
	{
	  desk = desk1;
	  ptr = &tline[len + 6];

	  if (sscanf(ptr, "%d", &desk) != 1)
	    {
	      g_log("fvwm-pager", G_LOG_LEVEL_INFO,"Parse FVWM2 options: Invalid Label line '%s'\n", tline);
	    }
	  else
	    {
	      if ((desk > desk1) && (desk <= desk2))
		{
		  while (isspace(*ptr)) ptr++;
		  while (!isspace(*ptr)) ptr++;
		  if (ptr[strlen(ptr)-1] == '\n')
		    ptr[strlen(ptr)-1] = '\0';
		  if (!noapplet)
		    desktop_names = g_list_append(desktop_names, strdup(ptr));
		}
	    }
	}
      GetConfigLine(fd, &tline);
    }
  
    
}
