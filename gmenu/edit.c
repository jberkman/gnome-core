/*
 * GNOME menu editor revision 2
 * (C)1999
 *
 * Authors: John Ellis <johne@bellatlantic.net>
 *          Nat Friedman <nat@nat.org>
 *
 */

#include "gmenu.h"

typedef struct _Edit_Area Edit_Area;
struct _Edit_Area
{
	GtkWidget *vbox;
	GtkObject *dee;
	GtkWidget *save_button;
	GtkWidget *revert_button;
	GtkWidget *pathlabel;

	gint isfolder;
	gchar *path;
	GnomeDesktopEntry *revert;
};

static Edit_Area *edit = NULL;

static void edit_area_enable_name_widgets(gint enabled, Edit_Area *ea);
static void edit_area_enable_basic_widgets(gint enabled, Edit_Area *ea);
static void edit_area_enable_non_dir_widgets(gint enabled, Edit_Area *ea);
static void edit_area_enable_save_button(gint enable, Edit_Area *ea);
static void edit_area_enable_revert_button(gint enable, Edit_Area *ea);
static void edit_area_grab_name_entry(Edit_Area *ea);

static void edit_area_set_as_top_menu(gchar *name, Edit_Area *ea);
static GnomeDesktopEntry *edit_area_real_get_dentry(Edit_Area *ea);
static void edit_area_set_path(gchar *path, Edit_Area *ea);
static void edit_area_sync_to(Desktop_Data *dd, Edit_Area *ea);
static void edit_area_real_clear(gchar *path, gchar *name, Edit_Area *ea);

static void edit_area_changed_cb(GtkObject *dee, gpointer data);
static void edit_area_revert_cb(GtkWidget *widget, gpointer data);
static void edit_area_save_cb(GtkWidget *widget, gpointer data);

static Edit_Area *edit_area_new();

/*
 *-----------------------------------------------------------------------------
 * Widget sensitivity functions (private)
 *-----------------------------------------------------------------------------
 */

static void edit_area_enable_name_widgets(gint enabled, Edit_Area *ea)
{
	gtk_widget_set_sensitive(GNOME_DENTRY_EDIT(ea->dee)->name_entry, enabled);
	gtk_widget_set_sensitive(GNOME_DENTRY_EDIT(ea->dee)->comment_entry, enabled);
}

static void edit_area_enable_basic_widgets(gint enabled, Edit_Area *ea)
{
	gtk_widget_set_sensitive(GNOME_DENTRY_EDIT(ea->dee)->icon_entry, enabled);
	edit_area_enable_name_widgets(enabled, ea);
}

static void edit_area_enable_non_dir_widgets(gint enabled, Edit_Area *ea)
{
	gtk_widget_set_sensitive(GNOME_DENTRY_EDIT(ea->dee)->exec_entry, enabled);
	gtk_widget_set_sensitive(GNOME_DENTRY_EDIT(ea->dee)->tryexec_entry, enabled);
	gtk_widget_set_sensitive(GNOME_DENTRY_EDIT(ea->dee)->doc_entry, enabled);
	gtk_widget_set_sensitive(GNOME_DENTRY_EDIT(ea->dee)->type_combo, enabled);
	gtk_widget_set_sensitive(GNOME_DENTRY_EDIT(ea->dee)->terminal_button, enabled);
}

static void edit_area_enable_save_button(gint enable, Edit_Area *ea)
{
	gtk_widget_set_sensitive(ea->save_button, enable);
}

static void edit_area_enable_revert_button(gint enable, Edit_Area *ea)
{
	gtk_widget_set_sensitive(ea->revert_button, enable);
}

static void edit_area_grab_name_entry(Edit_Area *ea)
{
	GtkWidget *entry;
	gint length;

	entry = GNOME_DENTRY_EDIT(ea->dee)->name_entry;
	length = strlen(gtk_entry_get_text(GTK_ENTRY(entry)));
	gtk_widget_grab_focus(entry);
	gtk_entry_select_region(GTK_ENTRY(entry), 0, length);
}

/*
 *-----------------------------------------------------------------------------
 *  get, set, sync, and clear functions (private)
 *-----------------------------------------------------------------------------
 */

static void edit_area_set_as_top_menu(gchar *name, Edit_Area *ea)
{
	edit_area_enable_name_widgets(FALSE, ea);
	gtk_entry_set_text(GTK_ENTRY(GNOME_DENTRY_EDIT(ea->dee)->name_entry), name);
	edit_area_enable_revert_button(FALSE, ea);
	if (ea->revert)
		{
		g_free(ea->revert->name);
		ea->revert->name = g_strdup(name);
		}
}

static GnomeDesktopEntry *edit_area_real_get_dentry(Edit_Area *ea)
{
	return gnome_dentry_get_dentry(GNOME_DENTRY_EDIT(ea->dee));
}

static void edit_area_set_path(gchar *path, Edit_Area *ea)
{
	g_free(ea->path);
	ea->path = g_strdup(path);
}

