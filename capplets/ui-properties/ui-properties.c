/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *   property-ui.c: Property page for configuring look and feel
 *      
 *   Copyright (C) 1998 Free Software Foundation 
 *   Author: Havoc Pennington <hp@pobox.com>
 *
 *   Ported to capplet by Martin Baulig.
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
#include "capplet-widget.h"

static GtkWidget *capplet;

static void
ui_read (void)
{
  gnome_preferences_load();
}

static void
ui_write (void)
{
  /* FIXME obviously this shouldn't be in both write and apply. */
  gnome_preferences_save();
}

static void
ui_apply (void)
{
  /* FIXME what is this supposed to be vs. write? */
  gnome_preferences_save();
}

#define NUM_BUTTONBOX_STYLES 5

static const gchar * const buttonbox_style_names[] = {
  N_("Default Gtk setting [FIXME - Describe this better]"),
  N_("Spread buttons out"),
  N_("Put buttons on edges"),
  N_("Left-justify buttons"),
  N_("Right-justify buttons")
};

#define NUM_DIALOG_POSITIONS 3

static const gchar * const dialog_positions_names[] = {
  N_("Let window manager decide"),
  N_("Center of the screen"),
  N_("At the mouse pointer")
};

#define NUM_DIALOG_TYPES 2

static const gchar * const dialog_types_names[] = {
  N_("Dialogs are like other windows"),
  N_("Dialogs are treated specially by window manager")
};

#define NUM_MDI_MODES 3

/* looking for better descriptions here... */
static const gchar * const mdi_mode_names[] = {
  N_("Notebook"),
  N_("Toplevel"),
  N_("Modal")
};

#define NUM_TAB_POSITIONS 4

static const gchar * const tab_position_names[] = {
  N_("Left"),
  N_("Right"),
  N_("Top"),
  N_("Bottom")
};

static void buttonbox_style_cb(GtkWidget * menuitem, gint style)
{
  gnome_preferences_set_button_layout (style);
  capplet_widget_state_changed(CAPPLET_WIDGET (capplet), TRUE);
}

static void dialog_position_cb(GtkWidget * menuitem, gint pos)
{
  gnome_preferences_set_dialog_position(pos);
  capplet_widget_state_changed(CAPPLET_WIDGET (capplet), TRUE);
}

static void dialog_type_cb(GtkWidget * menuitem, gint t)
{
  gnome_preferences_set_dialog_type(t);
  capplet_widget_state_changed(CAPPLET_WIDGET (capplet), TRUE);
}

static void mdi_mode_cb(GtkWidget * menuitem, gint mode)
{
  gnome_preferences_set_mdi_mode ((GnomeMDIMode)mode);
  capplet_widget_state_changed(CAPPLET_WIDGET (capplet), TRUE);
}

static void mdi_tab_pos_cb(GtkWidget * menuitem, gint pos)
{
  gnome_preferences_set_mdi_tab_pos ((GtkPositionType)pos);
  capplet_widget_state_changed(CAPPLET_WIDGET (capplet), TRUE);
}

static void checkbutton_cb    ( GtkWidget * button, 
                                void ( * set_func ) (gboolean) )
{
  gboolean b = GTK_TOGGLE_BUTTON(button)->active;

  (* set_func) (b);
  capplet_widget_state_changed(CAPPLET_WIDGET (capplet), TRUE);
}

static void checkbutton_bool_cb    ( GtkWidget * button, 
                                       gchar * str )
{
  gboolean b = GTK_TOGGLE_BUTTON(button)->active;

  gnome_config_set_bool(str, b);
  capplet_widget_state_changed(CAPPLET_WIDGET (capplet), TRUE);
}

static void
make_option_menu(GtkWidget * vbox, const gchar * labeltext,
                 gint (*getfunc)(),
                 const gchar * const names[], gint N,
                 void (*callback)(GtkWidget*,gint))
{
  GtkWidget * option_menu;
  GtkWidget * menu;
  GtkWidget * menuitem;
  GtkWidget * hbox;
  GtkWidget * label;
  gint i;

  option_menu = gtk_option_menu_new();
  menu = gtk_menu_new();
  gtk_option_menu_set_menu ( GTK_OPTION_MENU(option_menu), menu );

  i = 0;
  while ( i < N ) {
    menuitem = gtk_menu_item_new_with_label(_(names[i]));
    gtk_menu_append ( GTK_MENU(menu), menuitem );
    gtk_signal_connect ( GTK_OBJECT(menuitem), "activate", 
                         GTK_SIGNAL_FUNC(callback), 
                         GINT_TO_POINTER (i) );
    if ( i == (*getfunc)() ) {
      gtk_option_menu_set_history ( GTK_OPTION_MENU(option_menu), i );
    }
    ++i;
  }

  hbox = gtk_hbox_new (FALSE, 0);

  label = gtk_label_new(labeltext);

  gtk_box_pack_start ( GTK_BOX(hbox), label, FALSE, FALSE, GNOME_PAD );
  /* FIXME: option menu width should be just wide enough to display
     widest menu item.  Ideally the option menu code would handle this
     for us.  Also, the button is not tall enough -- descenders seem
     to get clipped on my display.  */
  gtk_box_pack_end ( GTK_BOX(hbox), option_menu, TRUE, TRUE, GNOME_PAD_SMALL );
  gtk_box_pack_start ( GTK_BOX(vbox), hbox, FALSE, FALSE, GNOME_PAD );
}

