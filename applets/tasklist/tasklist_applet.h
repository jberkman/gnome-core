#include "X11/Xatom.h"
#include "applet-widget.h"
#include "gwmh.h"

/* The row height of a task */
#define ROW_HEIGHT 24
typedef struct _TasklistTask TasklistTask;
typedef struct _TasklistConfig TasklistConfig;

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
	GdkPixmap *pixmap;
	GdkBitmap *mask;
	GwmhTask *gwmh_task;
};

struct _TasklistConfig {

	gboolean show_mini_icons; /* Show small icons next to tasks */
	gboolean show_minimized; /* Show minimized tasks */
	gboolean show_normal; /* Show normal tasks */
	gboolean all_desks_normal; /* Show normal tasks on all desktops */
	gboolean all_desks_minimized; /* Show minimized tasks on all desktops */
	gboolean confirm_before_kill; /* Confirm before killing windows */

	/* Stuff for horizontal mode */
	gint horz_width; /* The width of the tasklist */
	gint horz_rows; /* Number of rows */
	gboolean horz_fixed; /* Fixed or dynamic sizing */
	gint horz_taskwidth; /* Width of a single task (for dynamic sizing) */
	/* Stuff for vertical mode */
	gint vert_height; /* The height of the tasklist */
	gint vert_width; /* The width of the tasklist */
	gboolean vert_fixed; /* Fixed or dynamic sizing */
};

void menu_popup (TasklistTask *task, guint button, guint32 activate_time);
void display_properties (void);
void read_config (void);
gboolean write_config (gpointer data,
		       const gchar *privcfgpath,
		       const gchar *globcfgpath);
void change_size (gboolean layout);


