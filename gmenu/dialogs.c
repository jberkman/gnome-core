/*
 * This file is a part of gmenu, the GNOME panel menu editor.
 *
 * File: dialogs.c
 *
 * This file contains the callback routines for the toolbar buttons,
 * and the code which creates most of the dialogs in gmenu.
 *
 * Authors: Nat Friedman <nat@nat.org>
 *          John Ellis <johne@bellatlantic.net>
 */

#include <config.h>
#include <errno.h>

#include "gmenu.h"

static gint close_folder_dialog_cb(GtkWidget *b, gpointer data);
static gint create_folder_cb(GtkWidget *w, gpointer data);
static void delete_dialog_cb( gint button, gpointer data);

static gint close_folder_dialog_cb(GtkWidget *b, gpointer data)
{
	GtkWidget *w = data;
	gnome_dialog_close(GNOME_DIALOG(w));
	return TRUE;
}

gint
create_folder(gchar *full_path)
{
	gint write_file = TRUE;

/*	g_print("creating folder: %s\n",full_path);*/

	if (isdir(full_path) || isfile(full_path))
		{
		gnome_warning_dialog (_("File exists."));
		write_file = FALSE;
		}

	if (write_file)
		{
		if ( (mkdir (full_path, 0755) < 0) )
			{
			gnome_warning_dialog (_("Failed to create file."));
			}
		else
			{
			Desktop_Data *d;
			GtkCTreeNode *parent;
			GtkCTreeNode *node;
			d = get_desktop_file_info (full_path);
			if (d)
				{
				gchar *text[2];
				gboolean leaf;
				GnomeDesktopEntry *dentry;

				dentry = g_new0(GnomeDesktopEntry, 1);
				dentry->location = g_concat_dir_and_file(d->path, ".directory");
				dentry->name = g_strdup(d->name);
				dentry->type = g_strdup("Directory");
				gnome_desktop_entry_save(dentry);
				gnome_desktop_entry_destroy(dentry);

				text[0] = d->name;
				text[1] = NULL;

				gtk_ctree_get_node_info(GTK_CTREE(menu_tree_ctree),current_node,
					NULL,NULL,NULL,NULL,NULL,NULL,&leaf,NULL);

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
					node = gtk_ctree_insert_node (GTK_CTREE(menu_tree_ctree), parent, node, text, 5,
						GNOME_PIXMAP(d->pixmap)->pixmap,
						GNOME_PIXMAP(d->pixmap)->mask,
						GNOME_PIXMAP(d->pixmap)->pixmap,
						GNOME_PIXMAP(d->pixmap)->mask,
						FALSE, FALSE);
				else
					node = gtk_ctree_insert_node (GTK_CTREE(menu_tree_ctree), parent, node, text, 5,
						GNOME_PIXMAP(d->pixmap)->pixmap,
						GNOME_PIXMAP(d->pixmap)->mask, NULL, NULL, TRUE, FALSE);
				gtk_ctree_node_set_row_data_full (GTK_CTREE(menu_tree_ctree), node, d, remove_node_cb);
				save_order_of_dir(parent);

				gtk_ctree_expand(GTK_CTREE(menu_tree_ctree),
						 parent);

				add_tree_node(GTK_CTREE(menu_tree_ctree), node, NULL);

				tree_set_node(node);
				}
			}
		}

	g_free(full_path);

	return TRUE;
}

static gint create_folder_cb(GtkWidget *w, gpointer data)
{
	Misc_Dialog *dlg = data;
	gchar *new_folder;
	gchar *full_path;

	new_folder = gtk_entry_get_text(GTK_ENTRY(dlg->entry));

	if (current_path)
		full_path = g_strconcat(current_path, "/", new_folder, NULL);
	else
		full_path = g_strconcat(USER_APPS, "/", new_folder, NULL);

	create_folder(full_path);

	gnome_dialog_close(GNOME_DIALOG(dlg->dialog));
	return TRUE;
}

