/*
 * GNOME panel launcher module.
 * (C) 1997 The Free Software Foundation
 *
 * Authors: Miguel de Icaza
 *          Federico Mena
 * CORBAized by George Lebl
 * de-CORBAized by George Lebl
 */

#include <config.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include "gnome.h"
#include "panel.h"
#include "panel-util.h"

#define LAUNCHER_PROPERTIES "launcher_properties"

extern GtkTooltips *panel_tooltips;

static char *default_app_pixmap=NULL;

typedef struct {
	int                applet_id;
	GtkWidget         *button;
	gint               signal_click_tag;
	GnomeDesktopEntry *dentry;
} Launcher;

typedef struct {
	GtkWidget *dialog;

	GtkWidget *name_entry;
	GtkWidget *comment_entry;
	GtkWidget *execute_entry;
	GtkWidget *icon_entry;
	GtkWidget *documentation_entry;
	GtkWidget *terminal_toggle;

	/*information about this launcher*/
	Launcher *launcher;

	GnomeDesktopEntry *dentry;
} Properties;

static void
launch (GtkWidget *widget, void *data)
{
	GnomeDesktopEntry *item = data;

	gnome_desktop_entry_launch (item);
}

Launcher *
create_launcher (char *parameters)
{
	GtkWidget *pixmap;
	GnomeDesktopEntry *dentry;
	Launcher *launcher;

	if (!default_app_pixmap)
		default_app_pixmap = gnome_pixmap_file ("gnome-unknown.png");

	if (*parameters == '/')
		dentry = gnome_desktop_entry_load (parameters);
	else {
		char *apps_par, *entry, *extension;

		if (strstr (parameters, ".desktop"))
			extension = NULL;
		else
			extension = ".desktop";
		
		apps_par = g_copy_strings ("apps/", parameters, extension, NULL);
		entry = gnome_datadir_file (apps_par);
		g_free (apps_par);
		
		if (!entry)
			return NULL;
		dentry = gnome_desktop_entry_load (entry);
		g_free (entry);
	}
	if (!dentry)
		return NULL; /*button is null*/

	launcher = g_new(Launcher,1);

	launcher->button = gtk_button_new ();
	pixmap = NULL;
	if(dentry->icon)
		pixmap = gnome_pixmap_new_from_file (dentry->icon);
	else
		pixmap = NULL;
	if (!pixmap) {
		if (default_app_pixmap)
			pixmap = gnome_pixmap_new_from_file (default_app_pixmap);
		else
			pixmap = gtk_label_new (_("App"));
	}
	gtk_container_add (GTK_CONTAINER(launcher->button), pixmap);
	gtk_widget_show (pixmap);
	/*gtk_widget_set_usize (launcher->button, pixmap->requisition.width,
			      pixmap->requisition.height);*/
	/*FIXME: we'll do this better alter, but this makes it look fine with
	  normal icons*/
	gtk_widget_set_usize (launcher->button, 48, 48);
	
	gtk_widget_show (launcher->button);

	launcher->signal_click_tag = gtk_signal_connect (GTK_OBJECT(launcher->button),
							 "clicked",
							 (GtkSignalFunc) launch,
							 dentry);

	gtk_object_set_user_data(GTK_OBJECT(launcher->button), launcher);

	launcher->dentry = dentry;

	launcher->applet_id = -1;

	gtk_object_set_data(GTK_OBJECT(launcher->button),
			    LAUNCHER_PROPERTIES,NULL);

	return launcher;
}

static void
check_dentry_save(GnomeDesktopEntry *dentry)
{
	FILE *file;
	char *pruned;
	char *new_name;
	char *appsdir;

	file = fopen(dentry->location, "w");
	if (file) {
		fclose(file);
		return;
	}

	pruned = strrchr(dentry->location, '/');
	if (pruned)
		pruned++; /* skip over slash */
	else
		pruned = dentry->location;

	appsdir = gnome_util_home_file ("apps");
	mkdir (appsdir, 0755);

	new_name = g_concat_dir_and_file(appsdir, pruned);
	g_free(dentry->location);
	dentry->location = new_name;
}

#define free_and_nullify(x) { g_free(x); x = NULL; }

