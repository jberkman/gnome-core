/*###################################################################*/
/*##                       gmenu (GNOME menu editor) 0.3.0         ##*/
/*###################################################################*/

#include <config.h>
#include "gmenu.h"
#include "up.xpm"
#include "down.xpm"

gchar *SYSTEM_APPS;
gchar *SYSTEM_PIXMAPS;
gchar *USER_APPS;
gchar *USER_PIXMAPS;

GtkWidget *app;
GtkWidget *menu_tree_ctree;
GtkWidget *infolabel;
GtkWidget *infopixmap;
GtkWidget *pathlabel;

GtkObject *edit_area;

GtkCTreeNode *topnode;
GtkCTreeNode *usernode;
GtkCTreeNode *systemnode;
GtkCTreeNode *current_node = NULL;
gchar *current_path;

Desktop_Data *edit_area_orig_data = NULL;

static void sort_node( GtkCTreeNode *node);
static void sort_single_pressed();
static void sort_recurse_cb(GtkCTree *ctree, GtkCTreeNode *node, gpointer data);
static void sort_recursive_pressed();
int isfile(char *s);
int isdir(char *s);
char *filename_from_path(char *t);
char *strip_one_file_layer(char *t);
gchar *correct_path_to_file(gchar *path1, gchar *path2, gchar *filename);
static char *check_for_dir(char *d);
static void about_cb();
static void destroy_cb();
int main (int argc, char *argv[]);