void create_folder_pressed_cb(GtkWidget *w, gpointer data)
{
	Misc_Dialog *dlg;
	GtkWidget *label;

	if (!is_node_editable(current_node))
		{
		gnome_warning_dialog (_("You can't add an entry to that submenu.\nYou do not have the proper permissions."));
		return;
		}

	dlg = g_new(Misc_Dialog, 1);
	
	dlg->dialog = gnome_dialog_new(_("New Submenu"), GNOME_STOCK_BUTTON_OK, GNOME_STOCK_BUTTON_CANCEL, NULL);

	label = gtk_label_new(_("Create Submenu:"));
	gtk_box_pack_start(GTK_BOX(GNOME_DIALOG(dlg->dialog)->vbox),label,FALSE,FALSE,0);
	gtk_widget_show(label);

	dlg->entry = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(GNOME_DIALOG(dlg->dialog)->vbox),dlg->entry,FALSE,FALSE,0);
	gtk_widget_grab_focus(dlg->entry);
	gtk_widget_show(dlg->entry);

	gnome_dialog_button_connect(GNOME_DIALOG(dlg->dialog), 0, (GtkSignalFunc) create_folder_cb, dlg);
	gnome_dialog_button_connect(GNOME_DIALOG(dlg->dialog), 1, (GtkSignalFunc) close_folder_dialog_cb, dlg->dialog);
	gnome_dialog_set_default(GNOME_DIALOG(dlg->dialog), 0);
	gnome_dialog_editable_enters(GNOME_DIALOG(dlg->dialog), GTK_EDITABLE(dlg->entry));

	gtk_widget_show(dlg->dialog);
}



void
new_item_pressed_cb(GtkWidget *w, gpointer data)
{
  GtkCTreeNode *parent;
  Desktop_Data *d;
  GnomeDesktopEntry *dentry;
  gchar *filename, *path;

  if (! is_node_editable(current_node))
    {
      gnome_warning_dialog (_("You can't add an entry to that submenu.\nYou do not have the proper permissions."));
      return;
    }

  new_edit_area();
  dentry = gnome_dentry_get_dentry(GNOME_DENTRY_EDIT(edit_area));

  filename = "untitled.desktop";

  /*
   * HACK:
   *  I dont know this code well, but apparently current-path might
   * be null from time to time, account for that.
   */
  if (!current_path)
	  path = g_strconcat(USER_APPS, "/", filename, NULL);
  else
	  path = g_strconcat(current_path, "/", filename, NULL);
	  

  if (! save_desktop_entry_file(dentry, path, FALSE, FALSE, TRUE))
    {
      g_free(path);
      gnome_desktop_entry_destroy(dentry);
      return;
    }

  d = get_desktop_file_info(path);

  create_desktop_entry_node(d);

  edit_area_select_name();

  gnome_desktop_entry_destroy(dentry);
  g_free(path);
}

static void delete_dialog_cb( gint button, gpointer data)
{
  Desktop_Data *d;
  gchar *path;

  d = gtk_ctree_node_get_row_data(GTK_CTREE(menu_tree_ctree),
				  current_node);
  delete_desktop_entry(d->path);

  new_edit_area();
}

void delete_pressed_cb(GtkWidget *w, gpointer data)
{
	Desktop_Data *d;
	if (!current_node)
		{
		gnome_warning_dialog (_("You must select something first."));
		return;
		}

	if (current_node == topnode || current_node == usernode || current_node == systemnode)
		{
		gnome_warning_dialog (_("You can not delete a top level submenu."));
		return;
		}

	d = gtk_ctree_node_get_row_data(GTK_CTREE(menu_tree_ctree),current_node);

	if (!d->editable)
		{
		gnome_warning_dialog (_("You can't delete that file.\nYou do not have the proper permissions."));
		return;
		}

	if (isfile(d->path))
		{
		gnome_question_dialog (_("Delete this menu item?"),
			(GnomeReplyCallback) delete_dialog_cb, NULL);
		return;
		}

	if (isdir(d->path))
		{
		if (!GTK_CTREE_ROW(current_node)->children)
			{
			gnome_question_dialog (_("Delete empty submenu?"),
				(GnomeReplyCallback) delete_dialog_cb, NULL);
			}
		else
		  {
		    gnome_question_dialog(_("Are you sure you want to delete this submenu and all its contents?"),
					  (GnomeReplyCallback) delete_recursive_cb, current_node);
		  }
		}
	else
	  {
		gnome_question_dialog (_("Delete this menu item?"),
			(GnomeReplyCallback) delete_dialog_cb, NULL);
	  }

}

