/* property-mouse.c - Property page for configuring the mouse.  */

#include <config.h>
#include <X11/Xlib.h>
#include <assert.h>

#include <gdk/gdkx.h>

#include "gnome.h"
#include "gnome-desktop.h"

/* Maximum number of mouse buttons we handle.  */
#define MAX_BUTTONS 10

/* Half the number of acceleration levels we support.  */
#define MAX_ACCEL 3

/* Maximum threshold we support.  */
#define MAX_THRESH 7


/* The property configurator we're associated with.  */
static GnomePropertyConfigurator *config;

/* True if buttons are mapped right-to-left.  */
static gboolean mouse_rtol;

/* Number of buttons.  */
static int mouse_nbuttons;

/* Our acceleration number.  This is just an integer between 0 and
   2*MAX_ACCEL+1, inclusive.  */
static int mouse_acceleration;

/* Acceleration threshold.  */
static int mouse_thresh;

static void
mouse_read (void)
{
  unsigned char buttons[MAX_BUTTONS];
  int acc_num, acc_den, thresh;
  gboolean rtol_default;

  mouse_nbuttons = XGetPointerMapping (GDK_DISPLAY (), buttons, MAX_BUTTONS);
  assert (mouse_nbuttons <= MAX_BUTTONS);

  /* Note that we only handle right-to-left and left-to-right.
     Most weird mappings are treated as l-to-r.
     We could handle this by showing the mouse buttons and letting the
     user drag-and-drop them to reorder.  But I'm not convinced this
     is worth it.  */
  /* FIXME: this ignores the fact that a mouse with the weird little
     roller generates B4 and B5 when the roller is moved.  That
     shouldn't change when we remap the other mouse buttons.  */
  mouse_rtol = gnome_config_get_bool_with_default ("/Desktop/Mouse/right-to-left=false",
						   &rtol_default);
  if (rtol_default)
    mouse_rtol = (buttons[mouse_nbuttons - 1] == 1);

  mouse_thresh = gnome_config_get_int ("/Desktop/Mouse/threshold=-1");
  mouse_acceleration = gnome_config_get_int ("/Desktop/Mouse/acceleration=-1");

  if (mouse_thresh == -1 || mouse_acceleration == -1)
    {
      XGetPointerControl (GDK_DISPLAY (), &acc_num, &acc_den, &thresh);

      if (mouse_thresh == -1)
	mouse_thresh = thresh;
      if (mouse_acceleration == -1)
	{
	  /* Only support cases in our range.  If neither the numerator nor
	     denominator is 1, then rescale.  */
	  if (acc_num != 1 && acc_den != 1)
	    {
	      if (acc_num > acc_den)
		{
		  acc_num = (int) ((double) acc_num / acc_den);
		  acc_den = 1;
		}
	      else
		{
		  acc_den = (int) ((double) acc_den / acc_num);
		  acc_num = 1;
		}
	    }

	  if (acc_num > MAX_ACCEL)
	    acc_num = MAX_ACCEL;
	  if (acc_den > MAX_ACCEL)
	    acc_den = MAX_ACCEL;
	  if (acc_den == 1)
	    mouse_acceleration = acc_num + MAX_ACCEL - 1;
	  else
	    mouse_acceleration = MAX_ACCEL - acc_den;
	}
    }
}

static void
mouse_write (void)
{
  gnome_config_set_int ("/Desktop/Mouse/acceleration", mouse_acceleration);
  gnome_config_set_int ("/Desktop/Mouse/threshold", mouse_thresh);
  gnome_config_set_bool ("/Desktop/Mouse/right-to-left", mouse_rtol);
  gnome_config_sync ();
}

static void
mouse_apply (void)
{
  unsigned char buttons[MAX_BUTTONS], i;
  int num, den;

  assert (mouse_nbuttons <= MAX_BUTTONS);

  for (i = 0; i < mouse_nbuttons; ++i)
    buttons[i] = mouse_rtol ? (mouse_nbuttons - i) : (i + 1);
  XSetPointerMapping (GDK_DISPLAY (), buttons, mouse_nbuttons);


  if (mouse_acceleration < MAX_ACCEL)
    {
      num = 1;
      den = MAX_ACCEL - mouse_acceleration;
    }
  else
    {
      num = mouse_acceleration - MAX_ACCEL + 1;
      den = 1;
    }

  XChangePointerControl (GDK_DISPLAY (), True, True, num, den, mouse_thresh);

  property_applied ();
}

/* Run when the left- or right-handed radiobutton is clicked.  */
static void
button_toggled (GtkWidget *widget, gpointer data)
{
  mouse_rtol = (int) data;
  property_changed ();
}

/* Run when a scale widget is manipulated.  */
static void
scale_moved (GtkAdjustment *adj, gpointer data)
{
  int *value = (int *) data;
  *value = adj->value;
  property_changed ();
}

