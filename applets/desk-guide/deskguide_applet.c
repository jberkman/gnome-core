/* GNOME Desktop Guide
 * Copyright (C) 1999 Tim Janik
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#include <config.h>	/* PACKAGE, i18n */
#include "deskguide_applet.h"
#include "gwmdesktop.h"


#define CONFIG_OBOX_BORDER 6
#define CONFIG_OBOX_SPACING 8
#define CONFIG_IBOX_BORDER 6
#define CONFIG_IBOX_SPACING 4
#define CONFIG_ITEM_BORDER 0
#define CONFIG_ITEM_SPACING 4
  

/* --- prototypes --- */
static void	gp_init_gui		(void);
static void	gp_destroy_gui		(void);
static void	gp_create_desk_widgets	(void);
static gboolean	gp_desk_notifier	(gpointer	    func_data,
					 GwmhDesk	   *desk,
					 GwmhDeskInfoMask   change_mask);
static gboolean	gp_task_notifier	(gpointer	    func_data,
					 GwmhTask	   *task,
					 GwmhTaskNotifyType ntype,
					 GwmhTaskInfoMask   imask);
static void	gp_about		(void);
static void	gp_config_popup		(void);
static gpointer	gp_config_find_value	(const gchar *path);
static void	gp_load_config		(const gchar	   *privcfgpath);
static gboolean	gp_save_session		(gpointer	    func_data,
					 const gchar	   *privcfgpath,
					 const gchar	   *globcfgpath);
static gboolean	gp_check_task_visible	(GwmDesktop	   *desktop,
					 GwmhTask	   *task,
					 gpointer	    user_data);

static GtkWidget	*gp_applet = NULL;
static GtkTooltips	*gp_tooltips = NULL;
static GtkTooltips	*gp_desktips = NULL;
static GtkWidget	*gp_container = NULL;
static GtkWidget	*gp_desk_box = NULL;
static GtkWidget	*gp_desk_widget[MAX_DESKTOPS] = { NULL, };
static guint             gp_n_desk_widgets = 0;
static GdkWindow	*gp_atom_window = NULL;
static GtkOrientation    gp_orientation = GTK_ORIENTATION_HORIZONTAL;
static guint		 GP_TYPE_HBOX = 0;
static guint		 GP_TYPE_VBOX = 0;
static guint		 GP_ARROW_DIR = 0;
static gchar		*DESK_GUIDE_NAME = NULL;

