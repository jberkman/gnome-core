/* Gnome panel: floating widget
 * (C) 1999 the Free Software Foundation
 *
 * Authors:  Jacob Berkman
 *           George Lebl
 *
 */

#include "floating-widget.h"
#include "border-widget.h"
#include "panel_config_global.h"
#include "foobar-widget.h"

extern GlobalConfig global_config;
extern int pw_minimized_size;

static void floating_pos_class_init (FloatingPosClass *klass);
static void floating_pos_init (FloatingPos *pos);

static void floating_pos_set_hidebuttons (BasePWidget *basep);
static PanelOrientType floating_pos_get_applet_orient (BasePWidget *basep);

static PanelOrientType floating_pos_get_hide_orient (BasePWidget *basep);
static void floating_pos_get_hide_pos (BasePWidget *basep,
				     PanelOrientType hide_orient,
				     int *x, int *y,
				     int w, int h);

static void floating_pos_get_pos(BasePWidget *basep,
				 int *x, int *y,
				 int w, int h);

static void floating_pos_set_pos (BasePWidget *basep,
				  int x, int y,
				  int w, int h);

static void floating_pos_get_hide_size (BasePWidget *basep,
					PanelOrientType hide_orient,
					int *x, int *y);

static void floating_pos_get_menu_pos (BasePWidget *basep,
				       GtkWidget *widget,
				       GtkRequisition *mreq,
				       int *x, int *y,
				       int wx, int wy,
				       int ww, int wh);

static void floating_pos_pre_convert_hook (BasePWidget *basep);

static void floating_pos_show_hide_left (BasePWidget *basep);
static void floating_pos_show_hide_right (BasePWidget *basep);

static BasePPosClass *parent_class;

GtkType
floating_pos_get_type ()
{
	static GtkType floating_pos_type = 0;

	if (!floating_pos_type) {
		GtkTypeInfo floating_pos_info = {
			"FloatingPos",
			sizeof (FloatingPos),
			sizeof (FloatingPosClass),
			(GtkClassInitFunc) floating_pos_class_init,
			(GtkObjectInitFunc) floating_pos_init,
			NULL, NULL
		};

		floating_pos_type = gtk_type_unique (BASEP_POS_TYPE,
						   &floating_pos_info);
	}

	return floating_pos_type;
}

enum {
	COORDS_CHANGE_SIGNAL,
	LAST_SIGNAL
};
static guint floating_pos_signals[LAST_SIGNAL] = { 0 };

static void
floating_pos_class_init (FloatingPosClass *klass)
{
	GtkObjectClass *object_class = GTK_OBJECT_CLASS(klass);
	BasePPosClass *pos_class = BASEP_POS_CLASS(klass);

	parent_class = gtk_type_class(BASEP_POS_TYPE);

	floating_pos_signals[COORDS_CHANGE_SIGNAL] =
		gtk_signal_new ("floating_coords_change",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET (FloatingPosClass,
						   coords_change),
				gtk_marshal_NONE__INT_INT,
				GTK_TYPE_NONE,
				2, GTK_TYPE_INT, GTK_TYPE_INT);
	
	gtk_object_class_add_signals (object_class,
				      floating_pos_signals,
				      LAST_SIGNAL);
	
	/* fill out the virtual funcs */
	pos_class->set_hidebuttons = floating_pos_set_hidebuttons;
	pos_class->get_applet_orient = floating_pos_get_applet_orient;
	pos_class->get_hide_orient = floating_pos_get_hide_orient;
	pos_class->get_hide_pos = floating_pos_get_hide_pos;
	pos_class->get_hide_size = floating_pos_get_hide_size;
	pos_class->get_pos = floating_pos_get_pos;
	pos_class->set_pos = floating_pos_set_pos;
	pos_class->get_menu_pos = floating_pos_get_menu_pos;
	pos_class->pre_convert_hook = floating_pos_pre_convert_hook;
	pos_class->north_clicked = pos_class->west_clicked = 
		floating_pos_show_hide_left;
	pos_class->south_clicked = pos_class->east_clicked =
		floating_pos_show_hide_right;
}

static void
floating_pos_init (FloatingPos *pos) { }

