/*###################################################################*/
/*##                       gqmenu (GNOME menu editor) 0.2.2        ##*/
/*###################################################################*/

#include "gmenu.h"
#include "top.xpm"
#include "unknown.xpm"
#include "folder.xpm"
#include "up.xpm"
#include "down.xpm"

static gchar *SYSTEM_APPS;
static gchar *SYSTEM_PIXMAPS;
static gchar *USER_APPS;
static gchar *USER_PIXMAPS;

static GtkWidget *app;
static GtkWidget *menu_tree_ctree;
static GtkWidget *infolabel;
static GtkWidget *infopixmap;
static GtkWidget *pathlabel;

static GtkWidget *filename_entry;
static GtkWidget *name_entry;
static GtkWidget *comment_entry;
static GtkWidget *icon_entry;
static GtkWidget *type_entry;
static GtkWidget *exec_entry;
static GtkWidget *desktop_icon;
static GtkWidget *icon_entry;
static GtkWidget *terminal_button;
static GtkWidget *multi_args_button;

static GtkWidget *tryexec_entry;
static GtkWidget *doc_entry;

static GList *topnode;
static GList *usernode;
static GList *systemnode;
static GList *current_node = NULL;
static Desktop_Data *edit_area_orig_data;
static gchar *current_path;

static int isfile(char *s);
static int isdir(char *s);
static char *filename_from_path(char *t);
static char *strip_one_file_layer(char *t);
static char *check_for_dir(char *d);
static gchar *correct_path_to_file(gchar *path1, gchar *path2, gchar *filename);
static int save_desktop_file_info (gchar *path, gchar *name, gchar *comment, gchar *tryexec,
					gchar *exec, gchar *icon, gint terminal, gchar *type,
					gchar *doc, gint multiple_args);
static int is_node_editable(GList *node);
static void free_desktop_data(Desktop_Data *d);
static Desktop_Data * get_desktop_file_info (char *file);
static gint close_folder_dialog_cb(GtkWidget *b, gpointer data);
static gint create_folder_cb(GtkWidget *w, gpointer data);
static void create_folder_pressed();
static void delete_dialog_cb( gint button, gpointer data);
static void delete_pressed_cb();
static void save_dialog_cb( gint button, gpointer data);
static void save_pressed_cb();
static void icon_cb(void *data);
static void icon_button_pressed();
static void update_edit_area(Desktop_Data *d);
static void revert_edit_area();
static void new_edit_area();
static void tree_item_selected (GtkCTree *ctree, GdkEventButton *event, gpointer data);
static void add_tree_node(GtkCTree *ctree, GList *parent);
static void add_main_tree_node();
static void about_cb();
static void destroy_cb();
int main (int argc, char *argv[]);

