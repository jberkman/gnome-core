/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* Author: Jaka Mocnik <jaka.mocnik@kiss.uni-lj.si>
 * Based on gnome-core/desktop-properties/property-keyboard.c
 */
#include "capplet-widget.h"
#include <config.h>
#include <stdio.h>
#include <stdarg.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <X11/X.h>
#include <config.h>

#ifdef HAVE_X11_EXTENSIONS_XF86MISC_H
#include <X11/extensions/xf86misc.h>
#endif

#include "gnome.h"

static gint keyboard_rate;
static gint keyboard_delay;
static gint keyboard_repeat;
static gint click_volume;
static gint click_on_keypress;
static gint enable_auto_repeat;
static XKeyboardState kbdstate;
static XKeyboardControl kbdcontrol;
static GtkWidget *capplet;
static GtkWidget *rbutton, *cbutton, *rscale, *dscale, *vscale;
static GtkObject *vol_adjust, *rate_adjust, *del_adjust;

#ifdef HAVE_X11_EXTENSIONS_XF86MISC_H
static XF86MiscKbdSettings kbdsettings;
#endif

static void
keyboard_read(void)
{
	gboolean repeat_default, click_default;

	keyboard_rate = gnome_config_get_int("/Desktop/Keyboard/rate=-1");
	keyboard_delay = gnome_config_get_int("/Desktop/Keyboard/delay=-1");
	keyboard_repeat = gnome_config_get_bool_with_default ("/Desktop/Keyboard/repeat=false", &repeat_default);
        click_volume = gnome_config_get_int("/Desktop/Keyboard/clickvolume=-1");
        click_on_keypress = gnome_config_get_bool_with_default("/Desktop/Keyboard/click=false", &click_default);

	XGetKeyboardControl(GDK_DISPLAY(), &kbdstate);

	if (repeat_default) {
		keyboard_repeat = kbdstate.global_auto_repeat;
	}
	if (keyboard_rate == -1 || keyboard_delay == -1) {
#ifdef HAVE_X11_EXTENSIONS_XF86MISC_H
		XF86MiscGetKbdSettings(GDK_DISPLAY(), &kbdsettings);
		keyboard_rate = kbdsettings.rate;
		keyboard_delay = kbdsettings.delay;
#else
		/* FIXME: how to get the keyboard speed on non-xf86? */
		keyboard_rate = 5;
		keyboard_delay = 500;
#endif
	}

	if (click_default) {
		click_on_keypress =  (kbdstate.key_click_percent == 0);
	}
	if (click_volume == -1) {
		click_volume = kbdstate.key_click_percent;
	}
}

static void
keyboard_apply(void)
{
	if (keyboard_repeat) {
		XAutoRepeatOn(GDK_DISPLAY());
#ifdef HAVE_X11_EXTENSIONS_XF86MISC_H
		kbdsettings.rate =  keyboard_rate;
		kbdsettings.delay = keyboard_delay;
		XF86MiscSetKbdSettings(GDK_DISPLAY(), &kbdsettings);
#endif
	} else {
		XAutoRepeatOff(GDK_DISPLAY());
	}

	kbdcontrol.key_click_percent = click_on_keypress ? click_volume : 0;
	XChangeKeyboardControl(GDK_DISPLAY(), KBKeyClickPercent, &kbdcontrol);
}

static void
keyboard_write(void)
{
        keyboard_apply();
	gnome_config_set_bool("/Desktop/Keyboard/repeat", keyboard_repeat);
	gnome_config_set_int("/Desktop/Keyboard/delay", keyboard_delay);
	gnome_config_set_int("/Desktop/Keyboard/rate", keyboard_rate);
	gnome_config_set_bool("/Desktop/Keyboard/click", click_on_keypress);
	gnome_config_set_int("/Desktop/Keyboard/clickvolume", click_volume);
	gnome_config_sync ();
}

static void
keyboard_revert (void)
{
        keyboard_read();
        keyboard_apply();
        GTK_ADJUSTMENT (vol_adjust)->value = click_volume;
        GTK_ADJUSTMENT (rate_adjust)->value = keyboard_rate;
        GTK_ADJUSTMENT (del_adjust)->value = keyboard_delay;
        gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (rbutton), keyboard_repeat);
        gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (cbutton), click_on_keypress);
        gtk_adjustment_changed (GTK_ADJUSTMENT (vol_adjust));
        gtk_adjustment_changed (GTK_ADJUSTMENT (rate_adjust));
        gtk_adjustment_changed (GTK_ADJUSTMENT (del_adjust));
}

/* Run when a toggle button is clicked.  */
static void
button_toggled (GtkWidget *widget, gpointer data)
{
        gint sens;

        if(widget == cbutton) {
                if(GTK_TOGGLE_BUTTON(widget)->active) {
                        click_on_keypress = TRUE;
                        sens = TRUE;
                }
                else {
                        click_on_keypress = FALSE;
                        sens = FALSE;
                }
                gtk_widget_set_sensitive(vscale, sens);
        }
        else if(widget == rbutton) {
                if(GTK_TOGGLE_BUTTON(widget)->active) {
                        keyboard_repeat = TRUE;
                        sens = TRUE;
                }
                else {
                        keyboard_repeat = FALSE;
                        sens = FALSE;
                }
                gtk_widget_set_sensitive(rscale, sens);
                gtk_widget_set_sensitive(dscale, sens);
        }

        capplet_widget_state_changed(CAPPLET_WIDGET (capplet), TRUE);
}