void
save_pressed_cb(GtkWidget *w, gpointer data)
{
  gchar *orig_path, *path, *savepath;
  GnomeDesktopEntry *dentry;
  Desktop_Data *d, *old_d;
  gboolean is_folder;

  d = gtk_ctree_node_get_row_data(GTK_CTREE(menu_tree_ctree), current_node);
  if (! d->editable)
    {
      gnome_warning_dialog("You do not have the proper permissions to be able to modify this menu item.");
      revert_edit_area();
      return;
    }

  /*
   * (1) Determine the path to which the menu entry should be saved.
   */

  dentry = gnome_dentry_get_dentry(GNOME_DENTRY_EDIT(edit_area));

  old_d = (Desktop_Data *) gtk_ctree_node_get_row_data(
				      GTK_CTREE(menu_tree_ctree),
				      current_node);

  is_folder = FALSE;
  if (edit_area_orig_data != NULL)
    is_folder = edit_area_orig_data->isfolder;

  if (is_folder)
    {
      gchar *parent_dir;
      /*
       * Folder
       */

      /*
       * HACK: I dont know the code well, but current_path might become
       * NULL at some points.  Account for those cases.N
       */
      if (!current_path)
	      parent_dir = g_dirname(USER_APPS);
      else
	      parent_dir = g_dirname(current_path);
      
      path = g_concat_dir_and_file(parent_dir, dentry->name);
      g_free(parent_dir);

      if (current_path)
	      orig_path = g_strdup(current_path);
      else
	      orig_path = g_strdup (USER_APPS);
    }
  else
    {
      gchar *filename;
      /*
       * File
       */

      /*
       * We insist that the menu item name correspond to the name of the
       * .desktop entry.  This will prevent non-savvy users from shooting
       * themselves in the foot.
       */
      filename = g_strconcat(dentry->name, ".desktop", NULL);
      if (current_path)
	      path = g_concat_dir_and_file(current_path, filename);
      else
	      path = g_concat_dir_and_file(USER_APPS, filename);
      g_free(filename);
      orig_path = g_strdup(old_d->path);
    }


  /*
   * (2) If the path has changed, we need to:
   *    (a) make sure the new path doesn't conflict with something
   *    (b) rename the file
   * And if it is a folder that's being renamed, then:
   *    (c) Update current_path
   */
  

  if (orig_path && strcmp(path, orig_path))
    {
      if (g_file_exists(path))
	{
	  gnome_warning_dialog(_("This change conflicts with an existing menu item.\nNo two menu items in a submenu can have the same name."));
	  return;
	}

      if (rename(orig_path, path) < 0)
	g_warning("Unable to rename file %s to %s: %s\n",
		  orig_path, path, g_strerror(errno));

      if (is_folder)
	{
	  if (current_path != NULL)
	    g_free(current_path);
	  current_path = g_strdup(path);
	}
    }

  /*
   * (3) Save the changed menu data to disk.
   */

  if (is_folder)
    savepath = g_concat_dir_and_file(path, ".directory");
  else
    savepath = g_strdup(path);

  if (! save_desktop_entry_file(dentry, savepath, FALSE, FALSE, FALSE))

    {
      gnome_desktop_entry_destroy (dentry);
      g_free(path);
      g_free(orig_path);
      gnome_warning_dialog("Unable to write new menu data");
      return;
    }

  g_free(savepath);


  if (is_folder)
    {
      g_free(dentry->location);
      dentry->location = g_strdup(path);
    }

  /*
   * (4) Update the CTree
   */
  update_desktop_entry_node(dentry, orig_path);

  /*
   * (5) If it was a folder whose name has changed, update the paths
   * of its descendents.
   */
  if (is_folder && (strcmp (path, orig_path)))
    {
      GtkCTreeNode *node;

      node = find_file_in_tree(GTK_CTREE(menu_tree_ctree), path);
      gtk_ctree_pre_recursive(GTK_CTREE(menu_tree_ctree), node,
			      recalc_paths_cb, NULL);
    }

  /*
   * (6) Update the edit area
   */
  d = get_desktop_file_info (path);

  update_edit_area(d);
  edit_area_reset_revert(d);

  gnome_desktop_entry_destroy (dentry);
  free_desktop_data(d);
  g_free(orig_path);
  g_free(path);
}