GnomeUIInfo file_menu[] = {
	{ GNOME_APP_UI_ITEM, "New Folder...", NULL, create_folder_pressed, NULL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_NEW, 'F',
	  GDK_CONTROL_MASK, NULL },
	{ GNOME_APP_UI_ITEM, "Delete...", NULL, delete_pressed_cb, NULL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_CUT, 'D',
	  GDK_CONTROL_MASK, NULL },
	{ GNOME_APP_UI_ITEM, "Quit", NULL, destroy_cb, NULL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_EXIT, 'Q',
	  GDK_CONTROL_MASK, NULL },
	{ GNOME_APP_UI_ENDOFINFO }
};
GnomeUIInfo help_menu[] = {
	{ GNOME_APP_UI_HELP, NULL, NULL, NULL, NULL, NULL,
	  GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
	{ GNOME_APP_UI_ITEM, "About...", NULL, about_cb, NULL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_ABOUT, 0, 0,
	  NULL },
	{ GNOME_APP_UI_ENDOFINFO }
};
GnomeUIInfo main_menu[] = {
	{ GNOME_APP_UI_SUBTREE, ("File"), NULL, file_menu, NULL, NULL,
	  GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
	{ GNOME_APP_UI_SUBTREE, ("Help"), NULL, help_menu, NULL, NULL,
	  GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
	{ GNOME_APP_UI_ENDOFINFO }
};


static void dnd_data_request(GtkWidget *widget, GdkEventDragRequest *event)
{
	Desktop_Data *d;

	d = gtk_ctree_get_row_data(GTK_CTREE(widget),current_node);

	gdk_window_dnd_data_set (widget->window, (GdkEvent *)event, d->path, strlen (d->path) + 1);
	g_print("drag request %s\n",d->path);
}


static void dnd_data_begin(GtkWidget *widget, GdkEventDragBegin *event)
{
	Desktop_Data *d;

	if (!current_node) return;

	d = gtk_ctree_get_row_data(GTK_CTREE(widget),current_node);

	g_print("drag begin %s\n",d->path);
}

static void dnd_data_dropped(GtkWidget *widget, GdkEventDropDataAvailable *event)
{
	int count = event->data_numbytes;
	char *ptr = event->data;
	int row, col;
	int winx, winy;
	GList *node;
	Desktop_Data *d;

	g_print("drop");

	if (!current_node) return;

	gdk_window_get_origin (GTK_CLIST (widget)->clist_window, &winx, &winy);
	gtk_clist_get_selection_info (GTK_CLIST (menu_tree_ctree), event->coords.x - winx, event->coords.y - winy, &row, &col);

	node = g_list_nth (GTK_CLIST (menu_tree_ctree)->row_list, row);

	d = gtk_ctree_get_row_data(GTK_CTREE(menu_tree_ctree),node);

	g_print(" on %s\n",d->path);
}

static void dnd_set_drop(GtkWidget *widget, GdkWindow *window)
{
	static char *drop_types[]=
	{
	"url:ALL"
	};

	gdk_window_dnd_drop_set(window,TRUE,drop_types,1,FALSE);
	gtk_signal_connect(GTK_OBJECT(widget),"drop_data_available_event",
		GTK_SIGNAL_FUNC (dnd_data_dropped), NULL);
}

static void dnd_set_drag(GtkWidget *widget, GdkWindow *window)
{
	static char *drop_types[]=
	{
	"url:all"
	};

	gdk_window_dnd_drag_set(window,TRUE,drop_types,1);
	gtk_signal_connect(GTK_OBJECT(widget),"drag_request_event",
		GTK_SIGNAL_FUNC (dnd_data_request), NULL);
	gtk_signal_connect(GTK_OBJECT(widget),"drag_begin_event",
		GTK_SIGNAL_FUNC (dnd_data_begin), NULL);

}


/* ------------------ */

static int isfile(char *s)
{
   struct stat st;

   if ((!s)||(!*s)) return 0;
   if (stat(s,&st)<0) return 0;
   if (S_ISREG(st.st_mode)) return 1;
   return 0;
}

static int isdir(char *s)
{
   struct stat st;
   
   if ((!s)||(!*s)) return 0;
   if (stat(s,&st)<0) return 0;
   if (S_ISDIR(st.st_mode)) return 1;
   return 0;
}

static char *filename_from_path(char *t)
{
        char *p;

        p = t + strlen(t);
        while(p > &t[0] && p[0] != '/') p--;
        p++;
        return p;
}

static char *strip_one_file_layer(char *t)
{
	char *ret;
	char *p;

	ret = strdup(t);
	p = ret + strlen(ret);
        while(p > &ret[0] && p[0] != '/') p--;
	if (strcmp(ret,p) != 0) p[0] = '\0';
        return ret;
}

static char *check_for_dir(char *d)
{
	if (!g_file_exists(d))
		{
		g_print(_("creating user directory: %s\n"), d);
		if (mkdir( d, 0755 ) < 0)
			{
			g_print(_("unable to create user directory: %s\n"), d);
			g_free(d);
			d = NULL;
			}
		}
	return d;
}


/* this function returns the correct path to a file given multiple paths, it
   returns null if neither is correct. The returned pointer points to a string
   that is freed each time this function is called */
static gchar *correct_path_to_file(gchar *path1, gchar *path2, gchar *filename)
{
	static gchar *correct_path = NULL;

	if (correct_path) g_free(correct_path);
	correct_path = NULL;

	correct_path = g_copy_strings(path1, "/", filename, NULL);
	if (isfile(correct_path)) return correct_path;
	g_free(correct_path);

	correct_path = g_copy_strings(path2, "/", filename, NULL);
	if (isfile(correct_path)) return correct_path;
	g_free(correct_path);

	correct_path = NULL;
	return correct_path;
}

static GList *get_order_of_dir(char *dir)
{
	char buf[256];
	GList *list = NULL;
	char *order_file = g_copy_strings(dir, "/.order", NULL);
	FILE *f;

	g_print("reading .order file: %s\n", order_file);

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
		g_print("%s,",buf);
		if (strlen(buf) > 0) list = g_list_append(list,strdup(buf));
		}

	fclose(f);

	g_print("\n");

	g_free(order_file);
	return list;
}

static void save_order_of_dir(GList *node)
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

	g_print("saving .order file: %s\n", row_file);

	row = GTK_CTREE_ROW(parent)->children;

	if (row)
		{
		f = fopen(row_file, "w");
		if (!f)
			{
			g_print(_("Unable to create file: %s"),row_file);
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
			g_print(_("removing .order file: %s"),row_file);
			if (unlink (row_file) < 0)
				g_print(_("unable to remove .order file: %s"),row_file);
			}
		}

	g_free(row_file);
}

static void update_list_highlight(GtkWidget *w, GList *old, GList *new, gint move)
{
        if (old)
                {
                gtk_ctree_set_background(GTK_CTREE(w), old,
                        &GTK_WIDGET (w)->style->bg[GTK_STATE_PRELIGHT]);
                gtk_ctree_set_foreground(GTK_CTREE(w), old,
                        &GTK_WIDGET (w)->style->fg[GTK_STATE_PRELIGHT]);
                }
        if (new)
                {
                gtk_ctree_set_background(GTK_CTREE(w), new,
                        &GTK_WIDGET (w)->style->bg[GTK_STATE_SELECTED]);
                gtk_ctree_set_foreground(GTK_CTREE(w), new,
                        &GTK_WIDGET (w)->style->fg[GTK_STATE_SELECTED]);
                }
        if (move)
                gtk_ctree_scroll_to (GTK_CTREE(w), new, 0, 0.5, 0.0);
}

static void move_item_down(GList *node)
{
	GList *parent = GTK_CTREE_ROW(node)->parent;
	GList *sibling = GTK_CTREE_ROW(node)->sibling;

	if (!sibling) return;

	gtk_ctree_move(GTK_CTREE(menu_tree_ctree), sibling, parent, node);
	save_order_of_dir(parent);
}

static void move_item_up(GList *node)
{
	GList *parent = GTK_CTREE_ROW(node)->parent;
	GList *sibling = node;

	node = GTK_CTREE_ROW(parent)->children;

	if (node == sibling) return;

	while(GTK_CTREE_ROW(node)->sibling != sibling) node = GTK_CTREE_ROW(node)->sibling;

	gtk_ctree_move(GTK_CTREE(menu_tree_ctree), sibling, parent, node);
	save_order_of_dir(parent);
}

static void move_down_cb(GtkWidget *w, gpointer data)
{
	if (!is_node_editable(current_node)) return;
	if (current_node == systemnode || current_node == usernode) return;
	move_item_down(current_node);
}

static void move_up_cb(GtkWidget *w, gpointer data)
{
	if (!is_node_editable(current_node)) return;
	if (current_node == systemnode || current_node == usernode) return;
	move_item_up(current_node);
}

static int save_desktop_file_info (gchar *path, gchar *name, gchar *comment, gchar *tryexec,
					gchar *exec, gchar *icon, gint terminal, gchar *type,
					gchar *doc, gint multiple_args)
{
	FILE *f;

/*	g_print("saving file: %s\n", path);*/
	f = fopen(path,"w");
	if (!f)
		{
		gnome_warning_dialog (_("Could not write file."));
		return FALSE;
		}

	fprintf(f, "[Desktop Entry]\n");
	if (name) fprintf(f, "Name=%s\n",name);
	if (comment) fprintf(f, "Comment=%s\n",comment);
	if (tryexec) fprintf(f, "TryExec=%s\n",tryexec);
	if (exec) fprintf(f, "Exec=%s\n",exec);
	if (icon) fprintf(f, "Icon=%s\n",icon);

	if (terminal)
		fprintf(f, "Terminal=1\n");
	else
		fprintf(f, "Terminal=0\n");

	if (type) fprintf(f, "Type=%s\n",type);
	if (doc) fprintf(f, "DocPath=%s\n",doc);

	if (multiple_args)
		fprintf(f, "MultipleArgs=1\n");
	else
		fprintf(f, "MultipleArgs=0\n");

	fclose(f);
	return TRUE;
}

static int is_node_editable(GList *node)
{
	Desktop_Data *d;
	gboolean leaf;
	GList *parent;

	gtk_ctree_get_node_info(GTK_CTREE(menu_tree_ctree),node,
		NULL,NULL,NULL,NULL,NULL,NULL,&leaf,NULL);
	if (leaf)
		{
		parent = GTK_CTREE_ROW(node)->parent;
		}
	else
		{
		parent = node;
		}
	d = gtk_ctree_get_row_data(GTK_CTREE(menu_tree_ctree), parent);
	return d->editable;
}

static void free_desktop_data(Desktop_Data *d)
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

static Desktop_Data * get_desktop_file_info (char *file)
{
	Desktop_Data *d;
	FILE *f;
	gchar buf1[2048];
	gchar *buf2;

/*	g_print("reading file: %s\n",file);*/

	if (!isdir(file) && !isfile(file))
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
	d->multiple_args = 0;


	if (isdir(file))
		{
		d->isfolder = TRUE;
		d->name = strdup(filename_from_path(file));
		d->comment = g_copy_strings(d->name , _(" Folder"), NULL);
		d->pixmap = gnome_pixmap_new_from_xpm_d (folder_xpm);
		return d;
		}

	f = fopen(file,"r");
	if (!f)
		{
		return NULL;
		}

	while (fgets(buf1,2048,f))
		{
		gchar *p;
		int l;
		l = strlen(buf1);
		buf2 = buf1;
		while (buf2[0] != '=' && buf2[0] != '\n' && buf2 < buf1 + l) buf2++;
		buf2[0] = '\0';
		buf2 ++;

		p = buf1;
		while (p < buf1 + l)
			{
			if (p[0] == '\n') p[0] = '\0';
			p++;
			}

		if (!strcasecmp(buf1,"Name"))
			{
			if (d->name) free(d->name);
			d->name = strdup(buf2);
			}
		if (!strcasecmp(buf1,"Comment"))
			{
			if (d->comment) free(d->comment);
			d->comment = strdup(buf2);
			}
		if (!strcasecmp(buf1,"TryExec"))
			{
			if (d->tryexec) free(d->tryexec);
			d->tryexec = strdup(buf2);
			}
		if (!strcasecmp(buf1,"Exec"))
			{
			if (d->exec) free(d->exec);
			d->exec = strdup(buf2);
			}
		if (!strcasecmp(buf1,"Icon"))
			{
			if (d->icon) free(d->icon);
			d->icon = strdup(buf2);
			}
		if (!strcasecmp(buf1,"Type"))
			{
			if (d->type) free(d->type);
			d->type = strdup(buf2);
			}
		if (!strcasecmp(buf1,"Terminal"))
			{
			if (!strcmp(buf2,"1"))
				d->terminal = TRUE;
			else
				d->terminal = FALSE;
			}
		if (!strcasecmp(buf1,"DocPath"))
			{
			if (d->doc) free(d->doc);
			d->doc = strdup(buf2);
			}
		if (!strcasecmp(buf1,"MultipleArgs"))
			{
			if (!strcmp(buf2,"1"))
				d->multiple_args = TRUE;
			else
				d->multiple_args = FALSE;
			}
		}

	fclose(f);

	if (d->icon)
		{
		gchar *icon_path;
		icon_path = correct_path_to_file(SYSTEM_PIXMAPS, USER_PIXMAPS, d->icon);
		if (icon_path)
			d->pixmap = gnome_pixmap_new_from_file_at_size (icon_path, 20, 20);
		else
			d->pixmap = gnome_pixmap_new_from_xpm_d (unknown_xpm);
		}
	else
		{
		d->pixmap = gnome_pixmap_new_from_xpm_d (unknown_xpm);
		}

	return d;
}

static gint close_folder_dialog_cb(GtkWidget *b, gpointer data)
{
	GtkWidget *w = data;
	gnome_dialog_close(GNOME_DIALOG(w));
	return TRUE;
}

static gint create_folder_cb(GtkWidget *w, gpointer data)
{
	Misc_Dialog *dlg = data;
	gchar *new_folder;
	gchar *full_path;
	gint write_file = TRUE;

	new_folder = gtk_entry_get_text(GTK_ENTRY(dlg->entry));

	if (current_path)
		full_path = g_copy_strings(current_path, "/", new_folder, NULL);
	else
		full_path = g_copy_strings(USER_APPS, "/", new_folder, NULL);

/*	g_print("creating folder: %s\n",full_path);*/

	if (isdir(full_path) || isfile(full_path))
		{
		gnome_warning_dialog (_("File exists."));
		write_file = FALSE;
		}

	if (write_file)
		{
		if ( (mkdir (full_path, 0755) < 0) )
			{
			gnome_warning_dialog (_("Failed to create file."));
			}
		else
			{
			Desktop_Data *d;
			GList *parent;
			GList *node;
			d = get_desktop_file_info (full_path);
			if (d)
				{
				gchar *text[2];
				gboolean leaf;

				text[0] = d->name;
				text[1] = NULL;

				gtk_ctree_get_node_info(GTK_CTREE(menu_tree_ctree),current_node,
					NULL,NULL,NULL,NULL,NULL,NULL,&leaf,NULL);

				if (leaf)
					{
					node = current_node;
					parent = GTK_CTREE_ROW(current_node)->parent;
					}
				else
					{
					parent = current_node;
					if (GTK_CTREE_ROW(current_node)->children)
						node = GTK_CTREE_ROW(current_node)->children;
					else
						node = NULL;
					}

				if (d->isfolder)
					node = gtk_ctree_insert (GTK_CTREE(menu_tree_ctree), parent, node, text, 5,
						GNOME_PIXMAP(d->pixmap)->pixmap,
						GNOME_PIXMAP(d->pixmap)->mask, NULL, NULL, FALSE, FALSE);
				else
					node = gtk_ctree_insert (GTK_CTREE(menu_tree_ctree), parent, node, text, 5,
						GNOME_PIXMAP(d->pixmap)->pixmap,
						GNOME_PIXMAP(d->pixmap)->mask, NULL, NULL, TRUE, FALSE);
				gtk_ctree_set_row_data (GTK_CTREE(menu_tree_ctree), node, d);
				save_order_of_dir(parent);
				}

			if (current_path) free(current_path);
			current_path = strdup(full_path);
			gtk_label_set(GTK_LABEL(pathlabel),current_path);
			}
		}

	g_free(full_path);	
	gnome_dialog_close(GNOME_DIALOG(dlg->dialog));
	return TRUE;
}

static void create_folder_pressed()
{
	Misc_Dialog *dlg;
	GtkWidget *label;

	if (!is_node_editable(current_node))
		{
		gnome_warning_dialog (_("You can't add an entry to that folder!\nTo edit system entries you must be root."));
		return;
		}

	dlg = g_new(Misc_Dialog, 1);
	
	dlg->dialog = gnome_dialog_new("New Folder", GNOME_STOCK_BUTTON_OK, GNOME_STOCK_BUTTON_CANCEL, NULL);

	label = gtk_label_new(_("Create Folder:"));
	gtk_box_pack_start(GTK_BOX(GNOME_DIALOG(dlg->dialog)->vbox),label,FALSE,FALSE,0);
	gtk_widget_show(label);

	dlg->entry = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(GNOME_DIALOG(dlg->dialog)->vbox),dlg->entry,FALSE,FALSE,0);
	gtk_widget_grab_focus(dlg->entry);
	gtk_widget_show(dlg->entry);

	gnome_dialog_button_connect(GNOME_DIALOG(dlg->dialog), 0, (GtkSignalFunc) create_folder_cb, dlg);
	gnome_dialog_button_connect(GNOME_DIALOG(dlg->dialog), 1, (GtkSignalFunc) close_folder_dialog_cb, dlg->dialog);
	gnome_dialog_set_default(GNOME_DIALOG(dlg->dialog), 0);
	gnome_dialog_editable_enters(GNOME_DIALOG(dlg->dialog), GTK_EDITABLE(dlg->entry));

	gtk_widget_show(dlg->dialog);
}


