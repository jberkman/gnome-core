/*###################################################################*/
/*##                       gmenu (GNOME menu editor) 0.2.5         ##*/
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
#define GMENU_VERSION_MINOR 2
#define GMENU_VERSION_REV 5

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

int isfile(char *s);
int isdir(char *s);
char *filename_from_path(char *t);
char *strip_one_file_layer(char *t);
gchar *correct_path_to_file(gchar *path1, gchar *path2, gchar *filename);

/* tree.c ---------------- */

GtkCTreeNode *find_file_in_tree(GtkCTree * ctree, char *path);
void update_tree_highlight(GtkWidget *w, GtkCTreeNode *old, GtkCTreeNode *new, gint select);
void move_down_cb(GtkWidget *w, gpointer data);
void move_up_cb(GtkWidget *w, gpointer data);
int is_node_editable(GtkCTreeNode *node);
void edit_pressed_cb();
void tree_item_selected (GtkWidget *widget, gint row, gint column, GdkEventButton *bevent);
GtkCTreeNode *add_leaf_node(GtkCTree *ctree, GtkCTreeNode *parent, GtkCTreeNode *node, char *file);
void add_tree_node(GtkCTree *ctree, GtkCTreeNode *parent);
void add_main_tree_node();

/* edit.c ---------------- */

void edit_area_reset_revert(Desktop_Data *d);
gchar * edit_area_get_filename();
void update_edit_area(Desktop_Data *d);
void revert_edit_area();
void new_edit_area();
GtkWidget * create_edit_area();

/* order.c --------------- */

GList *get_order_of_dir(char *dir);
void save_order_of_dir(GtkCTreeNode *node);
void free_desktop_data(Desktop_Data *d);
Desktop_Data * get_desktop_file_info (char *file);

/* dialogs.c ------------- */

void create_folder_pressed();
void delete_pressed_cb();
void save_pressed_cb();


