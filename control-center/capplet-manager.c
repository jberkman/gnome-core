/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
#include "capplet-manager.h"
#include "callbacks.h"
#include "control-center.h"
#include <gnome.h>
#include <gdk/gdkx.h>

/* variables */
extern GtkWidget *container;
extern GtkWidget *notebook;
extern GtkWidget *main_window;
extern GtkWidget *splash_screen;
extern gchar *ior;
static gint current_page = 0;
static gint id = 0;
GList *capplet_list = NULL;
extern CORBA_Environment ev;


/* typedefs... */
typedef struct _capplet_entry capplet_entry;
struct _capplet_entry
{
        GtkWidget *socket;
        CORBA_Object *cco;
        gboolean alive;
};

/* prototypes */
void try_button_callback(GtkWidget *widget, gpointer data);
void revert_button_callback(GtkWidget *widget, gpointer data);
void ok_button_callback(GtkWidget *widget, gpointer data);
void cancel_button_callback(GtkWidget *widget, gpointer data);
void help_button_callback(GtkWidget *widget, gpointer data);
void close_capplet (GtkWidget *widget, gpointer data);
node_data *
find_node_by_id (gint id)
{
        GList *test;

        for (test = capplet_list; test; test = test->next)
                if (((node_data*)test->data)->id == id)
                        return (node_data*)test->data;
        return NULL;
                    
}
void
launch_capplet (node_data *data)
{
        gchar *argv[3];
        gchar *temp;
        GtkWidget *vbox;
        GtkWidget *separator;
        GtkWidget *bbox;
        
        /* set up the notebook if needed */
        /* This capplet has not been started yet.  We need to do that. */
        if ((data->id == -1) && (data->gde->exec_length)) {
                if (notebook == NULL) {
                        notebook = gtk_notebook_new();
                        gtk_notebook_set_show_tabs (GTK_NOTEBOOK (notebook), TRUE);
                        gtk_notebook_set_scrollable (GTK_NOTEBOOK (notebook), TRUE);
                        gtk_widget_ref (splash_screen);
                        gtk_container_remove (GTK_CONTAINER (container), splash_screen);
                        gtk_container_border_width (GTK_CONTAINER (container), 5);
                        gtk_container_add (GTK_CONTAINER (container), notebook);
                        gtk_widget_show (notebook);
                }
        
                vbox = gtk_vbox_new(FALSE, 5);
                data->socket = gtk_socket_new ();
                separator = gtk_hseparator_new ();
                bbox = gtk_hbutton_box_new ();
                gtk_button_box_set_layout (GTK_BUTTON_BOX (bbox), GTK_BUTTONBOX_SPREAD);
                gtk_container_border_width (GTK_CONTAINER (bbox), 5);

                data->try_button = gtk_button_new_with_label ("Try");
                gtk_widget_set_sensitive (data->try_button, FALSE);
                gtk_container_add (GTK_CONTAINER (bbox), data->try_button);
                gtk_signal_connect (GTK_OBJECT (data->try_button), "clicked", GTK_SIGNAL_FUNC (try_button_callback), data);

                data->revert_button = gtk_button_new_with_label ("Revert");
                gtk_widget_set_sensitive (data->revert_button, FALSE);
                gtk_container_add (GTK_CONTAINER (bbox), data->revert_button);
                gtk_signal_connect (GTK_OBJECT (data->revert_button), "clicked", GTK_SIGNAL_FUNC (revert_button_callback), data);

                data->ok_button = gtk_button_new_with_label ("OK");
                gtk_widget_set_sensitive (data->ok_button, FALSE);
                gtk_container_add (GTK_CONTAINER (bbox), data->ok_button);
                gtk_signal_connect (GTK_OBJECT (data->ok_button), "clicked", GTK_SIGNAL_FUNC (ok_button_callback), data);

                data->cancel_button = gtk_button_new_with_label ("Cancel");
                gtk_container_add (GTK_CONTAINER (bbox), data->cancel_button);
                gtk_signal_connect (GTK_OBJECT (data->cancel_button), "clicked", GTK_SIGNAL_FUNC (cancel_button_callback), data);
                data->help_button = gtk_button_new_with_label ("Help");
                gtk_container_add (GTK_CONTAINER (bbox), data->help_button);
                gtk_signal_connect (GTK_OBJECT (data->help_button), "clicked", GTK_SIGNAL_FUNC (help_button_callback), data);
                
                /* put it all together */
                gtk_box_pack_start (GTK_BOX (vbox), data->socket, TRUE, TRUE, 0);
                gtk_box_pack_end (GTK_BOX (vbox), bbox, FALSE, FALSE, 5);
                gtk_box_pack_end (GTK_BOX (vbox), separator, FALSE, FALSE, 0);
                data->label = gtk_label_new (data->gde->name);
                gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox, data->label);
                gtk_widget_show_all (vbox);
                data->notetab_id = current_page++;
                data->id = id++;

                /* set up the arguments for the capplet */
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
                
                gnome_desktop_entry_launch_with_args (data->gde, 3, argv);
                capplet_list = g_list_prepend (capplet_list, data);
        }
        if (data->notetab_id != -1)
                gtk_notebook_set_page (GTK_NOTEBOOK (notebook), data->notetab_id);
}


void try_button_callback(GtkWidget *widget, gpointer data)
{
        node_data *nd = (node_data *) data;
        gtk_widget_set_sensitive (nd->try_button, FALSE);

        GNOME_capplet_try (nd->capplet,nd->id, &ev);
}
void revert_button_callback(GtkWidget *widget, gpointer data)
{
        node_data *nd = (node_data *) data;
        GNOME_capplet_revert (nd->capplet,nd->id, &ev);
}
void ok_button_callback(GtkWidget *widget, gpointer data)
{
        node_data *nd = (node_data *) data;
        GNOME_capplet_ok (nd->capplet,nd->id, &ev);
        close_capplet (widget, data);
}
void cancel_button_callback(GtkWidget *widget, gpointer data)
{
        revert_button_callback (widget, data);
        close_capplet (widget, data);
}
void close_capplet (GtkWidget *widget, gpointer data)
{
        node_data *nd = (node_data *) data;
        GList *temp;

        gtk_notebook_remove_page (GTK_NOTEBOOK (notebook), nd->notetab_id);
        nd->id = -1;
        nd->modified = FALSE;
        capplet_list = g_list_remove (capplet_list, nd);
        if (nd->capplet) {
                CORBA_Object_release (nd->capplet, &ev);
                nd->capplet = NULL;
        }
        for (temp = capplet_list; temp; temp = temp->next)
                if (((node_data*)temp->data)->notetab_id > nd->notetab_id)
                        ((node_data*)temp->data)->notetab_id -=1;
                        
        if (--current_page == 0) {
                gtk_container_remove (GTK_CONTAINER (container), notebook);
                notebook = NULL;
                gtk_container_border_width (GTK_CONTAINER (container), 5);
                gtk_container_add (GTK_CONTAINER (container), splash_screen);
                g_list_remove (capplet_list, nd);
                gtk_widget_unref (splash_screen);
        }
}
void help_button_callback(GtkWidget *widget, gpointer data)
{
        node_data *nd = (node_data *) data;
        GNOME_capplet_help (nd->capplet,nd->id, &ev);
}
