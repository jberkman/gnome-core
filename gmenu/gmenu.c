/*###################################################################*/
/*##                       gmenu (GNOME menu editor)               ##*/
/*###################################################################*/

#include <config.h>
#include "gmenu.h"

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
static GtkCTreeNode *drop_data = NULL;

static void drag_data_get_cb (GtkWidget *widget, GdkDragContext *context,
			GtkSelectionData *selection_data, guint info,
			guint time, gpointer data);
static void menu_drag_data_received (GtkWidget *widget,
	    GdkDragContext *context, gint x, gint y, GtkSelectionData *data,
	    guint info, guint time, gpointer user_data);
static void possible_drag_item_pressed (GtkCTree *ctree, GdkEventButton *event, gpointer data);

static void sort_node( GtkCTreeNode *node);
static void sort_single_pressed(GtkWidget *w, gpointer data);
static void sort_recurse_cb(GtkCTree *ctree, GtkCTreeNode *node,
			    gpointer data);
static void sort_recursive_pressed(GtkWidget *w, gpointer data);

static gchar *check_for_dir(char *d);

static void about_cb(GtkWidget *w, gpointer data);
static void destroy_cb(GtkWidget *w, gpointer data);

static void new_item_cb(GtkWidget *w, gpointer data);

int main (int argc, char *argv[]);

/*
 * The table of data types which can be dragged
 */
enum {
	DRAG_TARGET_MENU_ITEM,
	DRAG_TARGET_URI
};

static GtkTargetEntry drag_target_table[] = {
	{ "gmenu/x-menu-item-same-address-space", 0, DRAG_TARGET_MENU_ITEM },
	{ "test/uri-list", 0,  DRAG_TARGET_URI }
};

static int drag_target_table_length = sizeof(drag_target_table) /
                                      sizeof(drag_target_table[0]);
/*
 * The table of data types which can be dropped on folders.
 */
enum {
	DROP_TARGET_MENU_ITEM
};

static GtkTargetEntry drop_target_table[] = {
	{ "gmenu/x-menu-item-same-address-space", 0,  DROP_TARGET_MENU_ITEM }
};

static int drop_target_table_length = sizeof(drop_target_table) /
                                      sizeof(drop_target_table[0]);


/* menu bar */
GnomeUIInfo file_menu[] = {
        GNOMEUIINFO_MENU_NEW_ITEM(N_("_New Folder..."),
				  N_("Create a new folder"),
				  create_folder_pressed, NULL),
	
	{ GNOME_APP_UI_ITEM, N_("_Delete..."), NULL, delete_pressed_cb, NULL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_CUT, 'D',
	  GDK_CONTROL_MASK, NULL },

	GNOMEUIINFO_SEPARATOR,

	GNOMEUIINFO_MENU_EXIT_ITEM(destroy_cb, NULL),

	GNOMEUIINFO_END
};
GnomeUIInfo sort_menu[] = {
	{ GNOME_APP_UI_ITEM, N_("_Sort Folder"), NULL, sort_single_pressed, NULL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_SPELLCHECK, 'S',
	  GDK_CONTROL_MASK, NULL },
	{ GNOME_APP_UI_ITEM, N_("Sort Folder _Recursive"), NULL, sort_recursive_pressed, NULL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_SPELLCHECK, 'R',
	  GDK_CONTROL_MASK, NULL },
	{ GNOME_APP_UI_ENDOFINFO }
};
GnomeUIInfo help_menu[] = {
/*	{ GNOME_APP_UI_HELP, NULL, NULL, NULL, NULL, NULL,
	  GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },*/
        GNOMEUIINFO_MENU_ABOUT_ITEM(about_cb, NULL), 
	{ GNOME_APP_UI_ENDOFINFO }
};
GnomeUIInfo main_menu[] = {
	{ GNOME_APP_UI_SUBTREE, N_("_File"), NULL, file_menu, NULL, NULL,
	  GNOME_APP_PIXMAP_NONE, NULL, 'F', GDK_MODIFIER_MASK, NULL },
	{ GNOME_APP_UI_SUBTREE, N_("_Sort"), NULL, sort_menu, NULL, NULL,
	  GNOME_APP_PIXMAP_NONE, NULL, 'S', GDK_MODIFIER_MASK, NULL },
	{ GNOME_APP_UI_SUBTREE, N_("_Help"), NULL, help_menu, NULL, NULL,
	  GNOME_APP_PIXMAP_NONE, NULL, 'H', GDK_MODIFIER_MASK, NULL },
	{ GNOME_APP_UI_ENDOFINFO }
};