static void delete_dialog_cb( gint button, gpointer data)
{
	if (!button)
		{
		Desktop_Data *d;
		GList *node;
		d = gtk_ctree_get_row_data(GTK_CTREE(menu_tree_ctree),current_node);

		if ( (unlink (d->path) < 0) )
			{
			gnome_warning_dialog (_("Failed to delete the file."));
			return;
			}

/*		g_print("deleted file: %s\n",d->path);*/

		node = GTK_CTREE_ROW(current_node)->parent;
		gtk_ctree_remove(GTK_CTREE(menu_tree_ctree),current_node);
		current_node = node;
		free_desktop_data(d);

		save_order_of_dir(node);
		edit_area_orig_data = NULL;
		new_edit_area();
		}
}

static void delete_pressed_cb()
{
	Desktop_Data *d;
	if (!current_node)
		{
		gnome_warning_dialog (_("You must select something first!"));
		return;
		}

	if (current_node == topnode || current_node == usernode || current_node == systemnode)
		{
		gnome_warning_dialog (_("You can not delete a top level Folder!"));
		return;
		}

	d = gtk_ctree_get_row_data(GTK_CTREE(menu_tree_ctree),current_node);

	if (!d->editable)
		{
		gnome_warning_dialog (_("You can't delete that file!\nTo edit system entries you must be root."));
		return;
		}

	if (isfile(d->path))
		{
		gnome_question_dialog (_("Delete file?"),
			(GnomeReplyCallback) delete_dialog_cb, NULL);
		return;
		}

	if (isdir(d->path))
		gnome_warning_dialog (_("Cannot delete a folder. (yet...)"));
	else
		gnome_warning_dialog (_("File or Folder does not exist on filesystem."));
}


