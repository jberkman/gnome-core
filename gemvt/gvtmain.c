/*  GemVt - GNU Emulator of a Virtual Terminal
 *  Copyright (C) 1997	Tim Janik
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include	"config.h"
#include	"gvtvt.h"
#include	"gvtconf.h"
#include	"gemvt.xpm"
#include	<gtktty/libgtktty.h>
#include	<gtk/gtk.h>
#include	<signal.h>
#include	<unistd.h>
#include	<string.h>

#ifdef	HAVE_GNOME
#  include	<libgnomeui/libgnomeui.h>
#endif	HAVE_GNOME

#ifdef	HAVE_LIBGLE
#  include	<gle/gle.h>
#endif	/* HAVE_LIBGLE */


/* --- typedefs --- */
enum
{
  GVT_VOIDLINE,
  GVT_ONLINE,
  GVT_OFFLINE
};

/* --- prototypes --- */
static	RETSIGTYPE gvt_signal_handler	(int		sig_num);
static  gint    gvt_tty_key_press       (GtkTty         *tty,
					 const gchar    *char_code,
					 guint          length,
					 guint          keyval,
					 guint          key_state,
					 gpointer       data);
static gint	gvt_tty_button_press	(GtkWidget	*widget,
					 GdkEventButton	*event);
static	void	gtk_menu_position_func	(GtkMenu	*menu,
					 gint		*x,
					 gint		*y,
					 gpointer	user_data);
static void	gvt_menu_adjust		(GtkMenu	*menu);
static	void	gvt_menus_setup		(void);
static	void	gvt_menus_shutdown	(void);



/* --- static variables --- */
static GvtColorEntry	linux_ct[GTK_TERM_MAX_COLORS] =
{
  /*  back      fore      dim       bold   */
  { 0x000000, 0x000000, 0x000000, 0x000000 }, /* black */
  { 0xd00000, 0xd00000, 0x880000, 0xff0000 }, /* red */
  { 0x00d000, 0x00d000, 0x008800, 0x00ff00 }, /* green */
  { 0xd0d000, 0xd0d000, 0x888800, 0xffff00 }, /* orange */
  { 0x0000d0, 0x0000d0, 0x000088, 0x0000ff }, /* blue */
  { 0xd000d0, 0xd000d0, 0x880088, 0xff00ff }, /* pink */
  { 0x00d0d0, 0x00d0d0, 0x008888, 0x00ffff }, /* cyan */
  { 0xd0d0d0, 0xd0d0d0, 0x888888, 0xffffff }, /* white */
};
static GtkMenu		*menu_1 = NULL;
static GtkMenu		*menu_2 = NULL;
static GtkMenu		*menu_3 = NULL;
static GtkTty		*menu_tty;
static gint		menu_pos_x, menu_pos_y;
static GvtConfig	config =
{
  TRUE	/* have_status_bar */,
  FALSE	/* prefix_dash */,
  NULL	/* strings */,
};


