/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */

/* Copyright (C) 1998 Redhat Software Inc. 
 * Authors: Jonathan Blandford <jrb@redhat.com>
 */
#include "tree.h"
#include "caplet-manager.h"
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>

/*GdkPixmap *pixmap1;
GdkPixmap *pixmap2;
GdkPixmap *pixmap3;
GdkBitmap *mask1;
GdkBitmap *mask2;
GdkBitmap *mask3;*/

extern GtkWidget *status_bar;

gboolean
compare_last_dir (gchar *first, gchar *second)
{
        gboolean retval;
        gchar *temp1;
        gchar *temp2;

        temp1 = strdup (first);
        temp2 = strdup (second);
        (rindex(temp1, '/'))[0] = 0;
        (rindex(temp2, '/'))[0] = 0;

        retval = !strcmp (rindex (temp1, '/'), rindex (temp2, '/'));
        g_free (temp1);
        g_free (temp2);
        return retval;
}
gboolean
compare_nodes (GnomeDesktopEntry *data1, GnomeDesktopEntry *data2)
{
        g_return_val_if_fail (data1, FALSE);
        g_return_val_if_fail (data2, FALSE);
        g_return_val_if_fail (data1->type, FALSE);
        g_return_val_if_fail (data2->type, FALSE);

        if (!strcmp (data1->type, "Directory") && 
            (!strcmp (data2->type, "Directory")))
                return compare_last_dir (data1->location,
                                         data2->location);
        else
                return (!strcmp (rindex (data1->location,'/'),
                                 rindex (data2->location,'/')));
}
/* 
 * This function is used to generate a node starting at a directory.
 * It doesn't do all that complex error checking -- if something
 * happens, it just returns null and skips the directory.
 *
 * It will try to use node1's data over node2's if possible, and will
 * write into node1's field.  It should handle all memory, so there is
 * no need to free stuff from node2 after the merger.
 */
