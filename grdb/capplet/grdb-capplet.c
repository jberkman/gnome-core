/*
 * grdb: prime xrdb settings with gtk theme colors
 *
 * This is a public release of grdb, a tool to apply gtk theme
 * color schemes to not gtk programs.
 * 
 * (C) Copyright 2000 Samuel Hunter <shunter@bigsky.net>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
 *
 * Author: Sam Hunter <shunter@bigsky.net>
 */

#include <config.h>
#include "capplet-widget.h"
#include <X11/Xlib.h>
#include <assert.h>

#include <gdk/gdkx.h>

#include "gnome.h"
#include "grdb-filtered-font-picker.h"

#define GRDB_CONFIG_PREFIX "/grdb-capplet/settings/"

#define DEFAULT_FIXED_FONT \
  "-Misc-Fixed-Medium-R-SemiCondensed--13-120-75-75-C-60-ISO8859-1"

static gboolean GrdbEnabled = FALSE;
static gboolean UseFixed    = FALSE;
static gchar*   FixedFont   = NULL;

static GtkWidget* capplet;
static GtkWidget* do_grdb_button;
static GtkWidget* use_fixed_button;
static GtkWidget* font_sel_button;

static void
button_toggled (GtkWidget* widget, gpointer data)
{
        gboolean* flag = (gboolean*)data;
        *flag = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));

        capplet_widget_state_changed (CAPPLET_WIDGET (capplet), TRUE);

        if (widget == use_fixed_button) {
                gtk_widget_set_sensitive (font_sel_button, UseFixed);
        }
}

static void
font_set_callback (GtkWidget *widget, gchar *font, gpointer data)
{
        g_free(FixedFont);
        FixedFont = g_strdup(font);

        capplet_widget_state_changed (CAPPLET_WIDGET (capplet), TRUE);
}

static void
grdb_capp_read (void)
{
	gnome_config_push_prefix (GRDB_CONFIG_PREFIX);

        GrdbEnabled = gnome_config_get_bool ("grdb_enabled=false");

        UseFixed    = gnome_config_get_bool ("use_fixed=false");

        FixedFont   = gnome_config_get_string ("fixed_font");
        if (!FixedFont) {
                FixedFont = g_strdup (DEFAULT_FIXED_FONT);
        }

	gnome_config_pop_prefix ();
}

static void
grdb_capp_help (void)
{
  gchar *tmp = NULL;

  tmp = gnome_help_file_find_file (
          "grdb", "grdb-capplet.html");

  if (tmp) {
          gnome_help_goto (0, tmp);
          g_free (tmp);
  } else {
          GtkWidget *mbox;
          
          mbox = gnome_message_box_new (
		  _("No help is available/installed for these settings."),
		  GNOME_MESSAGE_BOX_ERROR,
		  _("Close"), NULL);

          gtk_widget_show (mbox);
  }

}

static void
grdb_capp_apply (void)
{
        gchar *command;

        if (!GrdbEnabled) 
                return;

	if (!FixedFont ||
	    strcmp (FixedFont, DEFAULT_FIXED_FONT) == 0) {
                command = g_strdup ("grdb");
        } else {
                command = g_strdup_printf ("grdb --fixed=\"%s\"",
                                           FixedFont);
        }
        
        system (command);
        g_free (command);
}

static void
grdb_capp_write (void)
{
        grdb_capp_apply ();

	gnome_config_push_prefix (GRDB_CONFIG_PREFIX);

        gnome_config_set_bool   ("grdb_enabled", GrdbEnabled);
        gnome_config_set_bool   ("use_fixed",    UseFixed);
        if (FixedFont) {
                gnome_config_set_string ("fixed_font", FixedFont);
        }

	gnome_config_pop_prefix ();
        gnome_config_sync ();
}

static void
grdb_capp_revert (void)
{
        grdb_capp_read ();
        grdb_capp_apply ();

        /* update ui with old values */
        gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (do_grdb_button),
                                     GrdbEnabled);
        gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (use_fixed_button),
                                     UseFixed);

	gnome_font_picker_set_font_name (
		GNOME_FONT_PICKER (font_sel_button), FixedFont);
}


