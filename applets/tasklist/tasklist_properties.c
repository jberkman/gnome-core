#include "tasklist_applet.h"

/* The tasklist configuration */
extern TasklistConfig Config;

/* The tasklist properties configuration */
TasklistConfig PropsConfig;

/* The Property box */
GtkWidget *prop;

/* Callback for apply */
static gboolean
cb_apply (GtkWidget *widget, gint page, gpointer data)
{
	g_print ("Numrows: %d\n", PropsConfig.rows);

	/* Copy the Property struct back to the Config struct */
	memcpy (&Config, &PropsConfig, sizeof (TasklistConfig));

	/* Save properties */
	write_config ();

	/* Redraw everything */
	change_size ();
}

/* Callbacks for spin buttons */
static gboolean
cb_spin_button (GtkAdjustment *adj, int *data)
{
	gnome_property_box_changed (GNOME_PROPERTY_BOX (prop));

	*data = (gint) (adj->value);
}

/* Create a spin button */
GtkWidget *
create_spin_button (gchar *name,
		    gint *init_value,
		    gfloat min_value,
		    gfloat max_value,
		    gfloat page_value)
{
	GtkObject *adj;
	GtkWidget *spin;
	GtkWidget *hbox;

	adj = gtk_adjustment_new (*init_value,
				  min_value,
				  max_value,
				  1,
				  page_value,
				  page_value);

	hbox = gtk_hbox_new (FALSE, GNOME_PAD_SMALL);

	spin = gtk_spin_button_new (GTK_ADJUSTMENT (adj), 1, 0);
	gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
			    GTK_SIGNAL_FUNC (cb_spin_button), init_value);
	gtk_box_pack_start_defaults (GTK_BOX (hbox),
				     gtk_label_new (name));
	gtk_box_pack_start_defaults (GTK_BOX (hbox), spin);

	return hbox;
}

/* Create the geometry page */
void
create_geometry_page (void)
{
	GtkWidget *hbox, *table, *frame, *vbox;
	
	hbox = gtk_hbox_new (FALSE, GNOME_PAD_SMALL);
	gtk_container_border_width (GTK_CONTAINER (hbox), GNOME_PAD_SMALL);

	frame = gtk_frame_new (_("Horizontal"));
	gtk_box_pack_start_defaults (GTK_BOX (hbox), frame);
	
	vbox = gtk_vbox_new (FALSE, GNOME_PAD_SMALL);
	
	gtk_box_pack_start (GTK_BOX (vbox),
			    create_spin_button (_("Tasklist width"),
						&PropsConfig.width,
						48,
						1024,
						10),
			    FALSE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (vbox),
			    create_spin_button (_("Number of rows"),
						&PropsConfig.rows,
						1,
						8,
						1),
			    FALSE, TRUE, 0);

	gtk_container_add (GTK_CONTAINER (frame), vbox);
	
	frame = gtk_frame_new (_("Vertical"));
	gtk_box_pack_start_defaults (GTK_BOX (hbox), frame);
	
	vbox = gtk_vbox_new (FALSE, GNOME_PAD_SMALL);

	gtk_box_pack_start (GTK_BOX (vbox),
				     create_spin_button (_("Tasklist height"),
							 &PropsConfig.height,
							 48,
							 1024,
							 10),
			    FALSE, TRUE, 0);

	gtk_container_add (GTK_CONTAINER (frame), vbox);

	gnome_property_box_append_page (GNOME_PROPERTY_BOX (prop), hbox,
					gtk_label_new (_("Geometry")));
}

/* Display property dialog */
void
display_properties (void)
{
	/* Copy memory from the tasklist config 
	   to the tasklist properties config. */
	memcpy (&PropsConfig, &Config, sizeof (TasklistConfig));

	prop = gnome_property_box_new ();
	gtk_signal_connect (GTK_OBJECT (prop), "apply",
			    GTK_SIGNAL_FUNC (cb_apply), NULL);
	create_geometry_page ();

	gtk_widget_show_all (prop);
}

