/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * Author: Elliot Lee <sopwith@redhat.com>
 */

/* #define TESTING */

#include <config.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>
#include <dirent.h>
#include <gtk/gtk.h>
#include "capplet-widget.h"

#include <gnome.h>

typedef struct {
    GtkWidget *capplet;

    /* General setup */
    GtkWidget *enable_esd_startup, *enable_sound_events;

    /* Sound events setup */
    GtkWidget *table, *ctree;
    GHashTable *entries;

    GtkWidget *btn_filename, *btn_play;

    gint ignore_changed;
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
static void sound_properties_event_select(GtkCTree *ctree,
                                          GtkCTreeNode *row,
                                          gint column,
                                          SoundProps *props);
static void sound_properties_event_change_file(GtkEditable *entry,
                                               SoundProps *props);
static void sound_properties_set_sensitivity(GtkToggleButton *btn,
                                             SoundProps *props);
static void sound_properties_event_apply(GtkCTreeNode *node, SoundProps *props); 
static void sound_properties_apply(SoundProps *props); 
static void ui_do_revert(GtkWidget *w, SoundProps *props);
static void ui_do_ok(GtkWidget *w, SoundProps *props);
static void ui_do_cancel(GtkWidget *w, SoundProps *props);
static void sound_properties_play_sound(GtkWidget *btn, SoundProps *props); 

int
main(int argc,
     char *argv[])
{
    SoundProps *sound_properties;

    bindtextdomain (PACKAGE, GNOMELOCALEDIR);
    textdomain (PACKAGE);

#ifdef TESTING
    gnome_init ("sound-properties", VERSION, argc, argv);
#else
    gnome_capplet_init ("sound-properties", VERSION, argc, argv, NULL, 0, NULL);
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
        N_("Category"),
        N_("Event"),
        N_("File to Play")
    };
    gboolean btmp;

    retval = g_new0(SoundProps, 1);

#ifdef TESTING
    retval->capplet = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_policy(GTK_WINDOW(retval->capplet), TRUE, TRUE, TRUE);
    gtk_signal_connect(GTK_OBJECT(retval->capplet), "delete_event",
                       GTK_SIGNAL_FUNC(gtk_main_quit), NULL);
#else
    retval->capplet = capplet_widget_new();
#endif

    gtk_signal_connect(GTK_OBJECT(retval->capplet), "revert",
                       ui_do_revert, retval);
    gtk_signal_connect(GTK_OBJECT(retval->capplet), "ok",
                       ui_do_ok, retval);
    gtk_signal_connect(GTK_OBJECT(retval->capplet), "cancel",
                       ui_do_cancel, retval);

    notebook = gtk_notebook_new();

    /* * * * page "General" * * * */
    wtmp = gtk_vbox_new(FALSE, GNOME_PAD_SMALL);

    gtk_container_add(GTK_CONTAINER(wtmp),
                      (retval->enable_esd_startup =
                       gtk_check_button_new_with_label(_("Enable GNOME sound support"))));

    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(retval->enable_esd_startup),
                                gnome_config_get_bool("/sound/system/settings/start_esd"));

    gtk_container_add(GTK_CONTAINER(wtmp),
                      (retval->enable_sound_events =
                       gtk_check_button_new_with_label(_("Enable sounds for events"))));

    gtk_signal_connect(GTK_OBJECT(retval->enable_esd_startup),
                       "toggled",
                       GTK_SIGNAL_FUNC(sound_properties_set_sensitivity),
                       retval);

    gtk_signal_connect(GTK_OBJECT(retval->enable_sound_events),
                       "toggled",
                       GTK_SIGNAL_FUNC(sound_properties_set_sensitivity),
                       retval);

    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(retval->enable_sound_events),
                                gnome_config_get_bool("/sound/system/settings/event_sounds"));

    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), wtmp,
                             gtk_label_new(_("General")));

    /* * * * page "Sound Events" * * * */
    retval->table = table = gtk_table_new(2, 2, FALSE);
    /* FIXME: This needs to create the columns one at a time to enable i18n. */
    retval->ctree = gtk_ctree_new_with_titles(3, 0,
                                              (gchar **)ctree_column_titles);
    gtk_signal_connect(GTK_OBJECT(retval->ctree), "tree_select_row",
                       sound_properties_event_select, retval);

    gtk_clist_set_selection_mode(GTK_CLIST(retval->ctree), GTK_SELECTION_BROWSE);

    gtk_ctree_set_expander_style(GTK_CTREE(retval->ctree),
                                 GTK_CTREE_EXPANDER_SQUARE);

    sound_properties_regenerate_ctree(retval);

    /* Set size _after_ putting contents in */
#if 0
    gtk_clist_set_column_auto_resize(GTK_CLIST(retval->ctree), 0, TRUE);
    gtk_clist_set_column_auto_resize(GTK_CLIST(retval->ctree), 1, TRUE);
    gtk_clist_set_column_auto_resize(GTK_CLIST(retval->ctree), 2, TRUE);