static void
floating_pos_set_hidebuttons (BasePWidget *basep)
{
	if (PANEL_WIDGET(basep->panel)->orient == PANEL_HORIZONTAL) {
		gtk_widget_hide(basep->hidebutton_n);
		gtk_widget_show(basep->hidebutton_e);
		gtk_widget_show(basep->hidebutton_w);
		gtk_widget_hide(basep->hidebutton_s);
	} else { /*vertical*/
		gtk_widget_show(basep->hidebutton_n);
		gtk_widget_hide(basep->hidebutton_e);
		gtk_widget_hide(basep->hidebutton_w);
		gtk_widget_show(basep->hidebutton_s);
	}
}

static PanelOrientType
floating_pos_get_applet_orient (BasePWidget *basep)
{
	PanelWidget *panel = PANEL_WIDGET (basep->panel);
	if (panel->orient == PANEL_HORIZONTAL)
		return (FLOATING_POS (basep->pos)->y < 
			gdk_screen_height () / 2)
			? ORIENT_DOWN : ORIENT_UP;
	else
		return (FLOATING_POS (basep->pos)->x < 
			gdk_screen_width () /2)
			? ORIENT_RIGHT : ORIENT_LEFT;
}

static PanelOrientType
floating_pos_get_hide_orient (BasePWidget *basep)
{
	FloatingPos *pos = FLOATING_POS (basep->pos);
	PanelWidget *panel = PANEL_WIDGET (basep->panel);

	switch (basep->state) {
	case BASEP_HIDDEN_LEFT:
		return (panel->orient == PANEL_HORIZONTAL)
			? ORIENT_LEFT : ORIENT_UP;
	case BASEP_HIDDEN_RIGHT:
		return (panel->orient == PANEL_HORIZONTAL)
			? ORIENT_RIGHT : ORIENT_DOWN;
	case BASEP_AUTO_HIDDEN:
		if (panel->orient == PANEL_HORIZONTAL) {
			return ((pos->x > (gdk_screen_width () - pos->x -
					   basep->shown_alloc.width))
				? ORIENT_RIGHT : ORIENT_LEFT);
		} else {
			return ((pos->y > (gdk_screen_height () - pos->y -
					   basep->shown_alloc.height))
				? ORIENT_DOWN : ORIENT_UP);
		}
	default:
		g_warning ("not hidden");
		return -1;
	}
}

static void
floating_pos_get_menu_pos (BasePWidget *basep,
			   GtkWidget *widget,
			   GtkRequisition *mreq,
			   int *x, int *y,
			   int wx, int wy,
			   int ww, int wh)
{	
	PanelWidget *panel = PANEL_WIDGET(basep->panel);
	
	if(panel->orient==PANEL_VERTICAL) {
		*x = wx + ww;
		*y += wy;
	} else {
		*x += wx;
		*y = wy - mreq->height;
	}
}

static void
floating_pos_set_pos (BasePWidget *basep,
		      int x, int y,
		      int w, int h)
{
	int minx, miny, maxx, maxy;
	FloatingPos *pos = FLOATING_POS(basep->pos);
	gint16 newx, newy;

	gdk_window_get_geometry (GTK_WIDGET(basep)->window,
				 &minx, &miny, &maxx, &maxy, NULL);
	gdk_window_get_origin (GTK_WIDGET(basep)->window, &minx, &miny);
	maxx += minx;
	maxy += miny;
	if (x >= minx &&
	    x <= maxx &&
	    y >= miny &&
	    y <= maxy)
 	        return;

	newx = pos->x;
	newy = pos->y;

	if (x < minx || x > maxx) {
		int w2=w;
		int x2=x;
		if (PANEL_WIDGET (basep->panel)->orient == PANEL_HORIZONTAL) {
			switch (basep->state) {
			case BASEP_SHOWN:
			case BASEP_MOVING:
				break;
			case BASEP_AUTO_HIDDEN:
			case BASEP_HIDDEN_LEFT:
				w2 = basep->hidebutton_w->allocation.width;
				break;
			case BASEP_HIDDEN_RIGHT:
				w2 = basep->hidebutton_e->allocation.width;
				x2 -= w - w2;
				break;
			}
		}
		newx = CLAMP (x < minx ? x2 : x2 - w2, 0, gdk_screen_width () - w);
	}

	if (y < miny || y > maxy) {
		int h2=h;
		int y2=y;
		if (PANEL_WIDGET (basep->panel)->orient == PANEL_VERTICAL) {
			switch (basep->state) {
			case BASEP_SHOWN:
			case BASEP_MOVING:
				break;
			case BASEP_AUTO_HIDDEN:
			case BASEP_HIDDEN_LEFT:
				h2 = basep->hidebutton_n->allocation.height;
				break;
			case BASEP_HIDDEN_RIGHT:
				h2 = basep->hidebutton_s->allocation.height;
				y2 -= h - h2;
				break;
			}
		}
		newy = CLAMP (y < miny ? y2 : y2 - h2, 0, gdk_screen_height () - h);
	}

	if (newy != pos->y || newx != pos->x) {
		pos->x = newx;
		pos->y = newy;
		gtk_signal_emit (GTK_OBJECT (pos),
				 floating_pos_signals[COORDS_CHANGE_SIGNAL],
				 x, y);
		gtk_widget_queue_resize (GTK_WIDGET (basep));
	}
}

