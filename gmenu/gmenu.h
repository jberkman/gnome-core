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
#include <errno.h>

#include <gnome.h>

	/* definitions */
#define GMENU_VERSION_MAJOR 0
#define GMENU_VERSION_MINOR 3
#define GMENU_VERSION_REV 2

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
extern Desktop_Data *current_desktop;
extern gchar *current_path;

extern Desktop_Data *edit_area_orig_data;

/* gmenu.c --------------- */

void folder_update_child_paths(GtkCTreeNode *parent, gchar *path);

/* tree.c ---------------- */

void tree_sort_node(GtkCTreeNode *node);
GtkCTreeNode *find_file_in_tree(GtkCTree * ctree, char *path);
void move_down_cb(GtkWidget *w, gpointer data);
void move_up_cb(GtkWidget *w, gpointer data);
void tree_item_selected (GtkCTree *ctree, GdkEventButton *event,
			 gpointer data);
gint tree_row_selected (GtkCTree *ctree, GtkCTreeNode *node, gint column);
GtkCTreeNode *add_leaf_node(GtkCTree *ctree, GtkCTreeNode *parent, GtkCTreeNode *node, char *file);
void add_tree_node(GtkCTree *ctree, GtkCTreeNode *parent, GtkWidget *pbar);
void add_main_tree_node(void);

void tree_set_node(GtkCTreeNode *node);
void tree_item_collapsed(GtkCTree *ctree, GtkCTreeNode *node);
int is_node_editable(GtkCTreeNode *node);
int is_node_moveable(GtkCTreeNode *node);

/* edit.c ---------------- */


void edit_area_select_name(void);
void disable_edit_area(void);
void edit_area_reset_revert(Desktop_Data *d);
gchar * edit_area_get_filename(void);
void update_edit_area(Desktop_Data *d);
void revert_edit_area(void);
void new_edit_area(void);
GtkWidget * create_edit_area(void);

/* order.c --------------- */

GList *get_order_of_dir(gchar *dir);
void save_order_of_dir(GtkCTreeNode *node);
void free_desktop_data(Desktop_Data *d);
Desktop_Data * get_desktop_file_info (gchar *file);

/* dialogs.c ------------- */

void remove_node_cb(gpointer data);
void save_dialog_dentry(void);
void create_folder_pressed_cb(GtkWidget *w, gpointer data);
void new_item_pressed_cb(GtkWidget *w, gpointer data);
void delete_pressed_cb(GtkWidget *w, gpointer data);
void save_pressed_cb(GtkWidget *w, gpointer data);
gint create_folder(gchar *full_path);


/* delete.c -------------- */

gboolean delete_desktop_entry_node(Desktop_Data *d);
gboolean delete_desktop_entry_file(gchar *path);
gboolean delete_desktop_entry(gchar *path);
gboolean delete_desktop_entry_by_node(GtkCTree *ctree, GtkCTreeNode *node,
				      gpointer data);
void delete_recursive_cb(gint button, gpointer data);

/* utils.c -------------- */

gint isfile(char *s);
gint isdir(char *s);
gchar *filename_from_path(char *t);
gchar *strip_one_file_layer(char *t);
gchar *correct_path_to_file(gchar *path1, gchar *path2, gchar *filename);
gchar *check_for_dir(char *d);
char * basename_n(char *path, int n);


/* save.c */

gboolean create_desktop_entry_node(Desktop_Data *d);
gboolean update_desktop_entry_node(GnomeDesktopEntry *dentry, gchar *old_path);
gboolean save_desktop_entry_file(GnomeDesktopEntry *dentry, gchar *path,
				 gboolean prompt_first,
				 gboolean prompt_about_overwrite,
				 gboolean error_on_overwrite_conflict);

void recalc_paths_cb (GtkCTree *ctree, GtkCTreeNode *node, gpointer data);

/* dnd.c */
void gmenu_init_dnd(GtkCTree *ctree);
gboolean tree_move_test_cb(GtkCTree *ctree, GtkCTreeNode *source_node,
			GtkCTreeNode *new_parent, GtkCTreeNode *new_sibling);
void tree_moved(GtkCTree *ctree, GtkCTreeNode *node, GtkCTreeNode *new_parent,
			GtkCTreeNode *new_sibling, gpointer data);

/* load.c */
int is_desktop_file_editable(gchar *path);
Desktop_Data * get_desktop_file_info (gchar *file);
void free_desktop_data(Desktop_Data *d);
