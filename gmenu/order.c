/*###################################################################*/
/*##                       gmenu (GNOME menu editor)               ##*/
/*###################################################################*/

#include "gmenu.h"
#include "folder.xpm"
#include "unknown.xpm"

GList *get_order_of_dir(gchar *dir)
{
	gchar buf[256];
	GList *list = NULL;
	gchar *order_file = g_strconcat(dir, "/.order", NULL);
	FILE *f;

	f = fopen(order_file,"r");
	if (!f)
		{
		g_free(order_file);
		return NULL;
		}

	while(fgets(buf, 255, f)!=NULL)
		{
		char *buf_ptr;
		buf_ptr = strchr(buf,'\n');
		if (buf_ptr) buf_ptr[0] = '\0';
		if (strlen(buf) > 0) list = g_list_append(list,g_strdup(buf));
		}

	fclose(f);

	g_free(order_file);
	return list;
}

void save_order_of_dir(GtkCTreeNode *node)
{
	Desktop_Data *d;
	gboolean leaf;
	GtkCTreeNode *parent;
	GtkCTreeNode *row;
	gchar *row_file;
	FILE *f;

	gtk_ctree_get_node_info(GTK_CTREE(menu_tree_ctree),node,
		NULL,NULL,NULL,NULL,NULL,NULL,&leaf,NULL);
	if (leaf)
		parent = GTK_CTREE_ROW(node)->parent;
	else
		parent = node;

	d = gtk_ctree_node_get_row_data(GTK_CTREE(menu_tree_ctree), parent);
	row_file = g_strconcat(d->path, "/.order", NULL);

	row = GTK_CTREE_ROW(parent)->children;

	if (row)
		{
		f = fopen(row_file, "w");
		if (!f)
			{
			g_print(_("Unable to create file: %s\n"),row_file);
			g_free(row_file);
			return;
			}

		while (row)
			{
			d = gtk_ctree_node_get_row_data(GTK_CTREE(menu_tree_ctree), row);
			fprintf(f, "%s\n", d->path + g_filename_index(d->path));
			row = GTK_CTREE_ROW(row)->sibling;
			}
		fclose(f);
		}
	else
		{
		/* the folder is empty, so delete the .order file */
		if (g_file_exists(row_file))
			{
			if (unlink (row_file) < 0)
				g_print(_("unable to remove .order file: %s\n"),row_file);
			}
		}

	g_free(row_file);
}

Desktop_Data * get_desktop_file_info (gchar *file)
{
	Desktop_Data *d;
	GnomeDesktopEntry *dentry;

	if (!g_file_exists(file))
		{
		g_print("not a valid file or dir\n");
		return NULL;
		}

	d = g_new0(Desktop_Data, 1);

	d->path = g_strdup(file);
	d->name = NULL;
	d->comment = NULL;
	d->dentry = NULL;
	d->pixmap = NULL;
	d->isfolder = FALSE;
	d->expanded = FALSE;
	d->editable = TRUE;


	if (isdir(file))
		{
		gchar *dirfile = g_concat_dir_and_file(file, ".directory");
		d->isfolder = TRUE;
		dentry = gnome_desktop_entry_load_unconditional(dirfile);
		if (dentry)
			{
			if (dentry->name)
				d->name = g_strdup(dentry->name);
			else
				d->name = g_strdup(file + g_filename_index(file));
			if (dentry->comment)
				d->comment = g_strdup(dentry->comment);
			else
				d->comment = g_strconcat(d->name , _(" Folder"), NULL);
			if (dentry->icon)
				{
				d->pixmap = gnome_stock_pixmap_widget_at_size (NULL, dentry->icon, 20, 20);
				if (!d->pixmap)
					d->pixmap = gnome_pixmap_new_from_xpm_d (folder_xpm);
				}
			else
				d->pixmap = gnome_pixmap_new_from_xpm_d (folder_xpm);
			gnome_desktop_entry_destroy(dentry);
			}
		else
			{
			d->name = g_strdup(file + g_filename_index(file));
			d->comment = g_strconcat(d->name , _(" Folder"), NULL);
			d->pixmap = gnome_pixmap_new_from_xpm_d (folder_xpm);
			}
		g_free(dirfile);
		return d;
		}

	dentry = gnome_desktop_entry_load_unconditional(file);

	if (!dentry) return NULL;

	if (dentry->name)
		d->name = g_strdup(dentry->name);
	else
		d->name = g_strdup(file + g_filename_index(file));
	if (dentry->comment)
		d->comment = g_strdup(dentry->comment);
	else
		d->comment = g_strdup("");
	if (dentry->icon)
		{
		d->pixmap = gnome_stock_pixmap_widget_at_size (NULL, dentry->icon, 20, 20);
		if (!d->pixmap)
			d->pixmap = gnome_pixmap_new_from_xpm_d (unknown_xpm);
		}
	else
		{
		d->pixmap = gnome_pixmap_new_from_xpm_d (unknown_xpm);
		}

	gnome_desktop_entry_destroy(dentry);
	return d;
}

void free_desktop_data(Desktop_Data *d)
{
	if (!d) return;
	if (d->path) g_free (d->path);
	if (d->name) g_free (d->name);
	if (d->comment) g_free (d->comment);
	if (d->dentry) gnome_desktop_entry_destroy (d->dentry);
	g_free (d);
}

