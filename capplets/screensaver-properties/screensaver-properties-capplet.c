/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* Copyright (C) 1998 Redhat Software Inc. 
 * Authors: Jonathan Blandford <jrb@redhat.com>
 */
#include <config.h>
#include "capplet-widget.h"
#include "gnome.h"
#include "callbacks.h"
#include "parser.h"
#include <unistd.h>
#include <signal.h>
#include <dirent.h>

/* prototypes */
static GtkWidget *get_saver_frame ();
static GtkWidget *get_demo_frame ();
static GtkWidget *get_settings_frame ();
static void screensaver_setup ();

extern gboolean ignore_changes;

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
static GtkWidget *
get_saver_frame ()
{
        GtkWidget *retval;
        GtkWidget *vbox;
        GtkWidget *swindow;
        GtkWidget *temphbox;

        retval = gtk_frame_new (_("Screen Saver"));
        vbox = gtk_vbox_new (FALSE, GNOME_PAD_SMALL);
        gtk_container_set_border_width (GTK_CONTAINER (vbox), GNOME_PAD_SMALL);
        gtk_container_add (GTK_CONTAINER (retval), vbox);
        
        ignore_changes = TRUE;

	swindow = gtk_scrolled_window_new (NULL, NULL);
        gtk_widget_set_usize (swindow, 150, -1 );
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (swindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (swindow), get_and_set_mode());
        gtk_box_pack_start (GTK_BOX (vbox), swindow, TRUE, TRUE, 0);

        setup_label = gtk_label_new (_("Settings..."));
        setup_button = gtk_button_new ();
        gnome_widget_add_help (setup_button, _("Pressing this button will popup a dialog"\
                                               "box that will help you setup the current screensaver."));
        gtk_container_add (GTK_CONTAINER (setup_button), setup_label);
        gtk_signal_connect (GTK_OBJECT (setup_button), "clicked", (GtkSignalFunc) setup_callback, NULL);
        temphbox = gtk_hbox_new (FALSE, 0);
        gtk_box_pack_end (GTK_BOX (temphbox), setup_button, FALSE, FALSE, 0);
        gtk_box_pack_start (GTK_BOX (vbox), temphbox, FALSE, FALSE, 0);

        ignore_changes = FALSE;

        return retval;
}
static GtkWidget *
get_settings_frame()
{
        GtkWidget *retval;
        GtkWidget *frame, *vbox, *dpmscheck, *hbox, *label, *tempvbox;
        GtkWidget *temphbox, *alignment, *ssentry, *pword, *adjustment;
        GtkWidget *evbox;
        GtkWidget *nice;

        /* we set this to true to avoid interfering callbacks... */
        ignore_changes = TRUE;


        /* set up the initial frame, and get a vbox in which to pack things. */
        retval = gtk_frame_new (_("Screen Saver Settings"));
        vbox = gtk_vbox_new (FALSE, GNOME_PAD_SMALL);
        gtk_container_set_border_width (GTK_CONTAINER (vbox), GNOME_PAD_SMALL);
        gtk_container_add (GTK_CONTAINER (retval), vbox);

        /* the regular settings part */
        temphbox = gtk_hbox_new (FALSE, 0);
        alignment = gtk_alignment_new (0.0, 0.5, 0, 0);
        label = gtk_label_new (_("Start After "));
        ssentry = get_and_set_min_entry ();
        gnome_widget_add_help (ssentry, "This determines how long the system will "\
                                           "wait before starting the selected Screen Saver.");
        gtk_box_pack_start (GTK_BOX (temphbox), label, FALSE, FALSE, 0);
        gtk_box_pack_start (GTK_BOX (temphbox), ssentry, FALSE, FALSE, 0);
        label = gtk_label_new (_(" Minutes."));
        gtk_box_pack_start (GTK_BOX (temphbox), label, FALSE, FALSE, 0);
        gtk_container_add (GTK_CONTAINER (alignment), temphbox);
        gtk_box_pack_start (GTK_BOX (vbox), alignment, FALSE, FALSE, 0);


        pword = get_and_set_pword ();
        gtk_box_pack_start (GTK_BOX (vbox), pword, FALSE, FALSE, 0);
        
        evbox = gtk_event_box_new ();
        temphbox = gtk_hbox_new (FALSE, 0);
        tempvbox = gtk_vbox_new (FALSE, 0);
        gtk_container_add (GTK_CONTAINER (evbox), tempvbox);
        gnome_widget_add_help (evbox, "This slider will determine how much processer power the " \
                               "Screen Saver will use\n testing a paragraph.");
        gtk_box_pack_start (GTK_BOX (temphbox), gtk_label_new (_("Priority:")), FALSE, FALSE, 0);
        gtk_box_pack_start (GTK_BOX (tempvbox), temphbox, FALSE, FALSE, 0);
        temphbox = gtk_hbox_new (FALSE, 0);
	adjustment = GTK_OBJECT(get_and_set_nice ());
					 
	nice = gtk_hscale_new (GTK_ADJUSTMENT (adjustment));
        gtk_scale_set_draw_value (GTK_SCALE (nice), FALSE);
        gtk_box_pack_start (GTK_BOX (temphbox), gtk_label_new (_("Low ")), FALSE, FALSE, 0);
        gtk_box_pack_start (GTK_BOX (temphbox), nice, TRUE, TRUE, 0);
        gtk_box_pack_start (GTK_BOX (temphbox), gtk_label_new (_(" Normal")), FALSE, FALSE, 0);
        gtk_box_pack_start (GTK_BOX (tempvbox), temphbox, FALSE, FALSE, 0);
        gtk_box_pack_start (GTK_BOX (vbox), evbox, FALSE, FALSE, 0);

        
        gtk_box_pack_start (GTK_BOX (vbox), gtk_hseparator_new (), FALSE, FALSE, 0);
        /* the power mangement part */
        hbox = gtk_hbox_new (FALSE, 0);
        dpmscheck = get_and_set_dpmscheck (hbox);
        gtk_box_pack_start (GTK_BOX (vbox), dpmscheck, FALSE, FALSE, 0);

        gtk_box_pack_start (GTK_BOX (hbox), gtk_label_new (_("Shutdown monitor ")), FALSE, FALSE, 0);
        gtk_box_pack_start (GTK_BOX (hbox), get_and_set_dpmsmin (), FALSE, FALSE, 0);
        gtk_box_pack_start (GTK_BOX (hbox), gtk_label_new (_(" minutes after screen saver has started.")), FALSE, FALSE, 0);
        gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

        ignore_changes = FALSE;

        return retval;
}
static GtkWidget *
get_demo_frame()
{
        GtkWidget *retval;
        GtkWidget *inner_frame;

        retval = gtk_frame_new (_("Screen Saver Demo"));
        inner_frame = gtk_frame_new (NULL);
        gtk_frame_set_shadow_type (GTK_FRAME (inner_frame), GTK_SHADOW_IN);

        monitor = gtk_drawing_area_new ();
        gtk_widget_set_usize (monitor, 200, 150);
        
	gtk_container_set_border_width (GTK_CONTAINER (inner_frame), GNOME_PAD_SMALL);
        gtk_container_add (GTK_CONTAINER (retval), inner_frame);
        gtk_container_add (GTK_CONTAINER (inner_frame), monitor);
        return retval;
}
static void
screensaver_setup ()
{
        GtkWidget *hbox, *vbox, *bottom;
        GtkWidget *demo, *settings, *saver;
        

        capplet = capplet_widget_new ();
        vbox = gtk_vbox_new (FALSE, 0);
	hbox = gtk_hbox_new (FALSE, GNOME_PAD);

	gtk_container_set_border_width (GTK_CONTAINER (hbox), GNOME_PAD_SMALL);
	bottom = gtk_hbox_new (FALSE, GNOME_PAD_SMALL);
	gtk_container_set_border_width (GTK_CONTAINER (bottom), GNOME_PAD_SMALL);
        demo = get_demo_frame ();
        saver = get_saver_frame ();
        settings = get_settings_frame ();

	gtk_box_pack_start (GTK_BOX(hbox), saver, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX(hbox), demo, FALSE, FALSE, 0);
        gtk_box_pack_start (GTK_BOX (bottom), settings, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
	gtk_box_pack_end (GTK_BOX (vbox), bottom, FALSE, FALSE, 0);

        gtk_container_add (GTK_CONTAINER (capplet), vbox);
        gtk_widget_show_all (capplet);
}

int
main (int argc, char **argv)
{

        gchar *temp = strrchr(argv[0],'/');

        if (!temp)
                temp = argv [0];
        else
                temp++;
        if (strcmp (temp, "screensaver-properties-init")== 0) {
                gnome_init ("screensaver-properties", VERSION, argc, argv);
                temp = gnome_config_get_string ("/Screensaver/Default/command=xscreensaver");
                system ("xscreensaver-command -exit");
                system (temp);
                return 0;
        }

        bindtextdomain (PACKAGE, GNOMELOCALEDIR);
        textdomain (PACKAGE);

        gnome_capplet_init ("screensaver-properties", VERSION, argc,
                            argv, NULL, 0, NULL);

        screensaver_load ();
	screensaver_setup();
        signal (SIGCHLD, sig_child);
        gtk_signal_connect(GTK_OBJECT(capplet), "destroy", GTK_SIGNAL_FUNC(destroy_callback), NULL);
        gtk_signal_connect (GTK_OBJECT (capplet), "try", GTK_SIGNAL_FUNC (try_callback), NULL);
        gtk_signal_connect (GTK_OBJECT (capplet), "revert", GTK_SIGNAL_FUNC (revert_callback), NULL);
        gtk_signal_connect (GTK_OBJECT (capplet), "cancel", GTK_SIGNAL_FUNC (revert_callback), NULL);
        gtk_signal_connect (GTK_OBJECT (capplet), "ok", GTK_SIGNAL_FUNC (ok_callback), NULL);
        gtk_signal_connect (GTK_OBJECT (monitor), "expose_event", (GtkSignalFunc) list_expose_callback, sd);  /* disabled for now */
        sd = NULL;

        capplet_gtk_main ();
	return 0;
}