static void
floating_pos_get_pos(BasePWidget *basep,
		     int *x, int *y,
		     int w, int h)
{
	*x = CLAMP (FLOATING_POS (basep->pos)->x, 0, gdk_screen_width () - w);
	*y = CLAMP (FLOATING_POS (basep->pos)->y,
		    foobar_widget_get_height (),
		    gdk_screen_height () - h);
}

static void
floating_pos_get_hide_size (BasePWidget *basep, 
			    PanelOrientType hide_orient,
			    int *w, int *h)
{
	switch (hide_orient) {
	case ORIENT_UP:
		*h = basep->hidebutton_n->allocation.height;
		break;
	case ORIENT_RIGHT:
		*w = basep->hidebutton_w->allocation.width;
		break;
	case ORIENT_DOWN:
		*h = basep->hidebutton_s->allocation.height;
		break;
	case ORIENT_LEFT:
		*w = basep->hidebutton_e->allocation.width;
		break;
	}
	*w = MAX (*w, 1);
	*h = MAX (*h, 1);
}

static void
floating_pos_get_hide_pos (BasePWidget *basep,
			   PanelOrientType hide_orient,
			   int *x, int *y,
			   int w, int h)
{
	switch (hide_orient) {
	case ORIENT_UP:
	case ORIENT_LEFT:
		break;
	case ORIENT_RIGHT:
		*x += w - basep->hidebutton_w->allocation.width;
		break;
	case ORIENT_DOWN:
		*y += h - basep->hidebutton_s->allocation.height;
		break;
	}
}

static void
floating_pos_show_hide_left (BasePWidget *basep)
{
	switch (basep->state) {
	case BASEP_SHOWN:
		basep_widget_explicit_hide (basep, BASEP_HIDDEN_LEFT);
		break;
	case BASEP_HIDDEN_RIGHT:
		basep_widget_explicit_show (basep);
		break;
	default:
		break;
	}
}

static void
floating_pos_show_hide_right (BasePWidget *basep)
{
	switch (basep->state) {
	case BASEP_SHOWN:
		basep_widget_explicit_hide (basep, BASEP_HIDDEN_RIGHT);
		break;
	case BASEP_HIDDEN_LEFT:
		basep_widget_explicit_show (basep);
		break;
	default:
		break;
	}
}

static void
floating_pos_pre_convert_hook (BasePWidget *basep)
{
	basep->keep_in_screen = TRUE;
	PANEL_WIDGET (basep->panel)->packed = TRUE;
}

