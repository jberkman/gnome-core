#ifndef GNOME_DESKTOP_H
#define GNOME_DESKTOP_H

#include <gnome.h>

BEGIN_GNOME_DECLS

extern GtkWidget *main_window;

/* A property sheet should call this function when the user has made
   a change that could be applied.  */
void property_changed (void);

/* A property sheet should call this function when the user has
   clicked on the apply button. */
void property_applied (void);

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

extern int init;

extern int need_sync;

/* Options used by this program.  */
extern const struct poptOption parser[];

END_GNOME_DECLS

#endif
