#ifndef GNOME_DESKTOP_H
#define GNOME_DESKTOP_H

#include "gnome.h"
#include "libgnomeui/gnome-properties.h"

BEGIN_GNOME_DECLS

void register_extension (GtkWidget *title_widget, GtkWidget *content_widget,
			 int (*callback)(GnomePropertyRequest));

#define GNOME_MONITOR_WIDGET_X 20
#define GNOME_MONITOR_WIDGET_Y 10
#define GNOME_MONITOR_WIDGET_WIDTH 157
#define GNOME_MONITOR_WIDGET_HEIGHT 111

#define GNOME_PAD 10

GtkWidget *get_monitor_preview_widget (GtkWidget *window);

END_GNOME_DECLS

#endif
