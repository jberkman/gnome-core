/*
 * GNOME menu editor revision 2
 * (C)1999
 *
 * Authors: John Ellis <johne@bellatlantic.net>
 *          Nat Friedman <nat@nat.org>
 *
 */

#include <config.h>

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
#define GMENU_VERSION_MAJOR 1
#define GMENU_VERSION_MINOR 0
#define GMENU_VERSION_MICRO 1

typedef struct _Desktop_Data Desktop_Data;
struct _Desktop_Data
{
	gchar *path;
	gchar *name;
	gchar *comment;
	GtkWidget *pixmap;
	gint isfolder;
	gint expanded;
	gint editable;
};

extern GtkWidget *app;
extern GtkWidget *tree;
extern GtkWidget *infolabel;
extern GtkWidget *infopixmap;

extern gchar *system_apps_dir;
extern gchar *system_apps_merge_dir;
extern gchar *system_applets_dir;
extern gchar *user_apps_dir;
extern gchar *system_pixmap_dir;
extern gchar *user_pixmap_dir;

/* tree.c */
GtkCTreeNode *menu_tree_find_path(GtkCTree *ctree, gchar *path);
GtkCTreeNode *menu_tree_get_selection(GtkCTree *ctree);
gint menu_tree_node_is_editable(GtkWidget *ctree, GtkCTreeNode *node);
void menu_tree_update_paths(GtkWidget *ctree, GtkCTreeNode *node);
void menu_tree_path_updated(GtkWidget *ctree, gchar *old_path, gchar *new_path, GnomeDesktopEntry *dentry);
GtkCTreeNode *menu_tree_insert_node(GtkWidget *ctree, GtkCTreeNode *parent,
			GtkCTreeNode *sibling, Desktop_Data *dd, gint expanded);
void menu_tree_populate(GtkWidget *ctree);
void menu_tree_init_signals(GtkWidget *ctree, GnomeUIInfo *tree_popup_uiinfo);

/* treenew.c */
void menu_tree_new_folder(GtkWidget *ctree);
void menu_tree_new_item(GtkWidget *ctree);

/* treedel.c */
void menu_tree_delete_item(GtkWidget *ctree);

/* treeutil.c */
void menu_tree_move_up(GtkWidget *ctree);
void menu_tree_move_down(GtkWidget *ctree);
void menu_tree_sort_selected(GtkWidget *ctree);
void menu_tree_sort_selected_recursive(GtkWidget *ctree);

/* treednd.c */
void menu_tree_init_dnd(GtkWidget *ctree);

/* edit.c */
GtkWidget * edit_area_create(void);
void edit_area_set_to(Desktop_Data *dd);
void edit_area_change_path(gchar *path);
gchar *edit_area_path(void);
void edit_area_clear(gchar *path, gchar *name);
GnomeDesktopEntry *edit_area_get_dentry(void);
void edit_area_grab_name(void);

/* save.c */
gboolean save_desktop_entry_file(GnomeDesktopEntry *dentry, gchar *path,
			gboolean prompt_first, gboolean prompt_about_overwrite,
			gboolean error_on_overwrite_conflict);
void save_desktop_entry(GnomeDesktopEntry *dentry, gchar *path, gint isfolder);

/* order.c */
GList *get_order_of_dir(gchar *dir);
void save_order_of_dir(GtkCTree *ctree, GtkCTreeNode *node, gint is_parent);

/* desktop.c */
Desktop_Data *desktop_data_new(gchar *path, gchar *name, gchar *comment, GtkWidget *pixmap);
Desktop_Data *desktop_data_new_from_path(gchar *path);
void desktop_data_free(Desktop_Data *dd);

/* utils.c */
gchar *check_for_dir(char *d);
gint isfile(gchar *s);
gint isdir(gchar *s);
gint file_is_editable(gchar *path);
gchar *remove_level_from_path(gchar *path);
gchar *validate_filename(gchar *file);
GtkWidget *pixmap_top(void);
GtkWidget *pixmap_unknown(void);
GtkWidget *pixmap_load(gchar *path);

