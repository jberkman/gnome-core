/* Gnome panel: sliding widget
 * (C) 1999 the Free Software Foundation
 * 
 * Authors:  Jacob Berkman
 *           George Lebl
 */

#include "sliding-widget.h"
#include "panel_config_global.h"

extern GlobalConfig global_config;
extern int pw_minimized_size;

static void sliding_pos_class_init (SlidingPosClass *klass);
static void sliding_pos_init (SlidingPos *pos);

static void sliding_pos_set_pos (BasePWidget *basep,
				 gint16 x, gint16 y,
				 guint16 w, guint16 h);
static void sliding_pos_get_pos (BasePWidget *basep,
				 gint16 *x, gint16 *y,
				 guint16 w, guint16 h);

static BorderPosClass *parent_class;

GtkType
sliding_pos_get_type ()
{
	static GtkType sliding_pos_type = 0;

	if (!sliding_pos_type) {
		GtkTypeInfo sliding_pos_info = {
			"SlidingPos",
			sizeof (SlidingPos),
			sizeof (SlidingPosClass),
			(GtkClassInitFunc) sliding_pos_class_init,
			(GtkObjectInitFunc) sliding_pos_init,
			NULL, NULL
		};

		sliding_pos_type = gtk_type_unique (BORDER_POS_TYPE,
						    &sliding_pos_info);
	}
			       
	return sliding_pos_type;
}

enum {
	ANCHOR_CHANGE_SIGNAL,
	OFFSET_CHANGE_SIGNAL,
	LAST_SIGNAL
};

static int sliding_pos_signals[LAST_SIGNAL] = { 0, 0 };

static void
sliding_pos_class_init (SlidingPosClass *klass)
{
	BasePPosClass *pos_class = BASEP_POS_CLASS(klass);
	GtkObjectClass *object_class = GTK_OBJECT_CLASS(klass);
	parent_class = gtk_type_class(BORDER_POS_TYPE);

	sliding_pos_signals[ANCHOR_CHANGE_SIGNAL] =
		gtk_signal_new("anchor_change",
			       GTK_RUN_LAST,
			       object_class->type,
			       GTK_SIGNAL_OFFSET(SlidingPosClass,
						 anchor_change),
			       gtk_marshal_NONE__ENUM,
			       GTK_TYPE_NONE,
			       1, GTK_TYPE_ENUM);

	sliding_pos_signals[OFFSET_CHANGE_SIGNAL] =
		gtk_signal_new("offset_change",
			       GTK_RUN_LAST,
			       object_class->type,
			       GTK_SIGNAL_OFFSET(SlidingPosClass,
						 offset_change),
			       gtk_marshal_NONE__INT,
			       GTK_TYPE_NONE,
			       2, GTK_TYPE_INT,
			       GTK_TYPE_INT);

	gtk_object_class_add_signals (object_class,
				      sliding_pos_signals,
				      LAST_SIGNAL);
	
	pos_class->set_pos = sliding_pos_set_pos;
	pos_class->get_pos = sliding_pos_get_pos;
}

static void
sliding_pos_init (SlidingPos *pos) { }

