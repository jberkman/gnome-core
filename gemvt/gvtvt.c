/*  GemVt - GNU Emulator of a Virtual Terminal
 *  Copyright (C) 1997  Tim Janik
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
#include	"gem-r.xpm"
#include	"gem-g.xpm"
#include	"gem-b.xpm"
#include	<string.h>

/* --- external variables --- */
/* hm, seems like some systems miss the automatic declaration
 * of the environment char array, since configure already made
 * sure we got it, we can savely declare it here.
 */
#ifdef	HAVE_ENVIRON
extern char **environ;
#endif	/*HAVE_ENVIRON*/
#ifdef	HAVE___ENVIRON
extern char **__environ;
#endif	/*HAVE_ENVIRON*/


/* -- defines --- */
#define	SAVE_BUFFER_LENGTH	(64)


/* --- prototypes --- */
static void	gvt_vt_class_init	(GvtVtClass	*class);
static void	gvt_vt_init		(GvtVt		*vt);
static void	gvt_vt_destroy		(GtkObject      *object);
static void	gvt_vt_finalize		(GtkObject	*object);
static void	gvt_vt_gem_realize	(GvtVt		*vt);
static void	gvt_vt_labels_update	(GvtVt          *vt);
static void	gvt_vt_program_exec	(GtkTty		*tty,
					 const gchar	*prg_name,
					 gchar * const	 argv[],
					 gchar * const	 envp[],
					 GvtVt		*vt);
static void	gvt_vt_program_exit	(GtkTty         *tty,
					 const gchar    *prg_name,
					 const gchar     exit_status,
					 guint           exit_signal,
					 GvtVt		*vt);


/* --- typedefs --- */
typedef struct	_GvtQueuedColorEntry	GvtQueuedColorEntry;
struct _GvtQueuedColorEntry
{
  guint index;
  GvtColorEntry c_entry;
};


/* --- variables --- */
static GtkObjectClass	*parent_class = NULL;


/* --- functions --- */
GtkType
gvt_vt_get_type (void)
{
  static GtkType vt_type = 0;

  if (!vt_type)
    {
      GtkTypeInfo vt_info =
      {
	"GvtVt",
	sizeof (GvtVt),
	sizeof (GvtVtClass),
	(GtkClassInitFunc) gvt_vt_class_init,
	(GtkObjectInitFunc) gvt_vt_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
	(GtkClassInitFunc) NULL,
      };

      vt_type = gtk_type_unique (GTK_TYPE_OBJECT, &vt_info);
    }

  return vt_type;
}

static guint
gvt_vt_class_add_vt (GvtVtClass *class,
		     GvtVt	*vt)
{
  GvtVt         **vts;
  guint		i;

  g_return_val_if_fail (class != NULL, ~0);
  g_return_val_if_fail (vt != NULL, ~0);

  if (class->vts)
    {
      for (i = 0; i < class->n_vts; i++)
	if (!class->vts[i])
	  break;
      if (i < class->n_vts)
	{
	  class->vts[i] = vt;
	  return i;
	}
    }

  vts = g_new (GvtVt*, class->n_vts + 1);
  for (i = 0; i < class->n_vts; i++)
    vts[i] = class->vts[i];
  vts[i] = vt;

  g_free (class->vts);
  class->vts = vts;
  class->n_vts = i;

  return i;
}

static void
gvt_vt_class_init (GvtVtClass *class)
{
  GtkObjectClass *object_class;
  
  object_class = (GtkObjectClass*) class;
  
  parent_class = gtk_type_class (gtk_object_get_type ());
  
  object_class->destroy = gvt_vt_destroy;
  object_class->finalize = gvt_vt_finalize;

  class->realized = FALSE;
  class->color_context = NULL;
  class->vts = NULL;
  class->n_vts = 0;
  class->gem_red_bit = NULL;
  class->gem_red_pix = NULL;
  class->gem_green_bit = NULL;
  class->gem_green_pix = NULL;
  class->gem_blue_bit = NULL;
  class->gem_blue_pix = NULL;
}

