/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
#include "caplet-manager.h"
#include "callbacks.h"
#include <gnome.h>
#include <gdk/gdkx.h>

/* variables */
extern GtkWidget *container;
extern GtkWidget *notebook;
extern GtkWidget *splash_screen;
extern gchar *ior;
static gint current_page = 0;
GList *caplet_list = NULL;

/* typedefs... */
typedef struct _caplet_entry caplet_entry;
struct _caplet_entry
{
        GtkWidget *socket;
        CORBA_Object *cco;
        gboolean alive;
};

void
launch_caplet (node_data *data)
{
        static gint id = 0;
        gchar *argv[3];
        gchar *temp;
        GtkWidget *vbox;
        GtkWidget *separator;
        GtkWidget *bbox;
        GtkWidget *button;
        
        /* set up the notebook if needed */
        if (notebook == NULL) {
                notebook = gtk_notebook_new();
                gtk_notebook_set_show_tabs (GTK_NOTEBOOK (notebook), TRUE);
                gtk_notebook_set_scrollable (GTK_NOTEBOOK (notebook), TRUE);
                gtk_container_remove (GTK_CONTAINER (container), splash_screen);
                gtk_container_border_width (GTK_CONTAINER (container), 5);
                gtk_container_add (GTK_CONTAINER (container), notebook);
                gtk_widget_show (notebook);
        }
        
        /* This caplet has not been started yet.  We need to do that. */
        if (data->id == -1) {
                vbox = gtk_vbox_new(FALSE, 5);
                data->socket = gtk_socket_new ();
                separator = gtk_hseparator_new ();
                bbox = gtk_hbutton_box_new ();
                gtk_button_box_set_layout (GTK_BUTTON_BOX (bbox), GTK_BUTTONBOX_SPREAD);
                gtk_container_border_width (GTK_CONTAINER (bbox), 5);

                button = gtk_button_new_with_label ("Try");
                gtk_container_add (GTK_CONTAINER (bbox), button);
                gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (try_button_callback), data);
                button = gtk_button_new_with_label ("Revert");
                gtk_container_add (GTK_CONTAINER (bbox), button);
                gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (revert_button_callback), data);
                button = gtk_button_new_with_label ("OK");
                gtk_container_add (GTK_CONTAINER (bbox), button);
                gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (ok_button_callback), data);
                button = gtk_button_new_with_label ("Cancel");
                gtk_container_add (GTK_CONTAINER (bbox), button);
                gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (cancel_button_callback), data);
                button = gtk_button_new_with_label ("Help");
                gtk_container_add (GTK_CONTAINER (bbox), button);
                gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (help_button_callback), data);
       
                
                /* put it all together */
                gtk_box_pack_start (GTK_BOX (vbox), data->socket, TRUE, TRUE, 0);
                gtk_box_pack_end (GTK_BOX (vbox), bbox, FALSE, FALSE, 5);
                gtk_box_pack_end (GTK_BOX (vbox), separator, FALSE, FALSE, 0);
                gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox, gtk_label_new (data->gde->name));
                gtk_widget_show_all (vbox);
                data->notetab_id = current_page++;
                data->id = id++;

                /* set up the arguments for the caplet */
                temp = g_malloc (sizeof (char[11]));
                sprintf (temp, "--id=");
                sprintf (temp + 5, "%d", data->id);
                argv[0] = temp;
        
                temp = g_malloc (sizeof (char[17]));
                sprintf (temp, "--xid=");
                sprintf (temp + 6, "%d",  GDK_WINDOW_XWINDOW (data->socket->window));
                argv[1] = temp;

                temp = g_malloc (sizeof (gchar[7 + strlen(ior)]));
                sprintf (temp, "--ior=");
                sprintf (temp + 6, "%s",  ior);
                argv[2] = temp;
                //        gnome_desktop_entry_launch_with_args (data->gde, 3, argv);
                
                caplet_list = g_list_prepend (caplet_list, data);
        }
        gtk_notebook_set_page (GTK_NOTEBOOK (notebook), data->notetab_id);

}