static void
make_scale (char *title, char *max_title, char *min_title,
	    GtkObject *adjust, int *update_var, GtkWidget *table, int row)
{
  GtkWidget *scale, *low, *high, *ttl;

  ttl = gtk_label_new (title);
  gtk_misc_set_alignment (GTK_MISC (ttl), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), ttl,
		    0, 3, row, row + 1,
		    GTK_FILL | GTK_SHRINK,
		    GTK_FILL | GTK_SHRINK,
		    0, 0);
  gtk_widget_show (ttl);

  low = gtk_label_new (min_title);
  gtk_misc_set_alignment (GTK_MISC (low), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), low,
		    0, 1, row + 1, row + 2,
		    GTK_FILL | GTK_SHRINK,
		    GTK_FILL | GTK_SHRINK,
		    0, 0);
  gtk_widget_show (low);

  scale = gtk_hscale_new (GTK_ADJUSTMENT (adjust));
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_CONTINUOUS);
  gtk_scale_set_digits (GTK_SCALE (scale), 0);
  gtk_scale_set_draw_value (GTK_SCALE (scale), 0);
  gtk_signal_connect (GTK_OBJECT (adjust), "value_changed",
		      GTK_SIGNAL_FUNC (scale_moved),
		      (gpointer) update_var);
/*   gtk_widget_set_usize (scale, 200, -1); */
  gtk_table_attach (GTK_TABLE (table), scale,
		    1, 2, row + 1, row + 2,
		    GTK_EXPAND | GTK_FILL | GTK_SHRINK,
		    GTK_FILL | GTK_SHRINK,
		    0, 0);
  gtk_widget_show (scale);

  high = gtk_label_new (max_title);
  gtk_misc_set_alignment (GTK_MISC (high), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), high,
		    2, 3, row + 1, row + 2,
		    GTK_FILL | GTK_SHRINK,
		    GTK_FILL | GTK_SHRINK,
		    0, 0);
  gtk_widget_show (high);
}

static void
mouse_setup (void)
{
  GtkWidget *vbox, *frame, *hbox, *lbutton, *rbutton, *table, *sep;
  GtkObject *adjust;

  hbox = gtk_hbox_new (FALSE, GNOME_PAD_SMALL);
  gtk_container_border_width (GTK_CONTAINER (hbox), GNOME_PAD);

  /* Mouse buttons */

  frame = gtk_frame_new (_("Mouse buttons"));
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = gtk_vbox_new (FALSE, GNOME_PAD_SMALL);
  gtk_container_border_width (GTK_CONTAINER (vbox), GNOME_PAD_SMALL);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  lbutton = gtk_radio_button_new_with_label (NULL, _("Left handed"));
  rbutton = gtk_radio_button_new_with_label (gtk_radio_button_group (GTK_RADIO_BUTTON (lbutton)),
					     _("Right handed"));
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON ((mouse_rtol
						   ? lbutton
						   : rbutton)), TRUE);
  gtk_signal_connect (GTK_OBJECT (lbutton), "clicked",
		      GTK_SIGNAL_FUNC (button_toggled),
		      (gpointer) 1);
  gtk_signal_connect (GTK_OBJECT (rbutton), "clicked",
		      GTK_SIGNAL_FUNC (button_toggled),
		      (gpointer) 0);
  gtk_box_pack_start (GTK_BOX (vbox), lbutton, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), rbutton, FALSE, FALSE, 0);
  gtk_widget_show (lbutton);
  gtk_widget_show (rbutton);

  /* Mouse motion */

  frame = gtk_frame_new (_("Mouse motion"));
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (5, 3, FALSE);
  gtk_container_border_width (GTK_CONTAINER (table), GNOME_PAD_SMALL);
  gtk_table_set_row_spacings (GTK_TABLE (table), GNOME_PAD_SMALL);
  gtk_table_set_col_spacings (GTK_TABLE (table), GNOME_PAD_SMALL);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  adjust = gtk_adjustment_new (mouse_acceleration, 0, 2 * MAX_ACCEL + 1, 1, 1, 1);
  make_scale (_("Acceleration"), _("Fast"), _("Slow"),
	      adjust, &mouse_acceleration, table, 0);

  sep = gtk_hseparator_new ();
  gtk_table_attach (GTK_TABLE (table), sep,
		    0, 3, 2, 3,
		    GTK_FILL | GTK_SHRINK,
		    GTK_FILL | GTK_SHRINK,
		    0, 0);
  gtk_widget_show (sep);

  adjust = gtk_adjustment_new (mouse_thresh, 0, MAX_THRESH, 1, 1, 1);
  make_scale (_("Threshold"), _("Large"), _("Small"),
	      adjust, &mouse_thresh, table, 3);

  /* Done */
  
  gtk_widget_show (hbox);
  gnome_property_box_append_page (GNOME_PROPERTY_BOX (config->property_box),
				  hbox, gtk_label_new (_("Mouse")));
}

static gint
mouse_action (GnomePropertyRequest req)
{
  switch (req)
    {
    case GNOME_PROPERTY_READ:
      mouse_read ();
      break;
    case GNOME_PROPERTY_WRITE:
      mouse_write ();
      break;
    case GNOME_PROPERTY_APPLY:
      mouse_apply ();
      break;
    case GNOME_PROPERTY_SETUP:
      mouse_setup ();
      break;
    default:
      return 0;
    }

  return 1;
}

void
mouse_register (GnomePropertyConfigurator *c)
{
  config = c;
  gnome_property_configurator_register (config, mouse_action);
}
