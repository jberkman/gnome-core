/*   gnome-panel-properties: crapplet for global panel properties
 *
 *   Copyright (C) 1999 Free Software Foundation
 *   Copyright 2000 Helix Code, Inc.
 *   Authors: George Lebl <jirka@5z.com>
 *            Jacob Berkman <jacob@helixcode.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

#include <config.h>
#include <gnome.h>
#include <libgnorba/gnorba.h>
#include <gdk/gdkx.h>
#include "capplet-widget.h"
#include "global-keys.h"
/*we just use the GlobalConfig and LAST_TILE definitions, some panel-util stuff,
  and the DEFAULT_* defines from session.h !!!*/
#include "panel-types.h"
#include "panel_config_global.h"
#include "panel-util.h"
#include "session.h"

/* for MAIN_MENU_* */
#include "menu.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <libart_lgpl/art_alphagamma.h>
#include <libart_lgpl/art_filterlevel.h>
#include <libart_lgpl/art_pixbuf.h>
#include <libart_lgpl/art_rgb_pixbuf_affine.h>
#include <libart_lgpl/art_affine.h>
#include "nothing.cP"

static GlobalConfig global_config;
static GlobalConfig loaded_config;

/* animation page */
static GtkWidget *enable_animations_cb;
static GtkWidget *simple_movement_cb;
static GtkWidget *anim_frame;
static GtkAdjustment *auto_hide_step_size;
static GtkAdjustment *explicit_hide_step_size;
static GtkAdjustment *drawer_step_size;
static GtkAdjustment *minimize_delay;
static GtkAdjustment *minimized_size;


/* buttons page */
static GtkWidget *tile_enable_cb[LAST_TILE];
static GtkWidget *tile_file_box[LAST_TILE];
static GtkWidget *tile_border_box[LAST_TILE];
static GtkAdjustment *tile_border[LAST_TILE];
static GtkAdjustment *tile_depth[LAST_TILE];

static GtkWidget *entry_up[LAST_TILE];
static GtkWidget *entry_down[LAST_TILE];

static GtkWidget *hide_panel_frame_cb;
static GtkWidget *tile_when_over_cb;
static GtkWidget *saturate_when_over_cb;


/* applet page*/
static GtkWidget *movement_type_switch_rb;
static GtkWidget *movement_type_free_rb;
static GtkWidget *movement_type_push_rb;
static GtkAdjustment *applet_padding;


/* menu page */

static GtkWidget *show_dot_buttons_cb;
static GtkWidget *off_panel_popups_cb;
static GtkWidget *hungry_menus_cb;
static GtkWidget *use_large_icons_cb;

typedef struct {
	int inline_flag;
	int submenu_flag;
	char *label;

	GtkWidget *inline_rb;
	GtkWidget *submenu_rb;
	GtkWidget *none_rb;
} MenuOptions;

/* don't forget to update the table size when changing the num of elements
   in this array */
static MenuOptions menu_options[] = {
	{ MAIN_MENU_SYSTEM,  MAIN_MENU_SYSTEM_SUB,  N_("Programs: ") },
	{ MAIN_MENU_USER,    MAIN_MENU_USER_SUB,    N_("Favorites: ") },
	{ MAIN_MENU_APPLETS, MAIN_MENU_APPLETS_SUB, N_("Applets: ") },
	{ MAIN_MENU_REDHAT,  MAIN_MENU_REDHAT_SUB,  N_("AnotherLevel: ") },
	{ MAIN_MENU_KDE,     MAIN_MENU_KDE_SUB,     N_("KDE: ") },
	{ MAIN_MENU_DEBIAN,  MAIN_MENU_DEBIAN_SUB,  N_("Debian: ") },
	{ MAIN_MENU_PANEL,   MAIN_MENU_PANEL_SUB,   N_("Panel: ") },
	{ MAIN_MENU_DESKTOP, MAIN_MENU_DESKTOP_SUB, N_("Desktop: ") },
	{ 0 }
};

/* miscellaneous page */
static GtkWidget *tooltips_enabled_cb;
static GtkWidget *drawer_auto_close_cb;
static GtkWidget *autoraise_cb;
static GtkWidget *keep_bottom_cb;
static GtkWidget *keys_enabled_cb;
static GtkWidget *menu_key_entry;
static GtkWidget *run_key_entry;
static GtkWidget *confirm_panel_remove_cb;

static gboolean changing = TRUE;
static GtkWidget *capplet;

static void 
changed_cb(void)
{
	if(!changing)
		capplet_widget_state_changed(CAPPLET_WIDGET(capplet), TRUE);
}

static GtkWidget *
make_int_scale_box (char *title, GtkAdjustment **adj,
		    double min, double max, double step)
{
	GtkWidget *label;
	GtkWidget *box;
	GtkWidget *scale;

	/* vbox for frame */
	box = gtk_hbox_new (FALSE, GNOME_PAD_SMALL);
	gtk_widget_show (box);

	/* scale label */
	label = gtk_label_new(title);
	gtk_misc_set_alignment (GTK_MISC (label), 1.0, 1.0);
	gtk_box_pack_start (GTK_BOX (box), label, FALSE, TRUE, 0);
	gtk_widget_show (label);

	/* Animation step_size scale */
	*adj = GTK_ADJUSTMENT(gtk_adjustment_new(min, min, max, step, step, 0.0));
	scale = gtk_hscale_new (*adj);
	gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
	gtk_scale_set_digits (GTK_SCALE (scale), 0);
	gtk_scale_set_draw_value (GTK_SCALE (scale), TRUE);
	gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
	gtk_box_pack_start (GTK_BOX (box), scale, TRUE, TRUE, 0);
	gtk_signal_connect(GTK_OBJECT (*adj), 
			   "value_changed",
			   GTK_SIGNAL_FUNC (changed_cb),NULL);
	gtk_widget_show (scale);

	return box;
}

static int
set_anim_button_value(GtkWidget *w, gpointer data)
{
	int enable = GTK_TOGGLE_BUTTON(w)->active;

	gtk_widget_set_sensitive(simple_movement_cb,enable);
	gtk_widget_set_sensitive(anim_frame,enable);

	if(!changing)
		capplet_widget_state_changed(CAPPLET_WIDGET(capplet), TRUE);
	return FALSE;
}

