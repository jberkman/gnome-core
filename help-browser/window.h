#ifndef _GNOME_HELP_BROWSER_WINDOW_H_
#define _GNOME_HELP_BROWSER_WINDOW_H_

#include <gtk/gtk.h>
#include <glib.h>

#include "queue.h"
#include "history.h"

typedef struct _helpWindow *HelpWindow;

/* Eventually this should return a window structure */
HelpWindow helpWindowNew(GtkSignalFunc about_cb);
void helpWindowShowURL(HelpWindow win, gchar *ref);
void helpWindowSetHistory(HelpWindow win, History history);

void helpWindowQueueAdd(HelpWindow w, gchar *ref);
void helpWindowHistoryAdd(HelpWindow w, gchar *ref);

gchar *helpWindowCurrentRef(HelpWindow w);

void helpWindowHTMLSource(HelpWindow w, gchar *s, char *ref);
void helpWindowJumpToAnchor(HelpWindow w, gchar *s);
void helpWindowJumpToLine(HelpWindow w, gint n);

#endif
