/*
 * This file is a part of gmenu, the GNOME panel menu editor.
 *
 * Authors: John Ellis <johne@bellatlantic.net>
 *          Nat Friedman <nat@nat.org>
 */
 
#include "gmenu.h"

void
gmenu_init_dnd(GtkCTree *ctree)
{
  gtk_clist_set_reorderable (GTK_CLIST(menu_tree_ctree), TRUE);
  gtk_ctree_set_drag_compare_func(GTK_CTREE(menu_tree_ctree),
				  tree_move_test_cb);
  gtk_signal_connect(GTK_OBJECT(menu_tree_ctree),
		     "tree_move", GTK_SIGNAL_FUNC(tree_moved), "before");

  gtk_signal_connect_after(GTK_OBJECT(menu_tree_ctree),
				 "tree_move",
				 GTK_SIGNAL_FUNC(tree_moved),NULL);
}

gboolean tree_move_test_cb(GtkCTree *ctree, GtkCTreeNode *source_node,
			GtkCTreeNode *new_parent, GtkCTreeNode *new_sibling)
{
	Desktop_Data *d;

	if (source_node == topnode || source_node == usernode || source_node == systemnode ||
			new_parent == NULL)
		return FALSE;

	if (gtk_ctree_is_ancestor(ctree, usernode, source_node))
		{
		if (new_parent != usernode && !gtk_ctree_is_ancestor(ctree, usernode, new_parent))
			return FALSE;
		}

	if (gtk_ctree_is_ancestor(ctree, systemnode, source_node))
		{
		if (new_parent != systemnode && !gtk_ctree_is_ancestor(ctree, systemnode, new_parent))
			return FALSE;
		}

	d = gtk_ctree_node_get_row_data(ctree, new_parent);
	if (!d || !d->editable)
		return FALSE;

	d = gtk_ctree_node_get_row_data(ctree, source_node);
	if (!d || !d->editable)
		return FALSE;

	/* and a final check to make sure a DIFFERENT file of the same name does not exist */
	if (new_parent != GTK_CTREE_ROW(source_node)->parent)
		{
		gint index = g_filename_index(d->path);
		GtkCTreeNode *node = GTK_CTREE_ROW(new_parent)->children;
		while (node)
			{
			Desktop_Data *n = gtk_ctree_node_get_row_data(ctree, node);
			if (!strcmp(d->path + index, n->path + g_filename_index(n->path)))
				return FALSE;
			node = GTK_CTREE_ROW(node)->sibling;
			}
		}

	return TRUE;
}

void tree_moved(GtkCTree *ctree, GtkCTreeNode *node, GtkCTreeNode *new_parent,
			GtkCTreeNode *new_sibling, gpointer data)
{
	static GtkCTreeNode *old_parent;

	if (data)
		{
		/* this happens before the move, we need this to get the original parent,
		because we can't save or move anything until the node moves */
		old_parent = GTK_CTREE_ROW(node)->parent;
		}
	else
		{
		/* this happens after the move */
		Desktop_Data *node_data, *parent_data;
		gchar *old_filename;
		gchar *new_filename;
		GtkCTreeNode *parent;

		node_data = gtk_ctree_node_get_row_data(ctree, node);
		old_filename = node_data->path;

		parent = GTK_CTREE_ROW(node)->parent;
		parent_data = gtk_ctree_node_get_row_data(ctree, parent);

		if (parent == old_parent)
			{
			/* nothing to physically move, only update order file */
			save_order_of_dir(parent);
			}
		else
			{
			Desktop_Data *d = gtk_ctree_node_get_row_data(ctree, parent);
			new_filename = g_strconcat(d->path, "/",
					old_filename + g_filename_index(old_filename), NULL);

			if (rename (old_filename, new_filename) < 0)
				g_print("Failed to move file: %s\n", old_filename);

			g_free(old_filename);
			node_data->path = new_filename;

			save_order_of_dir(old_parent);
			save_order_of_dir(parent);

			/* it would probably be easier to reread the menus, but I'm doing
			it this way :)    --John */
			if (node_data->isfolder)
				{
				gtk_ctree_pre_recursive(ctree, node, recalc_paths_cb, NULL);
				}

			gtk_ctree_expand(ctree, parent);
			}
		}
}

#ifdef CRAPPY_OLD_DND
/*
 * This is the function which gets called when a menu item has been dropped
 * on a folder.
 */
