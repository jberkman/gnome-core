/*
 * This file is a part of gmenu, the GNOME panel menu editor.
 *
 * File: tree.c
 *
 * This file contains all the routines which handle manipulating the
 * GtkCTree used in gmenu, except some node creating/updating
 * routines, which are in save.c.
 *
 * Authors: John Ellis <johne@bellatlantic.net>
 *          Nat Friedman <nat@nat.org>
 */

#include <config.h>

#include "gmenu.h"
#include "top.xpm"

static void update_pbar(GtkWidget *pbar);
static gint find_file_cb(gconstpointer a, gconstpointer b);
static void move_item_down(GtkCTreeNode *node);
static void move_item_up(GtkCTreeNode *node);
static void add_tree_recurse_cb(GtkCTree *ctree, GtkCTreeNode *node, gpointer data);
static void get_ctree_count_cb(GtkCTree *ctree, GtkCTreeNode *node, gpointer data);
static gint get_ctree_count(GtkCTree *ctree);
static void edit_pressed_cb(GtkWidget *w, gpointer data);
static void update_tree_highlight(GtkWidget *w, GtkCTreeNode *old,
				  GtkCTreeNode *new, gint select);

void
tree_set_node(GtkCTreeNode *node)
{
  update_tree_highlight(menu_tree_ctree, current_node, node, TRUE);

  current_node = node;
  current_desktop = gtk_ctree_node_get_row_data(GTK_CTREE(menu_tree_ctree),
						node);
  update_edit_area(current_desktop);
}

void
tree_item_collapsed(GtkCTree *ctree, GtkCTreeNode *node)
{
  tree_set_node(node);
}

/* new = TRUE, increment feet = FALSE */
static void update_pbar(GtkWidget *pbar)
{
	gfloat val;
	if (!pbar) return;
	val = gtk_progress_get_value(GTK_PROGRESS(pbar));
	val += 1;
	if (val > 100) val = 0;
	gtk_progress_set_value(GTK_PROGRESS(pbar), val);
	gtk_widget_draw(pbar, NULL);
}

static gint find_file_cb(gconstpointer a, gconstpointer b)
{
	if (!((Desktop_Data *)(a))->path) return 1;

	return strcmp(((Desktop_Data *)(a))->path, (gchar *)b);
}

GtkCTreeNode *find_file_in_tree(GtkCTree * ctree, char *path)
{
	return gtk_ctree_find_by_row_data_custom (GTK_CTREE(ctree), NULL, path, find_file_cb);
}

/*
 * Select a new item in the menu tree.
 */
static void
update_tree_highlight(GtkWidget *w, GtkCTreeNode *old, GtkCTreeNode *new,
		      gint select)
{
	Desktop_Data *d;

	if (old) gtk_ctree_unselect(GTK_CTREE(w),old);
        if (new && select) gtk_ctree_select(GTK_CTREE(w),new);

	d = gtk_ctree_node_get_row_data(GTK_CTREE(w), new);
	gtk_label_set_text(GTK_LABEL(infolabel),d->comment);
	if (d->editable)
		gnome_stock_set_icon(GNOME_STOCK(infopixmap),
							GNOME_STOCK_MENU_BLANK );
	else
		gnome_stock_set_icon(GNOME_STOCK(infopixmap),
							GNOME_STOCK_MENU_BOOK_RED );
	if (current_path) g_free (current_path);
	if (d->isfolder)
		{
		current_path = strdup (d->path);
		}
	else
		{
		current_path = strip_one_file_layer(d->path);
		}
	gtk_label_set_text(GTK_LABEL(pathlabel),current_path);

}


