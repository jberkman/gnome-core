#include <stdio.h>
#include <stdarg.h>
#include <gtk/gtk.h>
#include <gdkx.h>
#include <X11/X.h>
#include <config.h>
#ifdef HAVE_LIBINTL
#include <libintl.h>
#endif
#ifdef HAVE_X11_EXTENSIONS_XF86MISC_H
#include <X11/extensions/xf86misc.h>
#endif

#include "gnome.h"
#include "gnome-desktop.h"

static GnomePropertyConfigurator *config;

static gint keyboard_rate;
static gint keyboard_delay;
static gint keyboard_repeat;
static gint click_volume;
static gint click_on_keypress;
static XKeyboardState kbdstate;
static XKeyboardControl kbdcontrol;
static GtkWidget *rscale, *dscale, *cscale;

#ifdef HAVE_X11_EXTENSIONS_XF86MISC_H
static XF86MiscKbdSettings kbdsettings;
#endif



static void keyboard_read(void)
{
	int repeat_default, click_default;

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

static void keyboard_write(void)
{
	gnome_config_set_bool("/Desktop/Keyboard/repeat", keyboard_repeat);
	gnome_config_set_int("/Desktop/Keyboard/delay", keyboard_delay);
	gnome_config_set_int("/Desktop/Keyboard/rate", keyboard_rate);
	gnome_config_set_bool("/Desktop/Keyboard/click", click_on_keypress);
	gnome_config_set_int("/Desktop/Keyboard/clickvolume", click_volume);
}

static void keyboard_apply(void)
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
rbutton_toggled(GtkWidget *widget, gpointer data)
{
	if (keyboard_repeat) {
		keyboard_repeat = 0;
	} else {
		keyboard_repeat = 1;
	}
	gtk_widget_set_sensitive(dscale, keyboard_repeat);
	gtk_widget_set_sensitive(rscale, keyboard_repeat);
	property_changed ();
}

static void
rate_changed(GtkAdjustment *adj, gpointer data)
{
	keyboard_rate = adj->value;
	property_changed ();
}

static void
delay_changed(GtkAdjustment *adj, gpointer *data)
{
	keyboard_delay = adj->value;
	property_changed ();
}

static void
cbutton_toggled(GtkWidget *widget, gpointer data)
{
	if (click_on_keypress) {
		click_on_keypress = 0;
	} else {
		click_on_keypress = 1;
	}
	gtk_widget_set_sensitive(cscale, click_on_keypress);
	property_changed ();
}

static void
cvol_changed(GtkAdjustment *adj, gpointer *data)
{
	click_volume = adj->value;
	property_changed ();
}


static void keyboard_setup(void)
{
	GtkWidget *vbox, *vbox2;
	GtkWidget *frame;
	GtkWidget *hbox;
	GtkWidget *rbutton, *cbutton;
	GtkWidget *label;
	GtkObject *radj, *dadj, *cadj;

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_border_width(GTK_CONTAINER(vbox), GNOME_PAD);

	frame = gtk_frame_new(_("Speed"));

	vbox2 = gtk_vbox_new(TRUE, 0);
	gtk_container_border_width(GTK_CONTAINER(vbox2), GNOME_PAD);
	
	hbox = gtk_hbox_new(TRUE, GNOME_PAD);
	rbutton = gtk_check_button_new_with_label(_("Auto repeat"));

	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(rbutton), keyboard_repeat);

	gtk_signal_connect(GTK_OBJECT(rbutton), "toggled",
			   GTK_SIGNAL_FUNC(rbutton_toggled), NULL);
	gtk_widget_show(rbutton);
	gtk_box_pack_start(GTK_BOX(hbox), rbutton, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox2), hbox, FALSE, FALSE, 0);
	gtk_widget_show(hbox);

	hbox = gtk_hbox_new(FALSE, GNOME_PAD);
	label = gtk_label_new(_("Repeat rate"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_widget_show(label);
	radj = gtk_adjustment_new(keyboard_rate, 0, 255, 1, 1, 0);
	rscale = gtk_hscale_new(GTK_ADJUSTMENT(radj));
	gtk_signal_connect(GTK_OBJECT(radj), "value_changed",
			   GTK_SIGNAL_FUNC(rate_changed), NULL);
	gtk_widget_set_usize(rscale, 200, -1);
	gtk_box_pack_start(GTK_BOX(hbox), rscale, FALSE, FALSE, 0);
	gtk_widget_show(rscale);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(vbox2), hbox, FALSE, FALSE, 0);

	hbox = gtk_hbox_new(FALSE, GNOME_PAD);
	label = gtk_label_new(_("Repeat delay"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_widget_show(label);
	dadj = gtk_adjustment_new(keyboard_delay, 0, 10000, 1, 1, 0);
	dscale = gtk_hscale_new(GTK_ADJUSTMENT(dadj));
	gtk_signal_connect(GTK_OBJECT(dadj), "value_changed",
			   GTK_SIGNAL_FUNC(delay_changed), NULL);
	gtk_widget_set_usize(dscale, 200, -1);
	gtk_box_pack_start(GTK_BOX(hbox), dscale, FALSE, FALSE, 0);
	gtk_widget_show(dscale);

	gtk_widget_set_sensitive(dscale, keyboard_repeat);
	gtk_widget_set_sensitive(rscale, keyboard_repeat);

	gtk_box_pack_start(GTK_BOX(vbox2), hbox, FALSE, FALSE, 0);
	gtk_widget_show(hbox);

	gtk_container_add(GTK_CONTAINER(frame), vbox2);
	gtk_widget_show(vbox2);
	
	gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, FALSE, GNOME_PAD);
	gtk_widget_show(frame);


	
	frame = gtk_frame_new(_("Click"));

	vbox2 = gtk_vbox_new(TRUE, 0);
	gtk_container_border_width(GTK_CONTAINER(vbox2), GNOME_PAD);

	hbox = gtk_hbox_new(TRUE, GNOME_PAD);
	cbutton = gtk_check_button_new_with_label(_("Click on keypress"));
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(cbutton), click_on_keypress);
	gtk_signal_connect(GTK_OBJECT(cbutton), "toggled",
			   GTK_SIGNAL_FUNC(cbutton_toggled), NULL);
	gtk_widget_show(cbutton);
	gtk_box_pack_start(GTK_BOX(hbox), cbutton, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox2), hbox, FALSE, FALSE, 0);
	gtk_widget_show(hbox);

	hbox = gtk_hbox_new(FALSE, GNOME_PAD);
	label = gtk_label_new(_("Click volume"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_widget_show(label);
	cadj = gtk_adjustment_new(click_volume, 0, 100, 1, 1, 0);
	cscale = gtk_hscale_new(GTK_ADJUSTMENT(cadj));
	gtk_signal_connect(GTK_OBJECT(cadj), "value_changed",
			   GTK_SIGNAL_FUNC(cvol_changed), NULL);
	gtk_widget_set_usize(cscale, 200, -1);

	gtk_box_pack_start(GTK_BOX(hbox), cscale, FALSE, FALSE, 0);

	gtk_widget_set_sensitive(cscale, click_on_keypress);

	gtk_widget_show(cscale);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(vbox2), hbox, FALSE, FALSE, 0);

	gtk_container_add(GTK_CONTAINER(frame), vbox2);
	gtk_widget_show(vbox2);

	gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, FALSE, GNOME_PAD);
	gtk_widget_show(frame);



	gtk_widget_show(vbox);
	gtk_notebook_append_page(GTK_NOTEBOOK(config->notebook),
				 vbox,
				 gtk_label_new(_(" Keyboard ")));
}

static gint keyboard_action(GnomePropertyRequest req)
{
	switch (req) {
	case GNOME_PROPERTY_READ:
		keyboard_read();
		break;
	case GNOME_PROPERTY_WRITE:
		keyboard_write();
		break;
	case GNOME_PROPERTY_APPLY:
		keyboard_apply();
		break;
	case GNOME_PROPERTY_SETUP:
		keyboard_setup();
		break;
	default:
		return 0;
	}

	return 1;
}

void keyboard_register(GnomePropertyConfigurator *c)
{
	config = c;
	gnome_property_configurator_register(config, keyboard_action);
}