static gchar *edit_area_get_path(Edit_Area *ea)
{
	return ea->path;
}

static void edit_area_sync_to(Desktop_Data *dd, Edit_Area *ea)
{
	GnomeDesktopEntry *dentry = NULL;
	gchar *path = dd->path;

	ea->isfolder = dd->isfolder;
	if (dd->isfolder)
		{
		gchar *dir_file = g_concat_dir_and_file(path, ".directory");
		dentry = gnome_desktop_entry_load_unconditional(dir_file);
		if (!dentry)
			{
			dentry = g_new0(GnomeDesktopEntry, 1);
			dentry->name = g_strdup(path + g_filename_index(path));
			dentry->type = g_strdup("Directory");
			}
		g_free(dir_file);
		}
	else
		{
		dentry = gnome_desktop_entry_load_unconditional(path);
		if (!dentry)
			{
			dentry = g_new0(GnomeDesktopEntry, 1);
			dentry->name = g_strdup(path + g_filename_index(path));
			dentry->type = g_strdup("Application");
			}
		}

	gnome_dentry_edit_set_dentry(GNOME_DENTRY_EDIT(ea->dee), dentry);

	/* ugly hack - cause warning otherwise because we didnt make a */
	/*             real dentry above in case of directory          */
	if (dd->isfolder) {
	  g_free(dentry->name);
	  g_free(dentry->type);
	  g_free(dentry);
	} else {
	  gnome_desktop_entry_destroy(dentry);
	}

	g_free(ea->path);
	ea->path = g_strdup(path);

	/* same hack as above */
	if (ea->revert)
	  if (dentry->type && ea->revert->location) {
	    gnome_desktop_entry_destroy(ea->revert);
	  } else {
	    if (ea->revert->name) g_free(ea->revert->name);
	    if (ea->revert->icon) g_free(ea->revert->icon);
	    if (ea->revert->type) g_free(ea->revert->type);
	    g_free(ea->revert);
	  }	  
	  
	ea->revert = gnome_dentry_get_dentry(GNOME_DENTRY_EDIT(ea->dee));

	if (dd->isfolder)
		gtk_label_set(GTK_LABEL(ea->pathlabel), path);
	else
		{
		gchar *buf = remove_level_from_path(path);
		gtk_label_set(GTK_LABEL(ea->pathlabel), buf);
		g_free(buf);
		}

	edit_area_enable_basic_widgets(dd->editable, ea);
	edit_area_enable_non_dir_widgets((!dd->isfolder && dd->editable), ea);
	edit_area_enable_save_button(dd->editable, ea);
	edit_area_enable_revert_button(FALSE, ea);
}

static void edit_area_real_clear(gchar *path, gchar *name, Edit_Area *ea)
{
	gnome_dentry_edit_clear(GNOME_DENTRY_EDIT(ea->dee));

	if (name) gtk_entry_set_text(GTK_ENTRY(GNOME_DENTRY_EDIT(ea->dee)->name_entry), name);

	g_free(ea->path);
	ea->path = g_strdup(path);

	if (ea->revert)
		{
		gnome_desktop_entry_destroy(ea->revert);
		ea->revert = NULL;
		}

	if (isdir(path))
		{
		gtk_label_set(GTK_LABEL(ea->pathlabel), path);
		gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(GNOME_DENTRY_EDIT(ea->dee)->type_combo)->entry), "Directory");
		ea->isfolder = TRUE;
		}
	else
		{
		gchar *buf = remove_level_from_path(path);
		gtk_label_set(GTK_LABEL(ea->pathlabel), buf);
		g_free(buf);
		gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(GNOME_DENTRY_EDIT(ea->dee)->type_combo)->entry), "Application");
		ea->isfolder = FALSE;
		}

	edit_area_enable_basic_widgets(TRUE, ea);
	edit_area_enable_non_dir_widgets(!ea->isfolder, ea);
	edit_area_enable_save_button(TRUE, ea);
	edit_area_enable_revert_button(FALSE, ea);
}

/*
 *-----------------------------------------------------------------------------
 * callback functions (private)
 *-----------------------------------------------------------------------------
 */

static void edit_area_changed_cb(GtkObject *dee, gpointer data)
{
	Edit_Area *ea = data;
	if (ea->revert) edit_area_enable_revert_button(TRUE, ea);
}

static void edit_area_revert_cb(GtkWidget *widget, gpointer data)
{
	Edit_Area *ea = data;
	if (ea->revert)
		{
		gnome_dentry_edit_set_dentry(GNOME_DENTRY_EDIT(ea->dee), ea->revert);
		}
	edit_area_enable_revert_button(FALSE, ea);
}

static void edit_area_save_cb(GtkWidget *widget, gpointer data)
{
	Edit_Area *ea = data;
	GnomeDesktopEntry *dentry = NULL;

	dentry = gnome_dentry_get_dentry(GNOME_DENTRY_EDIT(ea->dee));
	save_desktop_entry(dentry, ea->path, ea->isfolder);
	gnome_desktop_entry_destroy(dentry);
}

