/*###################################################################*/
/*##                     gnome icon selection widget               ##*/
/*###################################################################*/

#include "gmenu.h"

typedef struct _Icon_Dialog_Data Icon_Dialog_Data;
struct _Icon_Dialog_Data
{
        GtkWidget *dialog;
        GtkWidget *clist;
	gint row;
	void (*function_notify)(void *);
	gint fullpath;
	gint loading;
};

static void free_clist_data(gpointer data);
static void update_list_highlight(GtkWidget *w, gint old, gint new, gint move);
static gint select_ok_cb(GtkWidget *b, gpointer data);
static gint select_cancel_cb(GtkWidget *b, gpointer data);
static void select_icon_cb(GtkWidget *w, gint row, gint column, GdkEventButton *bevent, gpointer data);

static void free_clist_data(gpointer data)
{
	g_free(data);
}

static void update_list_highlight(GtkWidget *w, gint old, gint new, gint move)
{
	if (old >= 0)
		{
		gtk_clist_set_background(GTK_CLIST(w), old,
			&GTK_WIDGET (w)->style->bg[GTK_STATE_PRELIGHT]);
		gtk_clist_set_foreground(GTK_CLIST(w), old,
			&GTK_WIDGET (w)->style->fg[GTK_STATE_PRELIGHT]);
		}
	if (new >= 0)
		{
		gtk_clist_set_background(GTK_CLIST(w), new,
			&GTK_WIDGET (w)->style->bg[GTK_STATE_SELECTED]);
		gtk_clist_set_foreground(GTK_CLIST(w), new,
			&GTK_WIDGET (w)->style->fg[GTK_STATE_SELECTED]);
		}
	if (move)
		gtk_clist_moveto (GTK_CLIST(w), new, 0, 0.5, 0.0);
}

static gint select_ok_cb(GtkWidget *b, gpointer data)
{
	Icon_Dialog_Data *id = data;
	gchar *icon;

	if (id->function_notify)
		{
		icon = strdup(gtk_clist_get_row_data(GTK_CLIST(id->clist),id->row));
		id->function_notify(icon);
		}

	gnome_dialog_close(GNOME_DIALOG(id->dialog));
	g_free(id);
	return TRUE;
}

static gint select_cancel_cb(GtkWidget *b, gpointer data)
{
	Icon_Dialog_Data *id = data;

	/* if cancelled while loading icons, mark loading false and don't close dialog */
	if (id->loading)
		{
		id->loading = FALSE;
		return TRUE;
		}

	gnome_dialog_close(GNOME_DIALOG(id->dialog));
	g_free(id);
	return TRUE;
}

static void select_icon_cb(GtkWidget *w, gint row, gint column, GdkEventButton *bevent, gpointer data)
{
	Icon_Dialog_Data *id = data;
	if (bevent->button == 1)
		{
		update_list_highlight(w, id->row, row, FALSE);
		id->row = row;
		if (bevent->type == GDK_2BUTTON_PRESS) select_ok_cb(id->dialog, id);
		}
}