/* toolbar */
GnomeUIInfo toolbar[] = {
	{ GNOME_APP_UI_ITEM, N_("New Folder"), N_("Create a new folder"), create_folder_pressed, NULL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_NEW, 0, 0, NULL },

	{ GNOME_APP_UI_ITEM, N_("New Item"), N_("Create a new item"),
	  new_item_cb, NULL, NULL, GNOME_APP_PIXMAP_STOCK,
	  GNOME_STOCK_PIXMAP_NEW, 0, 0, NULL },

	{ GNOME_APP_UI_ITEM, N_("Delete"), N_("Delete selected menu item"), delete_pressed_cb, NULL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_CUT, 0, 0, NULL },
	GNOMEUIINFO_SEPARATOR,
	{ GNOME_APP_UI_ITEM, N_("Move up"), N_("Move selected menu up"), move_up_cb, NULL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_UP, 0, 0, NULL },
	{ GNOME_APP_UI_ITEM, N_("Move down"), N_("Move selected menu down"), move_down_cb, NULL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_DOWN, 0, 0, NULL },
	GNOMEUIINFO_SEPARATOR,

#if 0
	{ GNOME_APP_UI_ITEM, N_("Properties"), N_("Edit selected menu item properties"), edit_pressed_cb, NULL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_PROPERTIES, 0, 0, NULL },
	GNOMEUIINFO_SEPARATOR,
#endif

	{ GNOME_APP_UI_ITEM, N_("Sort Folder"), N_("Sort selected folder"), sort_single_pressed, NULL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_SPELLCHECK, 0, 0, NULL },
	GNOMEUIINFO_END
};

/*
 * This is the function which gets called when a menu item has been dropped
 * on a folder.
 */
static void
menu_drag_data_received (GtkWidget          *widget,
		    GdkDragContext     *context,
		    gint                x,
		    gint                y,
		    GtkSelectionData   *data,
		    guint               info,
		    guint               time,
		    gpointer            user_data)
{
  GtkCTree *ctree = GTK_CTREE(widget);
  GtkCTreeNode *src, *dest;
  Desktop_Data *src_desktop, *dest_desktop;

  gint row, col;

  if (data->length <= 0)
    return;

  /*
   * Figure out which node is the drop destination.
   */
  gtk_clist_get_selection_info (GTK_CLIST (ctree), x, y, &row, &col);
  dest = GTK_CTREE_NODE( g_list_nth(GTK_CLIST (ctree)->row_list, row));
  if (dest == NULL || dest == topnode)
    return;

  /*
   * Figure out which node is the node being dropped.
   */
  src = *(GtkCTreeNode **) data->data;
  if (src == NULL)
    return;

  if (src == dest)
    return;

  dest_desktop = gtk_ctree_node_get_row_data( GTK_CTREE(ctree), dest);
  src_desktop = gtk_ctree_node_get_row_data( GTK_CTREE(ctree), src);

  if (! (dest_desktop->editable && src_desktop->editable))
    return;

  /* FIXME context_>suggested-action */
  
  switch (info) {
  case DROP_TARGET_MENU_ITEM:
    printf("Menu item dropped on folder!\n");
    printf ("Item: %s\t Folder: %s\n", src_desktop->name,
	    dest_desktop->name);
    if (dest_desktop->isfolder)
      {
	/*
	 * Drop the menu item into the folder.
	 */
	tree_moved(ctree, src, dest, NULL,
		   (gpointer) 1);
	gtk_ctree_move(ctree, src, dest, NULL);
      }
    else
      {
	/*
	 * Move the menu item next to the drop
	 * site.
	 */
	if (GTK_CTREE_ROW(src)->sibling == dest)
	  {
	    tree_moved(ctree, dest,
		       GTK_CTREE_ROW(src)->parent, src,
		       (gpointer) 1);
	    gtk_ctree_move(ctree, dest,
			   GTK_CTREE_ROW(src)->parent,
			   src);
	  }
	else
	  {
	    tree_moved(ctree, src,
		       GTK_CTREE_ROW(dest)->parent, dest,
		       (gpointer) 1);
	    gtk_ctree_move(ctree, src,
			   GTK_CTREE_ROW(dest)->parent,
			   dest);
	  }

			}
		break;
	default:
		printf("unknown drop type\n");
		break;
	}

	gtk_drag_finish (context, FALSE, FALSE, time);
}

/*
 * This is the function that gets called when one of our menu items is
 * dropped somewhere and we need to provide the data to be dropped.
 */