/*
 *-----------------------------------------------------------------------------
 * create edit area (private)
 *-----------------------------------------------------------------------------
 */

static Edit_Area *edit_area_new()
{
	Edit_Area *ea;
	GtkWidget *notebook;
	GtkWidget *hbox;
	GtkWidget *hbox1;
	GtkWidget *label;
	GtkWidget *pixmap;
	GtkWidget *frame;

	ea = g_new0(Edit_Area, 1);

	ea->vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (ea->vbox), 5);

	notebook = gtk_notebook_new();
	gtk_notebook_set_tab_pos (GTK_NOTEBOOK(notebook), GTK_POS_TOP);
	gtk_box_pack_start (GTK_BOX(ea->vbox), notebook, TRUE, TRUE, 0);
	gtk_widget_show(notebook);

	ea->dee = gnome_dentry_edit_new_notebook (GTK_NOTEBOOK(notebook));
	gtk_signal_connect(GTK_OBJECT(ea->dee), "changed", GTK_SIGNAL_FUNC(edit_area_changed_cb), ea);

	hbox1 = gtk_hbox_new(TRUE, 0);
	gtk_box_pack_start(GTK_BOX(ea->vbox),hbox1,FALSE,FALSE,10);
	gtk_widget_show(hbox1);

	ea->save_button = gtk_button_new();
	gtk_signal_connect(GTK_OBJECT(ea->save_button), "clicked", GTK_SIGNAL_FUNC(edit_area_save_cb), ea);
	gtk_box_pack_start(GTK_BOX(hbox1), ea->save_button, FALSE, FALSE,0);
	gtk_widget_show(ea->save_button);
	GTK_WIDGET_SET_FLAGS (ea->save_button, GTK_CAN_DEFAULT);
	gtk_widget_grab_default(ea->save_button);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(ea->save_button),hbox);
	gtk_widget_show(hbox);

	pixmap = gnome_stock_pixmap_widget_new(app, GNOME_STOCK_PIXMAP_SAVE );
	gtk_box_pack_start(GTK_BOX(hbox), pixmap, FALSE,FALSE,0);
	gtk_widget_show(pixmap);

	label = gtk_label_new(_("Save"));
	gtk_box_pack_start(GTK_BOX(hbox),label,FALSE,FALSE,5);
	gtk_widget_show(label);

	ea->revert_button = gtk_button_new();
	gtk_signal_connect(GTK_OBJECT(ea->revert_button),"clicked",GTK_SIGNAL_FUNC(edit_area_revert_cb), ea);
	gtk_box_pack_start(GTK_BOX(hbox1), ea->revert_button, FALSE, FALSE, 0);
	gtk_widget_show(ea->revert_button);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(ea->revert_button), hbox);
	gtk_widget_show(hbox);

	pixmap = gnome_stock_pixmap_widget_new(app, GNOME_STOCK_PIXMAP_REVERT );
	gtk_box_pack_start(GTK_BOX(hbox), pixmap, FALSE, FALSE, 0);
	gtk_widget_show(pixmap);

	label = gtk_label_new(_("Revert"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);
	GTK_WIDGET_SET_FLAGS (ea->revert_button, GTK_CAN_DEFAULT);
	gtk_widget_show(label);

	frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
	gtk_box_pack_end(GTK_BOX(ea->vbox), frame, FALSE, FALSE,0);
	gtk_widget_show(frame);

	ea->pathlabel = gtk_label_new(user_apps_dir);
	gtk_container_add(GTK_CONTAINER(frame), ea->pathlabel);
	gtk_widget_show(ea->pathlabel);

	return ea;
}

/*
 *-----------------------------------------------------------------------------
 * Public interface
 *-----------------------------------------------------------------------------
 */

GtkWidget * edit_area_create(void)
{
	edit = edit_area_new();
	return edit->vbox;
}

void edit_area_set_to(Desktop_Data *dd)
{
	edit_area_sync_to(dd, edit);

	/* do not allow user to change the USER/SYSTEM menu name/comment, since
	 * the panel does not seem to honor them, only the icon.
         */
	if (dd->editable && strcmp(dd->path, user_apps_dir) == 0)
		{
		edit_area_set_as_top_menu(_("User menus"), edit);
		}
	if (dd->editable && strcmp(dd->path, system_apps_dir) == 0)
		{
		edit_area_set_as_top_menu(_("System menus"), edit);
		}
}

void edit_area_change_path(gchar *path)
{
	edit_area_set_path(path, edit);
}

gchar *edit_area_path(void)
{
	return edit_area_get_path(edit);
}

void edit_area_clear(gchar *path, gchar *name)
{
	edit_area_real_clear(path, name, edit);
}

GnomeDesktopEntry *edit_area_get_dentry(void)
{
	return edit_area_real_get_dentry(edit);
}

void edit_area_grab_name(void)
{
	edit_area_grab_name_entry(edit);
}
