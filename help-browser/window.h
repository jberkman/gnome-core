#ifndef _GNOME_HELP_BROWSER_WINDOW_H_
#define _GNOME_HELP_BROWSER_WINDOW_H_

#include <gtk/gtk.h>
#include <glib.h>

#include "queue.h"
#include "history.h"
#include "cache.h"
#include "toc2.h"

typedef struct _helpWindow *HelpWindow;

HelpWindow helpWindowNew(gchar *name,
			 GtkSignalFunc about_callback,
			 GtkSignalFunc new_window_callback,
			 GtkSignalFunc close_window_callback,
			 GHashFunc set_current_callback);
void helpWindowClose(HelpWindow win);
void helpWindowShowURL(HelpWindow win, gchar *ref);
void helpWindowSetHistory(HelpWindow win, History history);
void helpWindowSetToc(HelpWindow win, Toc toc);
Toc helpWindowGetToc(HelpWindow win);
void helpWindowSetCache(HelpWindow win, DataCache cache);
DataCache helpWindowGetCache(HelpWindow win);

void helpWindowQueueMark(HelpWindow w);
void helpWindowQueueAdd(HelpWindow w, gchar *ref);
void helpWindowHistoryAdd(HelpWindow w, gchar *ref);

gchar *helpWindowCurrentRef(HelpWindow w);

void helpWindowHTMLSource(HelpWindow w, gchar *s, char *ref);
void helpWindowJumpToAnchor(HelpWindow w, gchar *s);
void helpWindowJumpToLine(HelpWindow w, gint n);

#endif
