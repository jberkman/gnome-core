/*###################################################################*/
/*##                       gmenu (GNOME menu editor) 0.3.0         ##*/
/*###################################################################*/

#include <config.h>

#include "gmenu.h"

static gint close_folder_dialog_cb(GtkWidget *b, gpointer data);
static gint create_folder_cb(GtkWidget *w, gpointer data);
static void delete_dialog_cb( gint button, gpointer data);
static void save_dialog_cb( gint button, gpointer data);

static gint close_folder_dialog_cb(GtkWidget *b, gpointer data)
{
	GtkWidget *w = data;
	gnome_dialog_close(GNOME_DIALOG(w));
	return TRUE;
}

static gint create_folder_cb(GtkWidget *w, gpointer data)
{
	Misc_Dialog *dlg = data;
	gchar *new_folder;
	gchar *full_path;
	gint write_file = TRUE;

	new_folder = gtk_entry_get_text(GTK_ENTRY(dlg->entry));

	if (current_path)
		full_path = g_copy_strings(current_path, "/", new_folder, NULL);
	else
		full_path = g_copy_strings(USER_APPS, "/", new_folder, NULL);

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

				update_edit_area(d);

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
				gtk_ctree_node_set_row_data (GTK_CTREE(menu_tree_ctree), node, d);
				save_order_of_dir(parent);
				add_tree_node(GTK_CTREE(menu_tree_ctree), node, NULL);
				update_tree_highlight(menu_tree_ctree, current_node, node, TRUE);
				current_node = node;
				}
			}
		}

	g_free(full_path);	
	gnome_dialog_close(GNOME_DIALOG(dlg->dialog));
	return TRUE;
}

