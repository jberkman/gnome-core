/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * Author: Elliot Lee <sopwith@redhat.com>
 */

#define TESTING

#include <config.h>
#include <stdio.h>
#include <stdarg.h>
#include <gtk/gtk.h>
#include "capplet-widget.h"

#include <gnome.h>

typedef struct {
    GtkWidget *capplet;

    /* General setup */
    GtkWidget *enable_esd_startup, *enable_sound_events;

    /* Sound events setup */
    GtkWidget *ctree;
    GHashTable *entries;
} SoundProps;

typedef struct {
    gchar *category;
    gchar *name;
    gchar *file;
    GtkCTreeNode *row;
} SoundEvent;

static void sound_properties_regenerate_ctree(SoundProps *props);
static SoundProps *sound_properties_create(void);
static void sound_properties_event_free(SoundEvent *ev);
static void sound_properties_read_path(SoundProps *props,
                                       GString *tmpstr,
                                       const char *path);

int
main(int argc,
     char *argv[])
{
    SoundProps *sound_properties;

#ifdef TESTING
    gnome_init ("sound-properties", NULL, argc, argv, 0, NULL);
#else
    gnome_capplet_init ("sound-properties", NULL, argc, argv, 0, NULL);
#endif

    sound_properties = sound_properties_create();

#ifdef TESTING
    gtk_main();
#else
    capplet_gtk_main();
#endif

    return 0;
}

/**** sound_properties_create
      Outputs: 'retval' - info on newly created capplet thingie.
 */
static SoundProps *
sound_properties_create(void)
{
    SoundProps *retval;
    GtkWidget *table, *wtmp, *notebook;
    static const char *ctree_column_titles[] = {
        "Category",
        "Event",
        "File to Play"
    };

    retval = g_new0(SoundProps, 1);

#ifdef TESTING
    retval->capplet = gtk_window_new(GTK_WINDOW_TOPLEVEL);    
#else
    retval->capplet = capplet_widget_new();
#endif

    notebook = gtk_notebook_new();

    /* * * * page "General" * * * */
    wtmp = gtk_vbox_new(FALSE, GNOME_PAD_SMALL);

    gtk_container_add(GTK_CONTAINER(wtmp),
                      (retval->enable_esd_startup =
                       gtk_check_button_new_with_label("Enable GNOME sound support")));

    gtk_container_add(GTK_CONTAINER(wtmp),
                      (retval->enable_sound_events =
                       gtk_check_button_new_with_label("Enable sounds for events")));

    gtk_widget_set_sensitive(retval->enable_esd_startup, FALSE);
    gtk_widget_set_sensitive(retval->enable_sound_events, FALSE);

    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), wtmp,
                             gtk_label_new("General"));

    /* * * * page "Sound Events" * * * */
    table = gtk_table_new(3, 2, FALSE);
    retval->ctree = gtk_ctree_new_with_titles(3, 0,
                                              (gchar **)ctree_column_titles);

    sound_properties_regenerate_ctree(retval);

    gtk_table_attach_defaults(GTK_TABLE(table), retval->ctree, 0, 1, 0, 3);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), table,
                             gtk_label_new("Sound Events"));

    /* * * * end notebook page setup * * * */

    gtk_container_add(GTK_CONTAINER(retval->capplet), notebook);

    gtk_widget_show_all(retval->capplet);

    return retval;
}

/**** sound_properties_regenerate_ctree

      Inputs: 'ctree' - a GtkCTree to put the entries into.

      Side effects: Clears & populates 'ctree'.

      Description: clears out 'ctree' and then puts all the available
                   data into rows.
 */
static void
sound_properties_regenerate_ctree(SoundProps *props)
{
    GString *tmpstr = g_string_new(NULL);

    gtk_clist_freeze(GTK_CLIST(props->ctree));
    gtk_clist_clear(GTK_CLIST(props->ctree));

    sound_properties_read_path(props, tmpstr, "/sound/apps");

    g_string_free(tmpstr, TRUE);
    gtk_clist_thaw(GTK_CLIST(props->ctree));
}

static void
sound_properties_event_free(SoundEvent *ev)
{
    g_free(ev->category); g_free(ev->file); g_free(ev->name);

    g_free(ev);
}

/**** sound_properties_read_path
 */
static void
sound_properties_read_path(SoundProps *props,
                           GString *tmpstr,
                           const char *path)
{
    SoundEvent *new_entry;
    char *category_name, *sample_name, *sample_file, *ctmp;
    gpointer top_iter, event_iter;
    GtkCTreeNode *category_node, *event_node;
    char *arow[3] = {NULL,NULL,NULL};

    top_iter = gnome_config_init_iterator_sections(path);
    while((top_iter = gnome_config_iterator_next(top_iter,
                                                 NULL, &category_name))) {

        g_message("Got category %s", category_name);
        g_string_sprintf(tmpstr, "%s/%s/events", path, category_name);

        arow[0] = category_name; arow[1] = NULL; arow[2] = NULL;
        category_node = gtk_ctree_insert_node(GTK_CTREE(props->ctree),
                                              NULL, NULL,
                                              arow, GNOME_PAD_SMALL,
                                              NULL, NULL, NULL, NULL, FALSE,
                                              FALSE);
        gnome_config_push_prefix(tmpstr->str);

        event_node = NULL;
        event_iter = gnome_config_init_iterator("/");
        while((event_iter = gnome_config_iterator_next(event_iter,
                                                       &sample_name,
                                                       &sample_file))) {
            g_message("Got event %s -> %s in category %s",
                      sample_name, sample_file,
                      category_name);
            arow[0] = NULL; arow[1] = sample_name; arow[2] = sample_file;
            event_node = gtk_ctree_insert_node(GTK_CTREE(props->ctree),
                                               NULL, event_node,
                                               arow, GNOME_PAD_SMALL,
                                               NULL, NULL, NULL, NULL, TRUE,
                                               FALSE);
            new_entry = g_new0(SoundEvent, 1);
            new_entry->category = g_strdup(category_name);
            new_entry->name = sample_name;
            new_entry->file = sample_file;
            new_entry->row = event_node;
            gtk_ctree_node_set_row_data_full(GTK_CTREE(props->ctree), event_node,
                                             new_entry,
                                             (GtkDestroyNotify)sound_properties_event_free);
        }

        gnome_config_pop_prefix();

        g_free(category_name);
    }
}