static void
ui_setup (void)
{
  GtkWidget * vbox;
  GtkWidget * notebook;
  GtkWidget * button;

  capplet = capplet_widget_new();

  notebook = gtk_notebook_new ();

  vbox = gtk_vbox_new(FALSE, GNOME_PAD);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), GNOME_PAD);

  make_option_menu(vbox,_("Dialog buttons"),
                   gnome_preferences_get_button_layout,
                   buttonbox_style_names, NUM_BUTTONBOX_STYLES,
                   buttonbox_style_cb);

  make_option_menu(vbox,_("Dialog position"),
                   gnome_preferences_get_dialog_position,
                   dialog_positions_names, NUM_DIALOG_POSITIONS,
                   dialog_position_cb);

  make_option_menu(vbox,_("Dialog hints"),
                   gnome_preferences_get_dialog_type,
                   dialog_types_names, NUM_DIALOG_TYPES,
                   dialog_type_cb);


  button = 
    gtk_check_button_new_with_label(_("Use statusbar instead of dialog when possible"));
  gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(button), 
                              gnome_preferences_get_statusbar_dialog());
  gtk_signal_connect(GTK_OBJECT(button), "toggled",
                     GTK_SIGNAL_FUNC(checkbutton_cb),
                     gnome_preferences_set_statusbar_dialog);

  gtk_box_pack_start ( GTK_BOX(vbox), button, FALSE, FALSE, GNOME_PAD );

  button = 
    gtk_check_button_new_with_label(_("Place dialogs over application window when possible"));
  gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(button), 
                              gnome_preferences_get_dialog_centered());
  gtk_signal_connect(GTK_OBJECT(button), "toggled",
                     GTK_SIGNAL_FUNC(checkbutton_cb),
                     gnome_preferences_set_dialog_centered);

  gtk_box_pack_start ( GTK_BOX(vbox), button, FALSE, FALSE, GNOME_PAD );

  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox,
                            gtk_label_new (_("Dialogs")));
  gtk_widget_show_all(vbox);

  vbox = gtk_vbox_new(FALSE, GNOME_PAD_SMALL);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), GNOME_PAD);

  button = 
    gtk_check_button_new_with_label(_("Menubars are detachable"));
  gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(button), 
                              gnome_preferences_get_menubar_handlebox());
  gtk_signal_connect(GTK_OBJECT(button), "toggled",
                     GTK_SIGNAL_FUNC(checkbutton_cb),
                     gnome_preferences_set_menubar_handlebox);
  gtk_box_pack_start ( GTK_BOX(vbox), button, FALSE, FALSE, GNOME_PAD_SMALL );

  button = 
    gtk_check_button_new_with_label(_("Menubars have relieved border"));
  gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(button), 
                              gnome_preferences_get_menubar_relief());
  gtk_signal_connect(GTK_OBJECT(button), "toggled",
                     GTK_SIGNAL_FUNC(checkbutton_cb),
                     gnome_preferences_set_menubar_relief);
  gtk_box_pack_start ( GTK_BOX(vbox), button, FALSE, FALSE, GNOME_PAD_SMALL );

  button = 
    gtk_check_button_new_with_label(_("Toolbars are detachable"));
  gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(button), 
                              gnome_preferences_get_toolbar_handlebox());
  gtk_signal_connect(GTK_OBJECT(button), "toggled",
                     GTK_SIGNAL_FUNC(checkbutton_cb),
                     gnome_preferences_set_toolbar_handlebox);
  gtk_box_pack_start ( GTK_BOX(vbox), button, FALSE, FALSE, GNOME_PAD_SMALL );

  button = 
    gtk_check_button_new_with_label(_("Toolbar buttons have relieved border"));
  gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(button), 
                              gnome_preferences_get_toolbar_relief());
  gtk_signal_connect(GTK_OBJECT(button), "toggled",
                     GTK_SIGNAL_FUNC(checkbutton_cb),
                     gnome_preferences_set_toolbar_relief);
  gtk_box_pack_start ( GTK_BOX(vbox), button, FALSE, FALSE, GNOME_PAD_SMALL );

  button = 
    gtk_check_button_new_with_label(_("Toolbars have flat look"));
  gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(button), 
                              gnome_preferences_get_toolbar_flat());
  gtk_signal_connect(GTK_OBJECT(button), "toggled",
                     GTK_SIGNAL_FUNC(checkbutton_cb),
                     gnome_preferences_set_toolbar_flat);
  gtk_box_pack_start ( GTK_BOX(vbox), button, FALSE, FALSE, GNOME_PAD_SMALL );

  button = 
    gtk_check_button_new_with_label(_("Toolbars have line separators"));
  gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(button), 
                              gnome_preferences_get_toolbar_lines());
  gtk_signal_connect(GTK_OBJECT(button), "toggled",
                     GTK_SIGNAL_FUNC(checkbutton_cb),
                     gnome_preferences_set_toolbar_lines);
  gtk_box_pack_start ( GTK_BOX(vbox), button, FALSE, FALSE, GNOME_PAD_SMALL );

  button = 
    gtk_check_button_new_with_label(_("Toolbars have text labels"));
  gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(button), 
                              gnome_preferences_get_toolbar_labels());
  gtk_signal_connect(GTK_OBJECT(button), "toggled",
                     GTK_SIGNAL_FUNC(checkbutton_cb),
                     gnome_preferences_set_toolbar_labels);
  gtk_box_pack_start ( GTK_BOX(vbox), button, FALSE, FALSE, GNOME_PAD_SMALL );

  button = 
    gtk_check_button_new_with_label(_("Statusbar is interactive when possible"));
  gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(button), 
                              gnome_preferences_get_statusbar_interactive());
  gtk_signal_connect(GTK_OBJECT(button), "toggled",
                     GTK_SIGNAL_FUNC(checkbutton_cb),
                     gnome_preferences_set_statusbar_interactive);
  gtk_box_pack_start ( GTK_BOX(vbox), button, FALSE, FALSE, GNOME_PAD_SMALL );

  button = 
    gtk_check_button_new_with_label(_("Dialog buttons have icons"));
  gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(button), 
                              gnome_config_get_bool("/Gnome/Icons/ButtonUseIcons=true"));
  gtk_signal_connect(GTK_OBJECT(button), "toggled",
                     GTK_SIGNAL_FUNC(checkbutton_bool_cb),
                     "/Gnome/Icons/ButtonUseIcons");
  gtk_box_pack_start ( GTK_BOX(vbox), button, FALSE, FALSE, GNOME_PAD_SMALL );

  button = 
    gtk_check_button_new_with_label(_("Menu items have icons"));
  gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(button), 
                              gnome_config_get_bool("/Gnome/Icons/MenusUseIcons=true"));
  gtk_signal_connect(GTK_OBJECT(button), "toggled",
                     GTK_SIGNAL_FUNC(checkbutton_bool_cb),
                     "/Gnome/Icons/MenusUseIcons");
  gtk_box_pack_start ( GTK_BOX(vbox), button, FALSE, FALSE, GNOME_PAD_SMALL );

  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox,
                            gtk_label_new (_("Application")));
  gtk_widget_show_all(vbox);

  vbox = gtk_vbox_new(FALSE, GNOME_PAD);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), GNOME_PAD);

  make_option_menu(vbox,_("Default MDI mode"),
                   gnome_preferences_get_mdi_mode,
                   mdi_mode_names, NUM_MDI_MODES,
                   mdi_mode_cb);

  make_option_menu(vbox,_("MDI notebook tab position"),
                   gnome_preferences_get_mdi_tab_pos,
                   tab_position_names, NUM_TAB_POSITIONS,
                   mdi_tab_pos_cb);

  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox,
                            gtk_label_new (_("MDI")));
  gtk_widget_show_all(vbox);

	/* Finished */

  gtk_signal_connect (GTK_OBJECT (capplet), "ok",
                      GTK_SIGNAL_FUNC (ui_write), NULL);

  gtk_widget_show (notebook);
  gtk_container_add (GTK_CONTAINER (capplet), notebook);
  gtk_widget_show (capplet);
}

int
main (int argc, char **argv)
{
  bindtextdomain (PACKAGE, GNOMELOCALEDIR);
  textdomain (PACKAGE);

  gnome_capplet_init ("ui-properties", VERSION, argc,
                      argv, NULL, 0, NULL);
  
  ui_read ();
  ui_setup ();
  capplet_gtk_main ();
  
  return 0;
}