void 
floating_widget_change_params (FloatingWidget *floating,
			       gint16 x, gint16 y,
			       PanelOrientation orient,
			       BasePMode mode,
			       BasePState state,
			       int sz,
			       int hidebuttons_enabled,
			       int hidebutton_pixmap_enabled,
			       PanelBackType back_type,
			       char *back_pixmap,
			       gboolean fit_pixmap_bg,
			       gboolean strech_pixmap_bg,
			       gboolean rotate_pixmap_bg,
			       GdkColor *back_color)
{
	FloatingPos *pos = FLOATING_POS (BASEP_WIDGET (floating)->pos);

	if (PANEL_WIDGET (BASEP_WIDGET (floating)->panel)->orient != orient)
		BASEP_WIDGET (floating)->request_cube = TRUE;

	x = CLAMP (x, 0, gdk_screen_width () - 
		   BASEP_WIDGET (floating)->shown_alloc.width);
	y = CLAMP (y, 0, gdk_screen_height () - 
		   BASEP_WIDGET (floating)->shown_alloc.height);

	if (y != pos->y || x != pos->x) {
		pos->x = x;
		pos->y = y;
		gtk_signal_emit (GTK_OBJECT (pos),
				 floating_pos_signals[COORDS_CHANGE_SIGNAL],
				 x, y);
	}

	basep_widget_change_params (BASEP_WIDGET (floating),
				    orient, sz, mode, state,
				    hidebuttons_enabled,
				    hidebutton_pixmap_enabled,
				    back_type, back_pixmap,
				    fit_pixmap_bg, strech_pixmap_bg,
				    rotate_pixmap_bg,
				    back_color);
				    
}

void
floating_widget_change_orient (FloatingWidget *floating,
			     PanelOrientType orient)
{
	FloatingPos *pos = FLOATING_POS (floating->pos);
	if (PANEL_WIDGET (BASEP_WIDGET (floating)->panel)->orient != orient) {
		BasePWidget *basep = BASEP_WIDGET (floating);
		PanelWidget *panel = PANEL_WIDGET (basep->panel);
		floating_widget_change_params (floating, 
					       pos->x, pos->y,
					       orient,
					       basep->mode,
					       basep->state,
					       panel->sz,
					       basep->hidebuttons_enabled,
					       basep->hidebutton_pixmaps_enabled,
					       panel->back_type,
					       panel->back_pixmap,
					       panel->fit_pixmap_bg,
					       panel->strech_pixmap_bg,
					       panel->rotate_pixmap_bg,
					       &panel->back_color);
	}
}

void
floating_widget_change_coords (FloatingWidget *floating,
			       gint16 x, gint16 y)
{
	FloatingPos *pos = FLOATING_POS (floating->pos);
	if (pos->x != x || pos->y != y) {
		BasePWidget *basep = BASEP_WIDGET (floating);
		PanelWidget *panel = PANEL_WIDGET (basep->panel);
		floating_widget_change_params (floating, 
					       x, y,
					       panel->orient,
					       basep->mode,
					       basep->state,
					       panel->sz,
					       basep->hidebuttons_enabled,
					       basep->hidebutton_pixmaps_enabled,
					       panel->back_type,
					       panel->back_pixmap,
					       panel->fit_pixmap_bg,
					       panel->strech_pixmap_bg,
					       panel->rotate_pixmap_bg,
					       &panel->back_color);
	}
}

GtkWidget *
floating_widget_new (gint16 x, gint16 y,
		     PanelOrientation orient,
		     BasePMode mode,
		     BasePState state,
		     int sz,
		     int hidebuttons_enabled,
		     int hidebutton_pixmap_enabled,
		     PanelBackType back_type,
		     char *back_pixmap,
		     gboolean fit_pixmap_bg,
		     gboolean strech_pixmap_bg,
		     gboolean rotate_pixmap_bg,
		     GdkColor *back_color)
{
	FloatingWidget *floating;
	FloatingPos *pos;

	floating = gtk_type_new (FLOATING_WIDGET_TYPE);
	floating->pos = gtk_type_new (FLOATING_POS_TYPE);
	pos = FLOATING_POS (floating->pos);

	pos->x = x;
	pos->y = y;

	basep_widget_construct (BASEP_WIDGET (floating),
				TRUE, FALSE,
				orient,
				sz, mode, state,
				hidebuttons_enabled,
				hidebutton_pixmap_enabled,
				back_type,
				back_pixmap,
				fit_pixmap_bg,
				strech_pixmap_bg,
				rotate_pixmap_bg,
				back_color);

	return GTK_WIDGET (floating);
}
