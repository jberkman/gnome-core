#include "X11/Xatom.h"
#include <applet-widget.h>
#include "gwmh.h"

#define CONFIG_HEIGHT 48
#define CONFIG_ROWHEIGHT 24
#define CONFIG_ROWS 2
#define CONFIG_PIXMAP 0

typedef struct _TasklistTask TasklistTask;
typedef struct _Config Config;

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

