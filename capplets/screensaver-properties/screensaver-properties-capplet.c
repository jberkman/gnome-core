/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* Copyright (C) 1998 Redhat Software Inc. 
 * Authors: Jonathan Blandford <jrb@redhat.com>
 */
#include "capplet-widget.h"
#include "gnome.h"
#include "callbacks.h"
#include "parser.h"
#include <unistd.h>
#include <signal.h>
#include <config.h>
#include <dirent.h>

/* prototypes */
void create_list (GtkList *list, gchar *directory);
GtkWidget *get_saver_frame ();
GtkWidget *get_power_frame ();
void screensaver_setup ();

/* vars. */
GtkWidget *capplet;
GtkWidget *setup_button;
GtkWidget *setup_label;
GtkWidget *monitor;
GList *sdlist = NULL;
gint ss_priority;
gshort waitmins;
gshort dpmsmins;
gboolean dpms;
gboolean usessaver;
gboolean password;
gchar *screensaver;
screensaver_data *sd;


/* saving info */
void
create_list (GtkList *list, gchar *directory)
{
        DIR *dir;
        gchar *label;
        GtkWidget *list_item = NULL;
        GtkWidget *temp_item;
        struct dirent *child;
        gchar *prefix;
        GList *items = NULL;
        screensaver_data *sdnew;

        dir = opendir (directory);
        if (dir == NULL)
                return;

        while ((child = readdir (dir)) != NULL) {
                if (child->d_name[0] != '.') {
                        prefix = g_copy_strings ("=", directory, child->d_name, "=/Desktop Entry/", NULL);
                        gnome_config_push_prefix (prefix);
                        g_free (prefix);

                        sdnew = g_malloc (sizeof (screensaver_data));
                        sdnew->desktop_filename = g_copy_strings (directory, child->d_name, NULL);
                        sdnew->name = gnome_config_get_translated_string ("Name");
                        sdnew->tryexec = gnome_config_get_string ("TryExec");
                        gnome_config_pop_prefix ();
                        sdnew->args = NULL;

                        prefix = g_copy_strings ("=", directory, child->d_name, "=/Screensaver Data/", NULL);
                        gnome_config_push_prefix (prefix);
                        g_free (prefix);
                        sdnew->windowid = gnome_config_get_string ("WindowIdCommand");
                        sdnew->root = gnome_config_get_string ("RootCommand");
                        sdnew->author = gnome_config_get_string ("Author");
                        sdnew->comment = gnome_config_get_translated_string ("ExtendedComment");
                        sdnew->demo = gnome_config_get_string ("Demo");
                        prefix =  gnome_config_get_string ("Icon");
                        if (prefix) {
                                if (prefix[0] == '/')
                                        sdnew->icon = prefix;
                                else {
                                        /* we want to set up the initial settings */
                                        sdnew->icon = g_copy_strings (directory, prefix, NULL);

                                        g_free (prefix);
                                }
                        } else
                                sdnew->icon = NULL;
                        
                        gnome_config_pop_prefix ();
                        sdnew->dialog = NULL;
                        sdnew->setup_data = NULL;
                        if (!sdnew->name) {
                                /* bah -- useless file... */
                                break;
                        }
                        temp_item = gtk_list_item_new_with_label (sdnew->name);
                        items = g_list_prepend (items, temp_item);
                        if (strcmp (sdnew->name, screensaver) == 0) {
                                sd = sdnew;
                                list_item = temp_item;
                                init_screensaver_data (sdnew);
                                if (sdnew->setup_data == NULL)
                                        gtk_widget_set_sensitive (setup_button, FALSE);
                        }
                        gtk_signal_connect (GTK_OBJECT (items->data),
                                            "button_press_event",
                                            (GtkSignalFunc) list_click_callback,
                                            sdnew);
                }
        }
        if (items) {
                gtk_list_append_items (list, items);
                if (list_item) {
                        gtk_list_select_child (list, list_item);
                }
                else
                        gtk_widget_set_sensitive (setup_button, FALSE);
        } 

                
}
GtkWidget *
get_saver_frame ()
{
        GtkWidget *retval;
        GtkWidget *foobox;
        GtkWidget *hbox, *vbox, *temphbox, *tempvbox;
        GtkWidget *list, *label;
        GtkWidget *ssentry;
        GtkWidget *pword;
        GtkWidget *alignment;
        GtkWidget *swindow;
        GtkWidget *nice;
        GtkObject *adjustment;
        gchar tempmin [5];
        gchar *tempdir;
        GList *templist = NULL;
        retval = gtk_frame_new (_("Screen Saver"));


        /* we define setup_button way early to make it settable. */
        hbox = gtk_hbox_new (FALSE, GNOME_PAD_SMALL);
        setup_label = gtk_label_new (_("Settings..."));
        setup_button = gtk_button_new ();
        gtk_container_add (GTK_CONTAINER (setup_button), setup_label);

        /* left side -- with list */
	swindow = gtk_scrolled_window_new (NULL, NULL);
        list = gtk_list_new();
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (swindow),
					GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_list_set_selection_mode (GTK_LIST (list), GTK_SELECTION_BROWSE);
        templist = g_list_prepend (templist, gtk_list_item_new_with_label ("No Screensaver"));
        gtk_signal_connect (GTK_OBJECT (templist->data), 
                            "button_press_event", 
                            (GtkSignalFunc) list_click_callback, 
                            NULL); 

        gtk_list_prepend_items (GTK_LIST (list), templist);
        
        tempdir = gnome_unconditional_datadir_file ("control-center/.data/");
        create_list (GTK_LIST (list), tempdir);
        g_free (tempdir);
        tempdir = gnome_util_home_file ("control-center/.data/");
        create_list (GTK_LIST (list), tempdir);
        g_free (tempdir);
	gtk_container_add (GTK_CONTAINER (swindow), list);

        /* right side -- buttons et al. */
        vbox = gtk_vbox_new (FALSE, GNOME_PAD_SMALL);

        temphbox = gtk_hbox_new (FALSE, 0);
        alignment = gtk_alignment_new (0.0, 0.5, 0, 0);
        label = gtk_label_new (_("Start After "));
        ssentry = get_and_set_min_entry ();

        gtk_box_pack_start (GTK_BOX (temphbox), label, FALSE, FALSE, 0);
        gtk_box_pack_start (GTK_BOX (temphbox), ssentry, FALSE, FALSE, 0);
        label = gtk_label_new (_(" Min."));
        gtk_box_pack_start (GTK_BOX (temphbox), label, FALSE, FALSE, 0);
        gtk_container_add (GTK_CONTAINER (alignment), temphbox);
        gtk_box_pack_start (GTK_BOX (vbox), alignment, FALSE, FALSE, 0);
        pword = get_and_set_pword ();
        gtk_box_pack_start (GTK_BOX (vbox), pword, FALSE, FALSE, 0);
        
        temphbox = gtk_hbox_new (FALSE, 0);
        tempvbox = gtk_vbox_new (FALSE, 0);
        gtk_box_pack_start (GTK_BOX (temphbox), gtk_label_new (_("Priority:")), FALSE, FALSE, 0);
        gtk_box_pack_start (GTK_BOX (tempvbox), temphbox, FALSE, FALSE, 0);
        temphbox = gtk_hbox_new (FALSE, 0);
	adjustment = get_and_set_nice ();
					 
	nice = gtk_hscale_new (GTK_ADJUSTMENT (adjustment));
        gtk_scale_set_draw_value (GTK_SCALE (nice), FALSE);
        gtk_box_pack_start (GTK_BOX (temphbox), gtk_label_new (_("Low ")), FALSE, FALSE, 0);
        gtk_box_pack_start (GTK_BOX (temphbox), nice, TRUE, TRUE, 0);
        gtk_box_pack_start (GTK_BOX (temphbox), gtk_label_new (_(" Normal")), FALSE, FALSE, 0);
        gtk_box_pack_start (GTK_BOX (tempvbox), temphbox, FALSE, FALSE, 0);
        gtk_box_pack_start (GTK_BOX (vbox), tempvbox, FALSE, FALSE, 0);
        

        gtk_signal_connect (GTK_OBJECT (setup_button), "clicked", (GtkSignalFunc) setup_callback, NULL);
        temphbox = gtk_hbox_new (FALSE, 0);
        gtk_box_pack_end (GTK_BOX (temphbox), setup_button, FALSE, FALSE, 0);
        gtk_box_pack_start (GTK_BOX (vbox), temphbox, FALSE, FALSE, 0);

        gtk_box_pack_start (GTK_BOX (hbox), swindow, TRUE, TRUE, 0);
        gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);
        foobox = gtk_frame_new (NULL);
        gtk_frame_set_shadow_type (GTK_FRAME (foobox), GTK_SHADOW_NONE);
        gtk_container_add (GTK_CONTAINER (foobox), hbox);
        gtk_container_border_width (GTK_CONTAINER (foobox), GNOME_PAD_SMALL);
        gtk_container_add (GTK_CONTAINER (retval), foobox);
        return retval;
}
GtkWidget *
get_power_frame()
{
        GtkWidget *retval;
        GtkWidget *frame, *vbox, *dpmscheck, *hbox, *label, *entry, *tempvbox;
        gchar tempmin [5];
        
        retval = gtk_frame_new (_("Power Management"));
        frame = gtk_frame_new (NULL);
        gtk_container_border_width (GTK_CONTAINER (frame), GNOME_PAD_SMALL);
        gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_NONE);

        vbox = gtk_vbox_new (FALSE, GNOME_PAD_SMALL);
        tempvbox = gtk_vbox_new (FALSE, 0);
        dpmscheck = get_and_set_dpmscheck (tempvbox);
        gtk_box_pack_start (GTK_BOX (vbox), dpmscheck, FALSE, FALSE, 0);
        hbox = gtk_hbox_new (FALSE, 0);
        gtk_box_pack_start (GTK_BOX (hbox), gtk_label_new (_("Shutdown Monitor ")), FALSE, FALSE, 0);
        entry = get_and_set_dpmsmin ();
        gtk_box_pack_start (GTK_BOX (hbox), entry, FALSE, FALSE, 0);
        gtk_box_pack_start (GTK_BOX (hbox), gtk_label_new (_(" Mins.")), FALSE, FALSE, 0);
        gtk_box_pack_start (GTK_BOX (tempvbox), hbox, FALSE, FALSE, 0);
        hbox = gtk_hbox_new (FALSE, 0);
        label = gtk_label_new (_("after screen saver has started."));
        gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
        gtk_box_pack_start (GTK_BOX (tempvbox), hbox, FALSE, FALSE, 0);
        gtk_box_pack_start (GTK_BOX (vbox), tempvbox, FALSE, FALSE, 0);

        gtk_container_add (GTK_CONTAINER (frame), vbox);
        gtk_container_add (GTK_CONTAINER (retval), frame);
        return retval;
}
void
screensaver_setup ()
{
        GtkWidget *hbox, *vbox, *bottom;
        GtkWidget *align;
        GtkWidget *saver, *power;
        GtkWidget *frame;
        

        capplet = capplet_widget_new ();
        vbox = gtk_vbox_new (FALSE, 0);
	hbox = gtk_hbox_new (FALSE, 0);
        align = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
        frame = gtk_frame_new (NULL);
        gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
        
        
        monitor = gtk_drawing_area_new ();
        gtk_widget_set_usize (monitor, 200, 150);
        gtk_container_add (GTK_CONTAINER (frame), monitor);

	gtk_container_border_width (GTK_CONTAINER (hbox), GNOME_PAD_SMALL);
	bottom = gtk_hbox_new (FALSE, GNOME_PAD_SMALL);
	gtk_container_border_width (GTK_CONTAINER (bottom), GNOME_PAD_SMALL);
        saver = get_saver_frame ();
        power = get_power_frame ();

        gtk_container_add (GTK_CONTAINER (align), frame);
	gtk_box_pack_start (GTK_BOX(hbox), align, TRUE, TRUE, 0);
        gtk_box_pack_start (GTK_BOX (bottom), saver, TRUE, TRUE, 0);
        gtk_box_pack_start (GTK_BOX (bottom), power, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
	gtk_box_pack_end (GTK_BOX (vbox), bottom, TRUE, TRUE, 0);

        gtk_container_add (GTK_CONTAINER (capplet), vbox);
        gtk_widget_show_all (capplet);
        
}


void
main (int argc, char **argv)
{
        gnome_capplet_init ("Screensaver Properties", NULL, argc, argv, 0, NULL);
        screensaver_load ();
	screensaver_setup();
        signal (SIGCHLD, sig_child);
        gtk_signal_connect(GTK_OBJECT(capplet), "destroy", GTK_SIGNAL_FUNC(destroy_callback), NULL);
        gtk_signal_connect (GTK_OBJECT (capplet), "try", GTK_SIGNAL_FUNC (try_callback), NULL);
        gtk_signal_connect (GTK_OBJECT (capplet), "revert", GTK_SIGNAL_FUNC (revert_callback), NULL);
        gtk_signal_connect (GTK_OBJECT (capplet), "ok", GTK_SIGNAL_FUNC (ok_callback), NULL);
        gtk_signal_connect (GTK_OBJECT (monitor), "expose_event", (GtkSignalFunc) list_click_callback, sd);
        sd = NULL;

        capplet_gtk_main ();
}