static void
grdb_capp_setup (void)
{
        /* build gui */
        GtkWidget* vbox_main;
        GtkWidget* vbox;
        GtkWidget* frame;


        gchar *spacings[] = {"c", "m", NULL};

        capplet = capplet_widget_new ();

        gtk_signal_connect (GTK_OBJECT (capplet), "help",
                            GTK_SIGNAL_FUNC (grdb_capp_help),   NULL);
        gtk_signal_connect (GTK_OBJECT (capplet), "try",
                            GTK_SIGNAL_FUNC (grdb_capp_apply),  NULL);
        gtk_signal_connect (GTK_OBJECT (capplet), "revert",
                            GTK_SIGNAL_FUNC (grdb_capp_revert), NULL);
        gtk_signal_connect (GTK_OBJECT (capplet), "ok",
                            GTK_SIGNAL_FUNC (grdb_capp_write),  NULL);
        gtk_signal_connect (GTK_OBJECT (capplet), "cancel",
                            GTK_SIGNAL_FUNC (grdb_capp_revert), NULL);

        vbox_main = gtk_vbox_new (FALSE, GNOME_PAD_SMALL);

        do_grdb_button = gtk_check_button_new_with_label (
                _("Apply GTK theme to legacy applications"));
        gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (do_grdb_button),
                                     GrdbEnabled);
        gtk_signal_connect (GTK_OBJECT (do_grdb_button), "toggled",
                            button_toggled, &GrdbEnabled);
        gtk_box_pack_start (GTK_BOX (vbox_main), do_grdb_button,
                            FALSE, FALSE, 0);
        gtk_widget_show (do_grdb_button);

        frame = gtk_frame_new (_("Fixed Font"));
        gtk_box_pack_start (GTK_BOX (vbox_main), frame, FALSE, FALSE, 0);
        gtk_widget_show (frame);
        
        vbox = gtk_vbox_new (FALSE, GNOME_PAD);
        gtk_container_add (GTK_CONTAINER (frame), vbox);

        use_fixed_button = gtk_check_button_new_with_label (
                _("Use custom fixed font"));
        gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (use_fixed_button),
                                     UseFixed);
        gtk_signal_connect (GTK_OBJECT (use_fixed_button), "toggled",
                            button_toggled, &UseFixed);
        gtk_box_pack_start (GTK_BOX (vbox), use_fixed_button,
                            FALSE, FALSE, 0);
        gtk_widget_show (use_fixed_button);

        font_sel_button = grdb_filtered_font_picker_new ();
        gnome_font_picker_set_mode (GNOME_FONT_PICKER (font_sel_button),
                                    GNOME_FONT_PICKER_MODE_FONT_INFO);

        gnome_font_picker_set_font_name (
                GNOME_FONT_PICKER (font_sel_button),  FixedFont);

        gtk_widget_set_sensitive (font_sel_button, UseFixed);
       
        gnome_font_picker_fi_set_use_font_in_label (
                GNOME_FONT_PICKER (font_sel_button), TRUE, 12);
        gnome_font_picker_fi_set_show_size (
                GNOME_FONT_PICKER (font_sel_button), FALSE);

        grdb_filtered_font_picker_set_filter (
                GRDB_FILTERED_FONT_PICKER (font_sel_button),
                GTK_FONT_FILTER_BASE,
                GTK_FONT_ALL,
                NULL, NULL, NULL, NULL, spacings, NULL);

        gtk_signal_connect (GTK_OBJECT(font_sel_button),
                            "font_set",
                            font_set_callback,
                            NULL);

        gtk_box_pack_start (GTK_BOX (vbox), font_sel_button,
                            FALSE, FALSE, 0);
        gtk_widget_show (font_sel_button);

        gtk_container_add (GTK_CONTAINER (capplet), vbox_main);
        gtk_widget_show_all (capplet);
}

int
main (int argc, char **argv)
{
        GnomeClient *client = NULL;
        GnomeClientFlags flags;
        gchar *session_args[3];
        int token, init_results;

        bindtextdomain (PACKAGE, GNOMELOCALEDIR);
        textdomain (PACKAGE);

        init_results = gnome_capplet_init ("grdb-capplet", VERSION,
                                           argc, argv, NULL, 0, NULL);

	if (init_results < 0) {
                g_warning (_("an initialization error occurred while "
			   "starting 'grdb-capplet'.\n"
                           "aborting...\n"));
                exit (1);
	}

	client = gnome_master_client ();
	flags = gnome_client_get_flags (client);

	if (flags & GNOME_CLIENT_IS_CONNECTED) {
		token = gnome_startup_acquire_token (
                        "GRDB_CAPP_PROPERTIES",
                        gnome_client_get_id (client));

		if (token) {
			session_args[0] = argv[0];
			session_args[1] = "--init-session-settings";
			session_args[2] = NULL;
			gnome_client_set_priority (client, 20);
			gnome_client_set_restart_style (client, 
							GNOME_RESTART_ANYWAY);
			gnome_client_set_restart_command (client, 2, 
							  session_args);
		} else { 
			gnome_client_set_restart_style (client, 
							GNOME_RESTART_NEVER);
                }

                gnome_client_flush (client);
        } else {
		token = 1;
        }

        grdb_capp_read ();

        if(token) {
                grdb_capp_apply ();
        }

	if (init_results != 1) {
		grdb_capp_setup ();
	        capplet_gtk_main ();
	}
        return 0;
}