static void
drag_data_get_cb (GtkWidget *widget, GdkDragContext *context,
		  GtkSelectionData *selection_data, guint info,
		  guint time, gpointer data)
{
	GtkCTree *ctree = GTK_CTREE(widget);
	Desktop_Data *d;
	gchar *uri_list;

	/* drop_data is a global variable.  FIXME */
	if (drop_data == NULL)
		return;

	d = gtk_ctree_node_get_row_data( GTK_CTREE(ctree), drop_data );

	switch (info)
		{
		case DRAG_TARGET_URI:
			uri_list = g_strconcat("file:", d->path,
					       NULL);
			gtk_selection_data_set (selection_data,
						selection_data->target, 8,
						uri_list, strlen(uri_list));
			g_free(uri_list);

			break;

		case DRAG_TARGET_MENU_ITEM:
			gtk_selection_data_set (selection_data,
						selection_data->target, 8,
						(guchar *)&drop_data,
						sizeof(drop_data));
			break;

		}
}

static void
possible_drag_item_pressed (GtkCTree *ctree, GdkEventButton *event,
			    gpointer data)
{
	gint row, col;
	GtkCTreeNode *node;

	if (event->window != GTK_CLIST(ctree)->clist_window) return;
	if (event->button != 1) return;

	gtk_clist_get_selection_info (GTK_CLIST (ctree), event->x, event->y, &row, &col);

	node = GTK_CTREE_NODE(g_list_nth (GTK_CLIST (ctree)->row_list, row));

	if (!node || node == topnode)
		{
		drop_data = NULL;
		return;
		}

	drop_data = node;
}

static void sort_node( GtkCTreeNode *node)
{
	Desktop_Data *d;

	if (!node || node == topnode) return;

	d = gtk_ctree_node_get_row_data(GTK_CTREE(menu_tree_ctree),node);
	if (!d->isfolder) node = GTK_CTREE_ROW(node)->parent;

	gtk_ctree_sort_node(GTK_CTREE(menu_tree_ctree), node);
	save_order_of_dir(node);
}

static void sort_single_pressed(GtkWidget *w, gpointer data)
{
	sort_node(current_node);
}


static void sort_recurse_cb(GtkCTree *ctree, GtkCTreeNode *node, gpointer data)
{
	Desktop_Data *d;

	if (!node) return;

	d = gtk_ctree_node_get_row_data(GTK_CTREE(menu_tree_ctree),node);
	if (d->isfolder) sort_node (node);
}

static void sort_recursive_pressed(GtkWidget *w, gpointer data)
{
	Desktop_Data *d;
	GtkCTreeNode *node = current_node;

	if (!node || node == topnode) return;

	d = gtk_ctree_node_get_row_data(GTK_CTREE(menu_tree_ctree),node);
	if (!d->isfolder) node = GTK_CTREE_ROW(node)->parent;

	gtk_ctree_post_recursive(GTK_CTREE(menu_tree_ctree), node, sort_recurse_cb, NULL);
}

gint isfile(char *s)
{
   struct stat st;

   if ((!s)||(!*s)) return 0;
   if (stat(s,&st)<0) return 0;
   if (S_ISREG(st.st_mode)) return 1;
   return 0;
}

gint isdir(char *s)
{
   struct stat st;
   
   if ((!s)||(!*s)) return 0;
   if (stat(s,&st)<0) return 0;
   if (S_ISDIR(st.st_mode)) return 1;
   return 0;
}

gchar *filename_from_path(char *t)
{
        gchar *p;

        p = t + strlen(t);
        while(p > &t[0] && p[0] != '/') p--;
        p++;
        return p;
}

gchar *strip_one_file_layer(char *t)
{
	gchar *ret;
	gchar *p;

	ret = strdup(t);
	p = ret + strlen(ret);
        while(p > &ret[0] && p[0] != '/') p--;
	if (strcmp(ret,p) != 0) p[0] = '\0';
        return ret;
}

static gchar *check_for_dir(char *d)
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

	correct_path = g_strconcat(path1, "/", filename, NULL);
	if (isfile(correct_path)) return correct_path;
	g_free(correct_path);

	correct_path = g_strconcat(path2, "/", filename, NULL);
	if (isfile(correct_path)) return correct_path;
	g_free(correct_path);

	correct_path = NULL;
	return correct_path;
}

static void
new_item_cb(GtkWidget *w, gpointer data)
{
  new_edit_area();
  save_dialog_dentry();
}