static void move_item_down(GtkCTreeNode *node)
{
	GtkCTreeNode *parent = GTK_CTREE_ROW(node)->parent;
	GtkCTreeNode *sibling = GTK_CTREE_ROW(node)->sibling;
	GtkCTreeNode *next;
	int row;

	if (!sibling) return;

	gtk_ctree_move(GTK_CTREE(menu_tree_ctree), sibling, parent, node);
	save_order_of_dir(parent);

	if ((GList *)(((GList *)node)->next))
		next = GTK_CTREE_NODE((GList *)(((GList *)node)->next));
	else
		next = node;
	row = g_list_position((GList *)topnode, (GList *)next);
	if (!gtk_clist_row_is_visible(GTK_CLIST(menu_tree_ctree),row ))
		gtk_ctree_node_moveto (GTK_CTREE(menu_tree_ctree), next, 0, 1.0, 0.0);
}

static void move_item_up(GtkCTreeNode *node)
{
	GtkCTreeNode *parent = GTK_CTREE_ROW(node)->parent;
	GtkCTreeNode *sibling = GTK_CTREE_ROW(parent)->children;
	GtkCTreeNode *next;
	int row;

	if (sibling == node) return;

	while(GTK_CTREE_ROW(sibling)->sibling != node) sibling = GTK_CTREE_ROW(sibling)->sibling;

	gtk_ctree_move(GTK_CTREE(menu_tree_ctree), node, parent, sibling);
	save_order_of_dir(parent);

	if ((GList *)(((GList *)node)->prev))
		next = GTK_CTREE_NODE((GList *)(((GList *)node)->prev));
	else
		next = node;
	row = g_list_position((GList *)topnode, (GList *)next);
	if (!gtk_clist_row_is_visible(GTK_CLIST(menu_tree_ctree),row ))
		gtk_ctree_node_moveto (GTK_CTREE(menu_tree_ctree), next, 0, 0.0, 0.0);
}

void move_down_cb(GtkWidget *w, gpointer data)
{
	if (!is_node_moveable(current_node)) return;
	move_item_down(current_node);
}

void move_up_cb(GtkWidget *w, gpointer data)
{
	if (!is_node_moveable(current_node)) return;
	move_item_up(current_node);
}

int is_node_moveable(GtkCTreeNode *node)
{
	if (node == systemnode || node == usernode) return FALSE;
	if (!is_node_editable(node)) return FALSE;
	return is_node_editable(GTK_CTREE_ROW(node)->parent);
}

int is_node_editable(GtkCTreeNode *node)
{
	Desktop_Data *d;
	if (!node) return FALSE;

	d = gtk_ctree_node_get_row_data(GTK_CTREE(menu_tree_ctree), node);
	if (!d) return FALSE;
	return d->editable;
}


static void edit_pressed_cb(GtkWidget *w, gpointer data)
{
	Desktop_Data *d;

	if (!current_node || current_node == topnode || current_node == usernode ||
			current_node == systemnode) return;
	d = gtk_ctree_node_get_row_data(GTK_CTREE(menu_tree_ctree),current_node);
	update_edit_area(d);
}

void tree_item_selected (GtkCTree *ctree, GdkEventButton *event, gpointer data)
{
	gint row, col;
	Desktop_Data *d;
	GtkCTreeNode *node;

	if (event->window != GTK_CLIST(ctree)->clist_window) return;
	if (gtk_ctree_is_hot_spot(ctree, event->x, event->y)) return;
	if (event->button != 1 && event->button != 3) return;

	gtk_clist_get_selection_info (GTK_CLIST (ctree), event->x, event->y, &row, &col);

	node = GTK_CTREE_NODE(g_list_nth (GTK_CLIST (ctree)->row_list, row));

	if (!node) return;

	tree_set_node(node);

	if (node == topnode)
	  return;

	if (event->button == 3 && (node == systemnode || node == usernode)) return;

	d = gtk_ctree_node_get_row_data(GTK_CTREE(ctree),node);


	if (node == systemnode || node == usernode) return;

	if (event->button == 3)	update_edit_area(d);

	if (d->isfolder)
		{
		if (!d->expanded)
			{
			d->expanded = TRUE;
			add_tree_node(ctree, node, NULL);
			}
		}

	update_edit_area (d);

	return;
}