static void
sync_animation_page_with_config(GlobalConfig *conf)
{
	gtk_widget_set_sensitive(simple_movement_cb,
				 !conf->disable_animations);
	gtk_widget_set_sensitive(anim_frame,
				 !conf->disable_animations);

	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(enable_animations_cb),
				    /*notice the not*/
				    !conf->disable_animations);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(simple_movement_cb),
				    conf->simple_movement);
	gtk_adjustment_set_value(auto_hide_step_size,
				 conf->auto_hide_step_size);
	gtk_adjustment_set_value(explicit_hide_step_size,
				 conf->explicit_hide_step_size);
	gtk_adjustment_set_value(drawer_step_size,
				 conf->drawer_step_size);
	gtk_adjustment_set_value(minimize_delay,
				 conf->minimize_delay);
	gtk_adjustment_set_value(minimized_size,
				 conf->minimized_size);
}
static void
sync_config_with_animation_page(GlobalConfig *conf)
{
	/*notice the not*/
	conf->disable_animations =
		! GTK_TOGGLE_BUTTON(enable_animations_cb)->active;
	conf->simple_movement =
		GTK_TOGGLE_BUTTON(simple_movement_cb)->active;
	conf->auto_hide_step_size = auto_hide_step_size->value;
	conf->explicit_hide_step_size = explicit_hide_step_size->value;
	conf->drawer_step_size = drawer_step_size->value;
	conf->minimize_delay = minimize_delay->value;
	conf->minimized_size = minimized_size->value;
}

static GtkWidget *
animation_notebook_page(void)
{
	GtkWidget *frame;
	GtkWidget *box;
	GtkWidget *vbox;
	GtkWidget *frame_vbox;

	/* main vbox */
	vbox = gtk_vbox_new (FALSE, GNOME_PAD_SMALL);
	gtk_container_set_border_width(GTK_CONTAINER (vbox), GNOME_PAD_SMALL);

	/* Animation enable */
	enable_animations_cb = gtk_check_button_new_with_label (_("Enable animations"));
	gtk_signal_connect (GTK_OBJECT (enable_animations_cb), "toggled", 
			    GTK_SIGNAL_FUNC (set_anim_button_value),NULL); 
	gtk_box_pack_start (GTK_BOX (vbox), enable_animations_cb, FALSE, FALSE, 0);

	simple_movement_cb = gtk_check_button_new_with_label (_("Constant speed animations"));
	gtk_box_pack_start (GTK_BOX (vbox), simple_movement_cb, FALSE, FALSE, 0);
	gtk_signal_connect (GTK_OBJECT (simple_movement_cb), "toggled", 
			    GTK_SIGNAL_FUNC (changed_cb), NULL);

	/* AutoHide Animation step_size scale frame */
	anim_frame = gtk_frame_new (_("Animation speed"));
	gtk_container_set_border_width(GTK_CONTAINER (anim_frame), GNOME_PAD_SMALL);
	gtk_box_pack_start (GTK_BOX (vbox), anim_frame, TRUE, TRUE, 0);
	gtk_widget_show (anim_frame);

	frame_vbox = gtk_vbox_new (FALSE, GNOME_PAD_SMALL);
	gtk_container_set_border_width (GTK_CONTAINER (frame_vbox), GNOME_PAD_SMALL);
	gtk_container_add (GTK_CONTAINER (anim_frame), frame_vbox);
	gtk_widget_show (frame_vbox);

	box = make_int_scale_box(_("Auto hide"),
				 &auto_hide_step_size,
				 1.0, 100.0, 1.0);
	gtk_box_pack_start (GTK_BOX (frame_vbox), box, TRUE, FALSE,0);
	gtk_widget_show (box);

	/* ExplicitHide Animation step_size scale frame */
	box = make_int_scale_box(_("Explicit hide"),
				   &explicit_hide_step_size,
				   1.0, 100.0, 1.0);
	gtk_box_pack_start (GTK_BOX (frame_vbox), box, TRUE, FALSE,0);

	/* DrawerHide Animation step_size scale frame */
	box = make_int_scale_box(_("Drawer sliding"),
				   &drawer_step_size,
				   1.0, 100.0, 1.0);
	gtk_box_pack_start (GTK_BOX (frame_vbox), box, TRUE, FALSE,0);

	frame = gtk_frame_new (_("Auto hide"));
	gtk_container_set_border_width(GTK_CONTAINER (frame), GNOME_PAD_SMALL);
	gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);
	gtk_widget_show (frame);

	frame_vbox = gtk_vbox_new (FALSE, GNOME_PAD_SMALL);
	gtk_container_set_border_width (GTK_CONTAINER (frame_vbox), GNOME_PAD_SMALL);
	gtk_container_add (GTK_CONTAINER (frame), frame_vbox);
	gtk_widget_show (frame_vbox);

	/* Minimize Delay scale frame */
	box = make_int_scale_box (_("Delay (ms)"),
				  &minimize_delay,
				  30.0, 10000.0, 10.0);
	gtk_box_pack_start (GTK_BOX (frame_vbox), box, TRUE, FALSE, 0);

	/* Minimized size scale frame */
	box = make_int_scale_box (_("Size (pixels)"),
				  &minimized_size,
				  1.0, 10.0, 1.0);
	gtk_box_pack_start (GTK_BOX (frame_vbox), box, TRUE, FALSE, 0);

	return (vbox);
}

static void
sync_buttons_page_with_config(GlobalConfig *conf)
{
	int i;
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(hide_panel_frame_cb),
				    conf->hide_panel_frame);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(tile_when_over_cb),
				    conf->tile_when_over);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(saturate_when_over_cb),
				    conf->saturate_when_over);
	
	for(i=0;i<LAST_TILE;i++) {
		gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(tile_enable_cb[i]),
					    conf->tiles_enabled[i]);
		gnome_icon_entry_set_icon(GNOME_ICON_ENTRY(entry_up[i]),
					  conf->tile_up[i]);
		gnome_icon_entry_set_icon(GNOME_ICON_ENTRY(entry_down[i]),
					  conf->tile_down[i]);
		gtk_adjustment_set_value(tile_border[i],conf->tile_border[i]);
		gtk_adjustment_set_value(tile_depth[i],conf->tile_depth[i]);
	}
}
static void
sync_config_with_buttons_page(GlobalConfig *conf)
{
	int i;
	conf->hide_panel_frame =
		GTK_TOGGLE_BUTTON(hide_panel_frame_cb)->active;
	conf->tile_when_over =
		GTK_TOGGLE_BUTTON(tile_when_over_cb)->active;
	conf->saturate_when_over =
		GTK_TOGGLE_BUTTON(saturate_when_over_cb)->active;

	for(i=0;i<LAST_TILE;i++) {
		conf->tiles_enabled[i] =
			GTK_TOGGLE_BUTTON(tile_enable_cb[i])->active;
		g_free(conf->tile_up[i]);
		conf->tile_up[i] =
			gnome_icon_entry_get_filename(GNOME_ICON_ENTRY(entry_up[i]));
		g_free(conf->tile_down[i]);
		conf->tile_down[i] =
			gnome_icon_entry_get_filename(GNOME_ICON_ENTRY(entry_down[i]));
		conf->tile_border[i] = tile_border[i]->value;
		conf->tile_depth[i] = tile_depth[i]->value;
	}
}

static void
set_icon_button(GtkWidget *w, gpointer data)
{
	int i = GPOINTER_TO_INT(data);
	int active = GTK_TOGGLE_BUTTON(w)->active;

	gtk_widget_set_sensitive(tile_file_box[i],active);
	gtk_widget_set_sensitive(tile_border_box[i],active);

	changed_cb();
}