static void
gvt_vt_init (GvtVt *vt)
{
  GvtVtClass *class;
  guint i;

  class = GVT_VT_CLASS (GTK_OBJECT (vt)->klass);

  vt->mode = GVT_STATE_NONE;
  vt->nth = gvt_vt_class_add_vt (class, vt);

  vt->col_queue = NULL;

  vt->window = NULL;
  vt->vbox =
    gtk_widget_new (gtk_vbox_get_type (),
		    "GtkBox::homogeneous", FALSE,
		    "GtkBox::spacing", 0,
		    "GtkContainer::border_width", 0,
		    "GtkWidget::visible", TRUE,
		    NULL);
  vt->status_box =
    gtk_widget_new (gtk_hbox_get_type (),
		    "GtkBox::homogeneous", FALSE,
		    "GtkBox::spacing", 0,
		    "GtkContainer::border_width", 0,
		    "GtkWidget::visible", TRUE,
		    NULL);
  gtk_box_pack_start (GTK_BOX (vt->vbox), vt->status_box, FALSE, TRUE, 0);
  vt->tty = gtk_tty_new (80, 25, 99);
  gtk_widget_set (vt->tty,
		  "GtkWidget::visible", TRUE,
		  "GtkObject::signal::program_exec", gvt_vt_program_exec, vt,
		  "GtkObject::signal::program_exit", gvt_vt_program_exit, vt,
		  NULL);
  gtk_box_pack_start (GTK_BOX (vt->vbox), vt->tty, TRUE, TRUE, 0);
  gtk_signal_connect_object_after (GTK_OBJECT (vt->tty),
				   "realize",
				   GTK_SIGNAL_FUNC (gvt_vt_do_color_queue),
				   GTK_OBJECT (vt));
  vt->gem = gtk_type_new (gtk_pixmap_get_type ());
  gtk_box_pack_start (GTK_BOX (vt->status_box), vt->gem, FALSE, FALSE, 0);
  gtk_widget_show (vt->gem);
  gtk_signal_connect_object_after (GTK_OBJECT (vt->gem),
				   "realize",
				   GTK_SIGNAL_FUNC (gvt_vt_gem_realize),
				   GTK_OBJECT (vt));
  vt->label_box =
    gtk_widget_new (gtk_hbox_get_type (),
		    "GtkBox::homogeneous", FALSE,
		    "GtkBox::spacing", 5,
		    "GtkContainer::border_width", 0,
		    "GtkWidget::visible", TRUE,
		    NULL);
  gtk_box_pack_start (GTK_BOX (vt->status_box), vt->label_box, FALSE, FALSE, 0);
  vt->label_program =
    gtk_widget_new (gtk_label_get_type (),
		    "GtkLabel::label", "-Program-",
		    "GtkWidget::parent", vt->label_box,
		    "GtkWidget::visible", TRUE,
		    NULL);
  vt->label_status =
    gtk_widget_new (gtk_label_get_type (),
		    "GtkLabel::label", "-Status-",
		    "GtkWidget::parent", vt->label_box,
		    "GtkWidget::visible", TRUE,
		    NULL);
  vt->label_time =
    gtk_widget_new (gtk_label_get_type (),
		    "GtkLabel::label", "-Time-",
		    "GtkWidget::parent", vt->label_box,
		    "GtkWidget::visible", GTK_TTY_CLASS (GTK_OBJECT (vt->tty)->klass)->meassure_time,
		    NULL);
  vt->led_box =
    gtk_widget_new (gtk_hbox_get_type (),
		    "GtkBox::homogeneous", FALSE,
		    "GtkBox::spacing", 5,
		    "GtkContainer::border_width", 0,
		    "GtkWidget::visible", TRUE,
		    NULL);
  gtk_box_pack_end (GTK_BOX (vt->status_box), vt->led_box, FALSE, FALSE, 0);
  for (i = 0; i < 8; i++)
    {
      GtkWidget *led;

      led = gtk_led_new ();
      gtk_box_pack_start (GTK_BOX (vt->led_box), led, TRUE, TRUE, 5);
      gtk_widget_show (led);

      gtk_tty_add_update_led (GTK_TTY (vt->tty), GTK_LED (led), 1 << i);
    }

  gvt_vt_status_update (vt);
}

