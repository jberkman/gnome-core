/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* Copyright (C) 1998 Redhat Software Inc. 
 * Authors: Jonathan Blandford <jrb@redhat.com>
 */
#include "parser.h"
#include "gnome.h"
typedef enum
{
        UNSET,
        ENTRY,
        LRANGE,
        NRANGE,
        CHECK
} W_TYPES;
typedef struct _setup_data setup_data;
struct _setup_data
{
        gchar *name;
        W_TYPES widget_type;
        gchar *val_type;
        gchar *flag1;
        gchar *flag2; /* optional */
        gchar *label;
        gchar *right_label;
        gchar *left_label;
        gchar *comment;
        gchar *arg;
};
setup_data *
new_arg_vals ()
{
        setup_data *retval = g_malloc (sizeof (setup_data));

        retval->name = NULL;
        retval->widget_type = UNSET;
        retval->val_type = NULL;
        retval->flag1 = NULL;
        retval->flag2 = NULL;
        retval->label = NULL;
        retval->left_label = NULL;
        retval->right_label = NULL;
        retval->comment = NULL;
        retval->arg = NULL;
        return retval;
}
void
free_setup_data (setup_data *dat)
{
        if (dat->name)
                g_free (dat->name);
        if (dat->val_type)
                g_free (dat->val_type);
        if (dat->flag1)
                g_free (dat->flag1);
        if (dat->flag2)
                g_free (dat->flag1);
        if (dat->label)
                g_free (dat->label);
        if (dat->right_label)
                g_free (dat->right_label);
        if (dat->left_label)
                g_free (dat->left_label);
        if (dat->comment)
                g_free (dat->comment);
        if (dat->arg)
                g_free (dat->arg);
        g_free (dat);
}
void
print_data (setup_data *dat)
{
        g_print ("Name: %s\n", dat->name);
        switch (dat->widget_type) {
        case UNSET:
                g_print ("Type: UNSET\n");
                break;
        case ENTRY:
                g_print ("Type: ENTRY\n");
                break;
        case LRANGE:
                g_print ("Type: LRANGE\n");
                break;
        case NRANGE:
                g_print ("Type: NRANGE\n");
                break;
        case CHECK:
                g_print ("Type: CHECK\n");
                break;
        }
        if (!dat->val_type)
                g_print ("ValType: NULL\n");
        else
                g_print ("ValType: %s\n", dat->val_type);
        g_print ("Flag: %s\n", dat->flag1);
        if (dat->flag2)
                g_print ("Flag2: %s\n", dat->flag2);
        else
                g_print ("Flag2: NULL\n");
        if (dat->label)
                g_print ("Label: %s\n", dat->label);
        else
                g_print ("Label: NULL\n");
        if (dat->comment)
                g_print ("Comment: %s\n", dat->comment);
        else
                g_print ("Comment: NULL\n");
        g_print ("Arg: %s\n", dat->arg);
        g_print ("\n\n");
}
gboolean
validate_setup_data (setup_data *dat)
{
        /* We make sure that the entry is sane, and set up the arg field. */

        switch (dat->widget_type) {
        case UNSET:
                return FALSE;
        case ENTRY:
                if ((dat->val_type == NULL) ||
                    (dat->flag1 == NULL) ||
                    (dat->name == NULL))
                        return FALSE;
                return TRUE;
        case LRANGE:
                if ((dat->val_type == NULL) ||
                    (dat->flag1 == NULL) ||
                    (dat->name == NULL) ||
                    (dat->left_label == NULL) ||
                    (dat->right_label == NULL))
                        return FALSE;
                return TRUE;
        case NRANGE:
                if ((dat->val_type == NULL) ||
                    (dat->flag1 == NULL) ||
                    (dat->name == NULL))
                        return FALSE;
                return TRUE;
        case CHECK:
                if ((dat->flag1 == NULL) ||
                    (dat->flag2 == NULL) ||
                    (dat->name == NULL) ||
                    (dat->left_label == NULL))
                        return FALSE;
                return TRUE;
        default:
                return FALSE;
        }
}
void
parse_key (setup_data *arg_vals, gchar *key, gchar *value)
{
        if (strcmp (key, "Name") == 0)
                arg_vals->name = g_strdup (value);
        else if (strcmp (key, "Type") == 0) {
                if (strcmp (value, "Entry") == 0)
                        arg_vals->widget_type = ENTRY;
                else if (strcmp (value, "LRange") == 0)
                        arg_vals->widget_type = LRANGE;
                else if (strcmp (value, "NRange") == 0)
                        arg_vals->widget_type = NRANGE;
                else if (strcmp (value, "Check") == 0)
                        arg_vals->widget_type = CHECK;
        } else if (strcmp (key, "ValType") == 0)
                arg_vals->val_type = g_strdup (value);
        else if (strcmp (key, "Flag") == 0)
                arg_vals->flag1 = g_strdup (value);
        else if (strcmp (key, "Flag2") == 0)
                arg_vals->flag2 = g_strdup (value);
        else if (strcmp (key, "Label") == 0)
                arg_vals->label = g_strdup (value);
        else if (strcmp (key, "Comment") == 0)
                arg_vals->comment = g_strdup (value);
        else if (strcmp (key, "LeftLabel") == 0)
                arg_vals->left_label = g_strdup (value);
        else if (strcmp (key, "RightLabel") == 0)
                arg_vals->right_label = g_strdup (value);
}
GtkWidget *
get_entry (setup_data *ssd)
{
        GtkWidget *frame = NULL;
        GtkWidget *entry;
        GtkWidget *box;
        
        if (ssd->label)
                frame = gtk_frame_new (ssd->label);
        box = gtk_hbox_new (FALSE, GNOME_PAD_SMALL);

        gtk_container_border_width (GTK_CONTAINER (box), GNOME_PAD_SMALL);
        entry = gtk_entry_new ();
        if (ssd->left_label)
                gtk_box_pack_start (GTK_BOX (box), gtk_label_new (ssd->left_label), FALSE, FALSE, 0);
        gtk_box_pack_start (GTK_BOX (box), entry, TRUE, TRUE, 0);
        if (ssd->right_label)
                gtk_box_pack_start (GTK_BOX (box), gtk_label_new (ssd->right_label), FALSE, FALSE, 0);
        gtk_container_add (GTK_CONTAINER (frame), box);
        if (frame) {
                gtk_container_add (GTK_CONTAINER (frame), box);
                return frame;
        } else
                return box;
}
GtkWidget *
get_lrange (setup_data *ssd)
{
        GtkAdjustment *adj;
        GtkWidget *frame = NULL;
        GtkWidget *box;
        GtkWidget *range;

        if (ssd->label)
                frame = gtk_frame_new (ssd->label);
        adj = gtk_adjustment_new (1.0, 0.0, 2.0, 1.0, 1.0, 1.0);
        box = gtk_hbox_new (FALSE, GNOME_PAD_SMALL);
        gtk_container_border_width (GTK_CONTAINER (box), GNOME_PAD_SMALL);
        range = gtk_hscale_new (adj);
        gtk_scale_set_draw_value (GTK_SCALE (range), FALSE);
        gtk_box_pack_start (GTK_BOX (box), gtk_label_new (ssd->left_label), FALSE, FALSE, 0);
        gtk_box_pack_start (GTK_BOX (box), range, TRUE, TRUE, 0);
        gtk_box_pack_start (GTK_BOX (box), gtk_label_new (ssd->right_label), FALSE, FALSE, 0);
        if (frame) {
                gtk_container_add (GTK_CONTAINER (frame), box);
                return frame;
        } else
                return box;
}
GtkWidget *
get_nrange (setup_data *ssd)
{
        GtkAdjustment *adj;
        GtkWidget *label;
        GtkWidget *align;
        GtkWidget *frame = NULL;
        GtkWidget *box;
        GtkWidget *range;

        if (ssd->label)
                frame = gtk_frame_new (ssd->label);
        adj = gtk_adjustment_new (1.0, 0.0, 2.0, 1.0, 1.0, 1.0);
        box = gtk_hbox_new (FALSE, GNOME_PAD_SMALL);
        gtk_container_border_width (GTK_CONTAINER (box), GNOME_PAD_SMALL);
        range = gtk_hscale_new (adj);
        if (ssd->left_label) {
                label = gtk_label_new (ssd->left_label);
                align = gtk_alignment_new (0.5, 1.0, 0.0, 0.0);
                gtk_container_add (GTK_ALIGNMENT (align), label);
                gtk_box_pack_start (GTK_BOX (box), align, FALSE, FALSE, 0); 
        }
        gtk_box_pack_start (GTK_BOX (box), range, TRUE, TRUE, 0);
        if (ssd->right_label) {
                label = gtk_label_new (ssd->right_label);
                align = gtk_alignment_new (0.5, 1.0, 0.0, 0.0);
                gtk_container_add (GTK_ALIGNMENT (align), label);
                gtk_box_pack_start (GTK_BOX (box), align, FALSE, FALSE, 0); 
        }
        if (frame) {
                gtk_container_add (GTK_CONTAINER (frame), box);
                return frame;
        } else
                return box;
}
GtkWidget *
get_check (setup_data *ssd)
{
        GtkWidget *frame = NULL;
        GtkWidget *check;
        GtkWidget *box;
        
        if (ssd->label)
                frame = gtk_frame_new (ssd->label);
        box = gtk_hbox_new (FALSE, GNOME_PAD_SMALL);
        gtk_container_border_width (GTK_CONTAINER (box), GNOME_PAD_SMALL);
        check = gtk_check_button_new_with_label (ssd->left_label);
        
        gtk_box_pack_start (GTK_BOX (box), check, TRUE, TRUE, 0);
        if (frame) {
                gtk_container_add (GTK_CONTAINER (frame), box);
                return frame;
        } else
                return box;
}