static void save_dialog_cb( gint button, gpointer data)
{
	if (!button)
		{
		Desktop_Data *d;
		GList *node;
		GList *parent;
		gint overwrite;
		char *path;

		path = g_copy_strings(current_path, "/", gtk_entry_get_text(GTK_ENTRY(filename_entry)), NULL);

		overwrite = isfile(path);

		save_desktop_file_info (path,
					gtk_entry_get_text(GTK_ENTRY(name_entry)),
					gtk_entry_get_text(GTK_ENTRY(comment_entry)),
					gtk_entry_get_text(GTK_ENTRY(tryexec_entry)),
					gtk_entry_get_text(GTK_ENTRY(exec_entry)),
					gtk_entry_get_text(GTK_ENTRY(icon_entry)),
					GTK_TOGGLE_BUTTON (terminal_button)->active,
					gtk_entry_get_text(GTK_ENTRY(type_entry)),
					gtk_entry_get_text(GTK_ENTRY(doc_entry)),
					GTK_TOGGLE_BUTTON (multi_args_button)->active);
		if (overwrite)
			{
			gint8 spacing;
			gboolean leaf;
			gboolean expanded;
			d = gtk_ctree_get_row_data(GTK_CTREE(menu_tree_ctree), current_node);
			free_desktop_data(d);

			d = get_desktop_file_info (path);

			gtk_ctree_set_row_data(GTK_CTREE(menu_tree_ctree), current_node, d);

			gtk_ctree_get_node_info (GTK_CTREE(menu_tree_ctree), current_node, NULL, &spacing,
						NULL, NULL, NULL, NULL, &leaf, &expanded);
			gtk_ctree_set_node_info (GTK_CTREE(menu_tree_ctree), current_node, d->name, spacing,
						GNOME_PIXMAP(d->pixmap)->pixmap, GNOME_PIXMAP(d->pixmap)->mask,
						NULL, NULL, leaf, expanded);
			save_order_of_dir(current_node);
			}
		else
			{
			d = get_desktop_file_info (path);
			if (d)
				{
				gchar *text[2];
				gboolean leaf;

				text[0] = d->name;
				text[1] = NULL;

				gtk_ctree_get_node_info(GTK_CTREE(menu_tree_ctree),current_node,
					NULL,NULL,NULL,NULL,NULL,NULL,&leaf,NULL);

				if (leaf)
					{
					node = current_node;
					parent = GTK_CTREE_ROW(current_node)->parent;
					}
				else
					{
					parent = current_node;
					if (GTK_CTREE_ROW(current_node)->children)
						node = GTK_CTREE_ROW(current_node)->children;
					else
						node = NULL;
					}

				if (d->isfolder)
					node = gtk_ctree_insert (GTK_CTREE(menu_tree_ctree), parent, node, text, 5,
						GNOME_PIXMAP(d->pixmap)->pixmap,
						GNOME_PIXMAP(d->pixmap)->mask, NULL, NULL, FALSE, FALSE);
				else
					node = gtk_ctree_insert (GTK_CTREE(menu_tree_ctree), parent, node, text, 5,
						GNOME_PIXMAP(d->pixmap)->pixmap,
						GNOME_PIXMAP(d->pixmap)->mask, NULL, NULL, TRUE, FALSE);
				gtk_ctree_set_row_data (GTK_CTREE(menu_tree_ctree), node, d);
				save_order_of_dir(node);
				}
			}
		g_free(path);
		}
} 