static void
menu_drag_data_received (GtkWidget          *widget,
		    GdkDragContext     *context,
		    gint                x,
		    gint                y,
		    GtkSelectionData   *data,
		    guint               info,
		    guint               time,
		    gpointer            user_data)
{
  GnomeDesktopEntry *src_dentry;
  GtkCTree *ctree = GTK_CTREE(widget);
  GtkCTreeNode *src, *dest;
  Desktop_Data *src_desktop, *dest_desktop;

  gint row, col;

  if (data->length <= 0)
    return;

  /*
   * Figure out which node is the drop destination.
   */
  gtk_clist_get_selection_info (GTK_CLIST (ctree), x, y, &row, &col);
  dest = GTK_CTREE_NODE( g_list_nth(GTK_CLIST (ctree)->row_list, row));
  if (dest == NULL || dest == topnode)
    return;

  /*
   * Figure out which node is the node being dropped.
   */
  src = *(GtkCTreeNode **) data->data;
  if (src == NULL)
    return;

  if (src == dest)
    return;

  dest_desktop = gtk_ctree_node_get_row_data( GTK_CTREE(ctree), dest);
  src_desktop = gtk_ctree_node_get_row_data( GTK_CTREE(ctree), src);

  if (! (dest_desktop->editable && src_desktop->editable))
    {
      gnome_warning_dialog (_("You are not allowed to move that item there.\nYou do not have the proper permissions."));
      return;
    }

  if (info == DROP_TARGET_MENU_ITEM)
    {
      GnomeDesktopEntry *src_dentry, *dest_dentry;

#if 0
      dest_dentry = load_desktop_entry(dest_desktop);
      src_dentry = load_desktop_entry(src_desktop);
#endif

      if (dest_desktop->isfolder)
	{
	  gchar *path, *base;

	  if (src_desktop->isfolder)
	    base = basename_n(src_dentry->location, 2);
	  else
	    base = basename_n(src_dentry->location, 1);

	  path = g_concat_dir_and_file(dest_desktop->path, base);
	  g_free(base);

	  /*
	   * Drop the menu item into the folder.
	   */
	  if (save_desktop_entry_file(src_dentry, path, FALSE, FALSE, TRUE))
	    {
	      g_free(src_desktop->path);
	      src_desktop->path = g_dirname(path);
	      g_free(path);
	      create_desktop_entry_node(src_desktop);
	    }
#if 0	  

	  tree_moved(ctree, src, dest, NULL,
		     (gpointer) 1);
	  gtk_ctree_move(ctree, src, dest, NULL);
#endif
	}
      else
	{
	  /*
	   * Move the menu item next to the drop
	   * site.
	 */
	  if (GTK_CTREE_ROW(src)->sibling == dest)
	    {
	      tree_moved(ctree, dest,
			 GTK_CTREE_ROW(src)->parent, src,
			 (gpointer) 1);
	      gtk_ctree_move(ctree, dest,
			     GTK_CTREE_ROW(src)->parent,
			     src);
	    }
	  else
	    {
	      tree_moved(ctree, src,
			 GTK_CTREE_ROW(dest)->parent, dest,
			 (gpointer) 1);
	      gtk_ctree_move(ctree, src,
			     GTK_CTREE_ROW(dest)->parent,
			     dest);
	    }

	}
    }
  gtk_drag_finish (context, FALSE, FALSE, time);
}

/*
 * This is the function that gets called when one of our menu items is
 * dropped somewhere and we need to provide the data to be dropped.
 */
static void
drag_data_get_cb (GtkWidget *widget, GdkDragContext *context,
		  GtkSelectionData *selection_data, guint info,
		  guint time, gpointer data)
{
	GtkCTree *ctree = GTK_CTREE(widget);
	Desktop_Data *d;
	gchar *uri_list;

	d = gtk_ctree_node_get_row_data( GTK_CTREE(ctree), drop_data );

	switch (info)
		{
		case DRAG_TARGET_URI:
			uri_list = g_strconcat("file:", d->path,
					       NULL);
			gtk_selection_data_set (selection_data,
						selection_data->target, 8,
						uri_list, strlen(uri_list));
			g_free(uri_list);

			break;

		case DRAG_TARGET_MENU_ITEM:
			gtk_selection_data_set (selection_data,
						selection_data->target, 8,
						(guchar *)&drop_data,
						sizeof(drop_data));
			break;

		}
}

static void
possible_drag_item_pressed (GtkCTree *ctree, GdkEventButton *event,
			    gpointer data)
{
	gint row, col;
	GtkCTreeNode *node;

	if (event->window != GTK_CLIST(ctree)->clist_window) return;
	if (event->button != 1) return;

	gtk_clist_get_selection_info (GTK_CLIST (ctree), event->x, event->y, &row, &col);

	node = GTK_CTREE_NODE(g_list_nth (GTK_CLIST (ctree)->row_list, row));

	if (!node || node == topnode)
		{
		return;
		}

	drop_data = node;
}

#endif /* CRAPPY_OLD_DND */
