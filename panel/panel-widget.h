/* Gnome panel: panel widget
 * (C) 1997 the Free Software Foundation
 *
 * Authors:  George Lebl
 */
#ifndef __PANEL_WIDGET_H__
#define __PANEL_WIDGET_H__


#include <gtk/gtk.h>

BEGIN_GNOME_DECLS

#define PANEL_WIDGET(obj)          GTK_CHECK_CAST (obj, panel_widget_get_type (), PanelWidget)
#define PANEL_WIDGET_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, panel_widget_get_type (), PanelWidgetClass)
#define IS_PANEL_WIDGET(obj)       GTK_CHECK_TYPE (obj, panel_widget_get_type ())

#define PANEL_CELL_SIZE 1
#define PANEL_MINIMUM_WIDTH 48

#define PANEL_APPLET_PARENT_KEY "panel_applet_parent_key"
#define PANEL_APPLET_ASSOC_PANEL_KEY "panel_applet_assoc_panel_key"
#define PANEL_APPLET_FORBIDDEN_PANELS "panel_applet_forbidden_panels"
#define PANEL_APPLET_DATA "panel_applet_data"
#define PANEL_PARENT "panel_parent"

typedef struct _PanelWidget		PanelWidget;
typedef struct _PanelWidgetClass	PanelWidgetClass;

typedef struct _AppletRecord		AppletRecord;
typedef struct _AppletData		AppletData;
typedef struct _DNDRecord		DNDRecord;
typedef enum {
	PANEL_HORIZONTAL,
	PANEL_VERTICAL
} PanelOrientation;
typedef enum {
	PANEL_SWITCH_MOVE,
	PANEL_FREE_MOVE
} PanelMovementType;
typedef enum {
	PANEL_BACK_NONE,
	PANEL_BACK_COLOR,
	PANEL_BACK_PIXMAP
} PanelBackType;

struct _AppletData
{
	GtkWidget *applet;
	int pos;
	int cells;

	int prevx;
	int prevy;
	int prevwidth;
	int prevheight;
};

struct _PanelWidget
{
	GtkEventBox		ebox;

	GtkWidget		*frame;
	GtkWidget		*fixed;

	GList			*applet_list;

	int			size;
	PanelOrientation	orient;
	int			fit_pixmap_bg;

	int			packed;

	AppletData		*currently_dragged_applet;

	int			thick;

	PanelBackType		back_type;
	char                    *back_pixmap;
	GdkColor		back_color;
	
	int			postpone_adjust;
	int			adjust_applet_idle;
	
	int			drawers_open; /* a count which can be used
						 to block the autohide, until
						 it is 0 .. it's set by the
						 app not the widget*/
	GtkWidget		*master_widget;
	
	GtkWidget		*drop_widget; /*this is the widget that the
						panel checks for the cursor
						on drops usually the panel
					        widget itself*/
	
	/*this is used in mass send_moves, if these match we don't need to send
	  applet_move signal*/
	int			last_x,last_y,last_w,last_h;
};

struct _PanelWidgetClass
{
	GtkEventBoxClass parent_class;

	void (* orient_change) (PanelWidget *panel,
				PanelOrientation orient);
	void (* applet_move) (PanelWidget *panel,
			      GtkWidget *applet);
	void (* applet_added) (PanelWidget *panel,
			       GtkWidget *applet);
	void (* applet_removed) (PanelWidget *panel);
	void (* back_change) (PanelWidget *panel,
			      PanelBackType type,
			      char *pixmap,
			      GdkColor *color);
};

guint		panel_widget_get_type		(void);
GtkWidget *	panel_widget_new		(int packed,
						 PanelOrientation orient,
						 PanelBackType back_type,
						 char *back_pixmap,
						 int fit_pixmap_bg,
						 GdkColor *back_color);

/*add an applet to the panel, preferably at position pos*/
int		panel_widget_add		(PanelWidget *panel,
						 GtkWidget *applet,
						 int pos);

/*needs to be called for drawers after add*/
void		panel_widget_add_forbidden	(PanelWidget *panel);

/*move applet to newpos*/
int		panel_widget_move		(PanelWidget *panel,
						 GtkWidget *applet,
						 int pos);

/*remove an applet from the panel*/
int		panel_widget_remove		(PanelWidget *panel,
						 GtkWidget *applet);

/*move applet to a different panel*/
int		panel_widget_reparent		(PanelWidget *old_panel,
						 PanelWidget *new_panel,
						 GtkWidget *applet,
						 int pos);
/*return position of an applet*/
int		panel_widget_get_pos		(PanelWidget *panel,
						 GtkWidget *applet);
/*return a list of all applets*/
GList*		panel_widget_get_applets	(PanelWidget *panel);
/*run func for each applet*/
void		panel_widget_foreach		(PanelWidget *panel,
						 GFunc func,
						 gpointer user_data);

/*drag*/
void		panel_widget_applet_drag_start	(PanelWidget *panel,
						 GtkWidget *applet);
void		panel_widget_applet_drag_end	(PanelWidget *panel);

/* needed for corba */
void		panel_widget_applet_drag_start_no_grab(PanelWidget *panel,
						       GtkWidget *applet);
void		panel_widget_applet_drag_end_no_grab(PanelWidget *panel);
int		panel_widget_applet_move_to_cursor(PanelWidget *panel);
void		panel_widget_applet_move_use_idle(PanelWidget *panel);

/* changing parameters */
void		panel_widget_change_params	(PanelWidget *panel,
						 PanelOrientation orient,
						 PanelBackType back_type,
						 char *pixmap_name,
						 int fit_pixmap_bg,
						 GdkColor *back_color);

/* changing parameters (orient only) */
void		panel_widget_change_orient	(PanelWidget *panel,
						 PanelOrientation orient);

/*change global params*/
void		panel_widget_change_global	(int explicit_step,
						 int auto_step,
						 int drawer_step,
						 int minimized_size,
						 int minimize_delay,
						 PanelMovementType move_type,
						 int disable_animations);

/*get the number of applets*/
int		panel_widget_get_applet_count	(PanelWidget *panel);

/*tells us if an applet is "stuck" on the right side*/
int		panel_widget_is_applet_stuck	(PanelWidget *panel,
						 GtkWidget *applet);

/*needed for other panel types*/
void		panel_widget_pack_applets	(PanelWidget *panel);
void		panel_widget_applet_put		(PanelWidget *panel,
						 AppletData *ad,
						 int force);
void		panel_widget_put_all		(PanelWidget *panel,
						 int force);
AppletData	*get_applet_data_pos		(PanelWidget *panel,
						 int pos);
int		panel_widget_is_cursor		(PanelWidget *panel,
						 int overlap);
void		panel_widget_set_size		(PanelWidget *panel);

/*slightly hackish, the widget that owns the widget will call this
  function when it's window changes position or size ...*/
void		panel_widget_send_move		(PanelWidget *panel);

extern GList *panels;

extern int panel_applet_in_drag;


END_GNOME_DECLS

#endif /* __PANEL_WIDGET_H__ */