static ConfigItem gp_config_items[] = {
  CONFIG_PAGE (N_ ("Display")),
  CONFIG_SECTION (sect_layout,				  	N_ ("Layout")),
  CONFIG_BOOL (switch_arrow,	FALSE,
	       N_ ("Switch tasklist arrow")),
  CONFIG_BOOL (show_pager,	TRUE,
	       N_ ("Show desktop pager")),
  CONFIG_BOOL (current_only,	FALSE,
	       N_ ("Only show current desktop in pager")),
  CONFIG_BOOL (raise_grid,		FALSE,
	       N_ ("Raise area grid over tasks")),
  CONFIG_SECTION (sect_tooltips,			 	N_ ("Tooltips")),
  CONFIG_BOOL (tooltips,	TRUE,
	       N_ ("Show Desk-Guide tooltips")),
  CONFIG_RANGE (tooltips_delay,	500,	1,	5000,
		N_ ("Desk-Guide tooltip delay [ms]")),
  CONFIG_BOOL (desktips,	TRUE,
	       N_ ("Show desktop name tooltips")),
  CONFIG_RANGE (desktips_delay,	10,	1,	5000,
		N_ ("Desktop name tooltip delay [ms]")),
  
  CONFIG_PAGE (N_ ("Tasks")),
  CONFIG_SECTION (sect_task_visibility,			  	N_ ("Visibility")),
  CONFIG_BOOL (show_hidden_tasks,	TRUE,
	       N_ ("Show hidden tasks (HIDDEN)")),
  CONFIG_BOOL (show_shaded_tasks,	TRUE,
	       N_ ("Show shaded tasks (SHADED)")),
  CONFIG_BOOL (show_skip_winlist,	FALSE,
	       N_ ("Show tasks which hide from window list (SKIP-WINLIST)")),
  CONFIG_BOOL (show_skip_taskbar,	FALSE,
	       N_ ("Show tasks which hide from taskbar (SKIP-TASKBAR)")),
  /*  CONFIG_SECTION (sect_null_1, NULL), */
  
  CONFIG_PAGE (N_ ("Geometry")),
  CONFIG_SECTION (sect_horizontal,                     	     	N_ ("Horizontal Layout")),
  CONFIG_RANGE (area_height,	44,	4,	1024,
		N_ ("Desktop Height [pixels]")),
  CONFIG_RANGE (row_stackup,	1,	1,	64,
		N_ ("Rows of Desktops")),
  CONFIG_BOOL (div_by_vareas,		TRUE,
	       N_ ("Divide height by number of vertical areas")),
  CONFIG_SECTION (sect_vertical,                       	     	N_ ("Vertical Layout")),
  CONFIG_RANGE (area_width,	44,	4,	1024,
		N_ ("Desktop Width [pixels]")),
  CONFIG_RANGE (col_stackup,	1,	1,	64,
		N_ ("Columns of Desktops")),
  CONFIG_BOOL (div_by_hareas,		TRUE,
	       N_ ("Divide width by number of horizontal areas")),

  CONFIG_PAGE (N_ ("Advanced")),
  CONFIG_SECTION (sect_drawing,                   	     	N_ ("Drawing")),
  CONFIG_BOOL (double_buffer,	TRUE,
	       N_ ("Draw desktops double-buffered (recommended)")),
  CONFIG_SECTION (sect_workarounds,         	     	N_ ("Window Manager Workarounds")),
  CONFIG_BOOL (skip_movement_offset,		TRUE,
	       N_ ("Window manager moves decoration window instead\n"
		   "(AfterStep, Enlightenment, FVWM, IceWM, SawMill)")),
  CONFIG_BOOL (unified_areas,			TRUE,
	       N_ ("Window manager changes active area on all desktops\n"
		   "(FVWM, SawMill)")),
  CONFIG_BOOL (violate_client_msg,			TRUE,
	       N_ ("Window manager expects pager to modify area+desktop properties directly\n"
		   "(Enlightenment, FVWM, SawMill)")),
};
static guint  gp_n_config_items = (sizeof (gp_config_items) /
				   sizeof (gp_config_items[0]));


/* --- FIXME: bug workarounds --- */
GNOME_Panel_OrientType fixme_panel_orient = 0;
/* #define applet_widget_get_panel_orient(x)	(fixme_panel_orient) */
static void fixme_applet_widget_get_panel_orient (gpointer               dummy,
						  GNOME_Panel_OrientType orient)
{
  fixme_panel_orient = orient;
}