enum {
	LAUNCHER_PAGE,
	DRAWER_PAGE,
	MENU_PAGE,
	SPECIAL_PAGE,
	N_PAGES
};

static void
show_page (GtkWidget *w, gpointer data)
{
	GtkWidget *notebook = gtk_object_get_user_data (GTK_OBJECT (w));
	gtk_notebook_set_page (GTK_NOTEBOOK (notebook), GPOINTER_TO_INT (data));
}

static GtkWidget *
icon_notebook_page(int i)
{
	GtkWidget *frame;
	GtkWidget *w;
	GtkWidget *table;
	GtkWidget *vbox;

        char *icon_titles[]={
                N_("Launcher icon"),
                N_("Drawer icon"),
                N_("Menu icon"),
                N_("Special icon")};

	/* Image frame */
	frame = gtk_frame_new (_(icon_titles[i]));
	gtk_container_set_border_width(GTK_CONTAINER (frame), GNOME_PAD_SMALL);
	
	/* main vbox */
	vbox = gtk_vbox_new (FALSE, GNOME_PAD_SMALL);
	gtk_container_set_border_width(GTK_CONTAINER (vbox), GNOME_PAD_SMALL);
	gtk_container_add (GTK_CONTAINER (frame), vbox);

	/* toggle button */
 	tile_enable_cb[i] = gtk_check_button_new_with_label (_("Tiles enabled"));
	gtk_signal_connect (GTK_OBJECT (tile_enable_cb[i]), "toggled", 
			    GTK_SIGNAL_FUNC (set_icon_button), 
			    GINT_TO_POINTER(i));
	gtk_box_pack_start (GTK_BOX (vbox), tile_enable_cb[i], FALSE, FALSE, 0);
	
	
	/* table for frame */
	tile_file_box[i] = table = gtk_table_new(2,3,FALSE);
	gtk_container_set_border_width(GTK_CONTAINER (table), GNOME_PAD_SMALL);
	gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
	
	/* image file entry widgets */
	entry_up[i] = create_icon_entry(table,"tile_file",0, 1,
					_("Normal Tile"),
					global_config.tile_up[i],
					NULL);
	w = gnome_icon_entry_gtk_entry (GNOME_ICON_ENTRY (entry_up[i]));
	gtk_signal_connect_while_alive(GTK_OBJECT(w), "changed",
				       GTK_SIGNAL_FUNC(changed_cb), 
				       NULL,
				       GTK_OBJECT(capplet));

	entry_down[i] = create_icon_entry(table,"tile_file",1, 2,
					  _("Clicked Tile"),
					  global_config.tile_down[i],
					  NULL);
	w = gnome_icon_entry_gtk_entry (GNOME_ICON_ENTRY (entry_down[i]));
	gtk_signal_connect_while_alive(GTK_OBJECT(w), "changed",
				       GTK_SIGNAL_FUNC(changed_cb), 
				       NULL,
				       GTK_OBJECT(capplet));

	w = gtk_hseparator_new ();
	gtk_box_pack_start (GTK_BOX (vbox), w, FALSE, FALSE, 0);
	gtk_widget_show (w);

	/* Minimized size scale frame */
	tile_border_box[i] = w = make_int_scale_box (_("Border width (tile only)"),
						&tile_border[i],
						0.0, 10.0, 1.0);
	gtk_box_pack_start (GTK_BOX (vbox), w, FALSE, FALSE, 0);

	/* Minimized size scale frame */
	w = make_int_scale_box (_("Depth (displacement when pressed)"),
				&tile_depth[i],
				0.0, 10.0, 1.0);
	gtk_box_pack_start (GTK_BOX (vbox), w, FALSE, FALSE, 0);

	return frame;
}

static GtkWidget *
buttons_notebook_page (void)
{
	  GtkWidget *m, *w;
	  GtkWidget *box;
	  GtkWidget *vbox;
	  GtkWidget *label;
	  GtkWidget *notebook;
	  GtkWidget *page;
	  int i;

	  const char *labels[] = { N_("Launcher"),
				  N_("Drawer"),
				  N_("Menu"),
				  N_("Special") };

	  /* main vbox */
	  vbox = gtk_vbox_new (FALSE, GNOME_PAD_SMALL);
	  gtk_container_set_border_width(GTK_CONTAINER (vbox), GNOME_PAD_SMALL);

	  box = gtk_hbox_new (FALSE, GNOME_PAD_SMALL);
	  gtk_box_pack_start (GTK_BOX (vbox), box, FALSE, FALSE, 0);
	  gtk_widget_show (box);

	  label = gtk_label_new (_("Button Type: "));
	  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
	  gtk_widget_show (label);

	  m = gtk_menu_new ();
	  notebook = gtk_notebook_new ();
	  gtk_notebook_set_show_tabs (GTK_NOTEBOOK (notebook), FALSE);
	  gtk_notebook_set_show_border (GTK_NOTEBOOK (notebook), FALSE);

	  for (i = 0; i < N_PAGES; i++) {	  
		  w = gtk_menu_item_new_with_label (_(labels[i]));
		  gtk_signal_connect (GTK_OBJECT (w), "activate",
				      GTK_SIGNAL_FUNC (show_page),
				      GINT_TO_POINTER (i));
		  gtk_widget_show (w);
		  gtk_menu_append (GTK_MENU (m), w);

		  page = icon_notebook_page (i);
		  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page, NULL);

		  gtk_object_set_user_data (GTK_OBJECT (w), notebook);
	  }

	  w = gtk_option_menu_new ();
	  gtk_option_menu_set_menu (GTK_OPTION_MENU (w), m);
	  gtk_box_pack_start (GTK_BOX (box), w, FALSE, FALSE, 0);
	  gtk_widget_show (w);

	  gtk_box_pack_start (GTK_BOX (vbox), notebook, FALSE, FALSE, 0);
	  gtk_widget_show (notebook);
	  
	  
	  /* show/hide frame */
	  hide_panel_frame_cb = gtk_check_button_new_with_label (_("Make buttons flush with panel edge"));
	  gtk_signal_connect (GTK_OBJECT (hide_panel_frame_cb), "toggled",
			      GTK_SIGNAL_FUNC (changed_cb), NULL);
	  gtk_box_pack_start (GTK_BOX (vbox), hide_panel_frame_cb, FALSE, FALSE, 0);
	  

	  /* only show tiles when mouse is over the button */
	  tile_when_over_cb = gtk_check_button_new_with_label (_("Show button tiles only when cursor is over the button"));
	  gtk_signal_connect (GTK_OBJECT (tile_when_over_cb), "toggled",
			      GTK_SIGNAL_FUNC (changed_cb), NULL);
	  gtk_box_pack_start (GTK_BOX (vbox), tile_when_over_cb, FALSE, FALSE, 0);

	  /* saturate on mouseovers hack */
	  saturate_when_over_cb = gtk_check_button_new_with_label (_("Keep saturation low when cursor is not on the button"));
	  gtk_signal_connect (GTK_OBJECT (saturate_when_over_cb), "toggled",
			      GTK_SIGNAL_FUNC (changed_cb), NULL);
	  gtk_box_pack_start (GTK_BOX (vbox), saturate_when_over_cb, FALSE, FALSE, 0);
	  
	  return (vbox);
}