/* public functions */

void
init_screensaver_data (screensaver_data *sd)
{
        gchar *prefix;
        gchar *tempstring;
        gchar *tempstring2;
        gchar *sec;
        gchar *key;
        gchar *value;
        void *sec_iter;
        void *val_iter;
        setup_data *arg_vals;

        /* We set up the prefix for most of our gnome_config stuff. */
        prefix = g_copy_strings ("=", sd->desktop_filename, "=", NULL);
        gnome_config_push_prefix (prefix);
        
        /* We determine the current args */
        tempstring = gnome_config_get_string ("Screensaver Data/DefaultFlags");
        gnome_config_pop_prefix ();
        tempstring2 = g_copy_strings ("/Screensaver/", sd->name, "/args=", tempstring ,NULL);
        sd->args = gnome_config_get_string (tempstring2);
        g_free (tempstring);
        g_free (tempstring2);

        /* We determine the setup layout. */
        sec_iter = gnome_config_init_iterator_sections (prefix);
        sd->setup_data = NULL;
        while (sec_iter = gnome_config_iterator_next (sec_iter, &sec, NULL)) {
                if (strncmp ("Arg", sec, 3) == 0) {
                        arg_vals = new_arg_vals();
                        tempstring = g_copy_strings (prefix, "/", sec, "/", NULL);
                        val_iter = gnome_config_init_iterator (tempstring);
                        while (val_iter = gnome_config_iterator_next (val_iter, &key, &value)) {
                                parse_key (arg_vals, key, value);
                                g_free (key);
                                g_free (value);
                        }
                        if (validate_setup_data (arg_vals))
                                sd->setup_data = g_list_prepend (sd->setup_data, arg_vals);
                        /* FIXME: THIS IS REALLY ANNOYING!!! */
                        /* I don't know why this breaks, but this line here
                         * seems to break the g_copy_strings a few lines earlier
                         * on the next pass!!! */
                        /*else
                          free_setup_data (arg_vals);*/
                        g_free (tempstring);
                }
                g_free (sec);
        } 
        g_free (prefix);
}
GtkWidget *
get_screensaver_widget (screensaver_data *sd)
{
        GtkWidget *vbox;
        setup_data *ssd;
        GList *list;

        vbox = gtk_vbox_new (FALSE, GNOME_PAD);
        for (list = sd->setup_data; list; list = list->next) {
                ssd = ((setup_data *)list->data);
                switch (ssd->widget_type) {
                case ENTRY:
                        gtk_box_pack_start (GTK_BOX (vbox), get_entry (ssd), FALSE, FALSE, 0);
                        break;
                case LRANGE:
                        gtk_box_pack_start (GTK_BOX (vbox), get_lrange (ssd), FALSE, FALSE, 0);
                        break;
                case NRANGE:
                        gtk_box_pack_start (GTK_BOX (vbox), get_nrange (ssd), FALSE, FALSE, 0);
                        break;
                case CHECK:
                        gtk_box_pack_start (GTK_BOX (vbox), get_check (ssd), FALSE, FALSE, 0);
                        break;
                default:
                        g_warning ("unknown type trying to be realized");
                }
                
        }
        gtk_container_border_width (GTK_CONTAINER (vbox), GNOME_PAD_SMALL);
        return vbox;
}
void
free_screensaver_data (screensaver_data *sd)
{
}