GtkWidget * icon_selection_dialog( gchar *path, gchar *file, gint fullpath, void (*func)(void *) )
{
	Icon_Dialog_Data *id;
	GtkWidget *label;
	GtkWidget *progressbar;
	DIR *dp; 
	struct dirent *dir;
	struct stat ent_sbuf;
	int file_count, c;

	if((dp = opendir(path))==NULL) 
		{ 
		/* dir not found */ 
		return NULL; 
		}

	/* path opened ok, now create the dialog */

	id = g_new(Icon_Dialog_Data, 1);

	id->row = 0;
	id->function_notify = func;
	id->fullpath = fullpath;
	id->loading = TRUE;

	id->dialog = gnome_dialog_new(_("Icon Selection"), GNOME_STOCK_BUTTON_OK, GNOME_STOCK_BUTTON_CANCEL, NULL);
	gtk_widget_set_usize(id->dialog, 300, 400);
	
	label = gtk_label_new(_("Loading Icons..."));
	gtk_box_pack_start(GTK_BOX(GNOME_DIALOG(id->dialog)->vbox),label,FALSE,FALSE,0);
	gtk_widget_show(label);

	progressbar = gtk_progress_bar_new();
	gtk_box_pack_start(GTK_BOX(GNOME_DIALOG(id->dialog)->vbox),progressbar,FALSE,FALSE,0);
	gtk_widget_show(progressbar);

	id->clist = gtk_clist_new(1);
	gtk_clist_set_policy (GTK_CLIST (id->clist), GTK_POLICY_ALWAYS, GTK_POLICY_AUTOMATIC);
	gtk_clist_set_row_height(GTK_CLIST (id->clist),50);
	gtk_signal_connect (GTK_OBJECT (id->clist), "select_row",(GtkSignalFunc) select_icon_cb, id);
	gtk_box_pack_start(GTK_BOX(GNOME_DIALOG(id->dialog)->vbox),id->clist,TRUE,TRUE,0);
	gtk_widget_show(id->clist);

        gnome_dialog_button_connect(GNOME_DIALOG(id->dialog), 1, (GtkSignalFunc) select_cancel_cb, id);

	gtk_widget_show(id->dialog);

	gtk_clist_freeze(GTK_CLIST(id->clist));

	file_count = 0;
	while ((dir = readdir(dp)) != NULL) 
	{ 
		/* skips removed files */
		if (dir->d_ino > 0) file_count++;
	}

	rewinddir(dp);

	c = 0;
	while ((dir = readdir(dp)) != NULL && id->loading)
	{ 
		/* skips removed files */
		if (dir->d_ino > 0)
			{
			gchar *fp;
			fp = g_copy_strings(path, "/", dir->d_name, NULL);
			if (stat(fp,&ent_sbuf) >= 0 && S_ISDIR(ent_sbuf.st_mode))
				{
				/* is a dir */
				}
			else
				{
				/* is a file */
				GtkWidget *pixmap;
				gchar *text[2];
				int row;
				
				text[0] = dir->d_name;
				text[1] = '\0';


				pixmap = NULL;
				pixmap = gnome_pixmap_new_from_file_at_size (fp, 48, 48);

				if (pixmap)
					{
					gchar *data;
					row = gtk_clist_append(GTK_CLIST(id->clist),text);
					gtk_clist_set_pixtext (GTK_CLIST(id->clist), row, 0, dir->d_name, 5,
						GNOME_PIXMAP(pixmap)->pixmap, GNOME_PIXMAP(pixmap)->mask);
					if (fullpath)
						data = fp;
					else
						data = dir->d_name;
					gtk_clist_set_row_data_full(GTK_CLIST(id->clist), row , strdup(data),
						(GtkDestroyNotify)free_clist_data);
					if (file)
						{
						if (!strcmp(file,dir->d_name)) id->row = row;
						}
					}
				}
			g_free(fp);
			c++;
			gtk_progress_bar_update (GTK_PROGRESS_BAR (progressbar), (float)c / file_count);
			while ( gtk_events_pending() )
				{
			        gtk_main_iteration();
				}
			}
	} 
	closedir(dp);

	gtk_clist_thaw(GTK_CLIST(id->clist));

	gtk_label_set(GTK_LABEL(label),_("Select an Icon:"));
	gtk_widget_destroy(progressbar);

	update_list_highlight(id->clist, -1, id->row , TRUE);

	gnome_dialog_button_connect(GNOME_DIALOG(id->dialog), 0, (GtkSignalFunc) select_ok_cb, id);

	if (id->loading)
		{
		id->loading = FALSE;
		return id->dialog;
		}

	/* if we reach here, loading was cancelled, so close window */
	gnome_dialog_close(GNOME_DIALOG(id->dialog));
	g_free(id);
	return NULL;
}