/* --- main() --- */
int
main	(int	argc,
	 char	*argv[])
{
  gchar	*home_dir;
  gchar	*rc_file;
  guchar	*string;
  GtkObject	*vt;
  guint	id;
  gint		exit_status;
  guint		i;
  

  prg_name = g_strdup (argv[0]);
  
  /* parse arguments
   */
  exit_status = gvt_config_args (&config, stderr, argc, argv);
  if (exit_status > -129)
    return exit_status;

  
  /* Gtk+/GNOME/GLE initialization
   */
#ifdef	HAVE_GNOME
  gnome_init("gemvt", &argc, &argv);
#else	/* !HAVE_GNOME */
  gtk_init (&argc, &argv);
#endif	/* !HAVE_GNOME */

#ifdef	HAVE_LIBGLE
  gle_init (&argc, &argv);
#endif	/* HAVE_LIBGLE */

  
  /* parse ~/.gtkrc, (this file also ommitted by the gnome_init stuff
   */
  home_dir = getenv ("HOME");
  if (!home_dir) /* fatal! */
    home_dir = ".";
  
  string = "gtkrc";
  rc_file = g_new (gchar, strlen (home_dir) + 2 + strlen (string) + 1);
  *rc_file = 0;
  strcat (rc_file, home_dir);
  strcat (rc_file, "/.");
  strcat (rc_file, string);

  /* printf("%s\n", rc_file);
   */
  gtk_rc_parse (rc_file);
  g_free (rc_file);
  
  string = strrchr (prg_name, '/');
  if (string)
    string++;
  else
    string = prg_name;
  rc_file = g_new (gchar, strlen (home_dir) + 2 + strlen (string) + 2 + 1);
  *rc_file = 0;
  strcat (rc_file, home_dir);
  strcat (rc_file, "/.");
  strcat (rc_file, string);
  strcat (rc_file, "rc");

  /* parse ~/.<prgname>rc
   */
  /* printf("%s\n", rc_file);
   */
  /* FIXME: invoke rc-parser here */
  g_free (rc_file);

  
  /* catch system signals
   */
  gvt_signal_handler (0);


  /* fire up default vt (test code)
   */
  string =
    "Hi, this is GemVt bothering you ;)\n\r"
    "Enter `-bash' or something the like and\n\r"
    "Have Fun!!!\n\r";
  
  vt = gvt_vt_new (GVT_VT_WINDOW, NULL);
  for (i = 0; i < GTK_TERM_MAX_COLORS; i++)
    gvt_vt_set_color (GVT_VT (vt), i, &linux_ct[i]);
  gvt_vt_execute (GVT_VT (vt), "-/bin/bash");

  gvt_menus_setup ();

  /* gtk's main loop
   */
  gtk_main ();

  gvt_menus_shutdown ();

  /* exit program
   */
  return 0;
}


RETSIGTYPE
gvt_signal_handler (int	sig_num)
{
  static gboolean	in_call = FALSE;
  
  if (in_call)
  {
    fprintf (stderr,
	     "\naborting on another signal: `%s'\n",
	     g_strsignal (sig_num));
    fflush (stderr);
    gtk_exit (-1);
  }
  else
    in_call = TRUE;
  
  signal (SIGINT,	gvt_signal_handler);
  signal (SIGTRAP,	gvt_signal_handler);
  signal (SIGABRT,	gvt_signal_handler);
  signal (SIGBUS,	gvt_signal_handler);
  signal (SIGSEGV,	gvt_signal_handler);
  signal (SIGPIPE,	gvt_signal_handler);
  /* signal (SIGTERM,	gvt_signal_handler); */
  
  if (sig_num > 0)
   {
    fprintf (stderr, "%s: (pid: %d) caught signal: `%s'\n",
	     prg_name,
	     getpid (),
	     g_strsignal (sig_num));
    fflush (stderr);
    g_debug (prg_name);
  }
  in_call = FALSE;
}

static gint
gvt_tty_key_press (GtkTty         *tty,
		   const gchar    *char_code,
		   guint          length,
		   guint          keyval,
		   guint          key_state,
		   gpointer       data)
{
  GtkWidget *status_bar;
  register guint id;

  /* return wether we handled the key press:
   * FALSE) let GtkTty handle the keypress,
   * TRUE)  we do something like gtk_tty_put_in (tty, char_code, length) on
   *        our own.
   */

  status_bar = gtk_object_get_data (GTK_OBJECT (tty), "status-bar");


  id = (gint) gtk_object_get_data (GTK_OBJECT (tty), "key_press_hid");
  if (id)
  {
    gtk_signal_disconnect (GTK_OBJECT (tty), id);
    gtk_object_set_data (GTK_OBJECT (tty), "key_press_hid", (gpointer) 0);
  }

  return FALSE;
}

