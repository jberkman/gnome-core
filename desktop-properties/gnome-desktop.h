#ifndef GNOME_DESKTOP_H
#define GNOME_DESKTOP_H

#include <gnome.h>
#include <libgnomeui/gnome-properties.h>

BEGIN_GNOME_DECLS

void register_extension (GtkWidget *title_widget, GtkWidget *content_widget,
			 int (*callback)(GnomePropertyRequest));

#define GNOME_MONITOR_WIDGET_X 20
#define GNOME_MONITOR_WIDGET_Y 10
#define GNOME_MONITOR_WIDGET_WIDTH 157
#define GNOME_MONITOR_WIDGET_HEIGHT 111

#define GNOME_PAD 10

GtkWidget *get_monitor_preview_widget (GtkWidget *window);


/* A property sheet should call this function when the user has made
   a change that could be applied.  */
void property_changed (void);

/* This is what an application's main() should call.  It isn't named
   main() since that doesn't always work well in libraries.  */
int property_main (int argc, char *argv[]);

/* This is supplied by the application.  It is called to register all
   the pages that make up the app.  FIXME: This will eventually go
   away and be replaced by a better, more configurable, method.  */
void application_register (GnomePropertyConfigurator *);

/* This is called when the Help button is clicked.  */
void application_help (void);

/* This is called to get the title of the main window.  */
char *application_title (void);

END_GNOME_DECLS

#endif
