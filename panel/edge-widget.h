/* Gnome panel: edge (snapped) widget
 * (C) 1999 the Free Software Foundation
 *
 * Authors:  Jacob Berkman
 *           George Lebl
 */

#ifndef __EDGE_WIDGET_H__
#define __EDGE_WIDGET_H__

#include "border-widget.h"

BEGIN_GNOME_DECLS

/* even though edge_pos is currently structurally
   the same as border_pos, make it its own type 
   since we do need a seperate GtkType
*/

#define EDGE_POS_TYPE          (edge_pos_get_type ())
#define EDGE_POS(o)            (GTK_CHECK_CAST ((o), EDGE_POS_TYPE, EdgePos))
#define EDGE_POS_CLASS(k)      (GTK_CHECK_CLASS_CAST ((k), EDGE_POS_TYPE, EdgePosClass))
#define IS_EDGE_POS(o)         (GTK_CHECK_TYPE ((o), EDGE_POS_TYPE))
#define IS_EDGE_POS_CLASS(k)   (GTK_CHECK_CLASS_TYPE ((k), EDGE_POS_TYPE))

#define EDGE_WIDGET_TYPE       (BORDER_WIDGET_TYPE)
#define EDGE_WIDGET(o)         (BORDER_WIDGET(o))
#define EDGE_WIDGET_CLASS(k)   (BORDER_WIDGET_CLASS(k))
#define IS_EDGE_WIDGET(o)      (IS_BORDER_WIDGET(o) && IS_EDGE_POS(BASEP_WIDGET(o)->pos))
/* this is not reliable */
#define IS_EDGE_WIDGET_CLASS(k) (IS_BORDER_WIDGET_CLASS(k))

typedef BorderWidget            EdgeWidget;
typedef BorderWidgetClass       EdgeWidgetClass;

typedef struct _EdgePos         EdgePos;
typedef struct _EdgePosClass    EdgePosClass;

struct _EdgePos {
	BorderPos pos;
};

struct _EdgePosClass {
	BorderPosClass parent_class;
};

GtkType edge_pos_get_type (void);
GtkWidget *edge_widget_new (BorderEdge edge,
			    BasePMode mode,
			    BasePState state,
			    PanelSizeType sz,
			    int hidebuttons_enabled,
			    int hidebutton_pixmaps_enabled,
			    PanelBackType back_type,
			    char *back_pixmap,
			    int fit_pixmap_bg,
			    GdkColor *back_color);

void edge_widget_change_params (EdgeWidget *edgew,
				BorderEdge edge,
				BasePMode mode,
				BasePState state,
				PanelSizeType sz,
				int hidebuttons_enabled,
				int hidebutton_pixmaps_enabled,
				PanelBackType back_type,
				char *pixmap_name,
				int fit_pixmap_bg,
				GdkColor *back_color);

END_GNOME_DECLS

#endif
