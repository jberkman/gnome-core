/*
 * This file is a part of gmenu, the GNOME panel menu editor.
 *
 * File: delete.c
 *
 * This file contains the routines which are responsible for deleting
 * menu items and their associated desktop entries.
 *
 * Authors: Nat Friedman <nat@nat.org>
 *          John Ellis <johne@bellatlantic.net>
 */

#include "gmenu.h"

gboolean
delete_desktop_entry_file(gchar *path)
{
  Desktop_Data *d;

  d = get_desktop_file_info (path);
  if (d == NULL)
    {
      g_warning("Cannot find desktop file associated with menu entry!");
      return FALSE;
    }

  if (d->isfolder)
    {
      gchar *dirfile;
      gchar *orderfile;

      dirfile = g_concat_dir_and_file (d->path, ".directory");
      if (g_file_exists(dirfile))
	{
	  if ( (unlink (dirfile) < 0) )
	    g_warning("Failed to delete %s: %s\n", dirfile,
		      g_strerror(errno));
	}
      g_free(dirfile);

      orderfile = g_concat_dir_and_file (d->path, ".order");
      if (g_file_exists(orderfile))
	{
	  if ( (unlink (orderfile) < 0) )
	    g_warning("Failed to delete %s: %s\n", orderfile,
		      g_strerror(errno));
	}
      g_free(orderfile);

      if ( (rmdir (d->path) < 0) )
	{
	  g_warning("Could not remove the directory: %s",
		    g_strerror(errno));
	  return FALSE;
	}
    }
  else
    {
      if ( (unlink (d->path) < 0) )
	{
	  g_warning("Could not remove the menu item: %s",
		    g_strerror(errno));
	  return FALSE;
	}
    }

  return TRUE;
} /* delete_desktop_entry_file */

void
remove_node_cb(gpointer data)
{
  Desktop_Data *d = (Desktop_Data *) data;

  /*
   * Delete the file associated with the menu entry.
   */
  delete_desktop_entry_file(d->path);

  free_desktop_data(d);
}

gboolean
delete_desktop_entry(gchar *path)
{
  GtkCTreeNode *node, *new_current_node;
  gboolean retval;
  Desktop_Data *d;

  node = find_file_in_tree(GTK_CTREE(menu_tree_ctree), path);

  /*
   * If this node is selected, then we need to set a new current_node
   * after we delete this node.  Determine what the new current_node will
   * be before deleting the node.
   */
  if (node == current_node)
    {
      if (GTK_CTREE_ROW(node)->sibling)
	new_current_node = GTK_CTREE_ROW(node)->sibling;
      else
	{
	  new_current_node = GTK_CTREE_ROW(node)->parent;
	  if (GTK_CTREE_ROW(new_current_node)->children != node
	      && GTK_CTREE_ROW(new_current_node)->children != NULL)	{
	    new_current_node = GTK_CTREE_ROW(node)->children;
	    while(GTK_CTREE_ROW(new_current_node)->sibling != node
		  && GTK_CTREE_ROW(new_current_node)->sibling != NULL)
	      new_current_node = GTK_CTREE_ROW(new_current_node)->sibling;
	  }
	}
    }
  else
    new_current_node = current_node;

  /*
   * We just call this, and remove_node_cb, the RemoveNotify handler, gets
   * called and handles the rest.
   */
  gtk_ctree_remove_node(GTK_CTREE(menu_tree_ctree), node);

  if (is_node_editable(new_current_node))
    save_order_of_dir(new_current_node);

  tree_set_node(new_current_node);

  return TRUE;
} /* delete_desktop_entry */

void
delete_recursive_cb(gint button, gpointer data)
{
  GtkCTreeNode *node = (GtkCTreeNode *) data;
  Desktop_Data *d;

  if (button == 1)
    return;

  d = gtk_ctree_node_get_row_data(GTK_CTREE(menu_tree_ctree), node);
  delete_desktop_entry(d->path);
}
