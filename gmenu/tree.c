/*###################################################################*/
/*##                       gmenu (GNOME menu editor) 0.2.5         ##*/
/*###################################################################*/

#include <config.h>

#include "gmenu.h"
#include "top.xpm"

static void move_item_down(GtkCTreeNode *node);
static void move_item_up(GtkCTreeNode *node);
static void add_tree_recurse_cb(GtkCTree *ctree, GtkCTreeNode *node, gpointer data);

/* if ctree = null, reset ret. if node = null, return ret. */
static GtkCTreeNode *check_file_match(GtkCTree *ctree, GtkCTreeNode *node, gpointer data)
{
	static GtkCTreeNode *ret = NULL;
	char *path = data;
	Desktop_Data *d;

	if (!ctree)
		{
		ret = NULL;
		return ret;
		}

	if (!node)
		{
		return ret;
		}

	d = gtk_ctree_get_row_data(GTK_CTREE(ctree), node);
	if (d->path && !strcmp(d->path, path))
		{
		ret = node;
		return ret;
		}
	else
		return NULL;
}

GtkCTreeNode *find_file_in_tree(GtkCTree * ctree, char *path)
{
	GtkCTreeNode *node = NULL;

	/* reset the static pointer */
	check_file_match(NULL, NULL, NULL);

	/* do the check */
	gtk_ctree_pre_recursive(GTK_CTREE(ctree), NULL, (GtkCTreeFunc) check_file_match, path);

	/* get the static pointer */
	node = check_file_match(ctree, NULL, NULL);

	return node;
}