/* --- main ---*/
gint 
main (gint   argc,
      gchar *argv[])
{
  /* Initialize the i18n stuff */
  bindtextdomain (PACKAGE, GNOMELOCALEDIR);
  textdomain (PACKAGE);
  
  applet_widget_init ("deskguide_applet",
		      VERSION,
		      argc, argv,
		      NULL, 0, NULL);
  
  DESK_GUIDE_NAME = _ ("GNOME Desktop Guide (Pager)");
  
  /* setup applet widget
   */
  gp_tooltips = gtk_tooltips_new ();
  gp_desktips = gtk_tooltips_new ();
  gp_applet = applet_widget_new ("deskguide_applet");
  if (!gp_applet)
    g_error ("Unable to create applet widget");
  gtk_widget_ref (gp_applet);
  
  /* bail out for non GNOME window managers
   */
  if (!gwmh_init ())
    {
      GtkWidget *dialog;
      gchar *error_msg = _ ("You are not running a GNOME Compliant\n"
			    "Window Manager. GNOME support by the \n"
			    "window manager is strongly recommended\n"
			    "for proper Desk Guide operation.");
      
      dialog = gnome_error_dialog (_ ("Desk Guide Alert"));
      gtk_box_pack_start (GTK_BOX (GNOME_DIALOG (dialog)->vbox),
			  gtk_widget_new (GTK_TYPE_LABEL,
					  "visible", TRUE,
					  "label", DESK_GUIDE_NAME,
					  NULL),
			  TRUE, TRUE, 5);
      gtk_box_pack_start (GTK_BOX (GNOME_DIALOG (dialog)->vbox),
			  gtk_widget_new (GTK_TYPE_LABEL,
					  "visible", TRUE,
					  "label", error_msg,
					  NULL),
			  TRUE, TRUE, 5);
      gnome_dialog_run (GNOME_DIALOG (dialog));
      
      /* gtk_exit (1); */
    }
  
  /* main container
   */
  gp_container = gtk_widget_new (GTK_TYPE_ALIGNMENT,
				 "signal::destroy", gtk_widget_destroyed, &gp_container,
				 "child", gtk_type_new (GTK_TYPE_WIDGET),
				 NULL);
  applet_widget_add (APPLET_WIDGET (gp_applet), gp_container);
  
  /* notifiers and callbacks
   */
  gwmh_desk_notifier_add (gp_desk_notifier, NULL);
  gwmh_task_notifier_add (gp_task_notifier, NULL);
  gtk_widget_set (gp_applet,
		  "signal::change-orient", fixme_applet_widget_get_panel_orient, NULL,
		  "signal::change-orient", gp_destroy_gui, NULL,
		  "signal::change-orient", gp_init_gui, NULL,
		  "object_signal::save-session", gp_save_session, NULL,
		  "signal::destroy", gtk_main_quit, NULL,
		  NULL);
  applet_widget_register_stock_callback (APPLET_WIDGET (gp_applet),
					 "about",
					 GNOME_STOCK_MENU_ABOUT,
					 _ ("About..."),
					 (AppletCallbackFunc) gp_about,
					 NULL);
  applet_widget_register_stock_callback (APPLET_WIDGET (gp_applet),
					 "properties",
					 GNOME_STOCK_MENU_PROP,
					 _ ("Properties..."),
					 (AppletCallbackFunc) gp_config_popup,
					 NULL);
  
  /* load configuration
   */
  gp_load_config (APPLET_WIDGET (gp_applet)->privcfgpath);
  
  /* advertise to a WM that we exist and are here
   */
  gp_atom_window = gwmh_root_put_atom_window ("_GNOME_PAGER_ACTIVE",
					      GDK_WINDOW_TEMP,
					      GDK_INPUT_OUTPUT,
					      0);
  
  /* and away into the main loop
   */
  gtk_main ();
  
  /* shutdown
   */
  gwmh_desk_notifier_remove_func (gp_desk_notifier, NULL);
  gwmh_task_notifier_remove_func (gp_task_notifier, NULL);
  gdk_window_unref (gp_atom_window);
  gtk_widget_destroy (gp_applet);
  gtk_widget_unref (gp_applet);
  
  return 0;
}

static void
gp_load_config (const gchar *privcfgpath)
{
  static guint loaded = FALSE;
  guint i;
  gchar *section = "sect_general";
  
  if (loaded)
    return;
  loaded = TRUE;
  
  gnome_config_push_prefix (privcfgpath);
  
  for (i = 0; i < gp_n_config_items; i++)
    {
      ConfigItem *item = gp_config_items + i;
      
      if (!item->path)					/* page */
	continue;
      else if (item->min == -2 && item->max == -2)	/* section */
	section = item->path;
      else  if (item->min == -1 && item->max == -1)	/* boolean */
	{
	  gchar *path = g_strconcat (section, "/",
				     item->path, "=",
				     GPOINTER_TO_INT (item->value)
				     ? "true" :
				     "false",
				     NULL);
	  
	  item->value = GINT_TO_POINTER (gnome_config_get_bool (path));
	  g_free (path);
	}
      else						/* integer range */
	{
	  gchar *path = g_strdup_printf ("%s/%s=%d",
					 section,
					 item->path,
					 GPOINTER_TO_INT (item->value));
	  
	  item->value = GINT_TO_POINTER (gnome_config_get_int (path));
	  g_free (path);
	}
    }
  
  gnome_config_pop_prefix ();
}

static gpointer
gp_config_find_value (const gchar *path)
{
  guint i;
  
  for (i = 0; i < gp_n_config_items; i++)
    {
      ConfigItem *item = gp_config_items + i;
      
      if (path == item->path)
	return item->value;
    }
  
  g_warning (G_GNUC_PRETTY_FUNCTION "(): unable to find config value for <%s>", path);
  
  return NULL;
}