/* if node is null it is appended, if it is a sibling, it is inserted */
GtkCTreeNode *add_leaf_node(GtkCTree *ctree, GtkCTreeNode *parent, GtkCTreeNode *node, char *file)
{
	Desktop_Data *d;
	Desktop_Data *parent_data;
	char *path_buf;

	parent_data = gtk_ctree_node_get_row_data(GTK_CTREE(ctree), parent);

	path_buf = g_strconcat (parent_data->path, "/", file, NULL);
				
	if (!g_file_exists(path_buf))
		{
		g_free(path_buf);
		return node;
		}

	d = get_desktop_file_info (path_buf);
	if (d)
		{
		gchar *text[2];

		d->editable = is_desktop_file_editable(path_buf);

		text[0] = d->name;
		text[1] = NULL;
		if (d->isfolder)
		  {
			node = gtk_ctree_insert_node (GTK_CTREE(ctree), parent, node, text, 5,
				GNOME_PIXMAP(d->pixmap)->pixmap,
				GNOME_PIXMAP(d->pixmap)->mask,
				GNOME_PIXMAP(d->pixmap)->pixmap,
				GNOME_PIXMAP(d->pixmap)->mask,
				FALSE, FALSE);
		  }
		else
			node = gtk_ctree_insert_node (GTK_CTREE(ctree), parent, node, text, 5,
				GNOME_PIXMAP(d->pixmap)->pixmap,
				GNOME_PIXMAP(d->pixmap)->mask, NULL, NULL, TRUE, FALSE);
		gtk_ctree_node_set_row_data_full (GTK_CTREE(ctree), node, d,
						  remove_node_cb);
		}
	g_free(path_buf);
	return node;
}

void add_tree_node(GtkCTree *ctree, GtkCTreeNode *parent, GtkWidget *pbar)
{
	DIR *dp; 
	struct dirent *dir;

	GtkCTreeNode *node = NULL;
	GList *orderlist = NULL;
	Desktop_Data *parent_data;

	parent_data = gtk_ctree_node_get_row_data(GTK_CTREE(ctree), parent);
	parent_data->expanded = TRUE;

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
			if (pbar) update_pbar(pbar);
			}
		} 

	if (orderlist)
		{
		g_list_foreach(orderlist,(GFunc)g_free,NULL);
		g_list_free(orderlist);
		}

 
	closedir(dp);
}

static void add_tree_recurse_cb(GtkCTree *ctree, GtkCTreeNode *node, gpointer data)
{
	GtkWidget *pbar = data;
	Desktop_Data *d = gtk_ctree_node_get_row_data(GTK_CTREE(ctree), node);
	if (d->isfolder && !d->expanded )
		{
		add_tree_node(ctree, node, pbar);
		}
}

static void get_ctree_count_cb(GtkCTree *ctree, GtkCTreeNode *node, gpointer data)
{
	gint *p = data;
	*p = *p + 1;
}

static gint get_ctree_count(GtkCTree *ctree)
{
	gint count = 0;
	gtk_ctree_post_recursive (ctree, NULL, get_ctree_count_cb, &count);
	return count;
}

