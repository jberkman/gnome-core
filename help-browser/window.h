#ifndef _GNOME_HELP_BROWSER_WINDOW_H_
#define _GNOME_HELP_BROWSER_WINDOW_H_

#include <gtk/gtk.h>
#include <glib.h>

#include "queue.h"

typedef struct _helpWindow *HelpWindow;

/* Eventually this should return a window structure */
HelpWindow newWindow(gchar *ref);

Queue helpWindowQueue(HelpWindow w);
GtkWidget *helpWindowWidget(HelpWindow w);

#endif