void
merge_nodes (GNode *node1, GNode *node2)
{
        GNode *child1, *child2;

        if ((node1 == NULL) || (node2 == NULL))
                return;

        /* first we merge data */
        if (node1->data == NULL)
                node1->data = node2->data;
        else if (node2->data != NULL) 
                gnome_desktop_entry_free (node2->data);

        /* now we want to find subdirs to merge */
        /* it's not incredibly effecient, but it works... */
        for (child1 = node1->children; child1; child1 = child1->next)
                for (child2 = node2->children; child2; child2 = child2->next)
                        if (compare_nodes (child1->data, child2->data)) {
                                if (child2->prev == NULL)
                                        child2->parent->children = child2->next;
                                else
                                        child2->prev->next = child2->next;
                                merge_nodes (child1, child2);
                        }
        if (node2->children) {
                for (child2 = node2->children; child2->next; child2 = child2->next)
                        child2->parent = node1;
                child2->next = node1->children;
                child2->next->prev = child2;
                node1->children = node2->children;
                node2->children = NULL;
        }
        /*g_free (node2);*/
}
GNode *
read_directory (gchar *directory)
{
        DIR *parent_dir;
        struct dirent *child_dir;
        struct stat filedata;
        GNode *retval = g_node_new(NULL);

        parent_dir = opendir (directory);
        if (parent_dir == NULL)
                return NULL;

        while ((child_dir = readdir (parent_dir)) != NULL) {
                if (child_dir->d_name[0] != '.') {
                                        
                        /* we check to see if it is interesting. */
                        GString *name = g_string_new (directory);
                        g_string_append (name, "/");
                        g_string_append (name, child_dir->d_name);
                        
                        if (stat (name->str, &filedata) != -1) {
                                gchar* test;
                                if (S_ISDIR (filedata.st_mode)) {
                                        /* it might be interesting... */
                                        GNode *next_dir = read_directory (name->str);
                                        if (next_dir)
                                                /* it is interesting!!! */
                                                g_node_prepend (retval, next_dir);
                                }
                                test = rindex(child_dir->d_name, '.');
                                if (test && !strcmp (".desktop", test)) {
                                        /* it's a .desktop file -- it's interesting for sure! */
                                        GNode *new_node = g_node_new (gnome_desktop_entry_load (name->str));
                                        g_node_prepend (retval, new_node);
                                }
                        }
                        g_string_free (name, TRUE);
                }
                else if (!strcmp (child_dir->d_name, ".directory")) {
                        GString *name = g_string_new (directory);
                        g_string_append (name, "/.directory");
                        retval->data = gnome_desktop_entry_load (name->str);
                        g_string_free (name, TRUE);
                }
        }
        if (retval->children == NULL) {
                if (retval->data)
                        gnome_desktop_entry_free (retval->data);
                return NULL;
        }
        return retval;
}
void
generate_tree_helper (GtkCTree *ctree, GtkCTreeNode *parent, GNode *node)
{
        GNode *i;
        GtkCTreeNode *child;
        char *text[2];
        node_data *data;

        text[1] = NULL;

        for (i = node;i;i = i->next) {
                if (i->data && ((GnomeDesktopEntry *)i->data)->name)
                        text[0] = ((GnomeDesktopEntry *)i->data)->name;
                else
                        text[0] = "foo";
                if (i->data && (!strcmp(((GnomeDesktopEntry *)i->data)->type,"Directory")))
                        child = gtk_ctree_insert_node (ctree,parent,NULL, text, 3, NULL, NULL,NULL,NULL,FALSE,FALSE);
                else
                        child = gtk_ctree_insert_node (ctree,parent,NULL, text, 3, NULL, NULL,NULL,NULL,TRUE,FALSE);
                data = g_malloc (sizeof (node_data));
                data->gde = (GnomeDesktopEntry *)i->data;
                data->socket = NULL;
                data->id = -1;
                gtk_ctree_node_set_row_data (ctree, child, data);
                if (i->children)
                        generate_tree_helper (ctree, child, i->children);
        }
}
GtkWidget *
generate_tree ()
{
        GtkWidget *retval;
        GNode *global_node;
        GNode *user_node;
        gchar *root_prefix;
        gchar *user_prefix;

        retval = gtk_ctree_new (1, 0);

        /* First thing we want to do is to check directories to create the menus */
        
        gtk_clist_set_row_height(GTK_CLIST (retval),20);
        gtk_clist_set_column_width(GTK_CLIST (retval), 0, 150);
        gtk_clist_set_policy (GTK_CLIST (retval), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
        gtk_ctree_set_line_style (GTK_CTREE (retval), GTK_CTREE_LINES_DOTTED);
        gtk_ctree_set_indent (GTK_CTREE (retval), 10);
        gtk_widget_set_usize (retval, 200, 375);
        gtk_clist_set_selection_mode(GTK_CLIST(retval), GTK_SELECTION_BROWSE);
        gtk_signal_connect( GTK_OBJECT (retval),"tree_select_row", GTK_SIGNAL_FUNC (selected_row_callback), NULL);

        /*hard_coded for now... */
        root_prefix = gnome_unconditional_datadir_file ("control_center");
        global_node = read_directory (root_prefix);
        user_prefix = gnome_util_home_file ("control_center");
        user_node = read_directory (user_prefix);
        merge_nodes (user_node, global_node);

        /* now we actually set up the tree... */
        /* we prolly want to use the gtree_insert_node function to do this, 
         * but as it was written after the code here... 
         *
         * we do user_node->children to avoid the root menu.
         */
        if (user_node != NULL)
                generate_tree_helper (GTK_CTREE (retval), NULL, user_node->children);
        else {
                g_warning ("\nYou have no entries listed in either \n\t%s\nor\n\t%s\n\nThis " \
                         "probably means that either the control-center or GNOME is "
                         "incorrectly installed.\n",root_prefix, user_prefix);
                exit (1);
        }
        g_free (root_prefix);
        g_free (user_prefix);
        return retval;
}

void
selected_row_callback (GtkWidget *widget, GtkCTreeNode *node, gint column)
{
        node_data *data;
        GnomeDesktopEntry *gde;
        GdkEvent *event = gtk_get_current_event();

        if (column < 0)
                return;
        
        data = (node_data *) gtk_ctree_node_get_row_data (GTK_CTREE (widget),node);
        gde = (GnomeDesktopEntry *)data->gde; 
        gtk_statusbar_pop (GTK_STATUSBAR (status_bar), 1);
        if (gde->comment)
                gtk_statusbar_push (GTK_STATUSBAR (status_bar), 1, (gde->comment));
        else
                gtk_statusbar_push (GTK_STATUSBAR (status_bar), 1, (gde->name));

        if (event && event->type == GDK_2BUTTON_PRESS)
                launch_caplet (data);
}