static void
sync_applets_page_with_config(GlobalConfig *conf)
{
	switch(conf->movement_type) {
	case PANEL_SWITCH_MOVE:
		gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(movement_type_switch_rb),
					    TRUE);
		break;
	case PANEL_FREE_MOVE:
		gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(movement_type_free_rb),
					    TRUE);
		break;
	case PANEL_PUSH_MOVE:
		gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(movement_type_push_rb),
					    TRUE);
		break;
	default: break;
	}
	gtk_adjustment_set_value(applet_padding,conf->applet_padding);
}
static void
sync_config_with_applets_page(GlobalConfig *conf)
{
	if(GTK_TOGGLE_BUTTON(movement_type_switch_rb)->active)
		conf->movement_type = PANEL_SWITCH_MOVE;
	else if(GTK_TOGGLE_BUTTON(movement_type_free_rb)->active)
		conf->movement_type = PANEL_FREE_MOVE;
	else if(GTK_TOGGLE_BUTTON(movement_type_push_rb)->active)
		conf->movement_type = PANEL_PUSH_MOVE;
	
	conf->applet_padding = applet_padding->value;
}


static GtkWidget *
applets_notebook_page (void)
{
	GtkWidget *frame;
	GtkWidget *box;
	GtkWidget *vbox;

	/* main vbox */
	vbox = gtk_vbox_new (FALSE, GNOME_PAD_SMALL);
	gtk_container_set_border_width(GTK_CONTAINER (vbox), GNOME_PAD_SMALL);

	/* Movement frame */
	frame = gtk_frame_new (_("Default movement mode"));
	gtk_container_set_border_width(GTK_CONTAINER (frame), GNOME_PAD_SMALL);
	gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
	
	/* vbox for frame */
	box = gtk_vbox_new (FALSE, GNOME_PAD_SMALL);
	gtk_container_set_border_width(GTK_CONTAINER (box), GNOME_PAD_SMALL);
	gtk_container_add (GTK_CONTAINER (frame), box);
	
	/* Switched */
	movement_type_switch_rb = gtk_radio_button_new_with_label (NULL, _("Switched movement (or use Ctrl)"));
	gtk_signal_connect (GTK_OBJECT (movement_type_switch_rb), "toggled", 
			    GTK_SIGNAL_FUNC (changed_cb), NULL);
	gtk_box_pack_start (GTK_BOX (box), movement_type_switch_rb, FALSE, FALSE, 0);

	/* Free */
	movement_type_free_rb = gtk_radio_button_new_with_label (
		gtk_radio_button_group (GTK_RADIO_BUTTON (movement_type_switch_rb)),
		_("Free movement (doesn't disturb other applets) (or use Alt)"));
	gtk_signal_connect (GTK_OBJECT (movement_type_free_rb), "toggled", 
			    GTK_SIGNAL_FUNC (changed_cb), NULL);
	gtk_box_pack_start (GTK_BOX (box), movement_type_free_rb, FALSE, FALSE, 0);

	/* Push */
	movement_type_push_rb = gtk_radio_button_new_with_label (
		gtk_radio_button_group (GTK_RADIO_BUTTON (movement_type_switch_rb)),
		_("Push movement (or use Shift)"));
	gtk_signal_connect (GTK_OBJECT (movement_type_push_rb), "toggled", 
			    GTK_SIGNAL_FUNC (changed_cb), NULL);
	gtk_box_pack_start (GTK_BOX (box), movement_type_push_rb, FALSE, FALSE, 0);	

	box = make_int_scale_box(_("Applet padding"),
				 &applet_padding,
				 0.0, 10.0, 1.0);
	gtk_box_pack_start (GTK_BOX (vbox), box, FALSE, FALSE, 0);
	

	return vbox;
}

static void
sync_menu_page_with_config(GlobalConfig *conf)
{
	MenuOptions *opt;
	GtkWidget *w;
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(show_dot_buttons_cb),
				    conf->show_dot_buttons);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(off_panel_popups_cb),
				    conf->off_panel_popups);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(hungry_menus_cb),
				    conf->hungry_menus);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(use_large_icons_cb),
				    conf->use_large_icons);

	for (opt = menu_options; opt->inline_flag; ++opt) {
		if (conf->menu_flags & opt->inline_flag)
			w = opt->inline_rb;
		else if (conf->menu_flags & opt->submenu_flag)
			w = opt->submenu_rb;
		else
			w = opt->none_rb;
		gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (w), TRUE);
	}
}

static void
sync_config_with_menu_page(GlobalConfig *conf)
{
	MenuOptions *opt;
	conf->show_dot_buttons =
		GTK_TOGGLE_BUTTON(show_dot_buttons_cb)->active;
	conf->off_panel_popups =
		GTK_TOGGLE_BUTTON(off_panel_popups_cb)->active;
	conf->hungry_menus =
		GTK_TOGGLE_BUTTON(hungry_menus_cb)->active;
	conf->use_large_icons =
		GTK_TOGGLE_BUTTON(use_large_icons_cb)->active;
	conf->menu_flags = 0;
	for (opt = menu_options; opt->inline_flag; ++opt) {
		if (GTK_TOGGLE_BUTTON (opt->inline_rb)->active)
			conf->menu_flags |= opt->inline_flag;
		else if (GTK_TOGGLE_BUTTON (opt->submenu_rb)->active)
			conf->menu_flags |= opt->submenu_flag;
	}
}

static void
add_menu_options (GtkTable *table, MenuOptions *opt, int row)
{
	GtkWidget *w;
	GtkRadioButton *rb;

	w = gtk_label_new (_(opt->label));
	gtk_table_attach_defaults (table, w, 0, 1, row, row+1);

	w = opt->none_rb = gtk_radio_button_new_with_label (NULL, _("Off"));
	gtk_table_attach_defaults (table, w, 3, 4, row, row+1);
	gtk_signal_connect (GTK_OBJECT (w), "toggled", 
			    GTK_SIGNAL_FUNC (changed_cb),  NULL);
	rb = GTK_RADIO_BUTTON (w);

	w = opt->submenu_rb = gtk_radio_button_new_with_label (
		gtk_radio_button_group (rb), _("In a submenu"));
	gtk_table_attach_defaults (table, w, 2, 3, row, row+1);
	gtk_signal_connect (GTK_OBJECT (w), "toggled", 
			    GTK_SIGNAL_FUNC (changed_cb),  NULL);

	w = opt->inline_rb = gtk_radio_button_new_with_label (
		gtk_radio_button_group (rb), _("In the menu"));
	gtk_table_attach_defaults (table, w, 1, 2, row, row+1);
	gtk_signal_connect (GTK_OBJECT (w), "toggled", 
			    GTK_SIGNAL_FUNC (changed_cb),  NULL);
}

