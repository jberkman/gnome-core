/*###################################################################*/
/*##                       gmenu (GNOME menu editor) 0.2.4         ##*/
/*###################################################################*/

#include "gmenu.h"
#include "unknown.xpm"

static GnomeDesktopEntry *revert_dentry;

static void icon_cb(void *data);

static void icon_cb(void *data)
{
	gchar *icon = data;
/*	g_print("icon = %s\n",icon);*/
	gtk_entry_set_text(GTK_ENTRY(icon_entry), icon);

	gnome_pixmap_load_file(GNOME_PIXMAP(desktop_icon),
		correct_path_to_file(SYSTEM_PIXMAPS, USER_PIXMAPS, icon));

	g_free(icon);
}

void icon_button_pressed()
{
	char *extra_pixmaps;
	Desktop_Data *d = gtk_ctree_get_row_data(GTK_CTREE(menu_tree_ctree), systemnode);

	/* check if the user is root (systemnode is editable), and if so do not show
	   the root user's pixmaps, since other users cannot access the root's icons.
	   (this is so that the system menus always have system icons) */
	if (!d->editable)
		extra_pixmaps = USER_PIXMAPS;
	else
		extra_pixmaps = NULL;
	icon_selection_dialog(SYSTEM_PIXMAPS, extra_pixmaps,
		gtk_entry_get_text(GTK_ENTRY(icon_entry)), FALSE, icon_cb );
}

void update_edit_area(Desktop_Data *d)
{
	gnome_dentry_edit_load_file(edit_area, d->path);

	if (d->path)
		gtk_entry_set_text(GTK_ENTRY(filename_entry), d->path + g_filename_index (d->path));
	else
		gtk_entry_set_text(GTK_ENTRY(filename_entry), "");

	if (revert_dentry) gnome_desktop_entry_destroy(revert_dentry);
	revert_dentry = gnome_dentry_get_dentry(edit_area);

	edit_area_orig_data = d;

/*	if (d->name)
		gtk_entry_set_text(GTK_ENTRY(name_entry), d->name);
	else
		gtk_entry_set_text(GTK_ENTRY(name_entry), "");

	if (d->comment)
		gtk_entry_set_text(GTK_ENTRY(comment_entry), d->comment);
	else
		gtk_entry_set_text(GTK_ENTRY(comment_entry), "");

	if (d->path)
		gtk_entry_set_text(GTK_ENTRY(filename_entry), d->path + g_filename_index (d->path));
	else
		gtk_entry_set_text(GTK_ENTRY(filename_entry), "");

	if (d->exec)
		gtk_entry_set_text(GTK_ENTRY(exec_entry), d->exec);
	else
		gtk_entry_set_text(GTK_ENTRY(exec_entry), "");

	if (d->type)
		gtk_entry_set_text(GTK_ENTRY(type_entry), d->type);
	else
		gtk_entry_set_text(GTK_ENTRY(type_entry), "");

	if (d->icon)
		{
		gchar *buf;
		gtk_entry_set_text(GTK_ENTRY(icon_entry), d->icon);
		buf = correct_path_to_file(SYSTEM_PIXMAPS, USER_PIXMAPS, d->icon);
		if (buf)
			gnome_pixmap_load_file(GNOME_PIXMAP(desktop_icon), buf);
		else
			gnome_pixmap_load_xpm_d(GNOME_PIXMAP(desktop_icon), unknown_xpm);
		}
	else
		{
		gtk_entry_set_text(GTK_ENTRY(icon_entry), "");
		gnome_pixmap_load_xpm_d(GNOME_PIXMAP(desktop_icon), unknown_xpm);
		}

	gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON(terminal_button), d->terminal);

	if (d->tryexec)
		gtk_entry_set_text(GTK_ENTRY(tryexec_entry), d->tryexec);
	else
		gtk_entry_set_text(GTK_ENTRY(tryexec_entry), "");

	if (d->doc)
		gtk_entry_set_text(GTK_ENTRY(doc_entry), d->doc);
	else
		gtk_entry_set_text(GTK_ENTRY(doc_entry), "");

	gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON(multi_args_button), d->multiple_args);
*/
}

void revert_edit_area()
{
	if (revert_dentry) gnome_dentry_edit_set_dentry(edit_area, revert_dentry);

	if (edit_area_orig_data)
		gtk_entry_set_text(GTK_ENTRY(filename_entry),
			edit_area_orig_data->path + g_filename_index (edit_area_orig_data->path));
}

void new_edit_area()
{
	GnomeDesktopEntry *dentry;

	gnome_dentry_edit_clear(edit_area);
	if (revert_dentry)
		{
		gnome_desktop_entry_destroy(revert_dentry);
		revert_dentry = NULL;
		}

	gtk_entry_set_text(GTK_ENTRY(filename_entry), "untitled.desktop");

	edit_area_orig_data = NULL;

/*	gtk_entry_set_text(GTK_ENTRY(name_entry), "");
	gtk_entry_set_text(GTK_ENTRY(comment_entry), "");

	if (current_path)
		gtk_entry_set_text(GTK_ENTRY(filename_entry), "untitled.desktop");
	else
		gtk_entry_set_text(GTK_ENTRY(filename_entry), "");

	gtk_entry_set_text(GTK_ENTRY(exec_entry), "");
	gtk_entry_set_text(GTK_ENTRY(type_entry), "Application");

	gtk_entry_set_text(GTK_ENTRY(icon_entry), "");

	gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON(terminal_button), FALSE);

	gnome_pixmap_load_xpm_d(GNOME_PIXMAP(desktop_icon), unknown_xpm);

	gtk_entry_set_text(GTK_ENTRY(tryexec_entry), "");
	gtk_entry_set_text(GTK_ENTRY(doc_entry), "");

	gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON(multi_args_button), FALSE);
*/
}

