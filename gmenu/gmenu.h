/*###################################################################*/
/*##                       gmenu (GNOME menu editor) 0.2.4         ##*/
/*###################################################################*/

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <gnome.h>

	/* definitions */
#define GMENU_VERSION_MAJOR 0
#define GMENU_VERSION_MINOR 2
#define GMENU_VERSION_REV 4

typedef struct _Desktop_Data Desktop_Data;
struct _Desktop_Data
{
	gchar *path;
	gchar *name;
	gchar *comment;
	gchar *tryexec;
	gchar *exec;
	gchar *icon;
	gint terminal;
	gchar *doc;
	gchar *type;
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

extern GtkWidget *filename_entry;

extern GtkObject *edit_area;

extern GList *topnode;
extern GList *usernode;
extern GList *systemnode;
extern GList *current_node;
extern gchar *current_path;

/* gmenu.c --------------- */

int isfile(char *s);
int isdir(char *s);
char *filename_from_path(char *t);
char *strip_one_file_layer(char *t);
gchar *correct_path_to_file(gchar *path1, gchar *path2, gchar *filename);

/* tree.c ---------------- */

GList *find_file_in_tree(GtkCTree * ctree, char *path);
void update_tree_highlight(GtkWidget *w, GList *old, GList *new, gint move);
void move_down_cb(GtkWidget *w, gpointer data);
void move_up_cb(GtkWidget *w, gpointer data);
int is_node_editable(GList *node);
void tree_item_selected (GtkCTree *ctree, GdkEventButton *event, gpointer data);
GList *add_leaf_node(GtkCTree *ctree, GList *parent, GList *node, char *file);
void add_tree_node(GtkCTree *ctree, GList *parent);
void add_main_tree_node();

/* edit.c ---------------- */

void update_edit_area(Desktop_Data *d);
void revert_edit_area();
void new_edit_area();

/* order.c --------------- */

GList *get_order_of_dir(char *dir);
void save_order_of_dir(GList *node);
void free_desktop_data(Desktop_Data *d);
Desktop_Data * get_desktop_file_info (char *file);

/* dialogs.c ------------- */

void create_folder_pressed();
void delete_pressed_cb();
void save_pressed_cb();


