/* Gnome panel: edge (snapped) widget
 * (C) 1999 the Free Software Foundation
 *
 * Authors:  Jacob Berkman
 *           George Lebl
 */

#include "config.h"
#include "edge-widget.h"
#include "panel-config-global.h"
#include "foobar-widget.h"
#include "multiscreen-stuff.h"

extern GlobalConfig global_config;
extern int pw_minimized_size;

static void edge_pos_class_init (EdgePosClass *klass);
static void edge_pos_instance_init (EdgePos *pos);

static void edge_pos_set_pos (BasePWidget *basep,
			      int x, int y,
			      int w, int h,
			      gboolean force);
static void edge_pos_get_pos (BasePWidget *basep,
			      int *x, int *y,
			      int w, int h);

static void edge_pos_get_size (BasePWidget *basep,
			       int *w, int *h);

static void edge_pos_pre_convert_hook (BasePWidget *basep);

static BorderPosClass *edge_pos_parent_class;

GType
edge_pos_get_type (void)
{
	static GType object_type = 0;

	if (object_type == 0) {
		static const GTypeInfo object_info = {
                    sizeof (EdgePosClass),
                    (GBaseInitFunc)         NULL,
                    (GBaseFinalizeFunc)     NULL,
                    (GClassInitFunc)        edge_pos_class_init,
                    NULL,                   /* class_finalize */
                    NULL,                   /* class_data */
                    sizeof (EdgePos),
                    0,                      /* n_preallocs */
                    (GInstanceInitFunc)     edge_pos_instance_init
		};

		object_type = g_type_register_static (BORDER_TYPE_POS, "EdgePos", &object_info, 0);
		edge_pos_parent_class = g_type_class_ref (BORDER_TYPE_POS);
	}
			       
	return object_type;
}

static void
edge_pos_class_init (EdgePosClass *klass)
{
	BasePPosClass *pos_class = BASEP_POS_CLASS(klass);

	pos_class->set_pos = edge_pos_set_pos;
	pos_class->get_pos = edge_pos_get_pos;
	pos_class->get_size = edge_pos_get_size;
	pos_class->pre_convert_hook = edge_pos_pre_convert_hook;
}

static void
edge_pos_instance_init (EdgePos *pos) { }

static void
edge_pos_set_pos (BasePWidget *basep,
		  int x, int y,
		  int w, int h,
		  gboolean force)
{
	BorderEdge newloc;
	int innerx, innery;
	int screen_width, screen_height;
	
	if ( ! force) {
		int minx, miny, maxx, maxy;

		gdk_window_get_geometry (GTK_WIDGET (basep)->window,
					 &minx, &miny, &maxx, &maxy, NULL);
		gdk_window_get_origin (GTK_WIDGET (basep)->window,
				       &minx, &miny);

		newloc = BORDER_POS(basep->pos)->edge;

		maxx += minx;
		maxy += miny;

		if (x >= minx && 
		    x <= maxx &&
		    y >= miny &&
		    y <= maxy)
			return;
	}
	
	innerx = x - multiscreen_x (basep->screen);
	innery = y - multiscreen_y (basep->screen);
	screen_width = multiscreen_width (basep->screen);
	screen_height = multiscreen_height (basep->screen);

	if ( innerx > (screen_width / 3) &&
	     innerx < (2*screen_width / 3) &&
	     innery > (screen_height / 3) &&
	     innery < (2*screen_height / 3))
		return;

	if (innerx * screen_height > innery * screen_width) {
		if (screen_height * (screen_width - innerx) >
		    innery * screen_width)
			newloc = BORDER_TOP;
		else
			newloc = BORDER_RIGHT;
	} else {
		if (screen_height * (screen_width - innerx) >
		    innery * screen_width)
			newloc = BORDER_LEFT;
		else
			newloc = BORDER_BOTTOM;
	}
	if (newloc != BORDER_POS (basep->pos)->edge)
		border_widget_change_edge (BORDER_WIDGET (basep), newloc);
}

static void
edge_pos_get_pos (BasePWidget *basep, int *x, int *y,
		  int w, int h)
{
	BorderEdge edge;

	*x = *y = 0;

	edge = BORDER_POS(basep->pos)->edge;

	switch (edge) {
	case BORDER_RIGHT:
		basep_border_get (basep->screen, BORDER_TOP, NULL, NULL, y);
		*y += foobar_widget_get_height (basep->screen);
		*x = multiscreen_width(basep->screen) - w;

		break;
	case BORDER_LEFT:
		basep_border_get (basep->screen, BORDER_TOP, y, NULL, NULL);
		*y += foobar_widget_get_height (basep->screen);
		break;
	case BORDER_TOP:
		*y = foobar_widget_get_height (basep->screen);
		break;
	case BORDER_BOTTOM:
		*y = multiscreen_height(basep->screen) - h;
		break;
	}

	*x += multiscreen_x (basep->screen);
	*y += multiscreen_y (basep->screen);

	basep_border_queue_recalc (basep->screen);
}

static void
edge_pos_get_size (BasePWidget *basep, int *w, int *h)
{
	int a, b;

	BorderEdge edge = BORDER_POS(basep->pos)->edge;

	switch (edge) {
	case BORDER_RIGHT:
		basep_border_get (basep->screen, BORDER_TOP, NULL, NULL, &a);
		basep_border_get (basep->screen, BORDER_BOTTOM, NULL, NULL, &b);
		*h = multiscreen_height (basep->screen) - foobar_widget_get_height (basep->screen) - a - b;
		break;
	case BORDER_LEFT:
		basep_border_get (basep->screen, BORDER_TOP, &a, NULL, NULL);
		basep_border_get (basep->screen, BORDER_BOTTOM, &b, NULL, NULL);
		*h = multiscreen_height (basep->screen) - foobar_widget_get_height (basep->screen) - a - b;
		break;
	case BORDER_TOP:
	case BORDER_BOTTOM:
		*w = multiscreen_width (basep->screen);
		break;
	}
}

static void
edge_pos_pre_convert_hook (BasePWidget *basep)
{
	basep->keep_in_screen = TRUE;
	PANEL_WIDGET (basep->panel)->packed = FALSE;
}

GtkWidget *
edge_widget_new (gchar *panel_id,
		 int screen,
		 BorderEdge edge,
		 BasePMode mode,
		 BasePState state,
		 int sz,
		 gboolean hidebuttons_enabled,
		 gboolean hidebutton_pixmaps_enabled,
		 PanelBackType back_type,
		 char *back_pixmap,
		 gboolean fit_pixmap_bg,
		 gboolean stretch_pixmap_bg,
		 gboolean rotate_pixmap_bg,
		 GdkColor *back_color)
{
	EdgeWidget *edgew = g_object_new (EDGE_TYPE_WIDGET, NULL);
	BasePWidget *basep = BASEP_WIDGET (edgew);

	basep->pos = g_object_new (EDGE_TYPE_POS, NULL);

	border_widget_construct (panel_id,
				 BORDER_WIDGET (basep), 
				 screen,
				 edge, 
				 TRUE, FALSE,
				 sz, mode, state,
				 hidebuttons_enabled,
				 hidebutton_pixmaps_enabled,
				 back_type, back_pixmap,
				 fit_pixmap_bg, stretch_pixmap_bg,
				 rotate_pixmap_bg,
				 back_color);

	PANEL_WIDGET (basep->panel)->packed = FALSE;
	
	return GTK_WIDGET (edgew);
}
