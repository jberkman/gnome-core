#ifndef _GNOME_HELP_BOOKMARKS_H_
#define _GNOME_HELP_BOOKMARKS_H_

#include <gtk/gtk.h>
#include <glib.h>

typedef struct _bookmarks_struct *Bookmarks;

typedef void (*BookmarksCB) (gchar *ref);

Bookmarks newBookmarks(BookmarksCB callback, gpointer data, gchar *file);
void destroyBookmarks(Bookmarks h);
void addToBookmarks(Bookmarks h, gchar *ref);
void showBookmarks(Bookmarks h);
void hideBookmarks(Bookmarks h);
void saveBookmarks(Bookmarks h);

#endif _GNOME_HELP_BOOKMARKS_H_
