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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <gnome.h>

#include "panel-include.h"

#define LAUNCHER_PROPERTIES "launcher_properties"

extern GtkTooltips *panel_tooltips;

extern GSList *applets;
extern GSList *applets_last;
extern int applet_count;

static char *default_app_pixmap=NULL;

extern GlobalConfig global_config;


static void
launch (Launcher *launcher, int argc, char *argv[])
{
	GnomeDesktopEntry *item;

	g_return_if_fail(launcher != NULL);
	g_return_if_fail(launcher->dentry != NULL);

	item = launcher->dentry;

	if(!item->exec) {
		GtkWidget *dlg;
		dlg = gnome_message_box_new(_("This launch icon does not "
					      "specify a program to run"),
					    GNOME_MESSAGE_BOX_ERROR,
					    GNOME_STOCK_BUTTON_CLOSE,
					    NULL);
		gtk_window_set_wmclass(GTK_WINDOW(dlg),
				       "no_exec_dialog","Panel");
		gtk_widget_show_all(dlg);
		return;
	}

	/*UGLY HACK!*/
	if (item->exec_length > 0
	    && strstr(item->exec[0], "://"))
		gnome_url_show(item->exec[0]);
	else
		gnome_desktop_entry_launch_with_args (item,argc,argv);
	
	if(global_config.drawer_auto_close) {
		GtkWidget *parent =
			PANEL_WIDGET(launcher->button->parent)->panel_parent;
		g_return_if_fail(parent!=NULL);
		if(IS_DRAWER_WIDGET(parent)) {
			BasePWidget *basep = BASEP_WIDGET(parent);
			GtkWidget *grandparent = PANEL_WIDGET(basep->panel)->master_widget->parent;
			GtkWidget *grandparentw =
				PANEL_WIDGET(grandparent)->panel_parent;
			drawer_widget_close_drawer (DRAWER_WIDGET (parent),
						    BASEP_WIDGET (grandparentw));
		}
	}
}

static void
launch_cb (GtkWidget *widget, gpointer data)
{
	launch(data,0,NULL);
}

static void
drag_data_received_cb (GtkWidget        *widget,
		       GdkDragContext   *context,
		       gint              x,
		       gint              y,
		       GtkSelectionData *selection_data,
		       guint             info,
		       guint             time,
		       Launcher         *launcher)
{

	GList *li, *files;
	int argc;
	char **argv;
	int i;

	g_return_if_fail(launcher!=NULL);

	files = gnome_uri_list_extract_filenames(selection_data->data);
	argc = g_list_length(files);
	argv = g_new(char *,argc+1);
	argv[argc] = NULL;

	for(i=0,li = files; li; i++,li = g_list_next(li))
		argv[i]=li->data;
	
	launch(launcher,argc,argv);
	gnome_uri_list_free_strings (files);
	g_free(argv);
}

static void
destroy_launcher(GtkWidget *widget, gpointer data)
{
	Launcher *launcher = data;
	GtkWidget *prop_dialog = gtk_object_get_data(GTK_OBJECT(launcher->button),
						     LAUNCHER_PROPERTIES);
	if(prop_dialog)
		gtk_widget_destroy(prop_dialog);
	gnome_desktop_entry_free(launcher->dentry);
	g_free(launcher);
}

static void  
drag_data_get_cb (GtkWidget *widget, GdkDragContext     *context,
		  GtkSelectionData   *selection_data, guint info,
		  guint time, Launcher *launcher)
{
	gchar *uri_list;
	
	g_return_if_fail(launcher!=NULL);
	g_return_if_fail(launcher->dentry!=NULL);
	g_return_if_fail(launcher->dentry->location!=NULL);

	uri_list = g_strconcat ("file:", launcher->dentry->location, "\r\n", NULL);

	gtk_selection_data_set (selection_data,
				selection_data->target, 8, (guchar *)uri_list,
				strlen(uri_list));
	g_free(uri_list);
}