void update_tree_highlight(GtkWidget *w, GtkCTreeNode *old, GtkCTreeNode *new, gint select)
{
	Desktop_Data *d;

        if (old) gtk_ctree_unselect(GTK_CTREE(w),old);
        if (new && select) gtk_ctree_select(GTK_CTREE(w),new);

	d = gtk_ctree_get_row_data(GTK_CTREE(w), new);
	gtk_label_set(GTK_LABEL(infolabel),d->comment);
	if (d->editable)
		gnome_stock_pixmap_widget_set_icon(GNOME_STOCK_PIXMAP_WIDGET(infopixmap),
							GNOME_STOCK_MENU_BLANK );
	else
		gnome_stock_pixmap_widget_set_icon(GNOME_STOCK_PIXMAP_WIDGET(infopixmap),
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
	gtk_label_set(GTK_LABEL(pathlabel),current_path);

/*	if (move)
		gtk_ctree_moveto (GTK_CTREE(w), new, 0, 0.5, 0.0);
*/
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
		gtk_ctree_moveto (GTK_CTREE(menu_tree_ctree), next, 0, 1.0, 0.0);
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
		gtk_ctree_moveto (GTK_CTREE(menu_tree_ctree), next, 0, 0.0, 0.0);
}

void move_down_cb(GtkWidget *w, gpointer data)
{
	if (!is_node_editable(current_node)) return;
	if (current_node == systemnode || current_node == usernode) return;
	move_item_down(current_node);
}

void move_up_cb(GtkWidget *w, gpointer data)
{
	if (!is_node_editable(current_node)) return;
	if (current_node == systemnode || current_node == usernode) return;
	move_item_up(current_node);
}

int is_node_editable(GtkCTreeNode *node)
{
	Desktop_Data *d;
	gboolean leaf;
	GtkCTreeNode *parent;

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

void edit_pressed_cb()
{
	Desktop_Data *d;

	if (!current_node || current_node == topnode || current_node == usernode ||
			current_node == systemnode) return;
	d = gtk_ctree_get_row_data(GTK_CTREE(menu_tree_ctree),current_node);
	update_edit_area(d);
}

void tree_item_selected (GtkWidget *widget, gint row, gint column, GdkEventButton *bevent)
{
	Desktop_Data *d;
	GtkCTree *ctree = GTK_CTREE(widget);
	GtkCTreeNode *node;

	node = GTK_CTREE_NODE(g_list_nth (GTK_CLIST (ctree)->row_list, row));

	if (!node) return;

	if (node == topnode) return;

	d = gtk_ctree_get_row_data(GTK_CTREE(ctree),node);

	update_tree_highlight(menu_tree_ctree, current_node, node, FALSE);

	current_node = node;

	if (node == systemnode || node == usernode) return;

	if ( bevent->type == GDK_2BUTTON_PRESS || bevent->button == 2 )
		{
		update_edit_area(d);
		}

	if (d->isfolder)
		{
		if (!d->expanded)
			{
			d->expanded = TRUE;
			add_tree_node(ctree, node);
			}
		}

}

/* if node is null it is appended, if it is a sibling, it is inserted */
GtkCTreeNode *add_leaf_node(GtkCTree *ctree, GtkCTreeNode *parent, GtkCTreeNode *node, char *file)
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

void add_tree_node(GtkCTree *ctree, GtkCTreeNode *parent)
{
	DIR *dp; 
	struct dirent *dir;

	GtkCTreeNode *node = NULL;
	GList *orderlist = NULL;
	Desktop_Data *parent_data;

	parent_data = gtk_ctree_get_row_data(GTK_CTREE(ctree), parent);
	parent_data->expanded = TRUE;

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

static void add_tree_recurse_cb(GtkCTree *ctree, GtkCTreeNode *node, gpointer data)
{
	Desktop_Data *d = gtk_ctree_get_row_data(GTK_CTREE(ctree), node);
	if (d->isfolder && !d->expanded ) add_tree_node(ctree, node);
}


void add_main_tree_node()
{
	gchar *text[2];
	Desktop_Data *d;
	GtkWidget *pixmap;
	gint c;

/*	g_print("adding top node...\n");*/

	gtk_clist_freeze(GTK_CLIST(menu_tree_ctree));

	/* very top of tree */
	topnode = NULL;
	text[0] = "GNOME";
	text[1] = NULL;
	
	pixmap = gnome_pixmap_new_from_xpm_d(top_xpm);
	topnode = gtk_ctree_insert (GTK_CTREE(menu_tree_ctree), NULL, NULL, text, 5,
		NULL, NULL,
		GNOME_PIXMAP(pixmap)->pixmap, GNOME_PIXMAP(pixmap)->mask,
		FALSE, TRUE);

	d = g_new0(Desktop_Data, 1);

	d->comment = strdup(_("GNOME"));
	d->expanded = TRUE;
	d->isfolder = TRUE;
	d->editable = FALSE;

	gtk_ctree_set_row_data (GTK_CTREE(menu_tree_ctree), topnode, d);

	/* system's menu tree */
	text[0] = _("System Menus");
	pixmap = gnome_pixmap_new_from_xpm_d(top_xpm);
	systemnode = gtk_ctree_insert (GTK_CTREE(menu_tree_ctree), topnode, NULL, text, 5,
		NULL, NULL,
		GNOME_PIXMAP(pixmap)->pixmap, GNOME_PIXMAP(pixmap)->mask,
		FALSE, TRUE);

	d = get_desktop_file_info (SYSTEM_APPS);

	if (d->comment) free(d->comment);
	d->comment = strdup(_("Top of system menus"));
	d->expanded = FALSE;

	/* FIXME: are we root? then we can edit the system menu */
/*
	if (!strcmp("/root",getenv("HOME")) || 
	    ((getenv("USER"))&&(!strcmp("root",getenv("USER")))) ||
		((getenv("USERNAME"))&&(!strcmp("root",getenv("USERNAME")))) )
*/
	if (!access (SYSTEM_APPS, W_OK))
		{
		g_print(_("Running with root privileges.\n"));
		d->editable = TRUE;
		}
	else
		d->editable = FALSE;

	gtk_ctree_set_row_data (GTK_CTREE(menu_tree_ctree), systemnode, d);

	/* user's menu tree */
	text[0] = _("User Menus");
	pixmap = gnome_pixmap_new_from_xpm_d(top_xpm);
	usernode = gtk_ctree_insert (GTK_CTREE(menu_tree_ctree), topnode, systemnode, text, 5,
		NULL, NULL,
		GNOME_PIXMAP(pixmap)->pixmap, GNOME_PIXMAP(pixmap)->mask,
		FALSE, TRUE);

	d = get_desktop_file_info (USER_APPS);

	if (d->comment) free(d->comment);
	d->comment = strdup(_("Top of user menus"));
	d->expanded = FALSE;
	d->editable = TRUE;

	gtk_ctree_set_row_data (GTK_CTREE(menu_tree_ctree), usernode, d);

	/* now load the entire menu tree */
	c = 0;
	while (g_list_length((GList *)topnode) > c)
		{
		c = g_list_length((GList *)topnode);
		gtk_ctree_pre_recursive(GTK_CTREE(menu_tree_ctree), topnode, add_tree_recurse_cb, NULL);
		}

	current_node = usernode;

	gtk_clist_thaw(GTK_CLIST(menu_tree_ctree));

	update_tree_highlight(menu_tree_ctree, NULL, current_node, FALSE);
}