GtkObject*
gvt_vt_new (GvtVtMode      mode,
	    GtkContainer   *parent)
{
  GvtVt *vt;

  g_return_val_if_fail (mode >= GVT_VT_SIMPLE && mode <= GVT_VT_WINDOW, NULL);
  if (mode != GVT_VT_WINDOW)
    g_return_val_if_fail (parent != NULL, NULL);
  else
    g_return_val_if_fail (parent == NULL, NULL);

  vt = gtk_type_new (gvt_vt_get_type ());
  vt->mode = mode;
  if (mode == GVT_VT_WINDOW)
    {
      gchar string[SAVE_BUFFER_LENGTH];

      sprintf (string, "GemVt-%02d", vt->nth);
      vt->window =
	gtk_widget_new (gtk_window_get_type (),
			"GtkWindow::title", string,
			"GtkWindow::type", GTK_WINDOW_TOPLEVEL,
			"GtkWindow::allow_shrink", TRUE,
			"GtkWindow::allow_grow", TRUE,
			"GtkWindow::auto_shrink", FALSE,
			"GtkContainer::child", vt->vbox,
			"GtkWidget::visible", TRUE,
			NULL);
    }
  else
    {
      if (GTK_IS_BOX (parent))
	gtk_box_pack_start (GTK_BOX (parent), vt->vbox, TRUE, TRUE, 0);
      else
	gtk_container_add (parent, vt->vbox);
    }
      
  return GTK_OBJECT (vt);
}

void
gvt_vt_set_gem_state (GvtVt          *vt,
		      GvtStateType   state)
{
  GvtVtClass *class;
  
  g_return_if_fail (vt != NULL);
  g_return_if_fail (GVT_IS_VT (vt));
  g_return_if_fail (state >= GVT_STATE_NONE && state <= GVT_STATE_DEAD);

  class = GVT_VT_CLASS (GTK_OBJECT (vt)->klass);
  if (!class->realized)
    return;

  switch (state)
  {
  case	GVT_STATE_NONE:
    gtk_pixmap_set (GTK_PIXMAP (vt->gem), class->gem_blue_pix, class->gem_blue_bit);
    break;
  case	GVT_STATE_RUNNING:
    gtk_pixmap_set (GTK_PIXMAP (vt->gem), class->gem_green_pix, class->gem_green_bit);
    break;
  case	GVT_STATE_DEAD:
    gtk_pixmap_set (GTK_PIXMAP (vt->gem), class->gem_red_pix, class->gem_red_bit);
    break;
  default:
    g_assert_not_reached ();
  }
}

static void
gvt_vt_labels_update (GvtVt          *vt)
{
  GtkTty *tty;
  gchar *program;
  gchar	state[SAVE_BUFFER_LENGTH];
  gchar sys_time[SAVE_BUFFER_LENGTH];
  gchar user_time[SAVE_BUFFER_LENGTH];
  gchar *buffer;
  register guint i;

  g_return_if_fail (vt != NULL);
  g_return_if_fail (GVT_IS_VT (vt));

  tty = GTK_TTY (vt->tty);

  program = gtk_object_get_data (GTK_OBJECT (tty), "program");

  if (!program)
  {
    strcpy (state, "None");
    sys_time[0] = '0';
    sys_time[1] = 0;
    user_time[0] = '0';
    user_time[1] = 0;
  }
  else
  {
    if (tty->pid)
    {
      sprintf (state, "running (%d)", tty->pid);
      sys_time[0] = '-';
      sys_time[1] = 0;
      user_time[0] = '-';
      user_time[1] = 0;
    }
    else
    {
      if (tty->exit_signal)
	strcpy (state, g_strsignal (tty->exit_signal));
      else
	sprintf (state, "Exit [%+d]", tty->exit_status);
      
      sprintf (sys_time, "%f", tty->sys_sec + tty->sys_usec / 1000000.0);
      for (i = strlen (sys_time) - 1; sys_time[i] == '0'; i--)
	sys_time[i] = 0;
      sprintf (user_time, "%f", tty->user_sec + tty->user_usec / 1000000.0);
      for (i = strlen (user_time) - 1; user_time[i] == '0'; i--)
	user_time[i] = 0;
    }
  }
  
  gtk_container_block_resize (GTK_CONTAINER (vt->label_box));
    
  if (program)
  {
    register guint l;

    l = strlen (program);
    if (l > 32)
      buffer = g_strconcat ("Program: .../", program + 32 - l, NULL);
    else
      buffer = g_strconcat ("Program: ", program, NULL);
  }
  else
    buffer = g_strconcat ("Program: ", "None", NULL);
  gtk_label_set_text (GTK_LABEL (vt->label_program), buffer);
  g_free (buffer);
  
  buffer = g_strconcat ("Status: ", state, NULL);
  gtk_label_set_text (GTK_LABEL (vt->label_status), buffer);
  g_free (buffer);
  
  buffer = g_strconcat ("System: ", sys_time, "	 User: ", user_time, NULL);
  gtk_label_set_text (GTK_LABEL (vt->label_time), buffer);
  g_free (buffer);

  gtk_container_unblock_resize (GTK_CONTAINER (vt->label_box));
}

