/*
 * This file is a part of gmenu, the GNOME panel menu editor.
 *
 * File: save.c
 *
 * This file contains the routines which are responsible for updating
 * the menu information in-memory and on-disk after the user hits
 * the 'save' button.
 *
 * Authors: John Ellis <johne@bellatlantic.net>
 *          Nat Friedman <nat@nat.org>
 */

#include "gmenu.h"

void recalc_paths_cb (GtkCTree *ctree, GtkCTreeNode *node, gpointer data)
{
	GtkCTreeNode *parent;
	Desktop_Data *n;
	Desktop_Data *p;
	gchar *path;

	parent = GTK_CTREE_ROW(node)->parent;
	n = gtk_ctree_node_get_row_data(ctree, node);
	p = gtk_ctree_node_get_row_data(ctree, parent);

	path = g_strconcat(p->path, "/", n->path + g_filename_index(n->path), NULL);
	g_free(n->path);
	n->path = path;

	/* now update current state */
	if (node == current_node)
		{
		g_free(current_path);
		current_path = g_strdup(p->path);
		gtk_label_set_text(GTK_LABEL(pathlabel),current_path);
		}
}


gboolean
save_desktop_entry_file(GnomeDesktopEntry *dentry, gchar *path,
			gboolean prompt_first, gboolean prompt_about_overwrite,
			gboolean error_on_overwrite_conflict)
{

  /*
   * (1) Pop up some dialogs if necessary.
   */
  if ((dentry->name == NULL) || (strlen(dentry->name) == 0))
    {
      gnome_warning_dialog(_("The menu item must have a name"));
      return FALSE;
    }

  if ((path == NULL) || (strlen(path) == 0))
    {
      gnome_warning_dialog(_("The menu entry must have a filename"));
      return FALSE;
    }

  if (prompt_first)
    {
      GtkWidget *question;

      question = gnome_dialog_new(_("Save changes?"),
				  GNOME_STOCK_BUTTON_OK,
				  GNOME_STOCK_BUTTON_CANCEL, NULL);
      if (gnome_dialog_run_and_close(GNOME_DIALOG(question)) == 1)
	return FALSE;
    }

  /*
   * (2) Check to see if we're overwriting a file.
   */
  if (isfile(path) || isdir(path))
    {
      GtkWidget *question;

      if (error_on_overwrite_conflict)
	{
	  gnome_warning_dialog(
		  _("This change conflicts with an existing menu item"));
	  return FALSE;
	}

      if (prompt_about_overwrite)
	{
	  question = gnome_dialog_new(_("Overwrite existing menu entry?"),
				      GNOME_STOCK_BUTTON_OK,
				      GNOME_STOCK_BUTTON_CANCEL, NULL);
	  if (gnome_dialog_run_and_close(GNOME_DIALOG(question)) == 1)
	    return FALSE;
	}
    }

  dentry->location = g_strdup(path);
  gnome_desktop_entry_save (dentry);

  return TRUE;
}

gboolean
create_desktop_entry_node(Desktop_Data *d)
{
  GtkCTreeNode *node, *parent;
  gchar *text[2];
  gboolean leaf;
  text[0] = d->name;
  text[1] = NULL;

  gtk_ctree_get_node_info(GTK_CTREE(menu_tree_ctree), current_node,
			  NULL, NULL, NULL, NULL, NULL, NULL, &leaf, NULL);

  if (leaf)
    {
      node = current_node;
      parent = GTK_CTREE_ROW(current_node)->parent;
    }
  else
    {
      parent = current_node;
      if (GTK_CTREE_ROW(current_node)->children)
	node = GTK_CTREE_ROW(current_node)->children;
      else
	node = NULL;
    }

  if (d->isfolder)
    node = gtk_ctree_insert_node (GTK_CTREE(menu_tree_ctree), parent,
				  node, text, 5,
				  GNOME_PIXMAP(d->pixmap)->pixmap,
				  GNOME_PIXMAP(d->pixmap)->mask,
				  GNOME_PIXMAP(d->pixmap)->pixmap,
				  GNOME_PIXMAP(d->pixmap)->mask,
				  FALSE, FALSE);
  else
    node = gtk_ctree_insert_node (GTK_CTREE(menu_tree_ctree), parent,
				  node, text, 5,
				  GNOME_PIXMAP(d->pixmap)->pixmap,
				  GNOME_PIXMAP(d->pixmap)->mask, NULL,
				  NULL, TRUE, FALSE);
  gtk_ctree_node_set_row_data_full (GTK_CTREE(menu_tree_ctree), node, d,
				    remove_node_cb);

  gtk_ctree_expand(GTK_CTREE(menu_tree_ctree), parent);

  save_order_of_dir(node);

  tree_set_node(node);

  return TRUE;
}

static void
set_ctree_node_data(GtkCTreeNode *node, Desktop_Data *d)
{
  gboolean expanded, leaf;
  gint8 spacing;


  free_desktop_data(gtk_ctree_node_get_row_data(
		    GTK_CTREE(menu_tree_ctree), node));

  gtk_ctree_node_set_row_data_full(GTK_CTREE(menu_tree_ctree), node, d,
				   remove_node_cb);

  gtk_ctree_get_node_info (GTK_CTREE(menu_tree_ctree), node, NULL,
			   &spacing, NULL, NULL, NULL, NULL, &leaf,
			   &expanded);

  gtk_ctree_set_node_info (GTK_CTREE(menu_tree_ctree), node, d->name,
			   spacing, GNOME_PIXMAP(d->pixmap)->pixmap,
			   GNOME_PIXMAP(d->pixmap)->mask,
			   GNOME_PIXMAP(d->pixmap)->pixmap,
			   GNOME_PIXMAP(d->pixmap)->mask, leaf, expanded);

  save_order_of_dir(node);
}

gboolean
update_desktop_entry_node(GnomeDesktopEntry *dentry, gchar *old_path)
{
  Desktop_Data *d;
  GtkCTreeNode *node;

  node = find_file_in_tree(GTK_CTREE(menu_tree_ctree), old_path);
  if (node == NULL)
    {
      g_error("Cannot find node in tree!");
      abort();
    }

  d = get_desktop_file_info(dentry->location);
  if (d == NULL)
    {
      g_error("error");
      abort();
    }

  if (d->isfolder) d->expanded = TRUE;

  set_ctree_node_data(node, d);

  tree_set_node(node);

  return TRUE;
}