static Launcher *
create_launcher (char *parameters, GnomeDesktopEntry *dentry)
{
	char *icon;
	Launcher *launcher;
        static GtkTargetEntry dnd_targets[] = {
		{ "text/uri-list", 0, 0 }
	};

	if (!default_app_pixmap)
		default_app_pixmap = gnome_pixmap_file ("gnome-unknown.png");

	if(!dentry) {
		if (!parameters)
			return NULL;
		else if (*parameters == '/')
			dentry = gnome_desktop_entry_load_unconditional(parameters);
		else {
			char *apps_par, *entry, *extension;

			if (strstr (parameters, ".desktop"))
				extension = NULL;
			else
				extension = ".desktop";

			apps_par = g_strconcat ("apps/", parameters,
						   extension, NULL);
			entry = gnome_datadir_file (apps_par);
			g_free (apps_par);

			if (!entry)
				return NULL;
			dentry = gnome_desktop_entry_load_unconditional (entry);
			g_free (entry);
		}
	}
	if (!dentry)
		return NULL; /*button is null*/

	launcher = g_new(Launcher,1);

	launcher->button = NULL;
	launcher->dedit = NULL;
	icon = dentry->icon;
	if (icon && *icon) {
		/* Sigh, now we need to make them local to the gnome install */
		if (*icon != '/') {
			dentry->icon = gnome_pixmap_file (icon);
			g_free (icon);
		}
		launcher->button = button_widget_new(dentry->icon,
						     -1,
						     LAUNCHER_TILE,
						     FALSE,
						     ORIENT_UP,
						     dentry->name);
	}
	if (!launcher->button) {
		launcher->button =
			button_widget_new(default_app_pixmap,-1,
					  LAUNCHER_TILE,
					  FALSE,ORIENT_UP,
					  _("App"));
	}
	gtk_widget_show (launcher->button);

	/*A hack since this function only pretends to work on window
	  widgets (which we actually kind of are) this will select
	  some (already selected) events on the panel instead of
	  the launcher window (where they are also selected) but
	  we don't mind*/
	GTK_WIDGET_UNSET_FLAGS(launcher->button,GTK_NO_WINDOW);
	gtk_drag_source_set(launcher->button,
			    GDK_BUTTON1_MASK,
			    dnd_targets, 1,
			    GDK_ACTION_COPY);
	GTK_WIDGET_SET_FLAGS(launcher->button,GTK_NO_WINDOW);
	
	gtk_drag_dest_set (GTK_WIDGET (launcher->button),
			   GTK_DEST_DEFAULT_MOTION |
			   GTK_DEST_DEFAULT_HIGHLIGHT |
			   GTK_DEST_DEFAULT_DROP,
			   dnd_targets, 1,
			   GDK_ACTION_COPY);

	gtk_signal_connect(GTK_OBJECT(launcher->button), "drag_data_get",
			   drag_data_get_cb, launcher);
	gtk_signal_connect(GTK_OBJECT(launcher->button), "drag_data_received",
			   drag_data_received_cb, launcher);

	gtk_signal_connect (GTK_OBJECT(launcher->button), "clicked",
			    (GtkSignalFunc) launch_cb,
			    launcher);
	
	gtk_signal_connect (GTK_OBJECT(launcher->button), "destroy",
			    GTK_SIGNAL_FUNC(destroy_launcher),
			    launcher);

	gtk_object_set_user_data(GTK_OBJECT(launcher->button), launcher);

	launcher->dentry = dentry;

	gtk_object_set_data(GTK_OBJECT(launcher->button),
			    LAUNCHER_PROPERTIES,NULL);

	return launcher;
}

