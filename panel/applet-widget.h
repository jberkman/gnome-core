#ifndef __APPLET_WIDGET_H__
#define __APPLET_WIDGET_H__

#include <gtk/gtk.h>
#include <gnome.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifndef PANEL_H
/*from panel.h*/
typedef enum {
	ORIENT_UP,
	ORIENT_DOWN,
	ORIENT_LEFT,
	ORIENT_RIGHT
} PanelOrientType;
#endif

#define APPLET_WIDGET(obj)          GTK_CHECK_CAST (obj, applet_widget_get_type (), AppletWidget)
#define APPLET_WIDGET_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, applet_widget_get_type (), AppletWidgetClass)
#define IS_APPLET_WIDGET(obj)       GTK_CHECK_TYPE (obj, applet_widget_get_type ())

typedef struct _AppletWidget		AppletWidget;
typedef struct _AppletWidgetClass	AppletWidgetClass;

typedef void (*AppletCallbackFunc)(AppletWidget *applet, gpointer data);

struct _AppletWidget
{
	GtkPlug			window;

	gint			applet_id;

	/* use these as prefixes when loading saving data */
	gchar			*cfgpath;
	gchar			*globcfgpath;
};

struct _AppletWidgetClass
{
	GtkPlugClass parent_class;

	/* when the orientation of the parent panel changes */
	void (* change_orient) (AppletWidget *applet,
				PanelOrientType orient);
	/*when the panel wants to save a session it will call this signal 
	  if you trap it make sure you do gnome_config_sync() and
	  gnome_config_drop_all() after your done otherwise the changes
	  might not be written to file, also make sure you return
	  FALSE from this signal or your position wil not get saved!*/
	gint (* session_save) (AppletWidget *applet,
			       gchar *cfgpath,
			       gchar *globcfgpath);
};

guint		applet_widget_get_type		(void);
GtkWidget*	applet_widget_new		(gchar *argv0);

/*set tooltip over the applet, NULL to remove a tooltip*/
void		applet_widget_set_tooltip	(AppletWidget *applet,
						 gchar *text);

/* add a widget to the plug and register the applet, this has to
   be done after all the children had been added so that the applet-lib
   can bind the events over them so that peoplce can move them with
   the second button, get the menu, etc ...*/
void		applet_widget_add		(AppletWidget *applet,
						 GtkWidget *widget);

/* remove the plug from the panel, this will destroy the applet */
void		applet_widget_remove_from_panel (AppletWidget *applet);


/* Add a callback onto the applet's right click menu, use for properies
   callback, help, etc... etc...
  */
void		applet_widget_register_callback	(AppletWidget *applet,
						 gchar *name,
						 gchar *menutext,
						 AppletCallbackFunc func,
						 gpointer data);

/* get the applet widget with the id of applet_id */
AppletWidget*	applet_widget_get_by_id		(gint applet_id);

/* use this as gtk_main in applets */
void		applet_widget_gtk_main		(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __APPLET_WIDGET_H__ */
