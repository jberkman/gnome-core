#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "applet-widget.h"
#include "gwmh.h"

#ifndef TASKLIST_APPLET_H
#define TASKLIST_APPLET_H

/* The row height of a task */
#define ROW_HEIGHT 24
typedef struct _TasklistTask TasklistTask;
typedef struct _TasklistConfig TasklistConfig;
typedef struct _TasklistIcon TasklistIcon;

/* Simple enum for which tasks to show */
enum
{
	TASKS_SHOW_ALL,
	TASKS_SHOW_MINIMIZED,
	TASKS_SHOW_NORMAL
};

typedef enum
{
	MENU_ACTION_CLOSE,
	MENU_ACTION_SHOW_HIDE,
	MENU_ACTION_SHADE_UNSHADE,
	MENU_ACTION_STICK_UNSTICK,
	MENU_ACTION_KILL,
	MENU_ACTION_LAST
} MenuAction;

struct _TasklistTask {
	gint x, y;
	gint width, height;
	gint fullwidth;
	TasklistIcon *icon;
	Pixmap wmhints_icon;
	GwmhTask *gwmh_task;
	GtkWidget *menu;
        gint serial_number;  /* for sorting */
};

struct _TasklistConfig {

	gboolean show_mini_icons; /* Show small icons next to tasks */
	gboolean show_minimized; /* Show minimized tasks */
	gboolean show_normal; /* Show normal tasks */
	gboolean all_desks_normal; /* Show normal tasks on all desktops */
	gboolean all_desks_minimized; /* Show minimized tasks on all desktops */
	gboolean confirm_before_kill; /* Confirm before killing windows */
	gboolean move_to_current; /* Move iconified tasks to current workspace */
        gboolean sort_tasklist; /* show the task list in sorted order? */
	
	/* Follow the panel sizes */
	gboolean follow_panel_size;

	/* Stuff for horizontal mode */
	gint horz_width; /* The width of the tasklist */
	gint horz_rows; /* Number of rows */
	gboolean horz_fixed; /* Fixed or dynamic sizing */
	gint horz_taskwidth; /* Width of a single task (for dynamic sizing) */
	/* Stuff for vertical mode */
	gint vert_height; /* The height of the tasklist */
	gint vert_width; /* The width of the tasklist */
	gboolean vert_fixed; /* Fixed or dynamic sizing */
	gboolean vert_width_full;  /* a mode where th width is the maximum width */
				   /* of any window title. */


};

struct _TasklistIcon {
	GdkPixbuf *normal;
	GdkPixbuf *minimized;
};

void menu_popup (TasklistTask *task, guint button, guint32 activate_time);
void display_properties (void);
void read_config (void);
gboolean write_config (gpointer data,
		       const gchar *privcfgpath,
		       const gchar *globcfgpath);
void resort_tasklist (void);
/* fullwidth can be -1 so if it's to be recomputed (only if needed */
void change_size (gboolean layout, int fullwidth);
void layout_tasklist (gboolean call_change_size);
GdkPixbuf *tasklist_icon_create_minimized_icon (GdkPixbuf *pixbuf);
void tasklist_icon_set (TasklistTask *task);
void tasklist_icon_destroy (TasklistTask *task);
Pixmap tasklist_icon_get_pixmap (TasklistTask *task);

#endif /* TASKLIST_APPLET_H */
