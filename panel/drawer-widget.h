/* Gnome panel: drawer widget
 * (C) 1997 the Free Software Foundation
 *
 * Authors:  George Lebl
 */
#ifndef __DRAWER_WIDGET_H__
#define __DRAWER_WIDGET_H__


#include <gtk/gtk.h>
#include "applet.h"
#include "basep-widget.h"

BEGIN_GNOME_DECLS

#define DRAWER_WIDGET(obj)          GTK_CHECK_CAST (obj, drawer_widget_get_type (), DrawerWidget)
#define DRAWER_WIDGET_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, drawer_widget_get_type (), DrawerWidgetClass)
#define IS_DRAWER_WIDGET(obj)       GTK_CHECK_TYPE (obj, drawer_widget_get_type ())

typedef struct _DrawerWidget		DrawerWidget;
typedef struct _DrawerWidgetClass	DrawerWidgetClass;

typedef enum {
	DRAWER_SHOWN,
	DRAWER_MOVING,
	DRAWER_HIDDEN
} DrawerState;

struct _DrawerWidget
{
	BasePWidget		basep;

	GtkWidget		*panel;
	GtkWidget		*table;
	GtkWidget		*handle_n;
	GtkWidget		*handle_e;
	GtkWidget		*handle_w;
	GtkWidget		*handle_s;
	
	GtkWidget		*frame;

	DrawerState		state;
	PanelOrientType		orient;
};

struct _DrawerWidgetClass
{
	BasePWidgetClass parent_class;

	void (* state_change) (DrawerWidget *panel,
			       DrawerState state);
};

guint		drawer_widget_get_type		(void);
GtkWidget*	drawer_widget_new		(PanelOrientType orient,
						 DrawerState state,
						 PanelBackType back_type,
						 char *back_pixmap,
						 int fit_pixmap_bg,
						 GdkColor *back_color,
						 int hidebutton_pixmap_enabled,
						 int hidebutton_enabled);

/*open and close drawers*/
void		drawer_widget_open_drawer	(DrawerWidget *panel);
void		drawer_widget_close_drawer	(DrawerWidget *panel);

/* changing parameters */
void		drawer_widget_change_params	(DrawerWidget *drawer,
						 PanelOrientType orient,
						 DrawerState state,
						 PanelBackType back_type,
						 char *pixmap_name,
						 int fit_pixmap_bg,
						 GdkColor *back_color,
						 int hidebutton_pixmap_enabled,
						 int hidebutton_enabled);

/* changing parameters (orient only) */
void		drawer_widget_change_orient	(DrawerWidget *drawer,
						 PanelOrientType orient);

void		drawer_widget_restore_state	(DrawerWidget *drawer);

END_GNOME_DECLS

#endif /* __DRAWER_WIDGET_H__ */
