#ifndef APPLET_H
#define APPLET_H

#include <glib.h>
#include <gdk/gdktypes.h>
#include "panel-widget.h"

G_BEGIN_DECLS

#define EMPTY_ID "Empty" /*this applet should not be loaded*/
#define MENU_ID "Menu"
#define DRAWER_ID "Drawer"
#define LOGOUT_ID "Logout"
#define SWALLOW_ID "Swallow"
#define EXTERN_ID "Extern"
#define LAUNCHER_ID "Launcher"
#define LOCK_ID "Lock"
#define STATUS_ID "Status"
#define RUN_ID "Run"

typedef enum {
	APPLET_EXTERN,
	APPLET_EXTERN_RESERVED,
	APPLET_EXTERN_PENDING,
	APPLET_DRAWER,
	APPLET_MENU,
	APPLET_LOGOUT,
	APPLET_SWALLOW,
	APPLET_LAUNCHER,
	APPLET_EMPTY,
	APPLET_LOCK,
	APPLET_STATUS,
	APPLET_RUN,
	APPLET_BONOBO
} AppletType;

#define APPLET_EVENT_MASK (GDK_BUTTON_PRESS_MASK |		\
			   GDK_BUTTON_RELEASE_MASK |		\
			   GDK_POINTER_MOTION_MASK |		\
			   GDK_POINTER_MOTION_HINT_MASK)


typedef struct _AppletUserMenu AppletUserMenu;
typedef struct _AppletInfo AppletInfo;

struct _AppletUserMenu {
	char *name;
	char *stock_item;
	char *text;
	int sensitive;
	AppletInfo *info;
	GtkWidget *menuitem;
	GtkWidget *submenu;
};

struct _AppletInfo {
	AppletType type;
	int applet_id;
	GtkWidget *widget; /*an event box*/
	GtkWidget *menu; /*the applet menu*/
	int menu_age;
	GList *user_menu; /*list of AppletUserMenu items for callbacks*/
	gpointer data; /*the per applet structure, if it exists*/
	GDestroyNotify data_destroy;
};

AppletInfo *panel_applet_register (GtkWidget      *applet,
				   gpointer        data,
				   GDestroyNotify  data_destroy,
				   PanelWidget    *panel,
				   gint            pos,
				   gboolean        exactpos,
				   AppletType      type);

void panel_clean_applet(AppletInfo *info);

/*applet menu stuff*/
void create_applet_menu(AppletInfo *info, gboolean is_basep);
void applet_add_callback(AppletInfo *info,
			 const char *callback_name,
			 const char *stock_item,
			 const char *menuitem_text);
void applet_remove_callback(AppletInfo *info,
			    const char *callback_name);
AppletUserMenu * applet_get_callback (GList *user_menu, const char *name);
void applet_callback_set_sensitive(AppletInfo *info,
				   const char *callback_name,
				   int sensitive);

/* during session save, we call this to unlink the .desktop files of
 * removed launchers */
void remove_unused_launchers (void);

G_END_DECLS

#endif
