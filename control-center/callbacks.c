/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */

/* Copyright (C) 1998 Redhat Software Inc. 
 * Authors: Jonathan Blandford <jrb@redhat.com>
 */
#include "callbacks.h"
extern GtkWidget *exit_dialog;
extern GtkWidget *create_exit_dialog();
extern void control_panel_corba_gtk_main_quit(void);

gint
exit_callback(GtkWidget *widget, gpointer data) 
{
        GList* templist = NULL;
        templist = g_list_prepend (templist, "entry1");
        templist = g_list_prepend (templist, "entry2");
        
        if (exit_dialog)
                gnome_dialog_close (GNOME_DIALOG (exit_dialog));
        exit_dialog = create_exit_dialog(templist);
        if (gnome_dialog_run (GNOME_DIALOG (exit_dialog)) == 1) {
                /* if they double click on an entry, then 
                 * that callback will handle destruction 
                 */
                gnome_dialog_close (GNOME_DIALOG (exit_dialog));
                exit_dialog = NULL;
                return 0;
        }
        else 
                control_panel_corba_gtk_main_quit();
                

        return 0;
}
void
help_callback(GtkWidget *widget, gpointer data)
{
        /* nothing here yet, <: */
        /* as per most gnome apps... */
}
void
about_callback(GtkWidget *widget, gpointer data)
{
	GtkWidget *about = NULL;
	gchar *authors[] = {
                "Jonathan Blandford <jrb@redhat.com>",
		NULL
	};

	about = gnome_about_new(_("Desktop Manager"), "0.1",
				"Copyright (C) 1998 the Free Software Foundation",
				(const gchar **) authors,
				_("Desktop Properties manager."),
				NULL);
        gtk_widget_show(about);
}