static void about_cb(GtkWidget *w, gpointer data)
{
	GtkWidget *about;
	const gchar *authors[2];
	gchar version[32];

	sprintf(version,"%d.%d.%d",GMENU_VERSION_MAJOR, GMENU_VERSION_MINOR, GMENU_VERSION_REV);

	authors[0] = "John Ellis <johne@bellatlantic.net>";
	authors[1] = NULL;

	about = gnome_about_new ( _("GNOME menu editor"), version,
			"(C) 1998",
			authors,
			_("Released under the terms of the GNU Public License.\n"
			"GNOME menu editor."),
			NULL);
	gtk_widget_show (about);
}

static void destroy_cb(GtkWidget *w, gpointer data)
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
	GtkWidget *scrolled;
	GtkTooltips *tooltips;


	bindtextdomain(PACKAGE, GNOMELOCALEDIR);
	textdomain(PACKAGE);

	gnome_init ("GNOME menu editor", VERSION, argc, argv);

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
	gtk_widget_set_usize (app, 600, 460);
	gtk_signal_connect(GTK_OBJECT(app), "delete_event", GTK_SIGNAL_FUNC(destroy_cb), NULL);

	gnome_app_create_menus_with_data (GNOME_APP(app), main_menu, app);
/*	gtk_menu_item_right_justify(GTK_MENU_ITEM(main_menu[2].widget));
*/
	gnome_app_create_toolbar (GNOME_APP(app), toolbar);
/*	gtk_toolbar_set_style (GTK_TOOLBAR (GNOME_APP(app)->toolbar), GTK_TOOLBAR_ICONS);
*/
	tooltips = gtk_tooltips_new();

	mainbox = gtk_hbox_new (FALSE, 0);
        gnome_app_set_contents(GNOME_APP(app),mainbox);
        gtk_widget_show (mainbox);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);
	gtk_box_pack_start(GTK_BOX(mainbox),vbox,TRUE,TRUE,0);
	gtk_widget_show(vbox);

	scrolled = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
				GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
	gtk_box_pack_start(GTK_BOX(vbox),scrolled,TRUE,TRUE,0);
	gtk_widget_show(scrolled);

	gtk_widget_push_visual (gdk_imlib_get_visual ());
	gtk_widget_push_colormap (gdk_imlib_get_colormap ());
	menu_tree_ctree = gtk_ctree_new(1, 0);
	gtk_widget_pop_visual ();
	gtk_widget_pop_colormap ();
	
	gtk_clist_set_row_height(GTK_CLIST(menu_tree_ctree),22);
	gtk_clist_set_column_width(GTK_CLIST(menu_tree_ctree),0,300);
	gtk_clist_set_selection_mode(GTK_CLIST(menu_tree_ctree),GTK_SELECTION_SINGLE);
	gtk_clist_set_reorderable (GTK_CLIST(menu_tree_ctree), TRUE);
	gtk_ctree_set_drag_compare_func(GTK_CTREE(menu_tree_ctree), tree_move_test_cb);
	gtk_signal_connect_after(GTK_OBJECT(menu_tree_ctree),"button_release_event", GTK_SIGNAL_FUNC(tree_item_selected),NULL);
	gtk_signal_connect(GTK_OBJECT(menu_tree_ctree),"tree_move", GTK_SIGNAL_FUNC(tree_moved),"before");
	gtk_signal_connect_after(GTK_OBJECT(menu_tree_ctree),"tree_move", GTK_SIGNAL_FUNC(tree_moved),NULL);

	/*
	 * Initialize drag and drop
	 */

	/* Dragging ... */
	gtk_signal_connect(GTK_OBJECT(menu_tree_ctree),"button_press_event",
			   GTK_SIGNAL_FUNC(possible_drag_item_pressed),NULL);
	gtk_drag_source_set(menu_tree_ctree, GDK_BUTTON1_MASK,
			    drag_target_table, drag_target_table_length,
			    GDK_ACTION_COPY);
	gtk_signal_connect(GTK_OBJECT(menu_tree_ctree), "drag_data_get",
			   drag_data_get_cb, NULL);

	/* Dropping ... */
	gtk_signal_connect (GTK_OBJECT (menu_tree_ctree),
			    "drag_data_received",
			    GTK_SIGNAL_FUNC (menu_drag_data_received), NULL);

	gtk_drag_dest_set (GTK_WIDGET (menu_tree_ctree),
			   GTK_DEST_DEFAULT_ALL,
			   drop_target_table,
			   drop_target_table_length,
			   GDK_ACTION_MOVE | GDK_ACTION_COPY);

	gtk_container_add (GTK_CONTAINER (scrolled), menu_tree_ctree);
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

	gtk_main();
	return 0;
}