void add_main_tree_node(void)
{
	gchar *text[2];
	Desktop_Data *d;
	GtkWidget *pixmap;
	GtkWidget *progressbar;
	GtkWidget *dialog;
	GtkWidget *label;
	gint c;


	dialog = gnome_dialog_new(_("GNOME menu editor"),NULL);

	label = gtk_label_new(_("One moment, reading menus..."));
	gtk_box_pack_start(GTK_BOX(GNOME_DIALOG(dialog)->vbox), label, FALSE, FALSE, 5);
	gtk_widget_show(label);
	gtk_widget_realize(label);

	progressbar = gtk_progress_bar_new();
	gtk_progress_set_activity_mode(GTK_PROGRESS(progressbar), TRUE);
	gtk_box_pack_start(GTK_BOX(GNOME_DIALOG(dialog)->vbox), progressbar, FALSE, FALSE, 5);
	gtk_widget_show(progressbar);

	gtk_widget_show(dialog);

	while(gtk_events_pending()) gtk_main_iteration();

	gtk_clist_freeze(GTK_CLIST(menu_tree_ctree));

	/* very top of tree */
	topnode = NULL;
	text[0] = "GNOME";
	text[1] = NULL;
	
	pixmap = gnome_pixmap_new_from_xpm_d(top_xpm);
	topnode = gtk_ctree_insert_node (GTK_CTREE(menu_tree_ctree), NULL, NULL, text, 5,
		GNOME_PIXMAP(pixmap)->pixmap, GNOME_PIXMAP(pixmap)->mask,
		GNOME_PIXMAP(pixmap)->pixmap, GNOME_PIXMAP(pixmap)->mask,
		FALSE, TRUE);

	d = g_new0(Desktop_Data, 1);

	d->comment = strdup(_("GNOME"));
	d->expanded = TRUE;
	d->isfolder = TRUE;
	d->editable = FALSE;

	gtk_ctree_node_set_row_data_full (GTK_CTREE(menu_tree_ctree), topnode,
					  d, remove_node_cb);

	/* system's menu tree */
	text[0] = _("System Menus");
	pixmap = gnome_pixmap_new_from_xpm_d(top_xpm);
	systemnode = gtk_ctree_insert_node (GTK_CTREE(menu_tree_ctree), topnode, NULL, text, 5,
		GNOME_PIXMAP(pixmap)->pixmap, GNOME_PIXMAP(pixmap)->mask,
		GNOME_PIXMAP(pixmap)->pixmap, GNOME_PIXMAP(pixmap)->mask,
		FALSE, TRUE);

	d = get_desktop_file_info (SYSTEM_APPS);

	if (d->comment) free(d->comment);
	d->comment = strdup(_("Top of system menus"));
	d->expanded = FALSE;
	d->editable = is_desktop_file_editable(SYSTEM_APPS);

	gtk_ctree_node_set_row_data_full (GTK_CTREE(menu_tree_ctree),
					  systemnode, d, remove_node_cb);

	/* user's menu tree */
	text[0] = _("User Menus");
	pixmap = gnome_pixmap_new_from_xpm_d(top_xpm);
	usernode = gtk_ctree_insert_node (GTK_CTREE(menu_tree_ctree), topnode, systemnode, text, 5,
		GNOME_PIXMAP(pixmap)->pixmap, GNOME_PIXMAP(pixmap)->mask,
		GNOME_PIXMAP(pixmap)->pixmap, GNOME_PIXMAP(pixmap)->mask,
		FALSE, TRUE);

	d = get_desktop_file_info (USER_APPS);

	if (d->comment) free(d->comment);
	d->comment = strdup(_("Top of user menus"));
	d->expanded = FALSE;
	d->editable = TRUE;

	gtk_ctree_node_set_row_data_full (GTK_CTREE(menu_tree_ctree),
					  usernode, d, remove_node_cb);

	/* now load the entire menu tree */
	c = 0;
	while (get_ctree_count(GTK_CTREE(menu_tree_ctree)) > c)
		{
		c = get_ctree_count(GTK_CTREE(menu_tree_ctree));

		gtk_ctree_post_recursive(GTK_CTREE(menu_tree_ctree), topnode,
			add_tree_recurse_cb, progressbar);
		}


	gnome_dialog_close(GNOME_DIALOG(dialog));

	gtk_clist_thaw(GTK_CLIST(menu_tree_ctree));

	tree_set_node(usernode);
}

void tree_sort_node(GtkCTreeNode *node)
{
	Desktop_Data *d;

	if (!node || node == topnode) return;

	d = gtk_ctree_node_get_row_data(GTK_CTREE(menu_tree_ctree),node);
	if (!d->isfolder) node = GTK_CTREE_ROW(node)->parent;

	gtk_ctree_sort_node(GTK_CTREE(menu_tree_ctree), node);
	save_order_of_dir(node);
}