static gboolean
gp_save_session (gpointer     func_data,
		 const gchar *privcfgpath,
		 const gchar *globcfgpath)
{
  guint i;
  gchar *section = "sect_general";
  
  gnome_config_push_prefix (privcfgpath);
  
  for (i = 0; i < gp_n_config_items; i++)
    {
      ConfigItem *item = gp_config_items + i;
      gchar *path = g_strconcat (section, "/", item->path, NULL);
      
      if (!item->path)					/* page */
	continue;
      else if (item->min == -2 && item->max == -2)	/* section */
	section = item->path;
      else if (item->min == -1 && item->max == -1)	/* boolean */
	gnome_config_set_bool (path, GPOINTER_TO_INT (item->value));
      else						/* integer range */
	gnome_config_set_int (path, GPOINTER_TO_INT (item->value));
      g_free (path);
    }
  
  gnome_config_pop_prefix ();
  
  gnome_config_sync ();
  gnome_config_drop_all ();
  
  return FALSE;
}

static void
gp_config_reset_tmp_values (void)
{
  guint i;
  
  for (i = 0; i < gp_n_config_items; i++)
    {
      ConfigItem *item = gp_config_items + i;
      
      if (!item->path)					/* page */
	continue;
      else if (item->min == -2 && item->max == -2)	/* section */
	continue;
      else if (item->min == -1 && item->max == -1)	/* boolean */
	item->tmp_value = item->value;
      else						/* integer range */
	item->tmp_value = item->value;
    }
}

static void
gp_config_apply_tmp_values (void)
{
  guint i;
  
  for (i = 0; i < gp_n_config_items; i++)
    {
      ConfigItem *item = gp_config_items + i;
      
      if (!item->path)					/* page */
	continue;
      else if (item->min == -2 && item->max == -2)	/* section */
	continue;
      else if (item->min == -1 && item->max == -1)	/* boolean */
	item->value = item->tmp_value;
      else						/* integer range */
	item->value = item->tmp_value;
    }
}

static gboolean
gp_desk_notifier (gpointer	   func_data,
		  GwmhDesk	  *desk,
		  GwmhDeskInfoMask change_mask)
{
  if (change_mask & GWMH_DESK_INFO_BOOTUP)
    {
      gp_destroy_gui ();
      gp_init_gui ();
    }
  else
    {
      /* keep number of desktop widgets in sync with desk */
      if (change_mask & (GWMH_DESK_INFO_DESKTOP_NAMES |
			 GWMH_DESK_INFO_N_DESKTOPS |
			 GWMH_DESK_INFO_N_AREAS |
			 GWMH_DESK_INFO_BOOTUP))
	gp_create_desk_widgets ();
      
      if (gp_n_desk_widgets && BOOL_CONFIG (current_only))
	gwm_desktop_set_index (GWM_DESKTOP (gp_desk_widget[0]),
			       desk->current_desktop);
    }
  
  return TRUE;
}

static gboolean
gp_task_notifier (gpointer	     func_data,
		  GwmhTask	    *task,
		  GwmhTaskNotifyType ntype,
		  GwmhTaskInfoMask   imask)
{
  return TRUE;
}

static void 
gp_destroy_gui (void)
{
  if (gp_container)
    {
      gtk_widget_hide (gp_container);
      if (GTK_BIN (gp_container)->child)
	gtk_widget_destroy (GTK_BIN (gp_container)->child);
      else
	g_warning (G_GNUC_PRETTY_FUNCTION "(): missing container child");
    }
  else
    g_warning (G_GNUC_PRETTY_FUNCTION "(): missing container");
}

