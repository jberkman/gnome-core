#ifndef GNOME_DESKTOP_H
#define GNOME_DESKTOP_H

#include <gnome.h>
#include <libgnomeui/gnome-properties.h>

BEGIN_GNOME_DECLS

void register_extension (GtkWidget *title_widget, GtkWidget *content_widget,
			 int (*callback)(GnomePropertyRequest));

#define GNOME_PAD 10

/* Keep these in sync with the monitor image */

#define MONITOR_CONTENTS_X 20
#define MONITOR_CONTENTS_Y 10
#define MONITOR_CONTENTS_WIDTH 157
#define MONITOR_CONTENTS_HEIGHT 111

GtkWidget *get_monitor_preview_widget (void);

/* A property sheet should call this function when the user has made
   a change that could be applied.  */
void property_changed (void);

/* This is what an application's main() should call.  It isn't named
   main() since that doesn't always work well in libraries.  */
int property_main (char *app_id, int argc, char *argv[]);

/* This is supplied by the application.  It is called to register all
   the pages that make up the app.  FIXME: This will eventually go
   away and be replaced by a better, more configurable, method.  */
void application_register (GnomePropertyConfigurator *);

/* This is called when the Help button is clicked.  */
void application_help (void);

/* This is called to get the title of the main window.  */
char *application_title (void);

/* This is called to get the name of the property that is used as a
   mutex.  */
char *application_property (void);

END_GNOME_DECLS

#endif
