/* property-ui.c - Property page for configuring look and feel
   (libgnomeui).  */

#include <config.h>

#include "gnome.h"
#include "gnome-desktop.h"

/* The property configurator we're associated with.  */
static GnomePropertyConfigurator *config;

static void
ui_read (void)
{
  gnome_preferences_load();
}

static void
ui_write (void)
{
  gnome_preferences_save();
}

static void
ui_apply (void)
{
  /* FIXME what is this supposed to be vs. write? */
  gnome_preferences_save();
  property_applied();
}

#define NUM_BUTTONBOX_STYLES 5

static const gchar * const buttonbox_style_names[] = {
  N_("Default Gtk setting [FIXME - Describe this better]"),
  N_("Spread buttons out"),
  N_("Put buttons on edges"),
  N_("Left-justify buttons"),
  N_("Right-justify buttons")
};

static void buttonbox_style_cb(GtkWidget * menuitem, gint style)
{
  gnome_preferences_global.dialog_buttons_style = style;
  property_changed();
}

static void
ui_setup (void)
{
  GtkWidget * vbox;
  GtkWidget * option_menu;
  GtkWidget * menu;
  GtkWidget * menuitem;
  GtkWidget * label;
  gint i;

  vbox = gtk_vbox_new(TRUE, GNOME_PAD);

  option_menu = gtk_option_menu_new();
  menu = gtk_menu_new();
  gtk_option_menu_set_menu ( GTK_OPTION_MENU(option_menu), menu );

  i = 0;
  while ( i < NUM_BUTTONBOX_STYLES ) {
    menuitem = gtk_menu_item_new_with_label(buttonbox_style_names[i]);
    gtk_menu_append ( GTK_MENU(menu), menuitem );
    gtk_signal_connect ( GTK_OBJECT(menuitem), "activate", 
			 GTK_SIGNAL_FUNC(buttonbox_style_cb), 
			 (gpointer) i );
    if ( i == gnome_preferences_global.dialog_buttons_style ) {
      gtk_option_menu_set_history ( GTK_OPTION_MENU(option_menu), i );
    }
    ++i;
  }

  label = gtk_label_new(_("Dialog buttons"));

  gtk_box_pack_start ( GTK_BOX(vbox), label, FALSE, FALSE, GNOME_PAD_SMALL );
  gtk_box_pack_start ( GTK_BOX(vbox), option_menu, FALSE, FALSE, GNOME_PAD_SMALL );

  gnome_property_box_append_page (GNOME_PROPERTY_BOX (config->property_box),
				  vbox, gtk_label_new (_("Dialogs")));
  gtk_widget_show_all(vbox);
}

static gint
ui_action (GnomePropertyRequest req)
{
  switch (req)
    {
    case GNOME_PROPERTY_READ:
      ui_read ();
      break;
    case GNOME_PROPERTY_WRITE:
      ui_write ();
      break;
    case GNOME_PROPERTY_APPLY:
      ui_apply ();
      break;
    case GNOME_PROPERTY_SETUP:
      ui_setup ();
      break;
    default:
      return 0;
    }

  return 1;
}

void
ui_register (GnomePropertyConfigurator *c)
{
  config = c;
  gnome_property_configurator_register (config, ui_action);
}