#else
    gtk_clist_set_column_width(GTK_CLIST(retval->ctree), 0, 150);
    gtk_clist_set_column_width(GTK_CLIST(retval->ctree), 1, 200);
    gtk_clist_set_column_width(GTK_CLIST(retval->ctree), 2, 200);
#endif
    gtk_widget_set_usize(GTK_WIDGET(wtmp), 150+200+200+20,
                         250);

    gtk_container_set_border_width(GTK_CONTAINER(table), GNOME_PAD_SMALL);
    wtmp = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_usize(GTK_WIDGET(wtmp), 150+200+200+20,
                         250);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(wtmp), retval->ctree);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(wtmp), GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_ALWAYS);
    gtk_table_attach_defaults(GTK_TABLE(table), wtmp, 0, 2, 0, 1);
    gtk_table_attach_defaults(GTK_TABLE(table),
                              (retval->btn_play = gtk_button_new_with_label(_("Play"))),
                              0, 1, 1, 2);
    gtk_signal_connect(GTK_OBJECT(retval->btn_play), "clicked", sound_properties_play_sound, retval);

    gtk_table_attach_defaults(GTK_TABLE(table),
                              (retval->btn_filename = gnome_file_entry_new(NULL, _("Select sound file"))),
                              1, 2, 1, 2);
    gtk_signal_connect(GTK_OBJECT(gnome_file_entry_gtk_entry(GNOME_FILE_ENTRY(retval->btn_filename))),
                       "changed",
                       GTK_SIGNAL_FUNC(sound_properties_event_change_file),
                       retval);

    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), table,
                             gtk_label_new(_("Sound Events")));

    /* * * * end notebook page setup * * * */

    gtk_container_add(GTK_CONTAINER(retval->capplet), notebook);

    sound_properties_set_sensitivity(NULL, retval);

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
    char *ctmp, *ctmp2;

    gtk_clist_freeze(GTK_CLIST(props->ctree));
    gtk_clist_clear(GTK_CLIST(props->ctree));

    ctmp = gnome_config_file("/sound/events");
    if(ctmp)
        sound_properties_read_path(props, tmpstr, ctmp);
    g_free(ctmp);

    ctmp = gnome_util_home_file("sound/events");
    if(ctmp)
        sound_properties_read_path(props, tmpstr, ctmp);
    g_free(ctmp);

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
    DIR *dirh;
    SoundEvent *new_entry;
    char *category_name, *sample_name, *sample_file, *ctmp;
    gpointer top_iter, event_iter;
    GtkCTreeNode *category_node = NULL, *event_node = NULL;
    char *arow[4] = {NULL,NULL,NULL,NULL};
    struct dirent *dent;

    dirh = opendir(path);
    if(!dirh)
        return;

    while((dent = readdir(dirh))) {
	    if (!strcmp(dent->d_name, ".")
            || !strcmp(dent->d_name, ".."))
		    continue;

        g_string_sprintf(tmpstr, "=%s/%s=", path, dent->d_name);

        gnome_config_push_prefix(tmpstr->str);

        arow[1] = NULL; arow[2] = NULL;

        ctmp = gnome_config_get_translated_string("__section_info__/description");
        if(ctmp && *ctmp) {
            arow[0] = ctmp;
        } else {
            g_free(ctmp);
            arow[0] = g_strdup(dent->d_name);
            if(strstr(arow[0], ".soundlist")) {
                *strstr(arow[0], ".soundlist") = '\0';
            }
        }

        category_node = gtk_ctree_insert_node(GTK_CTREE(props->ctree),
                                              NULL, NULL,
                                              arow, GNOME_PAD_SMALL,
                                              NULL, NULL, NULL, NULL, FALSE,
                                              FALSE);
        gtk_ctree_node_set_selectable(GTK_CTREE(props->ctree), category_node,
                                      FALSE);

        g_free(arow[0]);

        event_node = NULL;
        event_iter = gnome_config_init_iterator_sections(tmpstr->str);
        while((event_iter = gnome_config_iterator_next(event_iter,
                                                       &sample_name,
                                                       NULL))) {
            if(!strcmp(sample_name, "__section_info__")) {
                g_free(sample_name);
                continue;
            }

            arow[0] = NULL;
            g_string_sprintf(tmpstr, "%s/description", sample_name);
            arow[1] = gnome_config_get_translated_string(tmpstr->str);
            if(!arow[1] || !*arow[1]) {
                g_free(arow[1]);
                arow[1] = g_strdup(sample_name);
            }
            g_string_sprintf(tmpstr, "%s/file", sample_name);
            arow[2] = sample_file = gnome_config_get_string(tmpstr->str);

            event_node = gtk_ctree_insert_node(GTK_CTREE(props->ctree),
                                               category_node, event_node,
                                               arow, GNOME_PAD_SMALL,
                                               NULL, NULL, NULL, NULL, TRUE,
                                               FALSE);
            g_free(arow[1]);
            new_entry = g_new0(SoundEvent, 1);
            new_entry->category = g_strdup(dent->d_name);
            new_entry->name = sample_name;
            new_entry->file = sample_file;
            new_entry->row = event_node;
            gtk_ctree_node_set_row_data_full(GTK_CTREE(props->ctree), event_node,
                                             new_entry,
                                             (GtkDestroyNotify)sound_properties_event_free);
        }

        gnome_config_pop_prefix();
    }
    closedir(dirh);
}