/* Run when a scale widget is manipulated.  */
static void
scale_moved (GtkAdjustment *adj, gpointer data)
{
        int *value = (int *) data;
        *value = adj->value;
        capplet_widget_state_changed(CAPPLET_WIDGET (capplet), TRUE);
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
keyboard_setup (void)
{
        GtkWidget *vbox, *frame, *table, *hbox, *label, *entry;

        vbox = gtk_vbox_new (FALSE, GNOME_PAD_SMALL);
        gtk_container_border_width (GTK_CONTAINER (vbox), GNOME_PAD);

        capplet = capplet_widget_new();
        frame = gtk_frame_new (_("Auto-repeat"));
        gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
        gtk_widget_show (frame);

        table = gtk_table_new (3, 2, FALSE);
        gtk_container_border_width (GTK_CONTAINER (table), GNOME_PAD_SMALL);
        gtk_table_set_row_spacings (GTK_TABLE (table), GNOME_PAD_SMALL);
        gtk_table_set_col_spacings (GTK_TABLE (table), GNOME_PAD_SMALL);
        gtk_container_add (GTK_CONTAINER (frame), table);
        gtk_widget_show (table);

        rbutton = gtk_check_button_new_with_label (_("Enable auto-repeat"));
        gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(rbutton), keyboard_repeat);
        gtk_table_attach (GTK_TABLE (table), rbutton,
                          0, 2, 0, 1,
                          GTK_FILL | GTK_SHRINK,
                          GTK_FILL | GTK_SHRINK,
                          0, 0);
        gtk_widget_show (rbutton);

        rate_adjust = gtk_adjustment_new (keyboard_rate, 0, 255, 1, 1, 0);
        rscale = make_scale (_("Repeat rate"), rate_adjust, &keyboard_rate, table, 1);

        del_adjust = gtk_adjustment_new (keyboard_delay, 0, 10000, 1, 1, 0);
        dscale = make_scale (_("Repeat Delay"), del_adjust, &keyboard_delay, table, 2);

        frame = gtk_frame_new (_("Keyboard click"));
        gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
        gtk_widget_show (frame);

        table = gtk_table_new (2, 2, FALSE);
        gtk_container_border_width (GTK_CONTAINER (table), GNOME_PAD_SMALL);
        gtk_table_set_row_spacings (GTK_TABLE (table), GNOME_PAD_SMALL);
        gtk_table_set_col_spacings (GTK_TABLE (table), GNOME_PAD_SMALL);
        gtk_container_add (GTK_CONTAINER (frame), table);
        gtk_widget_show (table);

        cbutton = gtk_check_button_new_with_label (_("Click on keypress"));
        gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(cbutton), click_on_keypress);
        gtk_table_attach (GTK_TABLE (table), cbutton,
                          0, 2, 0, 1,
                          GTK_FILL | GTK_SHRINK,
                          GTK_FILL | GTK_SHRINK,
                          0, 0);
        gtk_widget_show (cbutton);

        vol_adjust = gtk_adjustment_new (click_volume, 0, 100, 1, 1, 0);
        vscale = make_scale (_("Click volume"), vol_adjust, &click_volume, table, 1);

        hbox = gtk_hbox_new(FALSE, GNOME_PAD_SMALL);
        gtk_container_border_width (GTK_CONTAINER (vbox), GNOME_PAD);
        gtk_box_pack_start(GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
        gtk_widget_show(hbox);

        label = gtk_label_new(_("Test settings"));
        gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
        gtk_box_pack_start(GTK_BOX (hbox), label, FALSE, FALSE, 0);
        gtk_widget_show(label);

        entry = gtk_entry_new();
        gtk_box_pack_start(GTK_BOX (hbox), entry, TRUE, TRUE, 0);
        gtk_widget_show(entry);

        gtk_signal_connect (GTK_OBJECT (capplet), "try",
                            GTK_SIGNAL_FUNC (keyboard_apply), NULL);
        gtk_signal_connect (GTK_OBJECT (capplet), "revert",
                            GTK_SIGNAL_FUNC (keyboard_revert), NULL);
        gtk_signal_connect (GTK_OBJECT (capplet), "ok",
                            GTK_SIGNAL_FUNC (keyboard_write), NULL);
        gtk_signal_connect (GTK_OBJECT (cbutton), "clicked",
                            GTK_SIGNAL_FUNC (button_toggled), NULL);
        gtk_signal_connect (GTK_OBJECT (rbutton), "clicked",
                            GTK_SIGNAL_FUNC (button_toggled), NULL);

        gtk_widget_show (vbox);
        gtk_container_add (GTK_CONTAINER (capplet), vbox);
        gtk_widget_show (capplet);
}



void
main (int argc, char **argv)
{
        gnome_capplet_init ("Keyboard Properties", NULL, argc, argv, 0, NULL);
        
        keyboard_read ();
        keyboard_setup();
        capplet_gtk_main ();
}
