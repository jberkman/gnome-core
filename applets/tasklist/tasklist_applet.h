#include "X11/Xatom.h"
#include "applet-widget.h"
#include "gwmh.h"

/* The row height of a task */
#define ROW_HEIGHT 24
typedef struct _TasklistTask TasklistTask;
typedef struct _TasklistConfig TasklistConfig;

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

	gboolean show_pixmaps; /* Show pixmaps next to tasks */
	gboolean confirm_before_kill; /* Confirm before killing windows */

	/* Stuff for horizontal mode */
	gint width; /* The width of the tasklist */
	gint rows; /* Number of rows */

	/* Stuff for vertical mode */
	gint height; /* The height of the tasklist */
};

void menu_popup (TasklistTask *task, guint button, guint32 activate_time);
void display_properties (void);
void read_config (void);
void write_config (void);
void change_size (void);


