/*###################################################################*/
/*##                       gmenu (GNOME menu editor) 0.2.4         ##*/
/*###################################################################*/

#include "gmenu.h"
#include "folder.xpm"
#include "unknown.xpm"

GList *get_order_of_dir(char *dir)
{
	char buf[256];
	GList *list = NULL;
	char *order_file = g_copy_strings(dir, "/.order", NULL);
	FILE *f;

/*	g_print("reading .order file: %s\n", order_file);*/

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
/*		g_print("%s,",buf);*/
		if (strlen(buf) > 0) list = g_list_append(list,strdup(buf));
		}

	fclose(f);

/*	g_print("\n");*/

	g_free(order_file);
	return list;
}

void save_order_of_dir(GList *node)
{
	Desktop_Data *d;
	gboolean leaf;
	GList *parent;
	GList *row;
	char *row_file;
	FILE *f;

	gtk_ctree_get_node_info(GTK_CTREE(menu_tree_ctree),node,
		NULL,NULL,NULL,NULL,NULL,NULL,&leaf,NULL);
	if (leaf)
		parent = GTK_CTREE_ROW(node)->parent;
	else
		parent = node;

	d = gtk_ctree_get_row_data(GTK_CTREE(menu_tree_ctree), parent);
	row_file = g_copy_strings(d->path, "/.order", NULL);

/*	g_print("saving .order file: %s\n", row_file);*/

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
			d = gtk_ctree_get_row_data(GTK_CTREE(menu_tree_ctree), row);
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
/*			g_print(_("removing .order file: %s\n"),row_file);*/
			if (unlink (row_file) < 0)
				g_print(_("unable to remove .order file: %s\n"),row_file);
			}
		}

	g_free(row_file);
}

void free_order_list(GList *orderlist)
{
	if (orderlist)
		{
		int i;
		int l = g_list_length(orderlist);
		for (i=0;i<l;i++)
			{
			GList *list = g_list_nth(orderlist, i);
			g_free(list->data);
			}
		g_list_free(orderlist);
		}
}

Desktop_Data * get_desktop_file_info (char *file)
{
	Desktop_Data *d;
	GnomeDesktopEntry *dentry;

/*	g_print("reading file: %s\n",file);*/

	if (!g_file_exists(file))
		{
		g_print("not a valid file or dir\n");
		return NULL;
		}

	d = g_new(Desktop_Data, 1);

	d->path = strdup(file);
	d->name = NULL;
	d->comment = NULL;
	d->tryexec = NULL;
	d->exec = NULL;
	d->icon = NULL;
	d->terminal = 0;
	d->doc = NULL;
	d->type = NULL;
	d->pixmap = NULL;
	d->isfolder = FALSE;
	d->expanded = FALSE;
	d->editable = TRUE;


	if (isdir(file))
		{
		d->isfolder = TRUE;
		d->name = strdup(file + g_filename_index(file));
		d->comment = g_copy_strings(d->name , _(" Folder"), NULL);
		d->pixmap = gnome_pixmap_new_from_xpm_d (folder_xpm);
		return d;
		}

	dentry = gnome_desktop_entry_load_unconditional(file);

	if (!dentry) return NULL;

	if (dentry->name) d->name = strdup(dentry->name);
	if (dentry->comment) d->comment = strdup(dentry->comment);
	if (dentry->tryexec) d->tryexec = strdup(dentry->tryexec);
	d->exec = NULL;
	if (dentry->icon) d->icon = strdup(dentry->icon);
	if (dentry->type) d->type = strdup(dentry->type);
	d->terminal = dentry->terminal;
	if (dentry->docpath) d->doc = strdup(dentry->docpath);

	if (d->icon)
		{
/*		gchar *icon_path;
		icon_path = correct_path_to_file(SYSTEM_PIXMAPS, USER_PIXMAPS, d->icon);
		if (icon_path)
*/
		d->pixmap = gnome_pixmap_new_from_file_at_size (d->icon, 20, 20);
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
	if (d->path) free (d->path);
	if (d->name) free (d->name);
	if (d->comment) free (d->comment);
	if (d->tryexec) free (d->tryexec);
	if (d->exec) free (d->exec);
	if (d->icon) free (d->icon);
	if (d->type) free (d->type);
	if (d->doc) free (d->doc);

	/*what to do with this? if (d->pixmap) gtk_unref (d->pixmap); */

	free (d);
}