static gint
gvt_tty_button_press (GtkWidget      *widget,
		      GdkEventButton *event)
{
  GtkTty *tty;

  /* 1) return wether we handled the event
   * 2) if we handled it we need to prevent gtktty region selection,
   *    therefor we stop the emission of the signal
   */

  tty = GTK_TTY (widget);

  if ((event->state & (GDK_SHIFT_MASK |
		      GDK_LOCK_MASK |
		      GDK_CONTROL_MASK |
		      GDK_MOD1_MASK)) ==
      GDK_CONTROL_MASK)
  {
    switch (event->button)
    {
    case  1:
      gtk_signal_emit_stop_by_name (GTK_OBJECT (widget), "button_press_event");
      gdk_window_get_pointer (NULL, &menu_pos_x, &menu_pos_y, NULL);
      menu_tty = tty;
      gvt_menu_adjust (menu_1);
      gtk_menu_popup (menu_1,
		      NULL, NULL,
		      gtk_menu_position_func, NULL,
		      1, event->time);
      
      return TRUE;
    }
  }

  return FALSE;
}

static void
gtk_menu_position_func (GtkMenu        *menu,
			gint           *x,
			gint           *y,
			gpointer       user_data)
{
  *x = menu_pos_x;
  *y = menu_pos_y;
}

static void
gvt_menu_toggle_status_bar (GtkWidget *widget,
			    gpointer  data)
{
  GtkWidget *status_bar;

  status_bar = gtk_object_get_data (GTK_OBJECT (menu_tty), "status-bar");
  g_assert (status_bar != NULL);

  if (GTK_WIDGET_VISIBLE (status_bar))
    gtk_widget_hide (status_bar);
  else
    gtk_widget_show (status_bar);
}

static void
gvt_menu_tty_redraw (GtkWidget *widget,
		     gpointer  data)
{
  gtk_widget_draw (GTK_WIDGET (menu_tty), NULL);
}

static void
gvt_menu_send_signal (GtkWidget *widget,
		      gpointer  data)
{
  g_assert (menu_tty != NULL);
  g_assert (menu_tty->pid > 0);

  kill (menu_tty->pid, (gint) data);
}


static void
gvt_menu_adjust (GtkMenu *menu)
{
  GList *list, *free_list;

  g_assert (menu != NULL);

  list = free_list = gtk_container_children (GTK_CONTAINER (menu));

  while (list)
  {
    gint ac;

    ac = (gint) gtk_object_get_data (GTK_OBJECT (list->data), "activate");

    switch (ac)
    {
    case  GVT_ONLINE:
      gtk_widget_set_sensitive (GTK_WIDGET (list->data), menu_tty->pid > 0);
      break;

    case  GVT_OFFLINE:
      gtk_widget_set_sensitive (GTK_WIDGET (list->data), menu_tty->pid == 0);
      break;
    }

    list = list->next;
  }

  g_list_free (free_list);
}