static GtkWidget *
menu_notebook_page(void)
{
	GtkWidget *frame;
	GtkWidget *table;
	GtkWidget *vbox;
	int i;
	
	/* main vbox */
	vbox = gtk_vbox_new (FALSE, GNOME_PAD_SMALL);
	gtk_container_set_border_width(GTK_CONTAINER (vbox), GNOME_PAD_SMALL);
	
	/* Menu frame */
	frame = gtk_frame_new (_("Menus"));
	gtk_container_set_border_width(GTK_CONTAINER (frame), GNOME_PAD_SMALL);
	gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
	
	/* table for frame */
	table = gtk_table_new(2,2,FALSE);
	gtk_container_set_border_width(GTK_CONTAINER (table), GNOME_PAD_SMALL);
	gtk_container_add (GTK_CONTAINER (frame), table);
	
	/* Dot Buttons */
	show_dot_buttons_cb = gtk_check_button_new_with_label (_("Show [...] buttons"));
	gtk_signal_connect (GTK_OBJECT (show_dot_buttons_cb), "toggled", 
			    GTK_SIGNAL_FUNC (changed_cb),  NULL);
	gtk_table_attach_defaults(GTK_TABLE(table),show_dot_buttons_cb, 1,2,0,1);

	/* Off Panel Popup menus */
	off_panel_popups_cb = gtk_check_button_new_with_label (_("Show popup menus outside of panels"));
	gtk_signal_connect (GTK_OBJECT (off_panel_popups_cb), "toggled", 
			    GTK_SIGNAL_FUNC (changed_cb),  NULL);
	gtk_table_attach_defaults(GTK_TABLE(table),off_panel_popups_cb, 0,1,1,2);

	/* Hungry Menus */
	hungry_menus_cb = gtk_check_button_new_with_label (_("Keep menus in memory"));
	gtk_signal_connect (GTK_OBJECT (hungry_menus_cb), "toggled", 
			    GTK_SIGNAL_FUNC (changed_cb), NULL);
	gtk_table_attach_defaults(GTK_TABLE(table),hungry_menus_cb, 1,2,1,2);

	/* large icons */
	use_large_icons_cb = gtk_check_button_new_with_label (_("Use large icons"));
	gtk_signal_connect (GTK_OBJECT (use_large_icons_cb), "toggled",
			    GTK_SIGNAL_FUNC (changed_cb), NULL);
	gtk_table_attach_defaults(GTK_TABLE(table),use_large_icons_cb, 0,1,0,1);

	/* Menu frame */
	frame = gtk_frame_new (_("Panel menu"));
	gtk_container_set_border_width(GTK_CONTAINER (frame), GNOME_PAD_SMALL);
	gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
	
	/* table for frame */
	table = gtk_table_new(7,4,FALSE);
	gtk_container_set_border_width(GTK_CONTAINER (table), GNOME_PAD_SMALL);
	gtk_container_add (GTK_CONTAINER (frame), table);
	
	for (i=0; menu_options[i].inline_flag; ++i)
		add_menu_options (GTK_TABLE (table), menu_options+i, i);
	
	return vbox;
}

static void
sync_misc_page_with_config(GlobalConfig *conf)
{
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(tooltips_enabled_cb),
				    conf->tooltips_enabled);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(drawer_auto_close_cb),
				    conf->drawer_auto_close);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(autoraise_cb),
				    conf->autoraise);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(keep_bottom_cb),
				    conf->keep_bottom);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(confirm_panel_remove_cb),
				    conf->confirm_panel_remove);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(keys_enabled_cb),
				    conf->keys_enabled);
	gtk_entry_set_text (GTK_ENTRY(menu_key_entry),
			    conf->menu_key?conf->menu_key:"");
	gtk_entry_set_text (GTK_ENTRY(run_key_entry),
			    conf->run_key?conf->run_key:"");
}
static void
sync_config_with_misc_page(GlobalConfig *conf)
{
	conf->tooltips_enabled =
		GTK_TOGGLE_BUTTON(tooltips_enabled_cb)->active;
	conf->drawer_auto_close =
		GTK_TOGGLE_BUTTON(drawer_auto_close_cb)->active;
	conf->autoraise =
		GTK_TOGGLE_BUTTON(autoraise_cb)->active;
	conf->keep_bottom =
		GTK_TOGGLE_BUTTON(keep_bottom_cb)->active;
	conf->confirm_panel_remove =
		GTK_TOGGLE_BUTTON(confirm_panel_remove_cb)->active;
	conf->keys_enabled =
		GTK_TOGGLE_BUTTON(keys_enabled_cb)->active;
	g_free(conf->menu_key);
	conf->menu_key =
		g_strdup(gtk_entry_get_text (GTK_ENTRY(menu_key_entry)));
	g_free(conf->run_key);
	conf->run_key =
		g_strdup(gtk_entry_get_text (GTK_ENTRY(run_key_entry)));
}

static GtkWidget *grab_dialog;

static GdkFilterReturn
grab_key_filter(GdkXEvent *gdk_xevent, GdkEvent *event, gpointer data)
{
	XEvent *xevent = (XEvent *)gdk_xevent;
	GtkEntry *entry;
	char *key;

	if (xevent->type != KeyPress && xevent->type != KeyRelease)
		puts("EXIT X");
	if (event->type != GDK_KEY_PRESS && event->type != GDK_KEY_RELEASE)
		puts("EXIT GDK");

	if (xevent->type != KeyPress && xevent->type != KeyRelease)
	/*if (event->type != GDK_KEY_PRESS && event->type != GDK_KEY_RELEASE)*/
		return GDK_FILTER_CONTINUE;
	
	entry = GTK_ENTRY (data);

	/* note: GDK has already translated the keycode to a keysym for us */
	g_message ("keycode: %d\tstate: %d",
		   event->key.keyval, event->key.state);

	key = convert_keysym_state_to_string (event->key.keyval,
					      event->key.state);
	gtk_entry_set_text (entry, key?key:"");
	g_free(key);

	gdk_keyboard_ungrab (GDK_CURRENT_TIME);
	gtk_widget_destroy (grab_dialog);
	gdk_window_remove_filter (GDK_ROOT_PARENT(),
				  grab_key_filter, data);

	return GDK_FILTER_REMOVE;
}

