#include "X11/Xatom.h"
#include <applet-widget.h>
#include "gwmh.h"

#define CONFIG_HEIGHT 48
#define CONFIG_ROWHEIGHT 24
#define CONFIG_ROWS 2
#define CONFIG_PIXMAP 0

typedef struct _TasklistTask TasklistTask;
typedef struct _Config Config;

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

struct _Config {
	gboolean show_winops;
	gboolean confirm_kill;
	gboolean all_tasks;
	gboolean minimized_tasks;
	gboolean show_all;
	gboolean show_normal;
	gboolean show_minimized;
	gboolean numrows_follows_panel;
	gint tasklist_width;
};

void menu_popup (TasklistTask *task, guint button, guint32 activate_time);

