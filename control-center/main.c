/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* Copyright (C) 1998 Redhat Software Inc. 
 * Authors: Jonathan Blandford <jrb@redhat.com>
 */
#include <config.h>
#include <gnome.h>
#include <gdk/gdkkeysyms.h>
#include <stdio.h>
#include "callbacks.h"
#include "tree.h"
#include "corba-glue.h"
GtkWidget *main_window;
GtkWidget *status_bar;
GtkWidget *exit_dialog;
GtkWidget *hpane;
GtkWidget *container;
GtkWidget *notebook = NULL;
GtkWidget *splash_screen;
static GnomeUIInfo mainMenu[] = {
        {GNOME_APP_UI_HELP, NULL, NULL, NULL, NULL, NULL,
         GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL},
        {GNOME_APP_UI_ITEM, N_("Exit"), NULL, exit_callback, NULL, NULL,
         GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_ABOUT, 0, 0, NULL},
        {GNOME_APP_UI_ENDOFINFO, NULL, NULL, NULL, NULL, NULL,
         GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL}
};
static GnomeUIInfo helpMenu[] = {
        {GNOME_APP_UI_HELP, NULL, NULL, NULL, NULL, NULL,
         GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL},
        {GNOME_APP_UI_ITEM, N_("Help with GNOME..."), NULL, help_callback, NULL, NULL,
         GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_ABOUT, 0, 0, NULL},
        {GNOME_APP_UI_ITEM, N_("About..."), NULL, about_callback, NULL, NULL,
         GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_ABOUT, 0, 0, NULL},
        {GNOME_APP_UI_ENDOFINFO, NULL, NULL, NULL, NULL, NULL,
         GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL}
};
static GnomeUIInfo parentMenu[] = {
        {GNOME_APP_UI_SUBTREE, N_("Main"), NULL, mainMenu, NULL, NULL,
         GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL},
        {GNOME_APP_UI_SUBTREE, N_("Help"), NULL, helpMenu, NULL, NULL,
         GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL},
        {GNOME_APP_UI_ENDOFINFO}
};
GtkWidget *
create_exit_dialog (GList *apps)
{
        gchar *text[2];
        GtkWidget *hbox;
        GtkWidget *right_vbox;
        GtkWidget *retval;
        GtkWidget *label;
        GtkWidget *list;
        GtkWidget *pixmap = NULL;
        char *s;

        /* we create the widgets */
        retval = gnome_dialog_new ("Warning:", GNOME_STOCK_BUTTON_OK, GNOME_STOCK_BUTTON_CANCEL, NULL);
        gnome_dialog_set_default (GNOME_DIALOG (retval), 1);
        gnome_dialog_set_parent (GNOME_DIALOG (retval), GTK_WINDOW (main_window));

        /*...containers */
        hbox = gtk_hbox_new (FALSE, 5);
        right_vbox = gtk_vbox_new (FALSE, 5);

        /*...labels, etc */
        label = gtk_label_new (" The following modules have had changes made,\n but not committed. " \
                               " If you would like to edit them, please\n click on the appropriate entry.");
        gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);

        /* stolen from gnome-message-box (: */
        s = gnome_pixmap_file("gnome-warning.png");
        if (s)
                pixmap = gnome_pixmap_new_from_file(s);
        if ( (pixmap == NULL) ||
             (GNOME_PIXMAP(pixmap)->pixmap == NULL) ) {
                if (pixmap) gtk_widget_destroy(pixmap);
                s = gnome_pixmap_file("gnome-default.png");
                if (s)
                        pixmap = gnome_pixmap_new_from_file(s);
                else
                        pixmap = NULL;
        }

        /*...the list */
        list = gtk_clist_new (1);
        gtk_clist_set_selection_mode (GTK_CLIST (list), GTK_SELECTION_BROWSE);
        gtk_clist_set_policy (GTK_CLIST (list), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
        text[1] = NULL;
        for (;apps ;apps = apps->next) {
                text[0] = (gchar *)apps->data;
                gtk_clist_append (GTK_CLIST (list), text);
        }

        /* and put it all together */
        gtk_box_pack_start (GTK_BOX (right_vbox), label, FALSE, FALSE, 5);
        gtk_box_pack_start (GTK_BOX (right_vbox), list, FALSE, FALSE, 5);
        if (pixmap)
                gtk_box_pack_start (GTK_BOX(hbox), pixmap, FALSE, TRUE, 10);
        gtk_box_pack_start (GTK_BOX ( GNOME_DIALOG (retval)->vbox), hbox, FALSE, FALSE, 5);
        gtk_box_pack_start (GTK_BOX (hbox), right_vbox, FALSE, FALSE, 5);

        /* and shake it all about... */
        gtk_widget_show (label);
        gtk_widget_show (list);
        if (pixmap)
                gtk_widget_show (pixmap);
        gtk_widget_show (right_vbox);
        gtk_widget_show (hbox);
        
        return retval;
}
GtkWidget *
create_window ()
{
        GtkWidget *vbox;
        GtkWidget *retval;
        GtkWidget *tree;
        GdkPixmap *temp_splash;

        /* create the app */
        retval = gnome_app_new ("desktop-manager", "Desktop Manager");
	gtk_signal_connect (GTK_OBJECT (retval), "delete_event",
                            GTK_SIGNAL_FUNC (exit_callback), retval);
	gnome_app_create_menus (GNOME_APP (retval), parentMenu);
        exit_dialog = NULL;

        /* create the components */
        vbox = gtk_vbox_new (FALSE,0);
        hpane = gtk_hpaned_new();
        gtk_paned_handle_size (GTK_PANED (hpane), 10);
        gtk_paned_gutter_size (GTK_PANED (hpane), 10);
        tree = generate_tree();
        container = gtk_frame_new(NULL);
        splash_screen = gtk_drawing_area_new ();
        gdk_imlib_load_file_to_pixmap ("splash.png", &temp_splash, NULL);
        gtk_widget_set_usize (container, 500, 375);
        status_bar = gtk_statusbar_new();

        /* we put it all together... */
        gtk_paned_add1 (GTK_PANED (hpane), tree);
        gtk_container_add (GTK_CONTAINER (container), splash_screen);
        gtk_paned_add2 (GTK_PANED (hpane), container);
        gtk_box_pack_end (GTK_BOX (vbox), status_bar, FALSE, FALSE, 0);
        gtk_box_pack_end (GTK_BOX (vbox), hpane, TRUE, TRUE, 0);
        gnome_app_set_contents(GNOME_APP(retval), vbox);
        
        /* and make everyting visible */
        gtk_widget_show_all (retval);
        gdk_window_set_back_pixmap (splash_screen->window, temp_splash, FALSE);
        return retval;
}
gint
main (int argc, char *argv[])
{
        gnome_init("desktop-manager", NULL, argc, argv, 0, NULL);

        main_window = create_window ();
        control_center_corba_gtk_main (&argc, argv);
        return 0;
}