static void
sound_properties_event_play(GtkWidget *widget, SoundProps *props)
{
    GtkWidget *entry;

    entry = gnome_file_entry_gtk_entry(GNOME_FILE_ENTRY(props->btn_filename));

    gnome_sound_play(gtk_entry_get_text(GTK_ENTRY(entry)));
}

static void
sound_properties_event_change_file(GtkEditable *entry, SoundProps *props)
{
    if(props->ignore_changed)
        return;

    g_return_if_fail(GTK_CLIST(props->ctree)->selection);

    capplet_widget_state_changed(CAPPLET_WIDGET(props->capplet), TRUE);

    gtk_ctree_node_set_text(GTK_CTREE(props->ctree),
                            GTK_CLIST(props->ctree)->selection->data,
                            2,
                            gtk_entry_get_text(GTK_ENTRY(entry)));
}

static void
sound_properties_event_select(GtkCTree *ctree,
                              GtkCTreeNode *row,
                              gint column,
                              SoundProps *props)
{
    char *ctmp;

    ctmp = GTK_CLIST_ROW(&row->list)->cell[2].u.text;
    if(!ctmp)
        ctmp = "";

    props->ignore_changed++;
    gtk_entry_set_text(GTK_ENTRY(gnome_file_entry_gtk_entry(GNOME_FILE_ENTRY(props->btn_filename))), ctmp);
    props->ignore_changed--;
}


static void
sound_properties_event_apply(GtkCTreeNode *node,
                             SoundProps *props)
{
    SoundEvent *ev;
    char *cur_filename, *ctmp;
    GtkWidget *entry;

    ev = gtk_ctree_node_get_row_data(GTK_CTREE(props->ctree), node);

    if(!ev)
        return;

    gtk_ctree_node_get_text(GTK_CTREE(props->ctree),
                            node, 2, &cur_filename);

    /* If the user didn't change the setting, no need to set it */
    if(!strcmp(cur_filename, ev->file))
        return;

    ctmp = g_copy_strings("/sound/events/", ev->category, "/",
                          ev->name, "/file");
    gnome_config_set_string(ctmp, cur_filename);
    g_free(ctmp);
}

static void
sound_properties_apply(SoundProps *props)
{
    g_list_foreach(GTK_CLIST(props->ctree)->row_list,
                   (GFunc)sound_properties_event_apply, props);

    gnome_config_set_bool("/sound/system/settings/start_esd",
                          GTK_TOGGLE_BUTTON(props->enable_esd_startup)->active);
    gnome_config_set_bool("/sound/system/settings/event_sounds",
                          GTK_TOGGLE_BUTTON(props->enable_sound_events)->active);
    gnome_config_sync();
}

static void
sound_properties_set_sensitivity(GtkToggleButton *btn,
                                 SoundProps *props)
{
    gtk_widget_set_sensitive(props->enable_sound_events,
                             GTK_TOGGLE_BUTTON(props->enable_esd_startup)->active);
    gtk_widget_set_sensitive(props->table,
                             GTK_TOGGLE_BUTTON(props->enable_esd_startup)->active
                             && GTK_TOGGLE_BUTTON(props->enable_sound_events)->active);

    capplet_widget_state_changed(CAPPLET_WIDGET(props->capplet), TRUE);
}

static void
ui_do_revert(GtkWidget *w, SoundProps *props)
{
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(props->enable_esd_startup),
                                gnome_config_get_bool("/sound/system/settings/start_esd"));

    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(props->enable_sound_events),
                                gnome_config_get_bool("/sound/system/settings/event_sounds"));

    sound_properties_regenerate_ctree(props);
}

static void
ui_do_ok(GtkWidget *w, SoundProps *props)
{
    sound_properties_apply(props);
    gtk_main_quit();
}

static void
ui_do_cancel(GtkWidget *w, SoundProps *props)
{
    gtk_main_quit();
}

static void
sound_properties_play_sound(GtkWidget *btn, SoundProps *props)
{
    char *ctmp, *ctmp2;
    GtkCTreeNode *node;

    g_return_if_fail(GTK_CLIST(props->ctree)->selection);

    node = GTK_CTREE_NODE(GTK_CLIST(props->ctree)->selection->data);
    ctmp = GTK_CLIST_ROW(&node->list)->cell[2].u.text;

    if(*ctmp == '/')
        ctmp2 = g_strdup(ctmp);
    else
        ctmp2 = gnome_sound_file(ctmp);

    gnome_sound_play(ctmp2);

    g_free(ctmp2);
}
