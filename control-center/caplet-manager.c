/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
#include "caplet-manager.h"
#include <gnome.h>
#include <gdk/gdkx.h>

extern GtkWidget *container;
extern GtkWidget *notebook;
extern GtkWidget *splash_screen;
extern gchar *ior;
static gint current_page = 0;
static GPtrArray *cman = NULL; /* an array of caplet entries.
                               each one is our pointer to 
                               the caplet */

typedef struct _caplet_entry caplet_entry;
struct _caplet_entry
{
        GtkWidget *socket;
        CORBA_Object *cco;
        gboolean alive;
};

void
request_new_id (CORBA_Object *cco, CORBA_unsigned_long *xid, CORBA_short *id)
{
        caplet_entry *cen;
        if (cman == NULL)
                cman = g_ptr_array_new ();
        cen = (g_malloc (sizeof (caplet_entry)));
        cen->cco = cco;
        cen->alive = FALSE;
        g_ptr_array_add (cman, cen);
        *xid = GDK_WINDOW_XWINDOW (cen->socket->window);
        *id = g_array_length (cman, GPtrArray);
}
  
void
launch_caplet (node_data *data)
{
        gushort id;
        guint xid;
        gchar *argv[3];
        gchar *temp;
        GtkWidget *vbox;
        GtkWidget *separator;
        GtkWidget *bbox;
        GtkWidget *button;
        
        /* set up the notebook if needed */
        if (notebook == NULL) {
                notebook = gtk_notebook_new();
                gtk_container_remove (GTK_CONTAINER (container), splash_screen);
                gtk_container_border_width (GTK_CONTAINER (container), 5);
                gtk_container_add (GTK_CONTAINER (container), notebook);
                gtk_widget_show (notebook);
        }
        /* set up the layout if needed */
        if (data->id == -1) {
                vbox = gtk_vbox_new(FALSE, 5);
                data->socket = gtk_socket_new ();
                separator = gtk_hseparator_new ();
                bbox = gtk_hbutton_box_new ();
                gtk_button_box_set_layout (GTK_BUTTON_BOX (bbox), GTK_BUTTONBOX_SPREAD);
                gtk_container_border_width (GTK_CONTAINER (bbox), 5);

                button = gtk_button_new_with_label ("Try");
                gtk_container_add (GTK_CONTAINER (bbox), button);
                button = gtk_button_new_with_label ("Revert");
                gtk_container_add (GTK_CONTAINER (bbox), button);
                button = gtk_button_new_with_label ("OK");
                gtk_container_add (GTK_CONTAINER (bbox), button);
                button = gtk_button_new_with_label ("Cancel");
                gtk_container_add (GTK_CONTAINER (bbox), button);
                button = gtk_button_new_with_label ("Help");
                gtk_container_add (GTK_CONTAINER (bbox), button);
       
                
                /* put it all together */
                gtk_box_pack_start (GTK_BOX (vbox), data->socket, TRUE, TRUE, 0);
                gtk_box_pack_end (GTK_BOX (vbox), bbox, FALSE, FALSE, 5);
                gtk_box_pack_end (GTK_BOX (vbox), separator, FALSE, FALSE, 0);
                gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox, gtk_label_new (data->gde->name));
                gtk_widget_show_all (vbox);
                data->id = current_page++;
        }
        gtk_notebook_set_page (GTK_NOTEBOOK (notebook), data->id);

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
}
