#include <config.h>
#include <stdio.h>
#include <stdarg.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <X11/X.h>
#include <config.h>

#include "gnome.h"
#include "gnome-desktop.h"

static GnomePropertyConfigurator *config;

static gint bell_percent;
static gint bell_pitch;
static gint bell_duration;
static XKeyboardState kbdstate;
static XKeyboardControl kbdcontrol;

static GtkWidget *vscale, *pscale, *dscale;


static void bell_read(void)
{
	bell_percent = gnome_config_get_int("/Desktop/Bell/percent=-1");
	bell_pitch = gnome_config_get_int("/Desktop/Bell/pitch=-1");
	bell_duration = gnome_config_get_int("/Desktop/Bell/duration=-1");

	XGetKeyboardControl(GDK_DISPLAY(), &kbdstate);

	if (bell_percent == -1) {
		bell_percent = kbdstate.bell_percent;
	}
	if (bell_pitch == -1) {
	        bell_pitch = kbdstate.bell_pitch;
        }
	if (bell_duration == -1) {
	        bell_duration = kbdstate.bell_duration;
	}
}

static void bell_write(void)
{
	gnome_config_set_int("/Desktop/Bell/percent", bell_percent);
	gnome_config_set_int("/Desktop/Bell/pitch", bell_pitch);
	gnome_config_set_int("/Desktop/Bell/duration", bell_duration);
	gnome_config_sync ();
}

static void bell_apply(void)
{
	kbdcontrol.bell_percent = bell_percent;
	kbdcontrol.bell_pitch = bell_pitch;
	kbdcontrol.bell_duration = bell_duration;
	XChangeKeyboardControl(GDK_DISPLAY(), KBBellPercent | KBBellPitch | KBBellDuration, &kbdcontrol);

	property_applied ();
}

static void
percent_changed(GtkAdjustment *adj, gpointer data)
{
	bell_percent = adj->value;
	property_changed ();
}

static void
pitch_changed(GtkAdjustment *adj, gpointer *data)
{
	bell_pitch = adj->value;
	property_changed ();
}

static void
duration_changed(GtkAdjustment *adj, gpointer *data)
{
	bell_duration = adj->value;
	property_changed ();
}

static void
test_bell(GtkWidget *widget, void *data)
{
        gint save_percent;
	gint save_pitch;
	gint save_duration;

	XGetKeyboardControl(GDK_DISPLAY(), &kbdstate);

	save_percent = kbdstate.bell_percent;
	save_pitch = kbdstate.bell_pitch;
	save_duration = kbdstate.bell_duration;
	
	kbdcontrol.bell_percent = bell_percent;
	kbdcontrol.bell_pitch = bell_pitch;
	kbdcontrol.bell_duration = bell_duration;
	XChangeKeyboardControl(GDK_DISPLAY(), KBBellPercent | KBBellPitch | KBBellDuration, &kbdcontrol);

        XBell(gdk_display,0);

	kbdcontrol.bell_percent = save_percent;
	kbdcontrol.bell_pitch = save_pitch;
	kbdcontrol.bell_duration = save_duration;
	XChangeKeyboardControl(GDK_DISPLAY(), KBBellPercent | KBBellPitch | KBBellDuration, &kbdcontrol);

	return;
}