static void save_pressed_cb()
{
	char *path;

	path = g_copy_strings(current_path, "/", gtk_entry_get_text(GTK_ENTRY(filename_entry)), NULL);

	if (!is_node_editable(current_node))
		{
		if (isfile(path))
			gnome_warning_dialog (_("You can't edit an entry in that folder!\nTo edit system entries you must be root."));
		else
			gnome_warning_dialog (_("You can't add an entry to that folder!\nTo edit system entries you must be root."));
		return;
		}


	if (isfile(path))
		{
		gnome_question_dialog (_("Overwrite existing file?"),
			(GnomeReplyCallback) save_dialog_cb, NULL);
		return;
		}

	gnome_question_dialog (_("Save file?"),
		(GnomeReplyCallback) save_dialog_cb, NULL);
	g_free(path);
}

static void icon_cb(void *data)
{
	gchar *icon = data;
/*	g_print("icon = %s\n",icon);*/
	gtk_entry_set_text(GTK_ENTRY(icon_entry), icon);

	gnome_pixmap_load_file(GNOME_PIXMAP(desktop_icon),
		correct_path_to_file(SYSTEM_PIXMAPS, USER_PIXMAPS, icon));

	g_free(icon);
}

static void icon_button_pressed()
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

static void update_edit_area(Desktop_Data *d)
{
	edit_area_orig_data = d;

	if (d->name)
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
}

static void revert_edit_area()
{
	if (edit_area_orig_data)
		update_edit_area(edit_area_orig_data);
}

static void new_edit_area()
{
	edit_area_orig_data = NULL;
	gtk_entry_set_text(GTK_ENTRY(name_entry), "");
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
}

static void tree_item_selected (GtkCTree *ctree, GdkEventButton *event, gpointer data)
{
	gint row, col;
	GList *node;
	Desktop_Data *d;

	gtk_clist_get_selection_info (GTK_CLIST (ctree), event->x, event->y,
                                      &row, &col);
	node = g_list_nth (GTK_CLIST (ctree)->row_list, row);

	if (!node) return;

	d = gtk_ctree_get_row_data(GTK_CTREE(ctree),node);
	gtk_label_set(GTK_LABEL(infolabel),d->comment);

	if (d)
		{
		if (!d->editable)
			gnome_stock_pixmap_widget_set_icon(GNOME_STOCK_PIXMAP_WIDGET(infopixmap),
								GNOME_STOCK_MENU_BOOK_RED );
		else
			gnome_stock_pixmap_widget_set_icon(GNOME_STOCK_PIXMAP_WIDGET(infopixmap),
								GNOME_STOCK_MENU_BLANK );
		}

	if (node == topnode) return;

	current_node = node;

	if (!d->isfolder)
		{
		update_edit_area(d);
		if (current_path) free(current_path);
		current_path = strip_one_file_layer(d->path);
		}

	if (d->isfolder)
		{
		if (!d->expanded)
			{
			d->expanded = TRUE;
			add_tree_node(ctree, node);
			}
		if (current_path) free(current_path);
		current_path = strdup (d->path);

		}
	gtk_label_set(GTK_LABEL(pathlabel),current_path);
}

/* if node is null it is appended, if it is a sibling, it is inserted */
static GList *add_leaf_node(GtkCTree *ctree, GList *parent, GList *node, char *file)
{
	Desktop_Data *d;
	Desktop_Data *parent_data;
	char *path_buf;

/*	g_print("%s\n",file);*/

	parent_data = gtk_ctree_get_row_data(GTK_CTREE(ctree), parent);

	path_buf = g_copy_strings (parent_data->path, "/", file, NULL);
				
	if (!g_file_exists(path_buf))
		{
		g_free(path_buf);
		return node;
		}

	d = get_desktop_file_info (path_buf);
	if (d)
		{
		gchar *text[2];

		d->editable = parent_data->editable;

		text[0] = d->name;
		text[1] = NULL;
		if (d->isfolder)
			node = gtk_ctree_insert (GTK_CTREE(ctree), parent, node, text, 5,
				GNOME_PIXMAP(d->pixmap)->pixmap,
				GNOME_PIXMAP(d->pixmap)->mask, NULL, NULL, FALSE, FALSE);
		else
			node = gtk_ctree_insert (GTK_CTREE(ctree), parent, node, text, 5,
				GNOME_PIXMAP(d->pixmap)->pixmap,
				GNOME_PIXMAP(d->pixmap)->mask, NULL, NULL, TRUE, FALSE);
		gtk_ctree_set_row_data (GTK_CTREE(ctree), node, d);
		}
	g_free(path_buf);
	return node;
}

