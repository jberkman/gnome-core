/* Gnome panel: snapped widget
 * (C) 1997 the Free Software Foundation
 *
 * Authors:  George Lebl
 */
#ifndef __SNAPPED_WIDGET_H__
#define __SNAPPED_WIDGET_H__

#include <gtk/gtk.h>
#include "panel-widget.h"

BEGIN_GNOME_DECLS

#define SNAPPED_WIDGET(obj)          GTK_CHECK_CAST (obj, snapped_widget_get_type (), SnappedWidget)
#define SNAPPED_WIDGET_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, snapped_widget_get_type (), SnappedWidgetClass)
#define IS_SNAPPED_WIDGET(obj)       GTK_CHECK_TYPE (obj, snapped_widget_get_type ())

typedef struct _SnappedWidget		SnappedWidget;
typedef struct _SnappedWidgetClass	SnappedWidgetClass;

typedef enum {
	SNAPPED_TOP,
	SNAPPED_BOTTOM,
	SNAPPED_LEFT,
	SNAPPED_RIGHT
} SnappedPos;
typedef enum {
	SNAPPED_EXPLICIT_HIDE,
	SNAPPED_AUTO_HIDE
} SnappedMode;
typedef enum {
	SNAPPED_SHOWN,
	SNAPPED_MOVING,
	SNAPPED_HIDDEN,
	SNAPPED_HIDDEN_RIGHT,
	SNAPPED_HIDDEN_LEFT
} SnappedState;

struct _SnappedWidget
{
	GtkWindow		window;
	
	GtkWidget		*panel;

	GtkWidget		*table;
	GtkWidget		*hidebutton_n;
	GtkWidget		*hidebutton_e;
	GtkWidget		*hidebutton_w;
	GtkWidget		*hidebutton_s;

	SnappedPos		pos;
	SnappedMode		mode;
	SnappedState		state;

	int			leave_notify_timer_tag;

	int			autohide_inhibit;
};

struct _SnappedWidgetClass
{
	GtkWindowClass parent_class;

	void (* pos_change) (SnappedWidget *panel,
			     SnappedPos pos);
	void (* state_change) (SnappedWidget *panel,
			       SnappedState state);
};

guint		snapped_widget_get_type		(void);
GtkWidget*	snapped_widget_new		(SnappedPos pos,
						 SnappedMode mode,
						 SnappedState state,
						 PanelBackType back_type,
						 char *back_pixmap,
						 int fit_pixmap_bg,
						 GdkColor *back_color);

/* changing parameters */
void		snapped_widget_change_params	(SnappedWidget *snapped,
						 SnappedPos pos,
						 SnappedMode mode,
						 SnappedState state,
						 PanelBackType back_type,
						 char *pixmap_name,
						 int fit_pixmap_bg,
						 GdkColor *back_color);

/* changing parameters (pos only) */
void		snapped_widget_change_pos	(SnappedWidget *snapped,
						 SnappedPos pos);

/*popup the widget if it's popped down (autohide)*/
void		snapped_widget_pop_up		(SnappedWidget *snapped);

/*queue a pop_down in autohide mode*/
void		snapped_widget_queue_pop_down	(SnappedWidget *snapped);

void		snapped_widget_enable_buttons	(SnappedWidget *snapped);
void		snapped_widget_disable_buttons	(SnappedWidget *snapped);

END_GNOME_DECLS

#endif /* __SNAPPED_WIDGET_H__ */