static void
properties_apply_callback(GtkWidget *widget, int page, gpointer data)
{
	Launcher *launcher = data;
	char *icon;
	
	if (page != -1)
		return;

	gnome_desktop_entry_free(launcher->dentry);
	launcher->dentry =
		gnome_dentry_get_dentry(GNOME_DENTRY_EDIT(launcher->dedit));
	if(!launcher->dentry->name || !(*(launcher->dentry->name))) {
		g_free(launcher->dentry->name);
		launcher->dentry->name=g_strdup("???");
	}

	gtk_tooltips_set_tip (panel_tooltips,launcher->button,
			      launcher->dentry->comment,NULL);
	
	button_widget_set_text (BUTTON_WIDGET(launcher->button),
				launcher->dentry->name);
	icon = launcher->dentry->icon;
	if (icon && *icon) {
		/* Sigh, now we need to make them local to the gnome
		   install */
		if (*icon != '/') {
			launcher->dentry->icon = gnome_pixmap_file (icon);
			g_free (icon);
		}
		if(!button_widget_set_pixmap (BUTTON_WIDGET(launcher->button),
					      launcher->dentry->icon,
					      -1))
			button_widget_set_pixmap (BUTTON_WIDGET(launcher->button),
						  default_app_pixmap,
						  -1);
	} else {
		button_widget_set_pixmap(BUTTON_WIDGET(launcher->button),
					 default_app_pixmap, -1);
	}
	panel_config_sync();
}

static void
properties_close_callback(GtkWidget *widget, gpointer data)
{
	Launcher *launcher = data;
	gtk_object_set_data(GTK_OBJECT(launcher->button),
			    LAUNCHER_PROPERTIES,NULL);
	launcher->dedit = NULL;
}

static GtkWidget *
create_properties_dialog(Launcher *launcher)
{
	GtkWidget  *dialog;

	dialog = gnome_property_box_new();
	gtk_window_set_wmclass(GTK_WINDOW(dialog),
			       "launcher_properties","Panel");
	gtk_window_set_title(GTK_WINDOW(dialog), _("Launcher properties"));
	gtk_window_set_policy(GTK_WINDOW(dialog), FALSE, FALSE, TRUE);
	
	launcher->dedit =
		gnome_dentry_edit_new_notebook(
		      GTK_NOTEBOOK(GNOME_PROPERTY_BOX(dialog)->notebook));

	gnome_dentry_edit_set_dentry(GNOME_DENTRY_EDIT(launcher->dedit),
				     launcher->dentry);

	/* This sucks, but there is no other way to do this with the current
	   GnomeDEntry API.  */

#define SETUP_EDITABLE(entry_name)					\
	gnome_dialog_editable_enters					\
		(GNOME_DIALOG (dialog),					\
		 GTK_EDITABLE (gnome_dentry_get_##entry_name##_entry  	\
			       (GNOME_DENTRY_EDIT (launcher->dedit))));

	SETUP_EDITABLE (name);
	SETUP_EDITABLE (comment);
	SETUP_EDITABLE (exec);
	SETUP_EDITABLE (tryexec);
	SETUP_EDITABLE (doc);

#undef SETUP_EDITABLE
	
	gtk_signal_connect_object(GTK_OBJECT(launcher->dedit), "changed",
				  GTK_SIGNAL_FUNC(gnome_property_box_changed),
				  GTK_OBJECT(dialog));

	gtk_signal_connect(GTK_OBJECT(dialog), "destroy",
			   GTK_SIGNAL_FUNC(properties_close_callback),
			   launcher);

	gtk_signal_connect(GTK_OBJECT(dialog), "apply",
			   GTK_SIGNAL_FUNC(properties_apply_callback),
			   launcher);

	return dialog;
}

void
launcher_properties(Launcher *launcher)
{
	GtkWidget         *dialog;

	dialog = gtk_object_get_data(GTK_OBJECT(launcher->button),
				     LAUNCHER_PROPERTIES);
	if(dialog) {
		gdk_window_raise(dialog->window);
		gtk_widget_show(dialog);
		return;
	}

	dialog = create_properties_dialog(launcher);
	gtk_object_set_data(GTK_OBJECT(launcher->button),
			    LAUNCHER_PROPERTIES,dialog);
	gtk_widget_show_all (dialog);
}