static void
grab_button_pressed (GtkButton *button, gpointer data)
{
	GtkWidget *frame;
	GtkWidget *box;
	GtkWidget *label;
	grab_dialog = gtk_window_new (GTK_WINDOW_POPUP);


	gdk_keyboard_grab (GDK_ROOT_PARENT(), TRUE, GDK_CURRENT_TIME);
	gdk_window_add_filter (GDK_ROOT_PARENT(), grab_key_filter, data);

	gtk_window_set_policy (GTK_WINDOW (grab_dialog), FALSE, FALSE, TRUE);
	gtk_window_set_position (GTK_WINDOW (grab_dialog), GTK_WIN_POS_CENTER);
	gtk_window_set_modal (GTK_WINDOW (grab_dialog), TRUE);

	frame = gtk_frame_new (NULL);
	gtk_container_add (GTK_CONTAINER (grab_dialog), frame);

	box = gtk_hbox_new (0, 0);
	gtk_container_set_border_width (GTK_CONTAINER (box), 20);
	gtk_container_add (GTK_CONTAINER (frame), box);

	label = gtk_label_new (_("Press a key..."));
	gtk_container_add (GTK_CONTAINER (box), label);
	
	gtk_widget_show_all (grab_dialog);
	return;
}

static GtkWidget *
misc_notebook_page(void)
{
	GtkWidget *frame;
	GtkWidget *box;
	GtkWidget *table;
	GtkWidget *vbox;
	GtkWidget *w;
	GList *list;
	
	/* main vbox */
	vbox = gtk_vbox_new (FALSE, GNOME_PAD_SMALL);
	gtk_container_set_border_width(GTK_CONTAINER (vbox), GNOME_PAD_SMALL);
	
	/* Miscellaneous frame */
	frame = gtk_frame_new (_("Miscellaneous"));
	gtk_container_set_border_width(GTK_CONTAINER (frame), GNOME_PAD_SMALL);
	gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
	
	/* vbox for frame */
	box = gtk_vbox_new (FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER (box), GNOME_PAD_SMALL);
	gtk_container_add (GTK_CONTAINER (frame), box);

	/* Tooltips enable */
	tooltips_enabled_cb = gtk_check_button_new_with_label (_("Tooltips enabled"));
	gtk_signal_connect (GTK_OBJECT (tooltips_enabled_cb), "toggled", 
			    GTK_SIGNAL_FUNC (changed_cb), NULL);
	gtk_box_pack_start (GTK_BOX (box), tooltips_enabled_cb, FALSE, FALSE, 0);	
	
	/* Drawer/launcher auto close */
	drawer_auto_close_cb = gtk_check_button_new_with_label (_("Close drawer if a launcher inside it is pressed"));
	gtk_signal_connect (GTK_OBJECT (drawer_auto_close_cb), "toggled", 
			    GTK_SIGNAL_FUNC (changed_cb),  NULL);
	gtk_box_pack_start (GTK_BOX (box), drawer_auto_close_cb, FALSE, FALSE, 0);

	/* Autoraise */
	autoraise_cb = gtk_check_button_new_with_label (_("Raise panels on mouse-over"));
	gtk_signal_connect (GTK_OBJECT (autoraise_cb), "toggled", 
			    GTK_SIGNAL_FUNC (changed_cb), NULL);
	gtk_box_pack_start (GTK_BOX (box), autoraise_cb, FALSE, FALSE, 0);

	/* Keep on bottom */
	keep_bottom_cb = gtk_check_button_new_with_label (_("Keep panel below windows (GNOME compliant window managers only)"));
	gtk_signal_connect (GTK_OBJECT (keep_bottom_cb), "toggled", 
			    GTK_SIGNAL_FUNC (changed_cb), NULL);
	gtk_box_pack_start (GTK_BOX (box), keep_bottom_cb, FALSE, FALSE, 0);

	/* Confirm panel removal */
	confirm_panel_remove_cb = gtk_check_button_new_with_label (_("Confirm the removal of panels with a dialog"));
	gtk_signal_connect (GTK_OBJECT (confirm_panel_remove_cb), "toggled", 
			    GTK_SIGNAL_FUNC (changed_cb), NULL);
	gtk_box_pack_start (GTK_BOX (box), confirm_panel_remove_cb, FALSE, FALSE, 0);

	/* Key Bindings frame */
	frame = gtk_frame_new (_("Key Bindings"));
	gtk_container_set_border_width(GTK_CONTAINER (frame), GNOME_PAD_SMALL);
	gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);

	
	/* table for frame */
	table = gtk_table_new (3, 3, FALSE);
	gtk_table_set_row_spacings (GTK_TABLE (table), GNOME_PAD_SMALL);
	gtk_table_set_col_spacings (GTK_TABLE (table), GNOME_PAD_SMALL);
	gtk_container_set_border_width(GTK_CONTAINER (table), GNOME_PAD_SMALL);
	gtk_container_add (GTK_CONTAINER (frame), table);

	/* enabled */
	keys_enabled_cb = gtk_check_button_new_with_label (_("Enable panel keybindings"));
	gtk_signal_connect (GTK_OBJECT (keys_enabled_cb), "toggled",
			    GTK_SIGNAL_FUNC (changed_cb), NULL);
	gtk_table_attach_defaults (GTK_TABLE (table), keys_enabled_cb, 
				   0, 3, 0, 1);
	
	/* menu key */
	w = gtk_label_new (_("Popup menu key"));
	gtk_table_attach_defaults (GTK_TABLE (table), w, 0, 1, 1, 2);
	

	list = g_list_append(NULL, "Mod1-F1");
	list = g_list_append(list, "Menu");
	list = g_list_append(list, "Hyper_L");
	list = g_list_append(list, "Hyper_R");
	list = g_list_append(list, "Control-Mod1-m");
	list = g_list_append(list, _("Disabled"));
	w = gtk_combo_new();
	gtk_combo_set_popdown_strings(GTK_COMBO(w), list);
	g_list_free(list);
	menu_key_entry = GTK_COMBO(w)->entry;
	gtk_signal_connect (GTK_OBJECT (menu_key_entry),
			    "changed",
			    GTK_SIGNAL_FUNC (changed_cb), NULL);
	/*gtk_widget_set_sensitive (menu_key_entry, FALSE);*/
	gtk_table_attach_defaults (GTK_TABLE (table), w, 1, 2, 1, 2);

	w = gtk_button_new_with_label (_("Grab key..."));
	gtk_table_attach_defaults (GTK_TABLE (table), w, 2, 3, 1, 2);
	gtk_signal_connect (GTK_OBJECT (w), "clicked",
			    GTK_SIGNAL_FUNC (grab_button_pressed),
			    menu_key_entry);

	/* run key...*/
	w = gtk_label_new (_("Run dialog key"));
	gtk_table_attach_defaults (GTK_TABLE (table), w, 0, 1, 2, 3);
	
	list = g_list_append(NULL, "Mod1-F2");
	list = g_list_append(list, "Control-Mod1-r");
	list = g_list_append(list, "Hyper_R");
	list = g_list_append(list, "Menu");
	list = g_list_append(list, _("Disabled"));
	w = gtk_combo_new();
	gtk_combo_set_popdown_strings(GTK_COMBO(w), list);
	g_list_free(list);
	run_key_entry = GTK_COMBO(w)->entry;
	gtk_signal_connect (GTK_OBJECT (run_key_entry),
			    "changed",
			    GTK_SIGNAL_FUNC (changed_cb), NULL);
	/*gtk_widget_set_sensitive (run_key_entry, FALSE);*/
	gtk_table_attach_defaults (GTK_TABLE (table), w, 1, 2, 2, 3);

	w = gtk_button_new_with_label (_("Grab key..."));
	gtk_table_attach_defaults (GTK_TABLE (table), w, 2, 3, 2, 3);
	gtk_signal_connect (GTK_OBJECT (w), "clicked",
			    GTK_SIGNAL_FUNC (grab_button_pressed),
			    run_key_entry);
	

 	return (vbox);
}