static void
sliding_pos_set_pos (BasePWidget *basep,
		     gint16 x, gint16 y,
		     guint16 w, guint16 h)
{
	int minx, miny, maxx, maxy;
	SlidingPos *pos = SLIDING_POS(basep->pos);
	BorderEdge newedge = BORDER_POS(basep->pos)->edge;
	SlidingAnchor newanchor = pos->anchor;
	gint16 newoffset = pos->offset;
	gboolean check_pos = TRUE;

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

	/*if in the inner 1/3rd, don't change to avoid fast flickery
	  movement*/
	if ( x>(gdk_screen_width()/3) &&
	     x<(2*gdk_screen_width()/3) &&
	     y>(gdk_screen_height()/3) &&
	     y<(2*gdk_screen_height()/3))
		return;
	
	/* don't switch the position if we are along the edge.
	   do this so that it won't flip-flop orientations in
	   the corners */
	switch (BORDER_POS (pos)->edge) {
	case BORDER_TOP:
		check_pos = (y > maxy);
		break;
	case BORDER_BOTTOM:
		check_pos = (y < miny);
		break;
	case BORDER_LEFT:
		check_pos = (x > maxx);
		break;
	case BORDER_RIGHT:
		check_pos = (x < minx);
		break;
	}

	if (check_pos) {
		if ((x) * gdk_screen_height() > y * gdk_screen_width() ) {
			if(gdk_screen_height() * (gdk_screen_width()-(x)) >
			   y * gdk_screen_width() )
				newedge = BORDER_TOP;
			else
				newedge = BORDER_RIGHT;
		} else {
			if(gdk_screen_height() * (gdk_screen_width()-(x)) >
			   y * gdk_screen_width() )
				newedge = BORDER_LEFT;
			else
				newedge = BORDER_BOTTOM;
		}

		/* we need to do this since the sizes might have changed 
		   (orientation changes and what not) */
		if(newedge != BORDER_POS(basep->pos)->edge) {
			border_widget_change_edge (BORDER_WIDGET(basep), newedge);
			basep_widget_get_size (basep, &w, &h);
		}
	}

	g_assert (newedge == BORDER_POS (pos)->edge);
	g_assert (newanchor == pos->anchor);

	switch (PANEL_WIDGET (basep->panel)->orient) {
	case PANEL_HORIZONTAL:
		if (x >= minx && x <= maxx)
			break; /* we are still "inside" the panel */
		newanchor =  (x < 0.1 * gdk_screen_width ()) 
			? SLIDING_ANCHOR_LEFT
			: ( (x > 0.9 * gdk_screen_width ())
			    ? SLIDING_ANCHOR_RIGHT
			    : newanchor);
		if (newanchor == SLIDING_ANCHOR_LEFT) {
			newoffset = (x > maxx) ? x - (maxx - minx) : x;
			if (basep->state == BASEP_HIDDEN_RIGHT)
				newoffset -= w - basep->hidebutton_e->allocation.width;
		} else {
			newoffset =gdk_screen_width () - ((x < minx) ? x + (maxx - minx) : x);
			if (basep->state == BASEP_HIDDEN_LEFT)
				newoffset -= w - basep->hidebutton_w->allocation.width;
		}
		newoffset = CLAMP (newoffset, 0, gdk_screen_width () - w);
		break;
	case PANEL_VERTICAL:
		if (y >= miny && y <= maxy)
			break; /* bleh */
		newanchor =  (y < 0.1 * gdk_screen_height ()) 
			? SLIDING_ANCHOR_LEFT
			: ( (y > 0.9 * gdk_screen_height ())
			    ? SLIDING_ANCHOR_RIGHT
			    : newanchor);
		if (newanchor == SLIDING_ANCHOR_LEFT) {
			newoffset = (y > maxy) ? y - (maxy - miny) : y;
			if (basep->state == BASEP_HIDDEN_RIGHT)
				newoffset -= h - basep->hidebutton_s->allocation.height;
		} else {
			newoffset = gdk_screen_height () - ((y < miny) ? y + (maxy - miny) : y);
			if (basep->state == BASEP_HIDDEN_LEFT)
				newoffset -= h - basep->hidebutton_n->allocation.height;
		}
		newoffset = CLAMP (newoffset, 0, gdk_screen_height () - h);
		break;
	}

	if (newanchor != pos->anchor) 
		sliding_widget_change_anchor (SLIDING_WIDGET (basep), 
					      newanchor);

	if (newoffset != pos->offset)
		sliding_widget_change_offset (SLIDING_WIDGET (basep),
					      newoffset);
}

static void
sliding_pos_get_pos (BasePWidget *basep, gint16 *x, gint16 *y,
		     guint16 w, guint16 h)
{
	SlidingPos *pos = SLIDING_POS (basep->pos);
	*x = *y = 0;

	switch (BORDER_POS (basep->pos)->edge) {
	case BORDER_BOTTOM:
		*y = gdk_screen_height () - h;
		/* fall through */
	case BORDER_TOP:
		*x = (pos->anchor == SLIDING_ANCHOR_LEFT)
			? pos->offset
			: gdk_screen_width () - pos->offset - w;
		break;
	case BORDER_RIGHT:
		*x = gdk_screen_width () - w;
                /* fall through */
	case BORDER_LEFT:
		*y = (pos->anchor == SLIDING_ANCHOR_LEFT)
			? pos->offset
			: gdk_screen_height () - pos->offset - h;
		break;
	}
}