static void add_tree_node(GtkCTree *ctree, GList *parent)
{
	DIR *dp; 
	struct dirent *dir;

	GList *node = NULL;
	GList *orderlist = NULL;
	Desktop_Data *parent_data;

	parent_data = gtk_ctree_get_row_data(GTK_CTREE(ctree), parent);

/*	g_print("reading node: %s\n", parent_data->path);*/

	orderlist = get_order_of_dir(parent_data->path);
	if (orderlist)
		{
		int i;
		int l = g_list_length(orderlist);
		for (i=0;i<l;i++)
			{
			GList *list = g_list_nth(orderlist, i);
			node = add_leaf_node(ctree, parent, NULL, list->data);
			}
		}

	if((dp = opendir(parent_data->path))==NULL) 
		{ 
		/* dir not found */ 
		return; 
		}

	while ((dir = readdir(dp)) != NULL) 
		{ 
		/* skips removed files */
		if (dir->d_ino > 0)
			{
			int ordered = FALSE;
			if (orderlist)
				{
				int i;
				int l = g_list_length(orderlist);
				for (i=0;i<l;i++)
					{
					GList *list = g_list_nth(orderlist, i);
					if (strcmp(dir->d_name, list->data) == 0)
						{
						ordered = TRUE;
						}
					}
				}
			if (!ordered)			
				{
				if (strncmp(dir->d_name, ".", 1) != 0)
					{
					node = add_leaf_node(ctree, parent, NULL, dir->d_name);
					}
				}
			}
		} 

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

 
	closedir(dp);
}


static void add_main_tree_node()
{
	GList *subnode;
	gchar *text[2];
	Desktop_Data *d;
	GtkWidget *pixmap;

/*	g_print("adding top node...\n");*/

	/* very top of tree */
	topnode = NULL;
	text[0] = "GNOME";
	text[1] = NULL;
	
	pixmap = gnome_pixmap_new_from_xpm_d(top_xpm);
	topnode = gtk_ctree_insert (GTK_CTREE(menu_tree_ctree), NULL, NULL, text, 5,
		NULL, NULL,
		GNOME_PIXMAP(pixmap)->pixmap, GNOME_PIXMAP(pixmap)->mask,
		FALSE, TRUE);

	d = g_new(Desktop_Data, 1);

	d->comment = strdup(_("GNOME"));
	d->expanded = TRUE;
	d->isfolder = TRUE;
	d->editable = FALSE;

	gtk_ctree_set_row_data (GTK_CTREE(menu_tree_ctree), topnode, d);

	/* system's menu tree */
	text[0] = _("System Menus");
	pixmap = gnome_pixmap_new_from_xpm_d(top_xpm);
	subnode = gtk_ctree_insert (GTK_CTREE(menu_tree_ctree), topnode, NULL, text, 5,
		NULL, NULL,
		GNOME_PIXMAP(pixmap)->pixmap, GNOME_PIXMAP(pixmap)->mask,
		FALSE, TRUE);

	d = get_desktop_file_info (SYSTEM_APPS);

	if (d->comment) free(d->comment);
	d->comment = strdup(_("Top of system menus"));
	d->expanded = TRUE;

	/* FIXME: are we root? then we can edit the system menu */
	if (!strcmp("/root",getenv("HOME")) || 
	    ((getenv("USER"))&&(!strcmp("root",getenv("USER")))) ||
		((getenv("USERNAME"))&&(!strcmp("root",getenv("USERNAME")))) )
		{
		g_print(_("Running with root privileges.\n"));
		d->editable = TRUE;
		}
	else
		d->editable = FALSE;

	gtk_ctree_set_row_data (GTK_CTREE(menu_tree_ctree), subnode, d);

	add_tree_node(GTK_CTREE(menu_tree_ctree), subnode);
	systemnode = subnode;

	/* user's menu tree */
	text[0] = _("User Menus");
	pixmap = gnome_pixmap_new_from_xpm_d(top_xpm);
	subnode = gtk_ctree_insert (GTK_CTREE(menu_tree_ctree), topnode, subnode, text, 5,
		NULL, NULL,
		GNOME_PIXMAP(pixmap)->pixmap, GNOME_PIXMAP(pixmap)->mask,
		FALSE, TRUE);

	d = get_desktop_file_info (USER_APPS);

	if (d->comment) free(d->comment);
	d->comment = strdup(_("Top of user menus"));
	d->expanded = TRUE;
	d->editable = TRUE;

	current_path = strdup(d->path);
	gtk_label_set(GTK_LABEL(pathlabel),current_path);

	gtk_ctree_set_row_data (GTK_CTREE(menu_tree_ctree), subnode, d);

	add_tree_node(GTK_CTREE(menu_tree_ctree), subnode);

	usernode = subnode;
	current_node = subnode;
	gtk_ctree_select(GTK_CTREE(menu_tree_ctree), current_node);
}

static void about_cb()
{
	GtkWidget *about;
	gchar *authors[2];
	gchar version[32];

	sprintf(version,"%d.%d.%d",GMENU_VERSION_MAJOR, GMENU_VERSION_MINOR, GMENU_VERSION_REV);

	authors[0] = "John Ellis (gqview@geocities.com)";
	authors[1] = NULL;

	about = gnome_about_new ( _("GNOME menu editor"), version,
			"(C) 1998",
			authors,
			_("Released under the terms of the GNU Public License.\n"
			"GNOME menu editor."),
			NULL);
	gtk_widget_show (about);
}

static void destroy_cb()
{
	gtk_main_quit();
}