static void
gp_create_desk_widgets (void)
{
  gdouble area_size;

  if (N_DESKTOPS > MAX_DESKTOPS)
    g_error ("MAX_DESKTOPS limit reached, adjust source code");

  /* some gwmh configuration hacks ;( */
  gwmh_desk_set_hack_values (BOOL_CONFIG (unified_areas),
			     BOOL_CONFIG (violate_client_msg));

  if (!gp_desk_box)
    return;

  /* configure Desktop widget class for us */
  area_size = (gp_orientation == GTK_ORIENTATION_HORIZONTAL
	       ? RANGE_CONFIG (area_height)
	       : RANGE_CONFIG (area_width));
  if (gp_orientation == GTK_ORIENTATION_HORIZONTAL && BOOL_CONFIG (div_by_vareas))
    area_size /= (gdouble) N_VAREAS;
  if (gp_orientation == GTK_ORIENTATION_VERTICAL && BOOL_CONFIG (div_by_hareas))
    area_size /= (gdouble) N_HAREAS;
  gwm_desktop_class_config (gtk_type_class (GWM_TYPE_DESKTOP),
			    BOOL_CONFIG (double_buffer),
			    gp_orientation,
			    area_size,
			    BOOL_CONFIG (raise_grid),
			    BOOL_CONFIG (skip_movement_offset));

  gp_n_desk_widgets = 0;
  gtk_container_forall (GTK_CONTAINER (gp_desk_box), (GtkCallback) gtk_widget_destroy, NULL);
  if (BOOL_CONFIG (show_pager))
    {
      GtkWidget *desk_column = NULL;
      guint n_desks_per_column;
      guint i, max = BOOL_CONFIG (current_only) ? 1 : N_DESKTOPS;
      
      if (gp_orientation == GTK_ORIENTATION_HORIZONTAL)
	{
	  n_desks_per_column = RANGE_CONFIG (row_stackup);
	  n_desks_per_column = (N_DESKTOPS + n_desks_per_column - 1) / n_desks_per_column;
	}
      else
	n_desks_per_column = RANGE_CONFIG (col_stackup);
      
      for (i = 0; i < max; i++)
	{
	  GtkWidget *alignment;
	  
	  if (!desk_column)
	    desk_column = gtk_widget_new (GTK_TYPE_HBOX,
					  "visible", TRUE,
					  "spacing", 0,
					  "parent", gp_desk_box,
					  NULL);
	  gp_desk_widget[i] = gwm_desktop_new (max > 1 ? i : CURRENT_DESKTOP, gp_desktips);
	  alignment = gtk_widget_new (GTK_TYPE_ALIGNMENT,
				      "visible", TRUE,
				      "xscale", 0.0,
				      "yscale", 0.0,
				      NULL);
	  gtk_widget_set (gp_desk_widget[i],
			  "parent", alignment,
			  "visible", TRUE,
			  "signal::check-task", gp_check_task_visible, NULL,
			  "signal::destroy", gtk_widget_destroyed, &gp_desk_widget[i],
			  NULL);
	  gtk_box_pack_start (GTK_BOX (desk_column), alignment, FALSE, FALSE, 0);
	  if ((i + 1) % n_desks_per_column == 0)
	    desk_column = NULL;
	}
      gp_n_desk_widgets = i;
    }
}

static gboolean
gp_check_task_visible (GwmDesktop *desktop,
		       GwmhTask   *task,	
		       gpointer    user_data)
{
  if (GWMH_TASK_ICONIFIED (task) ||
      (GWMH_TASK_HIDDEN (task) && !BOOL_CONFIG (show_hidden_tasks)) ||
      (GWMH_TASK_SHADED (task) && !BOOL_CONFIG (show_shaded_tasks)) ||
      (GWMH_TASK_SKIP_WINLIST (task) && !BOOL_CONFIG (show_skip_winlist)) ||
      (GWMH_TASK_SKIP_TASKBAR (task) && !BOOL_CONFIG (show_skip_taskbar)))
    return FALSE;
  else
    return TRUE;
}

static inline gboolean
gp_widget_button_popup_task_editor (GtkWidget      *widget,
				    GdkEventButton *event,
				    gpointer        data)
{
  guint button = GPOINTER_TO_UINT (data);


  if (event->button == button)
    {
      g_message ("Task Editor unimplemented yet!");
      gdk_beep ();

      return TRUE;
    }
  else
    return FALSE;
}

static inline gboolean
gp_widget_button_popup_config (GtkWidget      *widget,
			       GdkEventButton *event,
			       gpointer        data)
{
  guint button = GPOINTER_TO_UINT (data);


  if (event->button == button)
    {
      gp_config_popup ();

      return TRUE;
    }
  else
    return FALSE;
}

static inline gboolean
gp_widget_ignore_button (GtkWidget      *widget,
			 GdkEventButton *event,
			 gpointer        data)
{
  guint button = GPOINTER_TO_UINT (data);
  
  if (event->button == button)
    gtk_signal_emit_stop_by_name (GTK_OBJECT (widget),
				  event->type == GDK_BUTTON_PRESS
				  ? "button_press_event"
				  : "button_release_event");
  return FALSE;
}

