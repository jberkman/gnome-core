/*
 * This file is a part of gmenu, the GNOME panel menu editor.
 *
 * File: edit.c
 *
 * This file contains the routines which handle the menu item editing
 * dialog on the right half of the display.
 *
 * Author: John Ellis <johne@bellatlantic.net>
 */

#include <config.h>

#include "gmenu.h"

static GtkWidget *button_save;
static GtkWidget *button_revert;
static GnomeDesktopEntry *revert_dentry;

static void edit_area_changed(void);
static void button_save_enable(gboolean enabled);
static void button_revert_enable(gboolean enabled);
static void edit_area_set_editable(gboolean enabled);

static void edit_area_changed(void)
{
	button_save_enable(TRUE);
	if (revert_dentry) button_revert_enable(TRUE);
}

static void button_save_enable(gboolean enabled)
{
	gtk_widget_set_sensitive(button_save, enabled);
}

static void button_revert_enable(gboolean enabled)
{
	gtk_widget_set_sensitive(button_revert, enabled);
}

static void edit_area_set_editable(gboolean enabled)
{
	gtk_widget_set_sensitive(GNOME_DENTRY_EDIT(edit_area)->exec_entry, enabled);
	gtk_widget_set_sensitive(GNOME_DENTRY_EDIT(edit_area)->tryexec_entry, enabled);
	gtk_widget_set_sensitive(GNOME_DENTRY_EDIT(edit_area)->doc_entry, enabled);
	gtk_widget_set_sensitive(GNOME_DENTRY_EDIT(edit_area)->type_combo, enabled);
	gtk_widget_set_sensitive(GNOME_DENTRY_EDIT(edit_area)->terminal_button, enabled);
}

void
edit_area_select_name(void)
{
  GtkWidget *name_entry;
  gchar *text;

  name_entry = gnome_dentry_get_name_entry(GNOME_DENTRY_EDIT(edit_area));

  text = gtk_entry_get_text(GTK_ENTRY(name_entry));
  
  gtk_entry_select_region(GTK_ENTRY(name_entry), 0, strlen(text));

  gtk_widget_grab_focus(name_entry);
}

void edit_area_reset_revert(Desktop_Data *d)
{
	if (d)
		{
		if (revert_dentry) gnome_desktop_entry_destroy(revert_dentry);
		revert_dentry = gnome_dentry_get_dentry(GNOME_DENTRY_EDIT(edit_area));
		edit_area_orig_data = d;
		}
	else
		{
		if (revert_dentry) gnome_desktop_entry_destroy(revert_dentry);
		revert_dentry = NULL;
		edit_area_orig_data = NULL;
		}
	button_revert_enable(FALSE);
}

void update_edit_area(Desktop_Data *d)
{
	GnomeDesktopEntry *dentry;

	if (isdir(d->path))
		{
		gchar *dirfile = g_concat_dir_and_file(d->path, ".directory");

		dentry = gnome_desktop_entry_load_unconditional(dirfile);
		if (dentry)
			{
			gnome_dentry_edit_set_dentry(GNOME_DENTRY_EDIT(edit_area), dentry);
			gnome_desktop_entry_destroy(dentry);
			}
		else
			{
			dentry = g_new0(GnomeDesktopEntry, 1);
			dentry->name = strdup(d->name);
			dentry->type = strdup("Directory");
			gnome_dentry_edit_set_dentry(GNOME_DENTRY_EDIT(edit_area), dentry);
			gnome_desktop_entry_destroy(dentry);
			}

		edit_area_set_editable(FALSE);

		g_free(dirfile);
		}
	else
		{
		gnome_dentry_edit_load_file(GNOME_DENTRY_EDIT(edit_area), d->path);
		edit_area_set_editable(TRUE);
		}

	button_save_enable(TRUE);
	button_revert_enable(FALSE);

	if (revert_dentry) gnome_desktop_entry_destroy(revert_dentry);
	revert_dentry = gnome_dentry_get_dentry(GNOME_DENTRY_EDIT(edit_area));

	edit_area_orig_data = d;
}

void revert_edit_area(void)
{
	if (revert_dentry) gnome_dentry_edit_set_dentry(GNOME_DENTRY_EDIT(edit_area), revert_dentry);

	button_revert_enable(FALSE);
}