GtkWidget *sliding_widget_new (SlidingAnchor anchor,
			       gint16 offset,
			       BorderEdge edge,
			       BasePMode mode,
			       BasePState state,
			       PanelSizeType sz,
			       int hidebuttons_enabled,
			       int hidebutton_pixmaps_enabled,
			       PanelBackType back_type,
			       char *back_pixmap,
			       int fit_pixmap_bg,
			       GdkColor *back_color)
{
	SlidingWidget *sliding = gtk_type_new (SLIDING_WIDGET_TYPE);
	SlidingPos *pos = gtk_type_new (SLIDING_POS_TYPE);

	pos->anchor = anchor;
	pos->offset = offset;
	
	BASEP_WIDGET (sliding)->pos = BASEP_POS (pos);

	border_widget_construct (BORDER_WIDGET (sliding),
				 edge, TRUE, FALSE,
				 sz, mode, state,
				 hidebuttons_enabled,
				 hidebutton_pixmaps_enabled,
				 back_type, back_pixmap,
				 fit_pixmap_bg, back_color);

	return GTK_WIDGET (sliding);
}

void 
sliding_widget_change_params (SlidingWidget *sliding,
			      SlidingAnchor anchor,
			      gint16 offset,
			      BorderEdge edge,
			      PanelSizeType sz,
			      BasePMode mode,
			      BasePState state,
			      int hidebuttons_enabled,
			      int hidebutton_pixmaps_enabled,
			      PanelBackType back_type,
			      char *pixmap_name,
			      int fit_pixmap_bg,
			      GdkColor *back_color)
{
	SlidingPos *pos = SLIDING_POS (BASEP_WIDGET (sliding)->pos);

	if (anchor != pos->anchor) {
		pos->anchor = anchor;
		gtk_signal_emit (GTK_OBJECT (pos),
				 sliding_pos_signals[ANCHOR_CHANGE_SIGNAL],
				 anchor);
		
	}

	if (offset != pos->offset) {
		pos->offset = offset;
		gtk_signal_emit (GTK_OBJECT (pos),
				 sliding_pos_signals[OFFSET_CHANGE_SIGNAL],
				 offset);
	}

	border_widget_change_params (BORDER_WIDGET (sliding),
				     edge, sz, mode, state,
				     hidebuttons_enabled,
				     hidebutton_pixmaps_enabled,
				     back_type, pixmap_name,
				     fit_pixmap_bg, back_color);
}

void
sliding_widget_change_offset (SlidingWidget *sliding, gint16 offset) {
	BasePWidget *basep = BASEP_WIDGET (sliding);
	PanelWidget *panel = PANEL_WIDGET (basep->panel);
	SlidingPos *pos = SLIDING_POS (basep->pos);

	if (offset == pos->offset)
		return;

	sliding_widget_change_params (sliding, pos->anchor, offset,
				      BORDER_POS (pos)->edge,
				      panel->sz, basep->mode,
				      basep->state,
				      basep->hidebuttons_enabled,
				      basep->hidebutton_pixmaps_enabled,
				      panel->back_type,
				      panel->back_pixmap,
				      panel->fit_pixmap_bg,
				      &panel->back_color);
}

void 
sliding_widget_change_anchor (SlidingWidget *sliding, SlidingAnchor anchor) 
{
	BasePWidget *basep = BASEP_WIDGET (sliding);
	PanelWidget *panel = PANEL_WIDGET (basep->panel);
	SlidingPos *pos = SLIDING_POS (basep->pos);

	if (anchor == pos->anchor)
		return;

	sliding_widget_change_params (sliding, anchor, pos->offset,
				      BORDER_POS (pos)->edge,
				      panel->sz, basep->mode,
				      basep->state,
				      basep->hidebuttons_enabled,
				      basep->hidebutton_pixmaps_enabled,
				      panel->back_type,
				      panel->back_pixmap,
				      panel->fit_pixmap_bg,
				      &panel->back_color);
}

void 
sliding_widget_change_anchor_offset_edge (SlidingWidget *sliding, 
					  SlidingAnchor anchor, 
					  gint16 offset,
					  BorderEdge edge) 
{
	BasePWidget *basep = BASEP_WIDGET (sliding);
	PanelWidget *panel = PANEL_WIDGET (basep->panel);

	sliding_widget_change_params (sliding, anchor, offset, edge,
				      panel->sz, basep->mode,
				      basep->state,
				      basep->hidebuttons_enabled,
				      basep->hidebutton_pixmaps_enabled,
				      panel->back_type,
				      panel->back_pixmap,
				      panel->fit_pixmap_bg,
				      &panel->back_color);
}