static void 
gp_init_gui (void)
{
  GtkWidget *button, *abox;
  gboolean arrow_at_end = FALSE;
  GtkWidget *main_box;
  
  gtk_widget_set_usize (gp_container, 0, 0);
  switch (applet_widget_get_panel_orient (APPLET_WIDGET (gp_applet)))
    {
    case ORIENT_UP:
      GP_TYPE_HBOX = GTK_TYPE_HBOX;
      GP_TYPE_VBOX = GTK_TYPE_VBOX;
      GP_ARROW_DIR = GTK_ARROW_UP;
      gtk_widget_set (gp_container,
		      NULL);
      gp_orientation = GTK_ORIENTATION_HORIZONTAL;
      arrow_at_end = FALSE;
      break;
    case ORIENT_DOWN:
      GP_TYPE_HBOX = GTK_TYPE_HBOX;
      GP_TYPE_VBOX = GTK_TYPE_VBOX;
      GP_ARROW_DIR = GTK_ARROW_DOWN;
      gtk_widget_set (gp_container,
		      NULL);
      gp_orientation = GTK_ORIENTATION_HORIZONTAL;
      arrow_at_end = TRUE;
      break;
    case ORIENT_LEFT:
      GP_TYPE_HBOX = GTK_TYPE_VBOX;
      GP_TYPE_VBOX = GTK_TYPE_HBOX;
      GP_ARROW_DIR = GTK_ARROW_LEFT;
      gtk_widget_set (gp_container,
		      NULL);
      gp_orientation = GTK_ORIENTATION_VERTICAL;
      arrow_at_end = FALSE;
      break;
    case ORIENT_RIGHT:
      GP_TYPE_HBOX = GTK_TYPE_VBOX;
      GP_TYPE_VBOX = GTK_TYPE_HBOX;
      GP_ARROW_DIR = GTK_ARROW_RIGHT;
      gtk_widget_set (gp_container,
		      NULL);
      gp_orientation = GTK_ORIENTATION_VERTICAL;
      arrow_at_end = TRUE;
      break;
    }
  
  /* configure tooltips
   */
  (BOOL_CONFIG (tooltips)
   ? gtk_tooltips_enable
   : gtk_tooltips_disable) (gp_tooltips);
  gtk_tooltips_set_delay (gp_tooltips, RANGE_CONFIG (tooltips_delay));
  (BOOL_CONFIG (desktips)
   ? gtk_tooltips_enable
   : gtk_tooltips_disable) (gp_desktips);
  gtk_tooltips_set_delay (gp_desktips, RANGE_CONFIG (desktips_delay));
  
  /* main container
   */
  main_box = gtk_widget_new (GP_TYPE_HBOX,
			     "visible", TRUE,
			     "spacing", 0,
			     "parent", gp_container,
			     NULL);
  
  /* provide box for arrow and button
   */
  abox = gtk_widget_new (GP_TYPE_VBOX,
			 "visible", TRUE,
			 "spacing", 0,
			 NULL);
  (BOOL_CONFIG (switch_arrow)
   ? gtk_box_pack_end
   : gtk_box_pack_start) (GTK_BOX (main_box), abox, FALSE, TRUE, 0);

  /* provide desktop widget container
   */
  gp_desk_box = gtk_widget_new (GTK_TYPE_VBOX,
				"visible", TRUE,
				"spacing", 0,
				"signal::destroy", gtk_widget_destroyed, &gp_desk_box,
				"parent", gtk_widget_new (GTK_TYPE_ALIGNMENT,
							  "visible", TRUE,
							  "xscale", 0.0,
							  "yscale", 0.0,
							  "parent", main_box,
							  NULL),
				NULL);
  
  /* add arrow and button
   */
  button = gtk_widget_new (GTK_TYPE_BUTTON,
			   "visible", TRUE,
			   "can_focus", FALSE,
			   "child", gtk_widget_new (GTK_TYPE_ARROW,
						    "arrow_type", GP_ARROW_DIR,
						    "visible", TRUE,
						    NULL),
			   "signal::button_press_event", gp_widget_button_popup_task_editor, GUINT_TO_POINTER (1),
			   "signal::button_press_event", gp_widget_ignore_button, GUINT_TO_POINTER (2),
			   "signal::button_release_event", gp_widget_ignore_button, GUINT_TO_POINTER (2),
			   "signal::button_press_event", gp_widget_ignore_button, GUINT_TO_POINTER (3),
			   "signal::button_release_event", gp_widget_ignore_button, GUINT_TO_POINTER (3),
			   NULL);
  gtk_tooltips_set_tip (gp_tooltips,
			button,
			DESK_GUIDE_NAME,
			NULL);
  (arrow_at_end
   ? gtk_box_pack_end
   : gtk_box_pack_start) (GTK_BOX (abox), button, FALSE, TRUE, 0);
  button = gtk_widget_new (GTK_TYPE_BUTTON,
			   "visible", TRUE,
			   "can_focus", FALSE,
			   "label", "?",
			   "signal::button_press_event", gp_widget_button_popup_config, GUINT_TO_POINTER (1),
			   "signal::button_press_event", gp_widget_ignore_button, GUINT_TO_POINTER (2),
			   "signal::button_release_event", gp_widget_ignore_button, GUINT_TO_POINTER (2),
			   "signal::button_press_event", gp_widget_ignore_button, GUINT_TO_POINTER (3),
			   "signal::button_release_event", gp_widget_ignore_button, GUINT_TO_POINTER (3),
			   NULL);
  gtk_tooltips_set_tip (gp_tooltips,
			button,
			DESK_GUIDE_NAME,
			NULL);
  (!arrow_at_end
   ? gtk_box_pack_end
   : gtk_box_pack_start) (GTK_BOX (abox), button, TRUE, TRUE, 0);
  
  /* desktop pagers
   */
  gp_create_desk_widgets ();
  
  gtk_widget_show (gp_container);
  gtk_widget_show (gp_applet);
}

