/*
 * GNOME menu editor revision 2
 * (C)1999
 *
 * Authors: John Ellis <johne@bellatlantic.net>
 *          Nat Friedman <nat@nat.org>
 *
 */

#include "gmenu.h"

/*
 *-----------------------------------------------------------------------------
 * Desktop_Data utils
 *-----------------------------------------------------------------------------
 */

Desktop_Data *desktop_data_new(gchar *path, gchar *name, gchar *comment, GtkWidget *pixmap)
{
	Desktop_Data *dd;

	dd = g_new0(Desktop_Data, 1);

	if (path)
		{
		dd->path = g_strdup(path);
		dd->editable = file_is_editable(dd->path);
		dd->isfolder = isdir(dd->path);
		}
	if (name) dd->name = g_strdup(name);
	if (comment) dd->comment = g_strdup(comment);

	dd->pixmap = pixmap;
	dd->expanded = FALSE;

	return dd;
}

Desktop_Data *desktop_data_new_from_path(gchar *path)
{
	Desktop_Data *dd;
	GnomeDesktopEntry *dentry = NULL;

	dd = g_new0(Desktop_Data, 1);

	dd->path = g_strdup(path);

	if (isdir(path))
		{
		gchar *dir_file = g_concat_dir_and_file(dd->path, ".directory");
		dd->isfolder = TRUE;
		if (isfile(dir_file))
			{
			dd->editable = file_is_editable(dir_file);
			dentry = gnome_desktop_entry_load_unconditional(dir_file);
			}
		else
			{
			dd->editable = file_is_editable(dd->path);
			}
		g_free(dir_file);
		}
	else
		{
		dd->isfolder = FALSE;
		dd->editable = file_is_editable(dd->path);
		dentry = gnome_desktop_entry_load_unconditional(path);
		}

	if (dentry && dentry->name)
		dd->name = g_strdup(dentry->name);
	else
		dd->name = g_strdup(dd->path + g_filename_index(dd->path));

	if (dentry && dentry->comment)
		dd->comment = g_strdup(dentry->comment);
	else
		{
		if (dd->isfolder)
			dd->comment = g_strconcat(dd->name , _(" Folder"), NULL);
		else
			dd->comment = g_strdup("");
		}

	if (dentry && dentry->icon)
		dd->pixmap = pixmap_load(dentry->icon);
	else
		dd->pixmap = pixmap_unknown();

	if (dentry) gnome_desktop_entry_destroy(dentry);

	return dd;
}

void desktop_data_free(Desktop_Data *dd)
{
	if (dd)
		{
		g_free(dd->path);
		g_free(dd->name);
		g_free(dd->comment);
		g_free(dd);
		}
}


