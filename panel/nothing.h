#ifndef NOTHING_H
#define NOTHING_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

void         start_screen_check      (void);
void	     start_geginv            (void);
gboolean     panel_dialog_window_event (GtkWidget *window,
					GdkEvent  *event);

G_END_DECLS

#endif
