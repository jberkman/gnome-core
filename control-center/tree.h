/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */

/* Copyright (C) 1998 Redhat Software Inc. 
 * Authors: Jonathan Blandford <jrb@redhat.com>
 */
#ifndef __TREE_H__
#define __TREE_H__

#include <gnome.h>

#ifdef __cplusplus
extern "C" {
#pragma }
#endif /* __cplusplus */


GNode *read_directory (gchar *directory);
void merge_nodes (GNode *node1, GNode *node2);
GtkWidget *generate_tree ();

/* callbacks */
void selected_row_callback (GtkWidget *widget, GtkCTreeNode *node, gint column);

typedef struct _node_data node_data;
struct _node_data
{
        GnomeDesktopEntry *gde;
        GtkWidget *socket;
        gint id;
        gint notetab_id;
        gboolean modified;
};




#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
