/*###################################################################*/
/*##                       gmenu (GNOME menu editor) 0.3.1         ##*/
/*###################################################################*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

#include <gnome.h>

	/* definitions */
#define GMENU_VERSION_MAJOR 0
#define GMENU_VERSION_MINOR 3
#define GMENU_VERSION_REV 1

typedef struct _Desktop_Data Desktop_Data;
struct _Desktop_Data
{
	gchar *path;
	gchar *name;
	gchar *comment;
	GnomeDesktopEntry *dentry;
	GtkWidget *pixmap;
	gint isfolder;
	gint expanded;
	gint editable;
};

typedef struct _Misc_Dialog Misc_Dialog;
struct _Misc_Dialog
{
	GtkWidget *dialog;
	GtkWidget *entry;
};

extern gchar *SYSTEM_APPS;
extern gchar *SYSTEM_PIXMAPS;
extern gchar *USER_APPS;
extern gchar *USER_PIXMAPS;

extern GtkWidget *app;
extern GtkWidget *menu_tree_ctree;
extern GtkWidget *infolabel;
extern GtkWidget *infopixmap;
extern GtkWidget *pathlabel;

extern GtkObject *edit_area;

extern GtkCTreeNode *topnode;
extern GtkCTreeNode *usernode;
extern GtkCTreeNode *systemnode;
extern GtkCTreeNode *current_node;
extern gchar *current_path;

extern Desktop_Data *edit_area_orig_data;

/* gmenu.c --------------- */

gint isfile(char *s);
gint isdir(char *s);
gchar *filename_from_path(char *t);
gchar *strip_one_file_layer(char *t);
gchar *correct_path_to_file(gchar *path1, gchar *path2, gchar *filename);

/* tree.c ---------------- */

GtkCTreeNode *find_file_in_tree(GtkCTree * ctree, char *path);
void update_tree_highlight(GtkWidget *w, GtkCTreeNode *old, GtkCTreeNode *new, gint select);
void tree_moved(GtkCTree *ctree, GtkCTreeNode *node, GtkCTreeNode *new_parent,
			GtkCTreeNode *new_sibling, gpointer data);
gboolean tree_move_test_cb(GtkCTree *ctree, GtkCTreeNode *source_node,
			GtkCTreeNode *new_parent, GtkCTreeNode *new_sibling);
void move_down_cb(GtkWidget *w, gpointer data);
void move_up_cb(GtkWidget *w, gpointer data);
gint is_node_editable(GtkCTreeNode *node);
void edit_pressed_cb();
void tree_item_selected (GtkCTree *ctree, GdkEventButton *event, gpointer data);
GtkCTreeNode *add_leaf_node(GtkCTree *ctree, GtkCTreeNode *parent, GtkCTreeNode *node, char *file);
void add_tree_node(GtkCTree *ctree, GtkCTreeNode *parent, GtkWidget *pbar);
void add_main_tree_node();

/* edit.c ---------------- */

void edit_area_reset_revert(Desktop_Data *d);
gchar * edit_area_get_filename();
void update_edit_area(Desktop_Data *d);
void revert_edit_area();
void new_edit_area();
GtkWidget * create_edit_area();

/* order.c --------------- */

GList *get_order_of_dir(gchar *dir);
void save_order_of_dir(GtkCTreeNode *node);
void free_desktop_data(Desktop_Data *d);
Desktop_Data * get_desktop_file_info (gchar *file);

/* dialogs.c ------------- */

void create_folder_pressed();
void delete_pressed_cb();
void save_pressed_cb();