static void
properties_apply_callback(GtkWidget *widget, int page, gpointer data)
{
	Properties        *prop;
	GnomeDesktopEntry *dentry;
	GtkWidget         *pixmap;
	int i, n_args;
	char **exec;

	if (page != -1)
		return;
	
	prop = data;
	dentry = prop->dentry;

	free_and_nullify(dentry->name);
	free_and_nullify(dentry->comment);
	gnome_string_array_free (dentry->exec);
	dentry->exec = NULL;
	free_and_nullify(dentry->exec);
	free_and_nullify(dentry->tryexec);
	free_and_nullify(dentry->icon);
	free_and_nullify(dentry->docpath);
	free_and_nullify(dentry->type);

	dentry->name      = g_strdup(gtk_entry_get_text(GTK_ENTRY(prop->name_entry)));
	dentry->comment   = g_strdup(gtk_entry_get_text(GTK_ENTRY(prop->comment_entry)));

	/* Handle exec specially: split at spaces.  Multiple spaces
	   must be compacted, hence the weirdness.  This
	   implementation wastes a little memory in some cases.  Big
	   deal.  */
	exec = gnome_string_split (gtk_entry_get_text(GTK_ENTRY(prop->execute_entry)),
				   " ", -1);
	for (n_args = i = 0; exec[i]; ++i) {
		if (! exec[i][0]) {
			/* Empty item means multiple spaces found.  Remove
			   it.  */
			g_free (exec[i]);
		} else {
			exec[n_args++] = exec[i];
		}
	}
	exec[n_args] = NULL;
	dentry->exec_length = n_args;
	dentry->exec = exec;

	dentry->icon      = g_strdup(gtk_entry_get_text(GTK_ENTRY(prop->icon_entry)));
	dentry->docpath   = g_strdup(gtk_entry_get_text(GTK_ENTRY(prop->documentation_entry)));
	dentry->type      = g_strdup("Application"); /* FIXME: should handle more cases */
	dentry->terminal  = GTK_TOGGLE_BUTTON(prop->terminal_toggle)->active;

	check_dentry_save(dentry);
	gnome_desktop_entry_save(dentry);

	dentry=gnome_desktop_entry_load(dentry->location);

	gnome_desktop_entry_free(prop->dentry);
	prop->dentry = dentry;

	gtk_tooltips_set_tip (panel_tooltips,prop->launcher->button->parent,
			      dentry->comment,NULL);
	
	pixmap=GTK_BUTTON(prop->launcher->button)->child;

	gtk_container_remove(GTK_CONTAINER(prop->launcher->button),pixmap);

	pixmap = gnome_pixmap_new_from_file (dentry->icon);
	if (!pixmap) {
		if (default_app_pixmap)
			pixmap = gnome_pixmap_new_from_file (default_app_pixmap);
		else
			pixmap = gtk_label_new (_("App"));
	}
	gtk_container_add (GTK_CONTAINER(prop->launcher->button), pixmap);
	gtk_widget_show(pixmap);

	/*FIXME: a bad hack to keep it all 48x48*/
	gtk_widget_set_usize (prop->launcher->button, 48, 48);

	/*gtk_widget_set_usize (prop->launcher->button, pixmap->requisition.width,
			      pixmap->requisition.height);*/

	gtk_signal_disconnect(GTK_OBJECT(prop->launcher->button),
			      prop->launcher->signal_click_tag);

	prop->launcher->signal_click_tag = gtk_signal_connect (GTK_OBJECT(prop->launcher->button), "clicked",
							       (GtkSignalFunc) launch,
							       dentry);

	/*replace the dentry in launcher structure with the new one */
	gnome_desktop_entry_free(prop->launcher->dentry);

	prop->launcher->dentry=dentry;
}

#undef free_and_nullify

static gint
properties_close_callback(GtkWidget *widget, gpointer data)
{
	Properties *prop = data;
	gtk_object_set_data(GTK_OBJECT(prop->launcher->button),
			    LAUNCHER_PROPERTIES,NULL);
	gtk_signal_disconnect_by_data(GTK_OBJECT(prop->name_entry),widget);
	gtk_signal_disconnect_by_data(GTK_OBJECT(prop->comment_entry),widget);
	gtk_signal_disconnect_by_data(GTK_OBJECT(prop->execute_entry),widget);
	gtk_signal_disconnect_by_data(GTK_OBJECT(prop->icon_entry),widget);
	gtk_signal_disconnect_by_data(GTK_OBJECT(prop->documentation_entry),widget);
	gtk_signal_disconnect_by_data(GTK_OBJECT(prop->terminal_toggle),widget);
	g_free (prop);
	return FALSE;
}