void
gvt_vt_status_update (GvtVt          *vt)
{
  g_return_if_fail (vt != NULL);
  g_return_if_fail (GVT_IS_VT (vt));

  if (gtk_object_get_data (GTK_OBJECT (vt->tty), "program"))
    gvt_vt_set_gem_state (vt, GTK_TTY (vt->tty)->pid ? GVT_STATE_RUNNING : GVT_STATE_DEAD);
  else
    gvt_vt_set_gem_state (vt, GTK_TTY (vt->tty)->pid ? GVT_STATE_RUNNING : GVT_STATE_NONE);
  
  gvt_vt_labels_update (vt);
}

static void
gvt_vt_destroy (GtkObject *object)
{
  GvtVt *vt;
  GSList *slist;

  g_return_if_fail (object != NULL);
  g_return_if_fail (GVT_IS_VT (object));

  vt = GVT_VT (object);

  if (vt->window)
    gtk_widget_destroy (vt->window);
  else
    gtk_widget_destroy (vt->vbox);

  for (slist = vt->col_queue; slist; slist = slist->next)
    g_free (slist->data);
  g_slist_free (vt->col_queue);
  vt->col_queue = NULL;
  
  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gvt_vt_finalize (GtkObject *object)
{
  GvtVt *vt;

  g_return_if_fail (object != NULL);
  g_return_if_fail (GVT_IS_VT (object));

  vt = GVT_VT (object);

  GTK_OBJECT_CLASS(parent_class)->finalize (object);
}

static void
gvt_vt_program_exec (GtkTty	   *tty,
		     const gchar   *prg_name,
		     gchar * const  argv[],
		     gchar * const  envp[],
		     GvtVt	   *vt)
{
  g_return_if_fail (vt != NULL);
  g_return_if_fail (GVT_IS_VT (vt));
  g_return_if_fail (tty == (GtkTty*) vt->tty);

  gtk_object_set_data (GTK_OBJECT (tty), "program", (gpointer) prg_name);

  gvt_vt_status_update (vt);

  if (!GTK_WIDGET_HAS_FOCUS (vt->tty))
    gtk_widget_grab_focus (vt->tty);
}

static void
gvt_vt_program_exit (GtkTty         *tty,
		     const gchar    *prg_name,
		     const gchar     exit_status,
		     guint           exit_signal,
		     GvtVt	    *vt)
{
  g_return_if_fail (vt != NULL);
  g_return_if_fail (GVT_IS_VT (vt));
  g_return_if_fail (tty == (GtkTty*) vt->tty);

  gvt_vt_status_update (vt);

  gtk_object_set_data (GTK_OBJECT (tty), "program", NULL);
}

void
gvt_vt_execute (GvtVt		*vt,
		const gchar	*exec_string)
{
  GtkTty      *tty;
  guchar      *prg = NULL;
  GList       *args;
  guchar      *p;
  guchar      *s, *e, *f;
  
  g_return_if_fail (vt != NULL);
  g_return_if_fail (GVT_IS_VT (vt));
  g_return_if_fail (exec_string != NULL);

  tty = GTK_TTY (vt->tty);

  if (tty->pid)
    {
      g_warning ("Program already running: %d", tty->pid);
      return;
    }

  
  s = g_strdup (exec_string);
  f = s;
  e = s + strlen (s);
  
  p = strchr (s, ' ');
  if (p)
    {
      *p = 0;
      prg = s;
      s = p + 1;
    }
  else if (s < e)
    {
      prg = s;
      s = e;
    }
  
  args = NULL;
  p = strchr (s, ' ');
  while (p || s < e)
    {
      if (p)
	{
	  *p = 0;
	  args = g_list_append (args, s);
	  s = p + 1;
	}
      else if (s < e)
	{
	  args = g_list_append (args, s);
	  s = e;
	}
      
      p = strchr (s, ' ');
    }
  
  if (prg)
    {
      gchar **argv;
      gchar **env = NULL;
      GList *list;
      guint i;

#ifdef  HAVE_ENVIRON
      env = environ;
#endif  /*HAVE_ENVIRON*/
#ifdef  HAVE___ENVIRON
      env = __environ;
#endif  /*HAVE_ENVIRON*/
      
      
      argv = g_new (gchar*, g_list_length (args) + 1 + 1);
      
      argv[0] = prg;
      
      if (prg[0] == '-')
	prg++;
      
      list = args;
      i = 1;
      while (list)
	{
	  argv[i++] = list->data;
	  list = list->next;
	}
      argv[i] = NULL;
      
      putenv ("TERM=");
      
      gtk_tty_execute (tty, prg, argv, env);
      
      g_free (argv);
    }
  
  g_list_free (args);
  g_free (f);
}

static void
gvt_vt_gem_realize (GvtVt          *vt)
{
  GvtVtClass *class;

  g_return_if_fail (vt != NULL);
  g_return_if_fail (GVT_IS_VT (vt));

  class = GVT_VT_CLASS (GTK_OBJECT (vt)->klass);
  if (!class->realized)
    {
      class->realized = TRUE;

      class->color_context = gdk_color_context_new (gdk_window_get_visual (vt->gem->window),
						    gdk_window_get_colormap (vt->gem->window));
      
      class->gem_red_bit = NULL;
      class->gem_red_pix =
	gdk_pixmap_create_from_xpm_d (vt->gem->window,
				      &class->gem_red_bit,
				      &gtk_widget_get_default_style ()->bg[GTK_STATE_NORMAL],
				      gem_r_xpm);
      class->gem_green_bit = NULL;
      class->gem_green_pix =
	gdk_pixmap_create_from_xpm_d (vt->gem->window,
				      &class->gem_green_bit,
				      &gtk_widget_get_default_style ()->bg[GTK_STATE_NORMAL],
				      gem_g_xpm);
      class->gem_blue_bit = NULL;
      class->gem_blue_pix =
	gdk_pixmap_create_from_xpm_d (vt->gem->window,
				      &class->gem_blue_bit,
				      &gtk_widget_get_default_style ()->bg[GTK_STATE_NORMAL],
				      gem_b_xpm);
    }

  gvt_vt_status_update (vt);
}

static gulong
gvt_color_entry_2_pixel (GdkColorContext *cc,
			 guint	          color)
{
  gulong pixel;
  gint failed;

  color &= 0x00ffffff;
  failed = FALSE;
  pixel = gdk_color_context_get_pixel (cc,
				       ((color & 0xff0000) >> 8) + ((color & 0xff0000) >> 16),
				       (color & 0xff00) + ((color & 0xff00) >> 8),
				       (color & 0xff) + ((color & 0xff) << 8),
				       &failed);
  if (failed)
    g_warning ("Failed to allocate color with color contex (strange): 0x%06x", color);
  
  return pixel;
}

void
gvt_vt_do_color_queue (GvtVt *vt)
{
  GSList *slist;
  GvtVtClass *class;
  
  g_return_if_fail (vt != NULL);
  g_return_if_fail (GVT_IS_VT (vt));

  class = GVT_VT_CLASS (GTK_OBJECT (vt)->klass);

  if (!vt->tty || !GTK_WIDGET_REALIZED (vt->tty) || !class->realized)
    return;

  for (slist = vt->col_queue; slist; slist = slist->next)
    {
      GvtQueuedColorEntry *q_entry;

      q_entry = slist->data;
      gtk_term_set_color (GTK_TERM (vt->tty),
			  q_entry->index,
			  gvt_color_entry_2_pixel (class->color_context, q_entry->c_entry.back_val),
			  gvt_color_entry_2_pixel (class->color_context, q_entry->c_entry.fore_val),
			  gvt_color_entry_2_pixel (class->color_context, q_entry->c_entry.dim_val),
			  gvt_color_entry_2_pixel (class->color_context, q_entry->c_entry.bold_val));

      g_free (q_entry);
    }
  g_slist_free (vt->col_queue);
  vt->col_queue = NULL;
}

void
gvt_vt_set_color (GvtVt          *vt,
		  guint           index,
		  GvtColorEntry  *c_entry)
{
  GvtQueuedColorEntry *q_entry;

  g_return_if_fail (vt != NULL);
  g_return_if_fail (GVT_IS_VT (vt));
  g_return_if_fail (index < GTK_TERM_MAX_COLORS);
  g_return_if_fail (c_entry != NULL);

  q_entry = g_new (GvtQueuedColorEntry, 1);
  q_entry->index = index;
  q_entry->c_entry = *c_entry;

  vt->col_queue = g_slist_prepend (vt->col_queue, q_entry);

  gvt_vt_do_color_queue (vt);
}
