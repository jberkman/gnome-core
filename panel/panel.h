#ifndef PANEL_H
#define PANEL_H

#include "panel_cmds.h"
#include "panel-widget.h"
#include "libgnomeui/gnome-session.h"

BEGIN_GNOME_DECLS

#define MENU_ID "Menu"
#define LAUNCHER_ID "Launcher"
#define DRAWER_ID "Drawer"

/*FIXME: maintain two global step sizes, one for autohide, one for
  drawers, and explicit (side) hide panels*/
#define DEFAULT_STEP_SIZE 40

/* amount of time in ms. to wait before lowering panel */
#define DEFAULT_MINIMIZE_DELAY 300

/* number of pixels it'll stick up from the bottom when using
 * PANEL_AUTO_HIDE */
#define DEFAULT_MINIMIZED_SIZE 6

#define DEFAULT_PANEL_NUM 0

typedef struct _PanelConfig PanelConfig;
struct _PanelConfig {
	PanelOrientation orient;
	PanelSnapped snapped;
	PanelMode mode;
	PanelState state;
	gint step_size;
	gint minimized_size;
	gint minimize_delay;
};

typedef enum {
	APPLET_HAS_PROPERTIES = 1L << 0
} AppletFlags;

typedef enum {
	APPLET_EXTERN,
	APPLET_DRAWER,
	APPLET_MENU,
	APPLET_LAUNCHER
} AppletType;

typedef struct _AppletInfo AppletInfo;
struct _AppletInfo {
	GtkWidget *widget;
	GtkWidget *assoc; /*associated widget, e.g. a drawer or a menu*/
	gpointer data;
	AppletType type;
	AppletFlags flags;
	gchar *id;
	gchar *params;
};


gpointer panel_command(PanelCommand *cmd);

int panel_session_save (gpointer client_data,
			GnomeSaveStyle save_style,
			int is_shutdown,
			GnomeInteractStyle interact_style,
			int is_fast);

GtkWidget * create_panel_root_menu(PanelWidget *panel);

void create_applet_menu(void);

void register_toy(GtkWidget *applet,
		  GtkWidget *assoc,
		  char *id,
		  char *params,
		  int pos,
		  int panel,
		  long flags,
		  AppletType type);

END_GNOME_DECLS

#endif
