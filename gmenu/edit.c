/*###################################################################*/
/*##                       gmenu (GNOME menu editor) 0.2.4         ##*/
/*###################################################################*/

#include "gmenu.h"

static Desktop_Data *edit_area_orig_data;
static GnomeDesktopEntry *revert_dentry;

void update_edit_area(Desktop_Data *d)
{
	gnome_dentry_edit_load_file(GNOME_DENTRY_EDIT(edit_area), d->path);

	if (d->path)
		gtk_entry_set_text(GTK_ENTRY(filename_entry), d->path + g_filename_index (d->path));
	else
		gtk_entry_set_text(GTK_ENTRY(filename_entry), "");

	if (revert_dentry) gnome_desktop_entry_destroy(revert_dentry);
	revert_dentry = gnome_dentry_get_dentry(GNOME_DENTRY_EDIT(edit_area));

	edit_area_orig_data = d;
}

void revert_edit_area()
{
	if (revert_dentry) gnome_dentry_edit_set_dentry(GNOME_DENTRY_EDIT(edit_area), revert_dentry);

	if (edit_area_orig_data)
		gtk_entry_set_text(GTK_ENTRY(filename_entry),
			edit_area_orig_data->path + g_filename_index (edit_area_orig_data->path));
}

void new_edit_area()
{
	gnome_dentry_edit_clear(GNOME_DENTRY_EDIT(edit_area));
	if (revert_dentry)
		{
		gnome_desktop_entry_destroy(revert_dentry);
		revert_dentry = NULL;
		}

	gtk_entry_set_text(GTK_ENTRY(filename_entry), "untitled.desktop");

	edit_area_orig_data = NULL;
}

