/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* Author: Martin Baulig <martin@home-of-linux.org>
 * Based on gnome-core/desktop-properties/property-bell.c with
 * ideas from capplets/keyboard-properties/keyboard-properties.c.
 */
#include <config.h>
#include "capplet-widget.h"
#include <stdio.h>
#include <stdarg.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <X11/X.h>

#ifdef HAVE_X11_EXTENSIONS_XF86MISC_H
#include <X11/extensions/xf86misc.h>
#endif

#include "gnome.h"

static gint bell_percent;
static gint bell_pitch;
static gint bell_duration;
static XKeyboardState kbdstate;
static XKeyboardControl kbdcontrol;

static GtkObject *vadj, *padj, *dadj;
static GtkWidget *vscale, *pscale, *dscale;

static GtkWidget *capplet;

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

static void bell_apply(void)
{
	kbdcontrol.bell_percent = bell_percent;
	kbdcontrol.bell_pitch = bell_pitch;
	kbdcontrol.bell_duration = bell_duration;
	XChangeKeyboardControl(GDK_DISPLAY(), KBBellPercent | KBBellPitch | KBBellDuration, &kbdcontrol);
}

static void bell_write(void)
{
	bell_apply();
	gnome_config_set_int("/Desktop/Bell/percent", bell_percent);
	gnome_config_set_int("/Desktop/Bell/pitch", bell_pitch);
	gnome_config_set_int("/Desktop/Bell/duration", bell_duration);
	gnome_config_sync ();
}

static void
bell_revert (void)
{
        bell_read();
        bell_apply();
        GTK_ADJUSTMENT (vadj)->value = bell_percent;
        GTK_ADJUSTMENT (padj)->value = bell_pitch;
        GTK_ADJUSTMENT (dadj)->value = bell_duration;
        gtk_adjustment_changed (GTK_ADJUSTMENT (vadj));
        gtk_adjustment_changed (GTK_ADJUSTMENT (padj));
        gtk_adjustment_changed (GTK_ADJUSTMENT (dadj));
}

/* Run when a scale widget is manipulated.  */
static void
scale_moved (GtkAdjustment *adj, gpointer data)
{
        int *value = (int *) data;
        *value = adj->value;
        capplet_widget_state_changed(CAPPLET_WIDGET (capplet), TRUE);
}

static void
bell_test(GtkWidget *widget, void *data)
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
}

static GtkWidget *
make_scale (char *title, GtkObject *adjust, int *update_var, GtkWidget *table, int row)
{
        GtkWidget *scale, *ttl;

        ttl = gtk_label_new (title);

        gtk_misc_set_alignment (GTK_MISC (ttl), 0.0, 0.5);
        gtk_table_attach (GTK_TABLE (table), ttl,
                          0, 1, row, row + 1,
                          GTK_FILL | GTK_SHRINK,
                          GTK_FILL | GTK_SHRINK,
                          0, 0);
        gtk_widget_show (ttl);

        scale = gtk_hscale_new (GTK_ADJUSTMENT (adjust));
        gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_CONTINUOUS);
        gtk_scale_set_digits (GTK_SCALE (scale), 0);
        gtk_signal_connect (GTK_OBJECT (adjust), "value_changed",
                            GTK_SIGNAL_FUNC (scale_moved),
                            (gpointer) update_var);

        gtk_table_attach (GTK_TABLE (table), scale,
                          1, 2, row, row + 1,
                          GTK_EXPAND | GTK_FILL | GTK_SHRINK,
                          GTK_FILL | GTK_SHRINK,
                          0, 0);
        gtk_widget_show (scale);

        return scale;
}

static void
bell_setup(void)
{
	GtkWidget *vbox;
	GtkWidget *frame;
	GtkWidget *table;

        capplet = capplet_widget_new();

	vbox = gtk_vbox_new(FALSE, GNOME_PAD_SMALL);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), GNOME_PAD);

	frame = gtk_frame_new(_("Sound"));
	gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
	gtk_widget_show (frame);

	table = gtk_table_new (4, 2, FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (table), GNOME_PAD);
	gtk_table_set_row_spacings (GTK_TABLE (table), GNOME_PAD_SMALL);
	gtk_table_set_col_spacings (GTK_TABLE (table), GNOME_PAD_SMALL);
	gtk_container_add (GTK_CONTAINER (frame), table);
	gtk_widget_show (table);
	
	vadj = gtk_adjustment_new(bell_percent, 0, 100, 1, 1, 0);
	vscale = make_scale(_("Volume"), vadj, &bell_percent, table, 1);

	padj = gtk_adjustment_new(bell_pitch, 0, 2000, 1, 1, 0);
        pscale = make_scale (_("Pitch"), padj, &bell_pitch, table, 2);

	dadj = gtk_adjustment_new(bell_duration, 0, 500, 1, 1, 0);
	dscale = make_scale(_("Duration"), dadj, &bell_duration, table, 3);

	/* Set sensitivity of scale */

	gtk_widget_set_sensitive(vscale, bell_percent);
	gtk_widget_set_sensitive(pscale, bell_pitch);
	gtk_widget_set_sensitive(dscale, bell_duration);

	/* Finished */

        gtk_signal_connect (GTK_OBJECT (capplet), "try",
                            GTK_SIGNAL_FUNC (bell_test), NULL);
        gtk_signal_connect (GTK_OBJECT (capplet), "revert",
                            GTK_SIGNAL_FUNC (bell_revert), NULL);
        gtk_signal_connect (GTK_OBJECT (capplet), "ok",
                            GTK_SIGNAL_FUNC (bell_write), NULL);

        gtk_widget_show (vbox);
        gtk_container_add (GTK_CONTAINER (capplet), vbox);
        gtk_widget_show (capplet);
}


int
main (int argc, char **argv)
{

        bindtextdomain (PACKAGE, GNOMELOCALEDIR);
        textdomain (PACKAGE);

        gnome_capplet_init ("bell-properties", VERSION, argc,
                            argv, NULL, 0, NULL);

        bell_read ();
        bell_setup ();
        capplet_gtk_main ();

	return 0;
}