/* menu bar */
GnomeUIInfo file_menu[] = {
	{ GNOME_APP_UI_ITEM, N_("New Folder..."), NULL, create_folder_pressed, NULL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_NEW, 'F',
	  GDK_CONTROL_MASK, NULL },
	{ GNOME_APP_UI_ITEM, N_("Delete..."), NULL, delete_pressed_cb, NULL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_CUT, 'D',
	  GDK_CONTROL_MASK, NULL },
	{ GNOME_APP_UI_ITEM, N_("Quit"), NULL, destroy_cb, NULL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_EXIT, 'Q',
	  GDK_CONTROL_MASK, NULL },
	{ GNOME_APP_UI_ENDOFINFO }
};
GnomeUIInfo sort_menu[] = {
	{ GNOME_APP_UI_ITEM, N_("Folder"), NULL, sort_single_pressed, NULL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_SPELLCHECK, 'S',
	  GDK_CONTROL_MASK, NULL },
	{ GNOME_APP_UI_ITEM, N_("Folder Recursive"), NULL, sort_recursive_pressed, NULL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_SPELLCHECK, 'R',
	  GDK_CONTROL_MASK, NULL },
	{ GNOME_APP_UI_ENDOFINFO }
};
GnomeUIInfo help_menu[] = {
	{ GNOME_APP_UI_HELP, NULL, NULL, NULL, NULL, NULL,
	  GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
	{ GNOME_APP_UI_ITEM, N_("About..."), NULL, about_cb, NULL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_ABOUT, 0, 0,
	  NULL },
	{ GNOME_APP_UI_ENDOFINFO }
};
GnomeUIInfo main_menu[] = {
	{ GNOME_APP_UI_SUBTREE, N_("File"), NULL, file_menu, NULL, NULL,
	  GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
	{ GNOME_APP_UI_SUBTREE, N_("Sort"), NULL, sort_menu, NULL, NULL,
	  GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
	{ GNOME_APP_UI_SUBTREE, N_("Help"), NULL, help_menu, NULL, NULL,
	  GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
	{ GNOME_APP_UI_ENDOFINFO }
};

/* toolbar */
GnomeUIInfo toolbar[] = {
	{ GNOME_APP_UI_ITEM, N_("New Folder"), N_("Create a new folder"), create_folder_pressed, NULL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_NEW, 0, 0, NULL },
	{ GNOME_APP_UI_ITEM, N_("Delete"), N_("Delete selected menu item"), delete_pressed_cb, NULL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_CUT, 0, 0, NULL },
	GNOMEUIINFO_SEPARATOR,
	{ GNOME_APP_UI_ITEM, N_("Move up"), N_("Move selected menu up"), move_up_cb, NULL, NULL,
	  GNOME_APP_PIXMAP_DATA, up_xpm, 0, 0, NULL },
	{ GNOME_APP_UI_ITEM, N_("Move down"), N_("Move selected menu down"), move_down_cb, NULL, NULL,
	  GNOME_APP_PIXMAP_DATA, down_xpm, 0, 0, NULL },
	GNOMEUIINFO_SEPARATOR,
	{ GNOME_APP_UI_ITEM, N_("Properties"), N_("Edit selected menu item properties"), edit_pressed_cb, NULL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_PROPERTIES, 0, 0, NULL },
	GNOMEUIINFO_SEPARATOR,
	{ GNOME_APP_UI_ITEM, N_("Sort Folder"), N_("Sort selected folder"), sort_single_pressed, NULL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_SPELLCHECK, 0, 0, NULL },
	GNOMEUIINFO_END
};

static void sort_node( GtkCTreeNode *node)
{
	Desktop_Data *d;

	if (!node || node == topnode) return;

	d = gtk_ctree_get_row_data(GTK_CTREE(menu_tree_ctree),node);
	if (!d->isfolder) node = GTK_CTREE_ROW(node)->parent;

	gtk_ctree_sort(GTK_CTREE(menu_tree_ctree), node);
	save_order_of_dir(node);
}

static void sort_single_pressed()
{
	sort_node(current_node);
}


static void sort_recurse_cb(GtkCTree *ctree, GtkCTreeNode *node, gpointer data)
{
	Desktop_Data *d;

	if (!node) return;

	d = gtk_ctree_get_row_data(GTK_CTREE(menu_tree_ctree),node);
	if (d->isfolder) sort_node (node);
}

static void sort_recursive_pressed()
{
	Desktop_Data *d;
	GtkCTreeNode *node = current_node;

	if (!node || node == topnode) return;

	d = gtk_ctree_get_row_data(GTK_CTREE(menu_tree_ctree),node);
	if (!d->isfolder) node = GTK_CTREE_ROW(node)->parent;

	gtk_ctree_post_recursive(GTK_CTREE(menu_tree_ctree), node, sort_recurse_cb, NULL);
}

static void dnd_data_request(GtkWidget *widget, GdkEvent *event)
{
	Desktop_Data *d;

	d = gtk_ctree_get_row_data(GTK_CTREE(widget),current_node);

	gtk_widget_dnd_data_set (widget, event, d->path, strlen (d->path) + 1);
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
/*	int count = event->data_numbytes;
	char *ptr = event->data;*/
	int row, col;
	int winx, winy;
	GtkCTreeNode *node;
	Desktop_Data *d;

	g_print("drop");

	if (!current_node) return;

	gdk_window_get_origin (GTK_CLIST (widget)->clist_window, &winx, &winy);
	gtk_clist_get_selection_info (GTK_CLIST (menu_tree_ctree), event->coords.x - winx, event->coords.y - winy, &row, &col);

	node = GTK_CTREE_NODE(g_list_nth (GTK_CLIST (menu_tree_ctree)->row_list, row));

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
	"url:ALL"
	};

	gtk_widget_realize(widget);
	gdk_window_dnd_drag_set(window,TRUE,drop_types,1);
	gtk_signal_connect(GTK_OBJECT(widget),"drag_request_event",
		GTK_SIGNAL_FUNC (dnd_data_request), NULL);
	gtk_signal_connect(GTK_OBJECT(widget),"drag_begin_event",
		GTK_SIGNAL_FUNC (dnd_data_begin), NULL);

}


/* ------------------ */

int isfile(char *s)
{
   struct stat st;

   if ((!s)||(!*s)) return 0;
   if (stat(s,&st)<0) return 0;
   if (S_ISREG(st.st_mode)) return 1;
   return 0;
}

int isdir(char *s)
{
   struct stat st;
   
   if ((!s)||(!*s)) return 0;
   if (stat(s,&st)<0) return 0;
   if (S_ISDIR(st.st_mode)) return 1;
   return 0;
}

char *filename_from_path(char *t)
{
        char *p;

        p = t + strlen(t);
        while(p > &t[0] && p[0] != '/') p--;
        p++;
        return p;
}

char *strip_one_file_layer(char *t)
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
gchar *correct_path_to_file(gchar *path1, gchar *path2, gchar *filename)
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


/* ------------------ */

static void about_cb()
{
	GtkWidget *about;
	const gchar *authors[2];
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
	GtkWidget *vbox;
	GtkWidget *frame;
	GtkWidget *button;
	GtkTooltips *tooltips;

	bindtextdomain(PACKAGE, GNOMELOCALEDIR);
	textdomain(PACKAGE);

	gnome_init ("GNOME menu editor", NULL, argc, argv, 0, NULL);

	SYSTEM_APPS = gnome_unconditional_datadir_file("apps");
	SYSTEM_PIXMAPS = gnome_unconditional_datadir_file("pixmaps");
	if (!g_file_exists(SYSTEM_APPS) || !g_file_exists(SYSTEM_PIXMAPS))
		{
		g_print("unable to retrieve GNOME installation directory\n");
		return 1;
		}

	USER_APPS = check_for_dir(gnome_util_home_file("apps"));
	USER_PIXMAPS = check_for_dir(gnome_util_home_file("pixmaps"));

	app = gnome_app_new ("gmenu",_("GNOME menu editor"));
	gtk_widget_set_usize (app, 600,460);
	gtk_signal_connect(GTK_OBJECT(app), "delete_event", GTK_SIGNAL_FUNC(destroy_cb), NULL);

	gnome_app_create_menus_with_data (GNOME_APP(app), main_menu, app);
	gtk_menu_item_right_justify(GTK_MENU_ITEM(main_menu[2].widget));

	gnome_app_create_toolbar (GNOME_APP(app), toolbar);
/*	gtk_toolbar_set_style (GTK_TOOLBAR (GNOME_APP(app)->toolbar), GTK_TOOLBAR_ICONS);
*/
	tooltips = gtk_tooltips_new();

	mainbox = gtk_hbox_new (FALSE, 0);
        gnome_app_set_contents(GNOME_APP(app),mainbox);
        gtk_widget_show (mainbox);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_border_width (GTK_CONTAINER (vbox), 5);
	gtk_box_pack_start(GTK_BOX(mainbox),vbox,TRUE,TRUE,0);
	gtk_widget_show(vbox);

	gtk_widget_push_visual (gdk_imlib_get_visual ());
	gtk_widget_push_colormap (gdk_imlib_get_colormap ());
	menu_tree_ctree = gtk_ctree_new(1, 0);
	gtk_widget_pop_visual ();
	gtk_widget_pop_colormap ();
	
	gtk_clist_set_row_height(GTK_CLIST(menu_tree_ctree),22);
	gtk_clist_set_column_width(GTK_CLIST(menu_tree_ctree),0,300);
	gtk_clist_set_selection_mode(GTK_CLIST(menu_tree_ctree),GTK_SELECTION_SINGLE);
	gtk_signal_connect_after(GTK_OBJECT(menu_tree_ctree),"button_release_event", GTK_SIGNAL_FUNC(tree_item_selected),NULL);
	gtk_box_pack_start(GTK_BOX(vbox),menu_tree_ctree,TRUE,TRUE,0);
	gtk_widget_show(menu_tree_ctree);

	/* tree info area */
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

	infolabel = gtk_label_new(_("GNOME menu editor"));
	gtk_container_add(GTK_CONTAINER(frame),infolabel);
	gtk_widget_show(infolabel);

	/* seperator */
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(mainbox),vbox,FALSE,FALSE,0);
	gtk_widget_show(vbox);

	button = gtk_vseparator_new ();
	gtk_box_pack_start(GTK_BOX(vbox),button,TRUE,TRUE,10);
	gtk_widget_show(button);

	/* edit area */
	vbox = create_edit_area();
	gtk_box_pack_start(GTK_BOX(mainbox),vbox,TRUE,TRUE,0);
	gtk_widget_show(vbox);

	add_main_tree_node();

	gtk_widget_show(app);
	
	new_edit_area();

	dnd_set_drag(menu_tree_ctree, GTK_CLIST(menu_tree_ctree)->clist_window);
	dnd_set_drop(menu_tree_ctree, GTK_CLIST(menu_tree_ctree)->clist_window);

	gtk_main();
	return 0;
}
