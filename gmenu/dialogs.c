/*###################################################################*/
/*##                       gmenu (GNOME menu editor) 0.2.4         ##*/
/*###################################################################*/

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
			GList *parent;
			GList *node;
			d = get_desktop_file_info (full_path);
			if (d)
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
					node = gtk_ctree_insert (GTK_CTREE(menu_tree_ctree), parent, node, text, 5,
						GNOME_PIXMAP(d->pixmap)->pixmap,
						GNOME_PIXMAP(d->pixmap)->mask, NULL, NULL, FALSE, FALSE);
				else
					node = gtk_ctree_insert (GTK_CTREE(menu_tree_ctree), parent, node, text, 5,
						GNOME_PIXMAP(d->pixmap)->pixmap,
						GNOME_PIXMAP(d->pixmap)->mask, NULL, NULL, TRUE, FALSE);
				gtk_ctree_set_row_data (GTK_CTREE(menu_tree_ctree), node, d);
				save_order_of_dir(parent);
				add_tree_node(GTK_CTREE(menu_tree_ctree), node);
				update_tree_highlight(menu_tree_ctree, current_node, node, FALSE);
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
	
	dlg->dialog = gnome_dialog_new("New Folder", GNOME_STOCK_BUTTON_OK, GNOME_STOCK_BUTTON_CANCEL, NULL);

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
		GList *node;
		d = gtk_ctree_get_row_data(GTK_CTREE(menu_tree_ctree),current_node);

		if (d->isfolder)
			{
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

		update_tree_highlight(menu_tree_ctree, current_node, node, FALSE);
		gtk_ctree_remove(GTK_CTREE(menu_tree_ctree),current_node);
		current_node = node;

		free_desktop_data(d);
		save_order_of_dir(node);

		edit_area_orig_data = NULL;
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

	d = gtk_ctree_get_row_data(GTK_CTREE(menu_tree_ctree),current_node);

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
		GList *node;
		GList *parent;
		gint overwrite;
		char *path;
		GnomeDesktopEntry *dentry = NULL;

		path = g_copy_strings(current_path, "/", gtk_entry_get_text(GTK_ENTRY(filename_entry)), NULL);

		overwrite = isfile(path);

		dentry = gnome_dentry_get_dentry(edit_area);
		dentry->location = strdup(path);
		gnome_desktop_entry_save (dentry);
		gnome_desktop_entry_destroy (dentry);

/*		save_desktop_file_info (path,
					gtk_entry_get_text(GTK_ENTRY(name_entry)),
					gtk_entry_get_text(GTK_ENTRY(comment_entry)),
					gtk_entry_get_text(GTK_ENTRY(tryexec_entry)),
					gtk_entry_get_text(GTK_ENTRY(exec_entry)),
					gtk_entry_get_text(GTK_ENTRY(icon_entry)),
					GTK_TOGGLE_BUTTON (terminal_button)->active,
					gtk_entry_get_text(GTK_ENTRY(type_entry)),
					gtk_entry_get_text(GTK_ENTRY(doc_entry)),
					GTK_TOGGLE_BUTTON (multi_args_button)->active);

*/
		if (overwrite)
			{
			gint8 spacing;
			gboolean leaf;
			gboolean expanded;

			node = find_file_in_tree(GTK_CTREE(menu_tree_ctree) ,path);
			d = gtk_ctree_get_row_data(GTK_CTREE(menu_tree_ctree), node);
			free_desktop_data(d);

			d = get_desktop_file_info (path);

			gtk_ctree_set_row_data(GTK_CTREE(menu_tree_ctree), node, d);

			gtk_ctree_get_node_info (GTK_CTREE(menu_tree_ctree), node, NULL, &spacing,
						NULL, NULL, NULL, NULL, &leaf, &expanded);
			gtk_ctree_set_node_info (GTK_CTREE(menu_tree_ctree), node, d->name, spacing,
						GNOME_PIXMAP(d->pixmap)->pixmap, GNOME_PIXMAP(d->pixmap)->mask,
						NULL, NULL, leaf, expanded);
			save_order_of_dir(node);
			}
		else
			{
			d = get_desktop_file_info (path);
			if (d)
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
					node = gtk_ctree_insert (GTK_CTREE(menu_tree_ctree), parent, node, text, 5,
						GNOME_PIXMAP(d->pixmap)->pixmap,
						GNOME_PIXMAP(d->pixmap)->mask, NULL, NULL, FALSE, FALSE);
				else
					node = gtk_ctree_insert (GTK_CTREE(menu_tree_ctree), parent, node, text, 5,
						GNOME_PIXMAP(d->pixmap)->pixmap,
						GNOME_PIXMAP(d->pixmap)->mask, NULL, NULL, TRUE, FALSE);
				gtk_ctree_set_row_data (GTK_CTREE(menu_tree_ctree), node, d);
				save_order_of_dir(node);
				}
			}
		update_tree_highlight(menu_tree_ctree, current_node, node, FALSE);
		current_node = node;
		g_free(path);
		}
} 

void save_pressed_cb()
{
	char *path;

	path = g_copy_strings(current_path, "/", gtk_entry_get_text(GTK_ENTRY(filename_entry)), NULL);

	if (!is_node_editable(current_node))
		{
		if (isfile(path))
			gnome_warning_dialog (_("You can't edit an entry in that folder!\nTo edit system entries you must be root."));
		else
			gnome_warning_dialog (_("You can't add an entry to that folder!\nTo edit system entries you must be root."));
		return;
		}


	if (isfile(path))
		{
		gnome_question_dialog (_("Overwrite existing file?"),
			(GnomeReplyCallback) save_dialog_cb, NULL);
		return;
		}

	gnome_question_dialog (_("Save file?"),
		(GnomeReplyCallback) save_dialog_cb, NULL);
	g_free(path);
}