int main (int argc, char *argv[])
{
	GtkWidget *mainbox;
	GtkWidget *hbox;
	GtkWidget *hbox1;
	GtkWidget *vbox;
	GtkWidget *notebook;
	GtkWidget *table;
	GtkWidget *frame;
	GtkWidget *pixmap;
	GtkWidget *label;
	GtkWidget *button;

	gnome_init ("GNOME menu editor", NULL, argc, argv, 0, NULL);

	SYSTEM_APPS = gnome_unconditional_datadir_file("apps");
	SYSTEM_PIXMAPS = gnome_unconditional_datadir_file("pixmaps");
	if (!g_file_exists(SYSTEM_APPS) || !g_file_exists(SYSTEM_PIXMAPS))
		{
		g_print(_("unable to retrieve GNOME installation directory\n"));
		return 1;
		}

	/* FIXME: is the user's menu ~/.gnome/apps or ~/.gnome/share/apps ? */
	USER_APPS = check_for_dir(gnome_util_home_file("apps"));
	USER_PIXMAPS = check_for_dir(gnome_util_home_file("pixmaps"));

	app = gnome_app_new ("gmenu","GNOME menu editor");
	gtk_widget_set_usize (app, 600,400);
	gtk_signal_connect(GTK_OBJECT(app), "delete_event", GTK_SIGNAL_FUNC(destroy_cb), NULL);

	gnome_app_create_menus_with_data (GNOME_APP(app), main_menu, app);
	gtk_menu_item_right_justify(GTK_MENU_ITEM(main_menu[1].widget));

	mainbox = gtk_hbox_new (FALSE, 0);
        gnome_app_set_contents(GNOME_APP(app),mainbox);
        gtk_widget_show (mainbox);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_border_width (GTK_CONTAINER (vbox), 5);
	gtk_box_pack_start(GTK_BOX(mainbox),vbox,TRUE,TRUE,0);
	gtk_widget_show(vbox);

	menu_tree_ctree = gtk_ctree_new(1, 0);
	gtk_clist_set_row_height(GTK_CLIST(menu_tree_ctree),22);
	gtk_clist_set_column_width(GTK_CLIST(menu_tree_ctree),0,300);
	gtk_signal_connect(GTK_OBJECT(menu_tree_ctree),"button_press_event", GTK_SIGNAL_FUNC(tree_item_selected),NULL);
	gtk_box_pack_start(GTK_BOX(vbox),menu_tree_ctree,TRUE,TRUE,0);
	gtk_widget_show(menu_tree_ctree);

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox),hbox,FALSE,FALSE,0);
        gtk_widget_show (hbox);

	frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(frame),GTK_SHADOW_IN);
	gtk_box_pack_start(GTK_BOX(hbox),frame,FALSE,FALSE,0);
	gtk_widget_show(frame);

	infopixmap = gnome_stock_pixmap_widget_new(app, GNOME_STOCK_MENU_BLANK );
	gtk_container_add(GTK_CONTAINER(frame),infopixmap);
	gtk_widget_show(infopixmap);

	frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(frame),GTK_SHADOW_IN);
	gtk_box_pack_end(GTK_BOX(hbox),frame,TRUE,TRUE,0);
	gtk_widget_show(frame);

	infolabel = gtk_label_new("Edit gnome desktop entries with this app");
	gtk_container_add(GTK_CONTAINER(frame),infolabel);
	gtk_widget_show(infolabel);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(mainbox),vbox,TRUE,TRUE,0);
	gtk_widget_show(vbox);

	button = gtk_button_new();
	gtk_signal_connect(GTK_OBJECT(button),"clicked",GTK_SIGNAL_FUNC(move_down_cb), NULL);
	gtk_box_pack_end(GTK_BOX(vbox),button,FALSE,FALSE,0);
	gtk_widget_show(button);

	pixmap = gnome_pixmap_new_from_xpm_d( down_xpm );
	gtk_container_add(GTK_CONTAINER(button),pixmap);
	gtk_widget_show(pixmap);

	button = gtk_button_new();
	gtk_signal_connect(GTK_OBJECT(button),"clicked",GTK_SIGNAL_FUNC(move_up_cb), NULL);
	gtk_box_pack_end(GTK_BOX(vbox),button,FALSE,FALSE,0);
	gtk_widget_show(button);

	pixmap = gnome_pixmap_new_from_xpm_d( up_xpm );
	gtk_container_add(GTK_CONTAINER(button),pixmap);
	gtk_widget_show(pixmap);

	/* edit area */
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_border_width (GTK_CONTAINER (vbox), 5);
	gtk_box_pack_start(GTK_BOX(mainbox),vbox,TRUE,TRUE,0);
	gtk_widget_show(vbox);

	notebook = gtk_notebook_new();
	gtk_notebook_set_tab_pos (GTK_NOTEBOOK(notebook), GTK_POS_TOP);
	gtk_box_pack_start (GTK_BOX(vbox), notebook, TRUE, TRUE, 0);
	gtk_widget_show(notebook);

	/* properties page */
	frame = gtk_frame_new(NULL);
	gtk_container_border_width (GTK_CONTAINER (frame), 5);
	gtk_widget_show(frame);
 
	table = gtk_table_new( 7, 2, FALSE);
	gtk_container_border_width (GTK_CONTAINER (table), 5);
	gtk_container_add (GTK_CONTAINER(frame),table);
	gtk_widget_show(table);

	label = gtk_label_new(_("Name:"));
	gtk_table_attach(GTK_TABLE(table),label, 0, 1, 0, 1, 0, 0, 0, 0);
	gtk_widget_show(label);

	name_entry = gtk_entry_new_with_max_length(255);
	gtk_table_attach_defaults(GTK_TABLE(table),name_entry, 1, 2, 0, 1);
	gtk_widget_show(name_entry);

	label = gtk_label_new(_("Comment:"));
	gtk_table_attach(GTK_TABLE(table),label, 0, 1, 1, 2, 0, 0, 0, 0);
	gtk_widget_show(label);

	comment_entry = gtk_entry_new_with_max_length(255);
	gtk_table_attach_defaults(GTK_TABLE(table),comment_entry, 1, 2, 1, 2);
	gtk_widget_show(comment_entry);

	label = gtk_label_new(_("File name:"));
	gtk_table_attach(GTK_TABLE(table),label, 0, 1, 2, 3, 0, 0, 0, 0);
	gtk_widget_show(label);

	filename_entry = gtk_entry_new_with_max_length(255);
	gtk_table_attach_defaults(GTK_TABLE(table),filename_entry, 1, 2, 2, 3);
	gtk_widget_show(filename_entry);

	label = gtk_label_new(_("Command:"));
	gtk_table_attach(GTK_TABLE(table),label, 0, 1, 3, 4, 0, 0, 0, 0);
	gtk_widget_show(label);

	exec_entry = gtk_entry_new_with_max_length(255);
	gtk_table_attach_defaults(GTK_TABLE(table),exec_entry, 1, 2, 3, 4);
	gtk_widget_show(exec_entry);

	label = gtk_label_new(_("Type:"));
	gtk_table_attach(GTK_TABLE(table),label, 0, 1, 4, 5, 0, 0, 0, 0);
	gtk_widget_show(label);

	type_entry = gtk_entry_new_with_max_length(255);
	gtk_table_attach_defaults(GTK_TABLE(table),type_entry, 1, 2, 4, 5);
	gtk_widget_show(type_entry);

	gtk_widget_realize(app);

	button = gtk_button_new();
	gtk_widget_set_usize(button, 48, 48);
	gtk_signal_connect (GTK_OBJECT(button),"clicked",(GtkSignalFunc) icon_button_pressed, NULL);
	gtk_table_attach(GTK_TABLE(table),button, 0, 1, 5, 6, 0, 0, 0, 0);
	gtk_widget_show(button);

	desktop_icon = gnome_pixmap_new_from_xpm_d (unknown_xpm);
	gtk_container_add(GTK_CONTAINER(button),desktop_icon);
	gtk_widget_show(desktop_icon);

	icon_entry = gtk_entry_new_with_max_length(255);
	gtk_entry_set_editable (GTK_ENTRY(icon_entry),FALSE);
	gtk_table_attach_defaults(GTK_TABLE(table),icon_entry, 1, 2, 5, 6);
	gtk_widget_show(icon_entry);

	terminal_button = gtk_check_button_new_with_label ("Run in Terminal");
	gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON(terminal_button), FALSE);
	gtk_table_attach_defaults(GTK_TABLE(table),terminal_button, 1, 2, 6, 7);
	gtk_widget_show(terminal_button);

	label = gtk_label_new(_("Properties"));
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), frame, label);

	/* advanced page */
	frame = gtk_frame_new(NULL);
	gtk_container_border_width (GTK_CONTAINER (frame), 5);
	gtk_widget_show(frame);
 
	table = gtk_table_new( 7, 2, FALSE);
	gtk_container_border_width (GTK_CONTAINER (table), 5);
	gtk_container_add (GTK_CONTAINER(frame),table);
	gtk_widget_show(table);

	label = gtk_label_new(_("Try and Exec:"));
	gtk_table_attach(GTK_TABLE(table),label, 0, 1, 0, 1, 0, 0, 0, 0);
	gtk_widget_show(label);

	tryexec_entry = gtk_entry_new_with_max_length(255);
	gtk_table_attach_defaults(GTK_TABLE(table),tryexec_entry, 1, 2, 0, 1);
	gtk_widget_show(tryexec_entry);

	label = gtk_label_new(_("Documentation:"));
	gtk_table_attach(GTK_TABLE(table),label, 0, 1, 1, 2, 0, 0, 0, 0);
	gtk_widget_show(label);

	doc_entry = gtk_entry_new_with_max_length(255);
	gtk_table_attach_defaults(GTK_TABLE(table),doc_entry, 1, 2, 1, 2);
	gtk_widget_show(doc_entry);

	multi_args_button = gtk_check_button_new_with_label ("Allow multiple Args");
	gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON(multi_args_button), FALSE);
	gtk_table_attach_defaults(GTK_TABLE(table),multi_args_button, 1, 2, 2, 3);
	gtk_widget_show(multi_args_button);

	label = gtk_label_new(_("Advanced"));
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), frame, label);

	hbox1 = gtk_hbox_new(TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox),hbox1,FALSE,FALSE,10);
	gtk_widget_show(hbox1);

	button = gtk_button_new();
	gtk_signal_connect(GTK_OBJECT(button),"clicked",GTK_SIGNAL_FUNC(save_pressed_cb), NULL);
	gtk_box_pack_start(GTK_BOX(hbox1),button,FALSE,FALSE,10);
	gtk_widget_show(button);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(button),hbox);
	gtk_widget_show(hbox);

	pixmap = gnome_stock_pixmap_widget_new(app, GNOME_STOCK_PIXMAP_SAVE );
	gtk_box_pack_start(GTK_BOX(hbox),pixmap,FALSE,FALSE,0);
	gtk_widget_show(pixmap);

	label = gtk_label_new(_("Save"));
	gtk_box_pack_start(GTK_BOX(hbox),label,FALSE,FALSE,5);
	gtk_widget_show(label);

	button = gtk_button_new();
	gtk_signal_connect(GTK_OBJECT(button),"clicked",GTK_SIGNAL_FUNC(revert_edit_area), NULL);
	gtk_box_pack_start(GTK_BOX(hbox1),button,FALSE,FALSE,10);
	gtk_widget_show(button);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(button),hbox);
	gtk_widget_show(hbox);

	pixmap = gnome_stock_pixmap_widget_new(app, GNOME_STOCK_PIXMAP_REVERT );
	gtk_box_pack_start(GTK_BOX(hbox),pixmap,FALSE,FALSE,0);
	gtk_widget_show(pixmap);

	label = gtk_label_new(_("Revert"));
	gtk_box_pack_start(GTK_BOX(hbox),label,FALSE,FALSE,5);
	gtk_widget_show(label);

	button = gtk_button_new();
	gtk_signal_connect(GTK_OBJECT(button),"clicked",GTK_SIGNAL_FUNC(new_edit_area), NULL);
	gtk_box_pack_start(GTK_BOX(hbox1),button,FALSE,FALSE,10);
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

	frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(frame),GTK_SHADOW_IN);
	gtk_box_pack_end(GTK_BOX(vbox),frame,FALSE,FALSE,0);
	gtk_widget_show(frame);

	pathlabel = gtk_label_new(USER_APPS);
	gtk_container_add(GTK_CONTAINER(frame),pathlabel);
	gtk_widget_show(pathlabel);

	gtk_widget_show(app);

	add_main_tree_node();
	
	new_edit_area();

/* in progress..
	dnd_set_drag(menu_tree_ctree, GTK_CLIST(menu_tree_ctree)->clist_window);
	dnd_set_drop(menu_tree_ctree, GTK_CLIST(menu_tree_ctree)->clist_window);
*/

	gtk_main();
	return 0;
}