void create_folder_pressed()
{
	Misc_Dialog *dlg;
	GtkWidget *label;

	if (!is_node_editable(current_node))
		{
		gnome_warning_dialog (_("You can't add an entry to that folder!\nTo edit system entries you must be root."));
		return;
		}

	dlg = g_new(Misc_Dialog, 1);
	
	dlg->dialog = gnome_dialog_new(_("New Folder"), GNOME_STOCK_BUTTON_OK, GNOME_STOCK_BUTTON_CANCEL, NULL);

	label = gtk_label_new(_("Create Folder:"));
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


static void delete_dialog_cb( gint button, gpointer data)
{
	if (!button)
		{
		Desktop_Data *d;
		GtkCTreeNode *node;
		d = gtk_ctree_node_get_row_data(GTK_CTREE(menu_tree_ctree),current_node);

		if (d->isfolder)
			{
			gchar *dirfile = g_concat_dir_and_file (d->path, ".directory");
			if (g_file_exists(dirfile))
				{
				if ( (unlink (dirfile) < 0) )
					g_print("Failed to delete %s\n", dirfile);
				}
			g_free(dirfile);

			if ( (rmdir (d->path) < 0) )
				{
				gnome_warning_dialog (_("Failed to delete the folder."));
				return;
				}
			}
		else
			{
			if ( (unlink (d->path) < 0) )
				{
				gnome_warning_dialog (_("Failed to delete the file."));
				return;
				}
			}

/*		g_print("deleted file: %s\n",d->path);*/

		if (GTK_CTREE_ROW(current_node)->sibling)
			node = GTK_CTREE_ROW(current_node)->sibling;
		else
			{
			node = GTK_CTREE_ROW(current_node)->parent;
			if (GTK_CTREE_ROW(node)->children != current_node)
				{
				node = GTK_CTREE_ROW(node)->children;
				while(GTK_CTREE_ROW(node)->sibling != current_node) node = GTK_CTREE_ROW(node)->sibling;
				}
			}

		update_tree_highlight(menu_tree_ctree, current_node, node, TRUE);
		gtk_ctree_remove_node(GTK_CTREE(menu_tree_ctree),current_node);
		current_node = node;

		free_desktop_data(d);
		save_order_of_dir(node);

		new_edit_area();
		}
}

void delete_pressed_cb()
{
	Desktop_Data *d;
	if (!current_node)
		{
		gnome_warning_dialog (_("You must select something first!"));
		return;
		}

	if (current_node == topnode || current_node == usernode || current_node == systemnode)
		{
		gnome_warning_dialog (_("You can not delete a top level Folder!"));
		return;
		}

	d = gtk_ctree_node_get_row_data(GTK_CTREE(menu_tree_ctree),current_node);

	if (!d->editable)
		{
		gnome_warning_dialog (_("You can't delete that file!\nTo edit system entries you must be root."));
		return;
		}

	if (isfile(d->path))
		{
		gnome_question_dialog (_("Delete file?"),
			(GnomeReplyCallback) delete_dialog_cb, NULL);
		return;
		}

	if (isdir(d->path))
		{
		if (!GTK_CTREE_ROW(current_node)->children)
			{
			gnome_question_dialog (_("Delete empty folder?"),
				(GnomeReplyCallback) delete_dialog_cb, NULL);
			}
		else
			gnome_warning_dialog (_("Cannot delete folder.\nTo delete a folder. it must be empty."));
		}
	else
		gnome_warning_dialog (_("File or Folder does not exist on filesystem."));
}


static void save_dialog_cb( gint button, gpointer data)
{
	if (!button)
		{
		Desktop_Data *d;
		GtkCTreeNode *node;
		GtkCTreeNode *parent;
		gint overwrite;
		char *path;
		GnomeDesktopEntry *dentry = NULL;


		if (edit_area_orig_data && edit_area_orig_data->isfolder)
			{
			path = g_strdup(edit_area_get_filename());
			overwrite = TRUE;
			}
		else
			{
			path = g_copy_strings(current_path, "/",
				edit_area_get_filename(), NULL);
			overwrite = isfile(path);
			}

		dentry = gnome_dentry_get_dentry(GNOME_DENTRY_EDIT(edit_area));
		dentry->location = g_strdup(path);
		gnome_desktop_entry_save (dentry);
		gnome_desktop_entry_destroy (dentry);

		if (overwrite && edit_area_orig_data && edit_area_orig_data->isfolder)
			{
			g_free(path);
			path = g_strdup(edit_area_orig_data->path);
			}

		d = get_desktop_file_info (path);
		if (!d)
			{
			g_print("unable to load desktop file for: %s\n", path);
			g_free(path);
			return;
			}

		if (overwrite) node = find_file_in_tree(GTK_CTREE(menu_tree_ctree), path);

		if (overwrite && node)
			{
			gint8 spacing;
			gboolean leaf;
			gboolean expanded;

			free_desktop_data(gtk_ctree_node_get_row_data(GTK_CTREE(menu_tree_ctree), node));

			/* since we are saving, it it safe to assume a folder's
			submenus have been read */
			if (d->isfolder) d->expanded = TRUE;

			gtk_ctree_node_set_row_data(GTK_CTREE(menu_tree_ctree), node, d);

			gtk_ctree_get_node_info (GTK_CTREE(menu_tree_ctree), node, NULL, &spacing,
						NULL, NULL, NULL, NULL, &leaf, &expanded);
			gtk_ctree_set_node_info (GTK_CTREE(menu_tree_ctree), node, d->name, spacing,
						GNOME_PIXMAP(d->pixmap)->pixmap, GNOME_PIXMAP(d->pixmap)->mask,
						GNOME_PIXMAP(d->pixmap)->pixmap, GNOME_PIXMAP(d->pixmap)->mask,
						leaf, expanded);
			save_order_of_dir(node);
			}
		else
			{
			gchar *text[2];
			gboolean leaf;

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
			gtk_ctree_node_set_row_data (GTK_CTREE(menu_tree_ctree), node, d);
			save_order_of_dir(node);
			}

		edit_area_reset_revert(d);

		update_tree_highlight(menu_tree_ctree, current_node, node, TRUE);
		current_node = node;
		g_free(path);
		}
} 

void save_pressed_cb()
{
	char *path;
	GnomeDesktopEntry *dentry;

	path = g_copy_strings(current_path, "/", edit_area_get_filename(), NULL);

	dentry = gnome_dentry_get_dentry(GNOME_DENTRY_EDIT(edit_area));
	if (!dentry->name)
		{
		gnome_warning_dialog (_("The Name text field can not be blank."));
		gnome_desktop_entry_destroy(dentry);
		return;
		}
	gnome_desktop_entry_destroy(dentry);

	if (!is_node_editable(current_node))
		{
		if (isfile(path))
			gnome_warning_dialog (_("You can't edit an entry in that folder!\nTo edit system entries you must be root."));
		else
			gnome_warning_dialog (_("You can't add an entry to that folder!\nTo edit system entries you must be root."));
		g_free(path);
		return;
		}

	if (edit_area_orig_data && edit_area_orig_data->isfolder)
		{
		gnome_question_dialog (_("Save Changes?"),
			(GnomeReplyCallback) save_dialog_cb, NULL);
		g_free(path);
		return;
		}

	if (isfile(path))
		{
		gnome_question_dialog (_("Overwrite existing file?"),
			(GnomeReplyCallback) save_dialog_cb, NULL);
		g_free(path);
		return;
		}

	gnome_question_dialog (_("Save file?"),
		(GnomeReplyCallback) save_dialog_cb, NULL);
	g_free(path);
}