static void
help(GtkWidget *capplet)
{
	gchar *tmp;

	tmp = gnome_help_file_find_file ("users-guide", "confglobal.html");
	if (tmp) {
		gnome_help_goto(0, tmp);
		g_free(tmp);
	} else {
		GtkWidget *mbox;

		mbox = gnome_message_box_new(_("No help is available/installed for these settings. Please make sure you\nhave the GNOME User's Guide installed on your system."),
					     GNOME_MESSAGE_BOX_ERROR,
					     _("Close"), NULL);

		gtk_widget_show(mbox);
	}
}

static void
loadup_vals(void)
{
	GString *buf;
	char *tile_def[]={"normal","purple","green","blue"};
	int i;
	
	buf = g_string_new(NULL);

	/*set up global options*/
	
	gnome_config_push_prefix("/panel/Config/");

	global_config.tooltips_enabled =
		gnome_config_get_bool("tooltips_enabled=TRUE");

	global_config.show_dot_buttons =
		gnome_config_get_bool("show_dot_buttons=FALSE");

	global_config.hungry_menus =
		gnome_config_get_bool("really_hungry_menus=FALSE");

	global_config.use_large_icons =
		gnome_config_get_bool("use_large_icons=TRUE");

	global_config.off_panel_popups =
		gnome_config_get_bool("off_panel_popups=TRUE");
		
	global_config.disable_animations =
		gnome_config_get_bool("disable_animations=FALSE");
		
	g_string_sprintf(buf,"auto_hide_step_size=%d",
			 DEFAULT_AUTO_HIDE_STEP_SIZE);
	global_config.auto_hide_step_size=gnome_config_get_int(buf->str);
		
	g_string_sprintf(buf,"explicit_hide_step_size=%d",
			 DEFAULT_EXPLICIT_HIDE_STEP_SIZE);
	global_config.explicit_hide_step_size=gnome_config_get_int(buf->str);
		
	g_string_sprintf(buf,"drawer_step_size=%d",
			 DEFAULT_DRAWER_STEP_SIZE);
	global_config.drawer_step_size=gnome_config_get_int(buf->str);
		
	g_string_sprintf(buf,"minimize_delay=%d", DEFAULT_MINIMIZE_DELAY);
	global_config.minimize_delay=gnome_config_get_int(buf->str);
		
	g_string_sprintf(buf,"minimized_size=%d", DEFAULT_MINIMIZED_SIZE);
	global_config.minimized_size=gnome_config_get_int(buf->str);
		
	g_string_sprintf(buf,"movement_type=%d", PANEL_SWITCH_MOVE);
	global_config.movement_type=gnome_config_get_int(buf->str);

	g_string_sprintf(buf,"menu_flags=%d", 
			 (int)(MAIN_MENU_SYSTEM_SUB | MAIN_MENU_USER_SUB|
			       MAIN_MENU_APPLETS_SUB | MAIN_MENU_REDHAT_SUB|
			       MAIN_MENU_DEBIAN_SUB | MAIN_MENU_KDE_SUB|
			       MAIN_MENU_PANEL | MAIN_MENU_DESKTOP));
	global_config.menu_flags = gnome_config_get_int(buf->str);

	global_config.keys_enabled = gnome_config_get_bool("keys_enabled=TRUE");

	global_config.menu_key = gnome_config_get_string("menu_key=Mod1-F1");
	/*convert_string_to_keysym_state(global_config.menu_key,
				       &global_config.menu_keysym,
				       &global_config.menu_state);*/

	global_config.run_key = gnome_config_get_string("run_key=Mod1-F2");
	/*convert_string_to_keysym_state(global_config.run_key,
				       &global_config.run_keysym,
				       &global_config.run_state);*/

	global_config.applet_padding=gnome_config_get_int("applet_padding=3");

	global_config.autoraise = gnome_config_get_bool("autoraise=TRUE");

	global_config.keep_bottom = gnome_config_get_bool("keep_bottom=FALSE");

	global_config.drawer_auto_close = gnome_config_get_bool("drawer_auto_close=FALSE");
	global_config.simple_movement = gnome_config_get_bool("simple_movement=FALSE");
	global_config.hide_panel_frame = gnome_config_get_bool("hide_panel_frame=FALSE");
	global_config.tile_when_over = gnome_config_get_bool("tile_when_over=FALSE");
	global_config.saturate_when_over = gnome_config_get_bool("saturate_when_over=TRUE");
	global_config.confirm_panel_remove = gnome_config_get_bool("confirm_panel_remove=TRUE");

	for(i=0;i<LAST_TILE;i++) {
		g_string_sprintf(buf,"tiles_enabled_%d=FALSE",i);
		global_config.tiles_enabled[i] =
			gnome_config_get_bool(buf->str);

		g_free(global_config.tile_up[i]);
		g_string_sprintf(buf,"tile_up_%d=tiles/tile-%s-up.png",
			   i, tile_def[i]);
		global_config.tile_up[i] = gnome_config_get_string(buf->str);

		g_free(global_config.tile_down[i]);
		g_string_sprintf(buf,"tile_down_%d=tiles/tile-%s-down.png",
			   i,tile_def[i]);
		global_config.tile_down[i] = gnome_config_get_string(buf->str);

		g_string_sprintf(buf,"tile_border_%d=2",i);
		global_config.tile_border[i] = gnome_config_get_int(buf->str);
		g_string_sprintf(buf,"tile_depth_%d=2",i);
		global_config.tile_depth[i] = gnome_config_get_int(buf->str);
	}
	g_string_free(buf,TRUE);
		
	gnome_config_pop_prefix();
}

static void
tell_panel(void)
{
	CORBA_Environment ev;
	GNOME_Panel panel_client = CORBA_OBJECT_NIL;

	panel_client =
		goad_server_activate_with_repo_id(NULL,
						  "IDL:GNOME/Panel:1.0",
						  GOAD_ACTIVATE_EXISTING_ONLY,
						  NULL);
	
	if(!panel_client) return;
	CORBA_exception_init(&ev);
	GNOME_Panel_notice_config_changes(panel_client, &ev);
	CORBA_exception_free(&ev);
}