static void 
gp_about (void)
{
  static GtkWidget *dialog = NULL;
  
  if (!dialog)
    {
      const char *authors[] = {
	"Tim Janik",
	NULL
      };
      
      dialog = gnome_about_new ("Desk Guide",
				"0.3",
				"Copyright (C) 1999 Tim Janik",
				authors,
				DESK_GUIDE_NAME,
				"gnome-deskguide-splash.png");
      gtk_widget_set (dialog,
		      "signal::destroy", gtk_widget_destroyed, &dialog,
		      NULL);
      gnome_dialog_close_hides (GNOME_DIALOG (dialog), TRUE);
      gtk_quit_add_destroy (1, GTK_OBJECT (dialog));
    }
  
  gtk_widget_show (dialog);
  gdk_window_raise (dialog->window);
}

static void
gp_config_toggled (GtkToggleButton *button,
		   ConfigItem      *item)
{
  item->tmp_value = GINT_TO_POINTER (button->active);
  gnome_property_box_changed (GNOME_PROPERTY_BOX (gtk_widget_get_toplevel (GTK_WIDGET (button))));
}

static inline GtkWidget*
gp_config_add_boolean (GtkWidget  *vbox,
		       ConfigItem *item)
{
  GtkWidget *widget;
  
  widget = gtk_widget_new (GTK_TYPE_CHECK_BUTTON,
			   "visible", TRUE,
			   "label", _ (item->name),
			   "active", GPOINTER_TO_INT (item->value),
			   "border_width", CONFIG_ITEM_BORDER,
			   "signal::toggled", gp_config_toggled, item,
			   NULL);
  gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, TRUE, 0);
  gtk_widget_set (GTK_BIN (widget)->child,
		  "xalign", 0.0,
		  NULL);
  
  return widget;
}

static void
gp_config_value_changed (GtkAdjustment *adjustment,
			 ConfigItem      *item)
{
  item->tmp_value = GINT_TO_POINTER ((gint) adjustment->value);
  gnome_property_box_changed (GNOME_PROPERTY_BOX (gtk_widget_get_toplevel (gtk_object_get_user_data (GTK_OBJECT (adjustment)))));
}

static inline GtkWidget*
gp_config_add_range (GtkWidget  *vbox,
		     ConfigItem *item)
{
  GtkObject *adjustment;
  GtkWidget *hbox, *label, *spinner;
  
  adjustment = gtk_adjustment_new (GPOINTER_TO_UINT (item->tmp_value),
				   item->min,
				   item->max,
				   1, 1,
				   (item->max - item->min) / 10);
  hbox = gtk_widget_new (GTK_TYPE_HBOX,
			 "homogeneous", FALSE,
			 "visible", TRUE,
			 "spacing", CONFIG_ITEM_SPACING,
			 "border_width", CONFIG_ITEM_BORDER,
			 NULL);
  label = gtk_widget_new (GTK_TYPE_LABEL,
			  "visible", TRUE,
			  "xalign", 0.0,
			  "label", _ (item->name),
			  "parent", hbox,
			  NULL);
  spinner = gtk_spin_button_new (GTK_ADJUSTMENT (adjustment), 0, 0);
  gtk_widget_set (spinner,
		  "visible", TRUE,
		  "width", 80,
		  NULL);
  gtk_object_set_user_data (GTK_OBJECT (adjustment), spinner);
  gtk_box_pack_end (GTK_BOX (hbox), spinner, FALSE, TRUE, 0);
  gtk_signal_connect (adjustment,
		      "value_changed",
		      GTK_SIGNAL_FUNC (gp_config_value_changed),
		      item);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);
  
  return hbox;
}