static void
bell_setup(void)
{
	GtkWidget *vbox;
	GtkWidget *frame;
	GtkWidget *hbox, *tbox;
	GtkWidget *table;
	GtkWidget *tbutton;
	GtkWidget *label;
	GtkObject *vadj, *padj, *dadj;

	vbox = gtk_vbox_new(FALSE, GNOME_PAD_SMALL);
	gtk_container_border_width(GTK_CONTAINER(vbox), GNOME_PAD);

	frame = gtk_frame_new(_("Sound"));
	gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
	gtk_widget_show (frame);

	table = gtk_table_new (4, 2, FALSE);
	gtk_container_border_width (GTK_CONTAINER (table), GNOME_PAD);
	gtk_table_set_row_spacings (GTK_TABLE (table), GNOME_PAD_SMALL);
	gtk_table_set_col_spacings (GTK_TABLE (table), GNOME_PAD_SMALL);
	gtk_container_add (GTK_CONTAINER (frame), table);
	gtk_widget_show (table);
	
	hbox = gtk_hbox_new (FALSE, 0);
	gtk_table_attach (GTK_TABLE (table), hbox,
			  0, 2, 0, 1,
			  GTK_FILL | GTK_SHRINK,
			  GTK_FILL | GTK_SHRINK,
			  0, 0);
	gtk_widget_show (hbox);

	label = gtk_label_new(_("Volume"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 1.0);
	gtk_table_attach (GTK_TABLE (table), label,
			  0, 1, 1, 2,
			  GTK_FILL | GTK_SHRINK,
			  GTK_FILL | GTK_SHRINK,
			  0, 0);
	gtk_widget_show(label);

	vadj = gtk_adjustment_new(bell_percent, 0, 100, 1, 1, 0);
	vscale = gtk_hscale_new(GTK_ADJUSTMENT(vadj));
	gtk_scale_set_digits (GTK_SCALE (vscale), 0);
	gtk_signal_connect(GTK_OBJECT(vadj), "value_changed",
			   GTK_SIGNAL_FUNC(percent_changed), NULL);
	gtk_table_attach (GTK_TABLE (table), vscale,
			  1, 2, 1, 2,
			  GTK_EXPAND | GTK_FILL | GTK_SHRINK,
			  GTK_FILL | GTK_SHRINK,
			  0, 0);
	gtk_widget_show(vscale);

	label = gtk_label_new(_("Pitch"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 1.0);
	gtk_table_attach (GTK_TABLE (table), label,
			  0, 1, 2, 3,
			  GTK_FILL | GTK_SHRINK,
			  GTK_FILL | GTK_SHRINK,
			  0, 0);
	gtk_widget_show(label);

	padj = gtk_adjustment_new(bell_pitch, 0, 2000, 1, 1, 0);
	pscale = gtk_hscale_new(GTK_ADJUSTMENT(padj));
	gtk_scale_set_digits (GTK_SCALE (pscale), 0);
	gtk_signal_connect(GTK_OBJECT(padj), "value_changed",
			   GTK_SIGNAL_FUNC(pitch_changed), NULL);
	gtk_table_attach (GTK_TABLE (table), pscale,
			  1, 2, 2, 3,
			  GTK_EXPAND | GTK_FILL | GTK_SHRINK,
			  GTK_FILL | GTK_SHRINK,
			  0, 0);
	gtk_widget_show(pscale);

	label = gtk_label_new(_("Duration"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 1.0);
	gtk_table_attach (GTK_TABLE (table), label,
			  0, 1, 3, 4,
			  GTK_FILL | GTK_SHRINK,
			  GTK_FILL | GTK_SHRINK,
			  0, 0);
	gtk_widget_show(label);

	dadj = gtk_adjustment_new(bell_duration, 0, 500, 1, 1, 0);
	dscale = gtk_hscale_new(GTK_ADJUSTMENT(dadj));
	gtk_scale_set_digits (GTK_SCALE (dscale), 0);
	gtk_signal_connect(GTK_OBJECT(dadj), "value_changed",
			   GTK_SIGNAL_FUNC(duration_changed), NULL);
	gtk_table_attach (GTK_TABLE (table), dscale,
			  1, 2, 3, 4,
			  GTK_EXPAND | GTK_FILL | GTK_SHRINK,
			  GTK_FILL | GTK_SHRINK,
			  0, 0);
	gtk_widget_show (dscale);

	/* Set sensitivity of scale */

	gtk_widget_set_sensitive(vscale, bell_percent);
	gtk_widget_set_sensitive(pscale, bell_pitch);
	gtk_widget_set_sensitive(dscale, bell_duration);

	/* Throw up a test button */

      	tbox = gtk_hbox_new(TRUE, 0);
	tbutton = gtk_button_new_with_label("Test");
	gtk_widget_set_usize(tbutton, 100, 30);
	gtk_signal_connect(GTK_OBJECT (tbutton), "clicked",
			   GTK_SIGNAL_FUNC (test_bell), NULL);
	gtk_box_pack_start (GTK_BOX (tbox), tbutton, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), tbox, FALSE, FALSE, 5);
	gtk_widget_show (tbutton);
	gtk_widget_show (tbox);

	/* Finished */

	gtk_widget_show(vbox);
	gnome_property_box_append_page(GNOME_PROPERTY_BOX(config->property_box),
				       vbox,
				       gtk_label_new(_("Bell")));
}

static gint bell_action(GnomePropertyRequest req)
{
	switch (req) {
	case GNOME_PROPERTY_READ:
		bell_read();
		break;
	case GNOME_PROPERTY_WRITE:
		bell_write();
		break;
	case GNOME_PROPERTY_APPLY:
		bell_apply();
		break;
	case GNOME_PROPERTY_SETUP:
		bell_setup();
		break;
	default:
		return 0;
	}

	return 1;
}

void bell_register(GnomePropertyConfigurator *c)
{
	config = c;
	gnome_property_configurator_register(config, bell_action);
}