static void
gvt_menus_setup (void)
{
  GtkWidget *menu;

  if (!menu_1)
  {
    GtkWidget *item;

    menu = gtk_widget_new (gtk_menu_get_type (),
			   "GtkObject::signal::destroy", gtk_widget_destroyed, &menu_1,
			   NULL);
    menu_1 = GTK_MENU (menu);
    item = gtk_menu_item_new_with_label ("Main Options");
    gtk_widget_set_sensitive (item, FALSE);
    gtk_widget_show (item);
    gtk_menu_append (GTK_MENU (menu), item);
    item = gtk_menu_item_new ();
    gtk_widget_set_sensitive (item, FALSE);
    gtk_widget_show (item);
    gtk_menu_append (GTK_MENU (menu), item);
    item = gtk_menu_item_new_with_label ("Redraw");
    gtk_signal_connect(GTK_OBJECT (item), "activate",
		       GTK_SIGNAL_FUNC (gvt_menu_tty_redraw),
		       NULL);
    gtk_widget_show (item);
    gtk_menu_append (GTK_MENU (menu), item);
    item = gtk_menu_item_new_with_label ("Toggle Status Bar");
    gtk_signal_connect(GTK_OBJECT (item), "activate",
		       GTK_SIGNAL_FUNC (gvt_menu_toggle_status_bar),
		       NULL);
    gtk_widget_show (item);
    gtk_menu_append (GTK_MENU (menu), item);
    item = gtk_menu_item_new ();
    gtk_widget_set_sensitive (item, FALSE);
    gtk_widget_show (item);
    gtk_menu_append (GTK_MENU (menu), item);
    item = gtk_menu_item_new_with_label ("SIGSTOP");
    gtk_object_set_data (GTK_OBJECT (item), "activate", (gpointer) GVT_ONLINE);
    gtk_signal_connect(GTK_OBJECT (item), "activate",
		       GTK_SIGNAL_FUNC (gvt_menu_send_signal),
		       (gpointer) SIGSTOP);
    gtk_widget_show (item);
    gtk_menu_append (GTK_MENU (menu), item);
    item = gtk_menu_item_new_with_label ("SIGCONT");
    gtk_object_set_data (GTK_OBJECT (item), "activate", (gpointer) GVT_ONLINE);
    gtk_signal_connect(GTK_OBJECT (item), "activate",
		       GTK_SIGNAL_FUNC (gvt_menu_send_signal),
		       (gpointer) SIGCONT);
    gtk_widget_show (item);
    gtk_menu_append (GTK_MENU (menu), item);
    item = gtk_menu_item_new_with_label ("SIGINT");
    gtk_object_set_data (GTK_OBJECT (item), "activate", (gpointer) GVT_ONLINE);
    gtk_signal_connect(GTK_OBJECT (item), "activate",
		       GTK_SIGNAL_FUNC (gvt_menu_send_signal),
		       (gpointer) SIGINT);
    gtk_widget_show (item);
    gtk_menu_append (GTK_MENU (menu), item);
    item = gtk_menu_item_new_with_label ("SIGHUP");
    gtk_object_set_data (GTK_OBJECT (item), "activate", (gpointer) GVT_ONLINE);
    gtk_signal_connect(GTK_OBJECT (item), "activate",
		       GTK_SIGNAL_FUNC (gvt_menu_send_signal),
		       (gpointer) SIGHUP);
    gtk_widget_show (item);
    gtk_menu_append (GTK_MENU (menu), item);
    item = gtk_menu_item_new_with_label ("SIGTERM");
    gtk_object_set_data (GTK_OBJECT (item), "activate", (gpointer) GVT_ONLINE);
    gtk_signal_connect(GTK_OBJECT (item), "activate",
		       GTK_SIGNAL_FUNC (gvt_menu_send_signal),
		       (gpointer) SIGTERM);
    gtk_widget_show (item);
    gtk_menu_append (GTK_MENU (menu), item);
    item = gtk_menu_item_new_with_label ("SIGKILL");
    gtk_object_set_data (GTK_OBJECT (item), "activate", (gpointer) GVT_ONLINE);
    gtk_signal_connect(GTK_OBJECT (item), "activate",
		       GTK_SIGNAL_FUNC (gvt_menu_send_signal),
		       (gpointer) SIGKILL);
    gtk_widget_show (item);
    gtk_menu_append (GTK_MENU (menu), item);
    item = gtk_menu_item_new ();
    gtk_widget_set_sensitive (item, FALSE);
    gtk_widget_show (item);
    gtk_menu_append (GTK_MENU (menu), item);
    item = gtk_menu_item_new_with_label ("Exit");
    gtk_signal_connect(GTK_OBJECT (item), "activate",
		       GTK_SIGNAL_FUNC (gtk_main_quit),
		       NULL);
    gtk_widget_show (item);
    gtk_menu_append (GTK_MENU (menu), item);
  }

  if (!menu_2)
  {
    menu = gtk_widget_new (gtk_menu_get_type (),
			   "GtkObject::signal::destroy", gtk_widget_destroyed, &menu_2,
			   NULL);
    menu_2 = GTK_MENU (menu);
  }

  if (!menu_3)
  {
    menu = gtk_widget_new (gtk_menu_get_type (),
			   "GtkObject::signal::destroy", gtk_widget_destroyed, &menu_3,
			   NULL);
    menu_3 = GTK_MENU (menu);
  }
}

static void
gvt_menus_shutdown (void)
{
  if (menu_1)
    gtk_object_sink (GTK_OBJECT (menu_1));
  if (menu_2)
    gtk_object_sink (GTK_OBJECT (menu_2));
  if (menu_3)
    gtk_object_sink (GTK_OBJECT (menu_3));
}
