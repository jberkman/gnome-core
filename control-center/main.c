/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* gcc `gnome-config --cflags gnomeui` `gnome-config --libs gnomeui` main.c callbacks.c tree.c -o desktop-manager */
/* Copyright (C) 1998 Redhat Software Inc. 
 * Authors: Jonathan Blandford <jrb@redhat.com>
 */
#include <config.h>
#include <gnome.h>
#include <gdk/gdkkeysyms.h>
#include <stdio.h>
#include <orb/orbit.h>
#include "callbacks.h"
#include "tree.h"
#include "control-center.h"

GtkWidget *main_window;
GtkWidget *config_window;
GtkWidget *status_bar;
GtkWidget *exit_dialog;
GtkWidget *cpw_socket;


CORBA_ORB orb = NULL;
CORBA_Environment ev;
GNOME_control_panel control_panel;
gchar *ior;
/* prototypes */


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
/* CORBA initialization... */
CORBA_short
GNOME_control_panel_cpo_request_id(GNOME_control_panel _obj,
                                   CORBA_char * cookie,
                                   CORBA_Environment * ev)
{
        g_print ("WooHoo!  got string:%s\n", cookie);
        fflush (stdout);
        return 1;
}

PortableServer_ServantBase__epv base_epv = {
        NULL,
        NULL,
        NULL
};
POA_GNOME_control_panel__epv control_panel_epv = 
{  
        NULL, 
        GNOME_control_panel_cpo_request_id,
        NULL,
        NULL,
};
/* Not really sure what these are for, but I'm sure they're necessary. ;-) */
POA_GNOME_control_panel__vepv poa_control_panel_vepv = { &base_epv, &control_panel_epv };
POA_GNOME_control_panel poa_control_panel_servant = { NULL, &poa_control_panel_vepv };

/* The following few functions are all glue for using ORBit inside of GTK+.  
 * Setting up gdk_input_add's, handlers for incoming data and 
 * exceptions. */
static void orb_handle_connection(GIOPConnection *cnx, gint source, GdkInputCondition cond)
{
        switch(cond) {
        case GDK_INPUT_EXCEPTION:
                giop_main_handle_connection_exception(cnx);
                break;
        default:
                giop_main_handle_connection(cnx);
        }
}

/* more glue */
static void
orb_add_connection(GIOPConnection *cnx)
{
        g_print ("in add_connection\n");
         cnx->user_data = (gpointer)gtk_input_add_full(GIOP_CONNECTION_GET_FD(cnx),
                                                      GDK_INPUT_READ|GDK_INPUT_EXCEPTION,
                                                      (GdkInputFunction)orb_handle_connection,
                                                      NULL, cnx, NULL);
}

static void
orb_remove_connection(GIOPConnection *cnx)
{
        g_print ("in remove_connection\n");
        gtk_input_remove((guint)cnx->user_data);
        cnx->user_data = (gpointer)-1;
}
void
control_panel_corba_gtk_main_quit(void)
{
       CORBA_ORB_shutdown(orb, CORBA_FALSE, &ev);
       gtk_main_quit();
}

void
control_panel_corba_gtk_init(gint *argc, char **argv)
{
        PortableServer_ObjectId objid = {0, sizeof("control_center_interface"), "control_center_interface"};
        PortableServer_POA poa;

        IIOPAddConnectionHandler = orb_add_connection;
        IIOPRemoveConnectionHandler = orb_remove_connection;
        CORBA_exception_init(&ev);
        orb = CORBA_ORB_init(argc, argv, "orbit-local-orb", &ev);

        POA_GNOME_control_panel__init(&poa_control_panel_servant, &ev);
        poa = orb->root_poa;
        PortableServer_POAManager_activate(PortableServer_POA__get_the_POAManager(poa, &ev), &ev);

        PortableServer_POA_activate_object_with_id(poa, 
                                                   &objid, &poa_control_panel_servant, &ev);
        control_panel = PortableServer_POA_servant_to_reference((PortableServer_POA)orb->root_poa, 
                                                      &poa_control_panel_servant, &ev);

        if (!control_panel) {
                g_error ("We cannot get objref\n");
                exit (1);
        }
        ior = CORBA_ORB_object_to_string(orb, control_panel, &ev);
       
        /* print IOR address so the client can use it to connect to us. */
        g_print ("%s\n", ior);fflush (stdout);
        
        CORBA_Object_release(control_panel, &ev);
        //        CORBA_ORB_run(orb, &ev);
        ORBit_custom_run_setup(orb, &ev);
}

void
control_panel_corba_gtk_main (gint *argc, char **argv)
{
        if(!orb)
                control_panel_corba_gtk_init( argc, argv);
        g_print ("calling gtk_main\n");
        gtk_main();
        g_print ("done calling gtk_main\n");
}
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
        /* stolen from gnome-message-box (:*/
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
        GtkWidget *hpane;
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
        config_window = gtk_drawing_area_new ();
        cpw_socket = gtk_socket_new ();
        gdk_imlib_load_file_to_pixmap ("splash.png", &temp_splash, NULL);
        gtk_widget_set_usize (config_window, 500, 375);
        gtk_widget_set_usize (cpw_socket, 500, 375);
        status_bar = gtk_statusbar_new();

        /* we put it all together... */
        gtk_paned_add1 (GTK_PANED (hpane), tree);
        //        gtk_paned_add2 (GTK_PANED (hpane), config_window);
        gtk_paned_add2 (GTK_PANED (hpane), cpw_socket);
        gtk_box_pack_end (GTK_BOX (vbox), status_bar, FALSE, FALSE, 0);
        gtk_box_pack_end (GTK_BOX (vbox), hpane, TRUE, TRUE, 0);
        gnome_app_set_contents(GNOME_APP(retval), vbox);
        
        /* and make everyting visible */
        gtk_widget_show (vbox);
        gtk_widget_show (hpane);
        gtk_widget_show (tree);
        gtk_widget_show (status_bar);
        gtk_widget_show (retval);
        gtk_widget_show (cpw_socket);
        gtk_widget_show (config_window);
        //gdk_window_set_back_pixmap (config_window->window, temp_splash, FALSE);
        gdk_window_set_back_pixmap (cpw_socket->window, temp_splash, FALSE);
        return retval;
}
gint
main (int argc, char *argv[])
{
        gnome_init("desktop-manager", NULL, argc, argv, 0, NULL);


        main_window = create_window ();
        control_panel_corba_gtk_main (&argc, argv);
        /* when does control_panel get set? */
        /*        CORBA_Object_release(control_panel, &ev); */
        return 0;
}