void new_edit_area(void)
{
	GnomeDesktopEntry *dentry;
	gnome_dentry_edit_clear(GNOME_DENTRY_EDIT(edit_area));

	dentry = gnome_dentry_get_dentry(GNOME_DENTRY_EDIT(edit_area));
	dentry->name = g_strdup("New Item");
	g_free(dentry->type);
	dentry->type = strdup("Application");
	gnome_dentry_edit_set_dentry(GNOME_DENTRY_EDIT(edit_area), dentry);
	gnome_desktop_entry_destroy(dentry);
	edit_area_set_editable(TRUE);
	button_save_enable(TRUE);
	button_revert_enable(FALSE);

	if (revert_dentry)
		{
		gnome_desktop_entry_destroy(revert_dentry);
		revert_dentry = NULL;
		}
	edit_area_orig_data = NULL;
}

GtkWidget * create_edit_area(void)
{
	GtkWidget *vbox;
	GtkWidget *notebook;
	GtkWidget *hbox;
	GtkWidget *hbox1;
	GtkWidget *label;
	GtkWidget *button;
	GtkWidget *pixmap;
	GtkWidget *frame;

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);

	notebook = gtk_notebook_new();
	gtk_notebook_set_tab_pos (GTK_NOTEBOOK(notebook), GTK_POS_TOP);
	gtk_box_pack_start (GTK_BOX(vbox), notebook, TRUE, TRUE, 0);
	gtk_widget_show(notebook);

	edit_area = gnome_dentry_edit_new_notebook (GTK_NOTEBOOK(notebook));
	gtk_signal_connect(GTK_OBJECT(edit_area), "changed", GTK_SIGNAL_FUNC(edit_area_changed), NULL);

	hbox = gtk_hbox_new(FALSE,5);
	gtk_box_pack_start(GTK_BOX(vbox),hbox,FALSE,FALSE,5);
	gtk_widget_show(hbox);

	hbox1 = gtk_hbox_new(TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox),hbox1,FALSE,FALSE,10);
	gtk_widget_show(hbox1);

	button_save = gtk_button_new();
	gtk_signal_connect(GTK_OBJECT(button_save),"clicked",GTK_SIGNAL_FUNC(save_pressed_cb), NULL);
	gtk_box_pack_start(GTK_BOX(hbox1),button_save,FALSE,FALSE,0);
	gtk_widget_show(button_save);
	GTK_WIDGET_SET_FLAGS (button_save, GTK_CAN_DEFAULT);
	gtk_widget_grab_default(button_save);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(button_save),hbox);
	gtk_widget_show(hbox);

	pixmap = gnome_stock_pixmap_widget_new(app, GNOME_STOCK_PIXMAP_SAVE );
	gtk_box_pack_start(GTK_BOX(hbox),pixmap,FALSE,FALSE,0);
	gtk_widget_show(pixmap);

	label = gtk_label_new(_("Save"));
	gtk_box_pack_start(GTK_BOX(hbox),label,FALSE,FALSE,5);
	gtk_widget_show(label);

	button_revert = gtk_button_new();
	gtk_signal_connect(GTK_OBJECT(button_revert),"clicked",GTK_SIGNAL_FUNC(revert_edit_area), NULL);
	gtk_box_pack_start(GTK_BOX(hbox1),button_revert,FALSE,FALSE,0);
	gtk_widget_show(button_revert);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(button_revert),hbox);
	gtk_widget_show(hbox);

	pixmap = gnome_stock_pixmap_widget_new(app, GNOME_STOCK_PIXMAP_REVERT );
	gtk_box_pack_start(GTK_BOX(hbox),pixmap,FALSE,FALSE,0);
	gtk_widget_show(pixmap);

	label = gtk_label_new(_("Revert"));
	gtk_box_pack_start(GTK_BOX(hbox),label,FALSE,FALSE,5);
	gtk_widget_show(label);

#if 0
	button = gtk_button_new();
	gtk_signal_connect(GTK_OBJECT(button),"clicked",GTK_SIGNAL_FUNC(new_edit_area), NULL);
	gtk_box_pack_start(GTK_BOX(hbox1),button,FALSE,FALSE,0);
	gtk_widget_show(button);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(button),hbox);
	gtk_widget_show(hbox);

	pixmap = gnome_stock_pixmap_widget_new(app, GNOME_STOCK_PIXMAP_NEW );
	gtk_box_pack_start(GTK_BOX(hbox),pixmap,FALSE,FALSE,0);
	gtk_widget_show(pixmap);

	label = gtk_label_new(_("New"));
	gtk_box_pack_start(GTK_BOX(hbox),label,FALSE,FALSE,5);
	gtk_widget_show(label);
#endif

	frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(frame),GTK_SHADOW_IN);
	gtk_box_pack_end(GTK_BOX(vbox),frame,FALSE,FALSE,0);
	gtk_widget_show(frame);

	pathlabel = gtk_label_new(USER_APPS);
	gtk_container_add(GTK_CONTAINER(frame),pathlabel);
	gtk_widget_show(pathlabel);

	return vbox;
}