static void
write_config(GlobalConfig *conf)
{
	int i;
	GString *buf;
	gnome_config_push_prefix("/panel/Config/");

	gnome_config_set_int("auto_hide_step_size",
			     conf->auto_hide_step_size);
	gnome_config_set_int("explicit_hide_step_size",
			     conf->explicit_hide_step_size);
	gnome_config_set_int("drawer_step_size",
			     conf->drawer_step_size);
	gnome_config_set_int("minimized_size",
			     conf->minimized_size);
	gnome_config_set_int("minimize_delay",
			     conf->minimize_delay);
	gnome_config_set_int("movement_type",
			     (int)conf->movement_type);
	gnome_config_set_bool("tooltips_enabled",
			      conf->tooltips_enabled);
	gnome_config_set_bool("show_dot_buttons",
			      conf->show_dot_buttons);
	gnome_config_set_bool("hungry_menus",
			      conf->hungry_menus);
	gnome_config_set_bool("use_large_icons",
			      conf->use_large_icons);
	gnome_config_set_bool("off_panel_popups",
			      conf->off_panel_popups);
	gnome_config_set_bool("disable_animations",
			      conf->disable_animations);
	gnome_config_set_int("applet_padding",
			     conf->applet_padding);
	gnome_config_set_bool("autoraise",
			      conf->autoraise);
	gnome_config_set_bool("keep_bottom",
			      conf->keep_bottom);
	gnome_config_set_bool("drawer_auto_close",
			      conf->drawer_auto_close);
	gnome_config_set_bool("simple_movement",
			      conf->simple_movement);
	gnome_config_set_bool("hide_panel_frame",
			      conf->hide_panel_frame);
	gnome_config_set_bool("tile_when_over",
			      conf->tile_when_over);
	gnome_config_set_bool("saturate_when_over",
			      conf->saturate_when_over);
	gnome_config_set_bool("confirm_panel_remove",
			      conf->confirm_panel_remove);
	gnome_config_set_int("menu_flags", conf->menu_flags);
	gnome_config_set_bool("keys_enabled", conf->keys_enabled);
	gnome_config_set_string("menu_key", conf->menu_key);
	gnome_config_set_string("run_key", conf->run_key);
			     
	buf = g_string_new(NULL);
	for(i=0;i<LAST_TILE;i++) {
		g_string_sprintf(buf,"tiles_enabled_%d",i);
		gnome_config_set_bool(buf->str,
				      conf->tiles_enabled[i]);
		g_string_sprintf(buf,"tile_up_%d",i);
		gnome_config_set_string(buf->str,
					conf->tile_up[i]);
		g_string_sprintf(buf,"tile_down_%d",i);
		gnome_config_set_string(buf->str,
					conf->tile_down[i]);
		g_string_sprintf(buf,"tile_border_%d",i);
		gnome_config_set_int(buf->str,
				     conf->tile_border[i]);
		g_string_sprintf(buf,"tile_depth_%d",i);
		gnome_config_set_int(buf->str,
				     conf->tile_depth[i]);
	}
	g_string_free(buf,TRUE);
	gnome_config_pop_prefix();
	gnome_config_sync();
	
	tell_panel();
}

static void
try(GtkWidget *capplet, gpointer data)
{
	sync_config_with_animation_page(&global_config);
	sync_config_with_buttons_page(&global_config);
	sync_config_with_applets_page(&global_config);
	sync_config_with_menu_page(&global_config);
	sync_config_with_misc_page(&global_config);
	write_config(&global_config);
}

static void
revert(GtkWidget *capplet, gpointer data)
{
	sync_animation_page_with_config(&loaded_config);
	sync_buttons_page_with_config(&loaded_config);
	sync_applets_page_with_config(&loaded_config);
	sync_menu_page_with_config(&loaded_config);
	sync_misc_page_with_config(&loaded_config);
	global_config = loaded_config;
	write_config(&global_config);
}

static void
setup_the_ui(GtkWidget *capplet)
{
	GtkWidget *page;
	GtkWidget *nbook;
	
	nbook = gtk_notebook_new();
	gtk_container_add(GTK_CONTAINER(capplet),nbook);

	/* Animation notebook page */
	page = animation_notebook_page ();
	gtk_notebook_append_page (GTK_NOTEBOOK(nbook),
				  page, gtk_label_new (_("Animation")));

	/* General icon notebook page */
	page = buttons_notebook_page ();
	gtk_notebook_append_page (GTK_NOTEBOOK(nbook),
				    page, gtk_label_new (_("Buttons")));

	/* applet settings */
	page = applets_notebook_page ();
	gtk_notebook_append_page (GTK_NOTEBOOK (nbook),
				  page, gtk_label_new (_("Applets")));

	/* Menu notebook page */
	page = menu_notebook_page ();
	gtk_notebook_append_page (GTK_NOTEBOOK (nbook),
				  page, gtk_label_new (_("Menu")));
		
	/* Miscellaneous notebook page */
	page = misc_notebook_page ();
	gtk_notebook_append_page (GTK_NOTEBOOK(nbook),
				  page, gtk_label_new (_("Miscellaneous")));


	gtk_signal_connect (GTK_OBJECT (nbook), "event",
			    GTK_SIGNAL_FUNC (config_event),
			    nbook);

	/* show notebook */
	gtk_widget_show_all(nbook);

	sync_animation_page_with_config(&loaded_config);
	sync_buttons_page_with_config(&loaded_config);
	sync_applets_page_with_config(&loaded_config);
	sync_menu_page_with_config(&loaded_config);
	sync_misc_page_with_config(&loaded_config);

	/* Finished */
	gtk_widget_show_all(capplet);
	gtk_signal_connect(GTK_OBJECT(capplet), "try",
			   GTK_SIGNAL_FUNC(try), NULL);
	gtk_signal_connect(GTK_OBJECT(capplet), "revert",
			   GTK_SIGNAL_FUNC(revert), NULL);
	gtk_signal_connect(GTK_OBJECT(capplet), "ok",
			   GTK_SIGNAL_FUNC(try), NULL);
	gtk_signal_connect(GTK_OBJECT(capplet), "cancel",
			   GTK_SIGNAL_FUNC(revert), NULL);
	gtk_signal_connect(GTK_OBJECT(capplet), "help",
			   GTK_SIGNAL_FUNC(help), NULL);
}

int
main (int argc, char **argv)
{
	bindtextdomain(PACKAGE, GNOMELOCALEDIR);
	textdomain(PACKAGE);

	changing = TRUE;

	if(gnome_capplet_init("gnome-panel-properties", VERSION, argc,
			      argv, NULL, 0, NULL) < 0)
		return 1;
	
	loadup_vals();
	
	loaded_config = global_config;

	capplet = capplet_widget_new();

	setup_the_ui(capplet);

	changing = FALSE;
	capplet_gtk_main();

	return 0;
}