static inline GtkWidget*
gp_config_add_section (GtkBox      *parent,
		       const gchar *section)
{
  GtkWidget *widget;
  GtkWidget *box;
  
  if (section)
    {
      widget = gtk_widget_new (GTK_TYPE_FRAME,
			       "visible", TRUE,
			       "label", section,
			       NULL);
      box = gtk_widget_new (GTK_TYPE_VBOX,
			    "visible", TRUE,
			    "parent", widget,
			    "border_width", CONFIG_IBOX_BORDER,
			    "spacing", CONFIG_IBOX_SPACING,
			    NULL);
    }
  else
    {
      box = gtk_widget_new (GTK_TYPE_VBOX,
			    "visible", TRUE,
			    "border_width", 0,
			    "spacing", CONFIG_OBOX_SPACING,
			    NULL);
      widget = box;
    }

  gtk_box_pack_start (parent, widget, FALSE, TRUE, 0);
  
  return box;
}

static GSList*
gp_config_create_page (GSList		*item_slist,
		       GnomePropertyBox *pbox)
{
  gchar *page_name;
  ConfigItem *item;
  GtkWidget *page, *vbox = NULL;
  
  g_return_val_if_fail (item_slist != NULL, NULL);
  
  item = item_slist->data;
  if (!item->path)
    {
      GSList *node = item_slist;
      
      item_slist = node->next;
      
      page_name = _ (item->name);
    }
  else
    page_name = _ ("Global");
  
  page = gtk_widget_new (GTK_TYPE_VBOX,
			 "visible", TRUE,
			 "border_width", CONFIG_OBOX_BORDER,
			 "spacing", CONFIG_OBOX_SPACING,
			 NULL);
  
  while (item_slist)
    {
      ConfigItem *item = item_slist->data;
      
      if (!item->path)						/* page */
	break;
      item_slist = item_slist->next;
      
      if (item->min == -2 && item->max == -2)			/* section */
	vbox = gp_config_add_section (GTK_BOX (page), _ (item->name));
      else if (item->min == -1 && item->max == -1)		/* boolean */
	{
	  if (!vbox)
	    vbox = gp_config_add_section (GTK_BOX (page), NULL);
	  gp_config_add_boolean (vbox, item);
	}
      else							/* integer range */
	{
	  if (!vbox)
	    vbox = gp_config_add_section (GTK_BOX (page), NULL);
	  gp_config_add_range (vbox, item);
	}
    }
  
  gnome_property_box_append_page (pbox, page, gtk_label_new (_ (page_name)));
  
  return item_slist;
}

static void 
gp_config_popup (void)
{
  static GtkWidget *dialog = NULL;
  
  g_return_if_fail (gp_n_config_items > 0);
  
  if (!dialog)
    {
      GSList *fslist, *slist = NULL;
      guint i;
      
      for (i = 0; i < gp_n_config_items; i++)
	slist = g_slist_prepend (slist, gp_config_items + i);
      slist = g_slist_reverse (slist);
      fslist = slist;
      
      dialog = gnome_property_box_new ();
      gtk_widget_set (dialog,
		      "title", _ ("Desk Guide Settings"),
		      "signal::apply", gp_destroy_gui, NULL,
		      "signal::apply", gp_config_apply_tmp_values, NULL,
		      "signal::apply", gp_init_gui, NULL,
		      "signal::destroy", gp_config_reset_tmp_values, NULL,
		      "signal::destroy", gtk_widget_destroyed, &dialog,
		      NULL);
      
      while (slist)
	slist = gp_config_create_page (slist, GNOME_PROPERTY_BOX (dialog));
      g_slist_free (fslist);

      gtk_quit_add_destroy (1, GTK_OBJECT (dialog));
    }
  
  gtk_widget_show (dialog);
  gdk_window_raise (dialog->window);
}