static void
notify_entry_change (GtkWidget *widget, void *data)
{
	GnomePropertyBox *box = GNOME_PROPERTY_BOX (data);

	gnome_property_box_changed (box);
}

static GtkWidget *
create_properties_dialog(GnomeDesktopEntry *dentry, Launcher *launcher)
{
	Properties *prop;
	GtkWidget  *dialog;
	GtkWidget  *table;
	GtkWidget  *toggle;
	gchar *exec;

	prop = g_new(Properties, 1);
	prop->dentry = dentry;

	prop->launcher = launcher;

	prop->dialog = dialog = gnome_property_box_new();
	gtk_window_set_title(GTK_WINDOW(dialog), _("Launcher properties"));
	gtk_window_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
	gtk_window_set_policy(GTK_WINDOW(dialog), FALSE, FALSE, TRUE);

	table = gtk_table_new(6, 2, FALSE);
	gtk_container_border_width(GTK_CONTAINER(table), 4);
	gtk_table_set_col_spacings(GTK_TABLE(table), 6);
	gtk_table_set_row_spacings(GTK_TABLE(table), 2);

	prop->name_entry          = create_text_entry(table,
						      "launcher_name", 0,
						      _("Name"), dentry->name,
						      dialog);
	prop->comment_entry       = create_text_entry(table,
						      "launcher_comment", 1,
						      _("Comment"),
						      dentry->comment, dialog);
	exec = gnome_string_joinv (" ", dentry->exec);
	prop->execute_entry       = create_file_entry(table,
						      "execute", 2,
						      _("Execute"), exec,
						      dialog);
	g_free (exec);
	prop->icon_entry          = create_file_entry(table,
						      "icon", 3,
						      _("Icon"), dentry->icon,
						      dialog);
	prop->documentation_entry = create_text_entry(table,
						      "launcher_document", 4,
						      _("Documentation"),
						      dentry->docpath, dialog);

	prop->terminal_toggle = toggle =
		gtk_check_button_new_with_label(_("Run inside terminal"));
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(toggle),
				    dentry->terminal ? TRUE : FALSE);
	gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
			    GTK_SIGNAL_FUNC (notify_entry_change), dialog);
				    
	gtk_table_attach(GTK_TABLE(table), toggle,
			 0, 2, 5, 6,
			 GTK_EXPAND | GTK_FILL | GTK_SHRINK,
			 GTK_FILL | GTK_SHRINK,
			 0, 0);

	gnome_property_box_append_page (GNOME_PROPERTY_BOX (prop->dialog),
					table, gtk_label_new (_("Item properties")));
	
	gtk_signal_connect(GTK_OBJECT(dialog), "destroy",
			   (GtkSignalFunc) properties_close_callback,
			   prop);

	gtk_signal_connect(GTK_OBJECT(dialog), "apply",
			   GTK_SIGNAL_FUNC(properties_apply_callback), prop);

	return dialog;
}

void
launcher_properties(Launcher *launcher)
{
	GnomeDesktopEntry *dentry;
	char              *path;
	GtkWidget         *dialog;

	dialog = gtk_object_get_data(GTK_OBJECT(launcher->button),
				     LAUNCHER_PROPERTIES);
	if(dialog) {
		gdk_window_raise(dialog->window);
		return;
	}

	path = launcher->dentry->location;

	dentry = gnome_desktop_entry_load(path);
	if (!dentry) {
		g_warning("launcher properties: oops, "
			  "gnome_desktop_entry_load() returned NULL\n"
			  "                     on \"%s\"\n", path);
		return;
	}

	dialog = create_properties_dialog(dentry,launcher);
	gtk_object_set_data(GTK_OBJECT(launcher->button),
			    LAUNCHER_PROPERTIES,dialog);
	gtk_widget_show_all (dialog);
}