void
load_launcher_applet_full (char *params, GnomeDesktopEntry *dentry,
			   PanelWidget *panel, int pos)
{
	Launcher *launcher;

	launcher = create_launcher(params,dentry);

	if(launcher) {
		register_toy(launcher->button,launcher, panel, pos,
			     APPLET_LAUNCHER);

		gtk_tooltips_set_tip (panel_tooltips,
				      launcher->button,
				      launcher->dentry->comment,NULL);

		applet_add_callback(applets_last->data,"properties",
				    GNOME_STOCK_MENU_PROP,
				    _("Properties..."));
	}
}

static void
really_add_launcher(GtkWidget *d,int button, gpointer data)
{
	GnomeDEntryEdit *dedit = GNOME_DENTRY_EDIT(data);
	int pos = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(d),"pos"));
	PanelWidget *panel = gtk_object_get_data(GTK_OBJECT(d),"panel");
	GnomeDesktopEntry *dentry;
	
	if(button == 0) {
		dentry = gnome_dentry_get_dentry(dedit);

		if(!dentry->name || !(*(dentry->name))) {
			g_free(dentry->name);
			dentry->name=g_strdup("???");
		}
		load_launcher_applet_full (NULL, dentry, panel, pos);
		panel_config_sync();
	}
	gtk_widget_destroy(d);
}

void
ask_about_launcher(char *file, PanelWidget *panel, int pos)
{
	GtkWidget *d;
	GtkWidget *notebook;
	GnomeDEntryEdit *dee;

	d = gnome_dialog_new(_("Create launcher applet"),
			     GNOME_STOCK_BUTTON_OK,
			     GNOME_STOCK_BUTTON_CANCEL,
			     NULL);
	gtk_window_set_wmclass(GTK_WINDOW(d),
			       "create_launcher","Panel");
	gtk_window_set_policy(GTK_WINDOW(d), FALSE, FALSE, TRUE);
	
	notebook = gtk_notebook_new();
	gtk_box_pack_start(GTK_BOX(GNOME_DIALOG(d)->vbox),notebook,
			   TRUE,TRUE,GNOME_PAD_SMALL);
	dee = GNOME_DENTRY_EDIT(gnome_dentry_edit_new_notebook(GTK_NOTEBOOK(notebook)));
	
	if(file)
		gtk_entry_set_text(GTK_ENTRY(dee->exec_entry), file);
	gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(dee->type_combo)->entry),
			   "Application");
	
	
	gtk_object_set_data(GTK_OBJECT(d),"pos", GINT_TO_POINTER(pos));
	gtk_object_set_data(GTK_OBJECT(d),"panel",panel);

	gtk_signal_connect(GTK_OBJECT(d),"clicked",
			   GTK_SIGNAL_FUNC(really_add_launcher),
			   dee);

	gnome_dialog_close_hides(GNOME_DIALOG(d),FALSE);

	gnome_dialog_set_default(GNOME_DIALOG(d),0);

	gtk_widget_show_all(d);
}

void
load_launcher_applet_from_info(char *name, char *comment,
			       char **exec, int execn, char *icon,
			       PanelWidget *panel, int pos)
{
	GnomeDesktopEntry *dentry = g_new0(GnomeDesktopEntry,1);
	dentry->name = g_strdup(name);
	dentry->comment = g_strdup(comment);
	dentry->exec_length = execn;
	dentry->exec = g_copy_vector(exec);
	if(icon && *icon != '/')
		dentry->icon = gnome_pixmap_file(icon);
	else
		dentry->icon = g_strdup(icon);

	load_launcher_applet_full (NULL,dentry,panel, pos);
	panel_config_sync();
}

void
load_launcher_applet(char *params, PanelWidget *panel, int pos)
{
	load_launcher_applet_full (params,NULL,panel, pos);
}
