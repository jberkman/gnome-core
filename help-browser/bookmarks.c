#include <stdio.h>
#include <string.h>
#include <time.h>

#include <gtk/gtk.h>
#include <glib.h>

#include "bookmarks.h"

struct _bookmarks_struct {
    GtkWidget *window;
    GtkWidget *clist;
    GHashTable *table;
    BookmarksCB callback;
    gpointer data;
    gchar *file;
};

struct _bookmarks_entry {
    /* Someday this will have more info :-) */
    gchar *ref;
};

static void createBookmarksWindow(Bookmarks h,
				GtkWidget **window, GtkWidget **clist);
static void mouseDoubleClick(GtkCList *clist, gint row, gint column,
			     gint button, Bookmarks h);
static void freeEntry(gchar *key, struct _bookmarks_entry *val, gpointer bar);
static int hideBookmarksInt(GtkWidget *window);
static void loadBookmarks(Bookmarks h);
static void appendEntry(Bookmarks h, gchar *ref);
static void removeBookmark(GtkWidget *w, Bookmarks b);

Bookmarks newBookmarks(BookmarksCB callback, gpointer data, gchar *file)
{
    gchar filename[BUFSIZ];
    Bookmarks res;

    res = g_new(struct _bookmarks_struct, 1);
    res->table = g_hash_table_new(g_str_hash, g_str_equal);
    res->callback = callback;
    res->data = data;

    if (file) {
	if (*(file) != '/') {
	    g_snprintf(filename, sizeof(filename), "%s/%s",
		       getenv("HOME"), file);
	} else {
	    strncpy(filename, file, sizeof(filename));
	}
	res->file = g_strdup(filename);
    } else {
	res->file = NULL;
    }

    createBookmarksWindow(res, &res->window, &res->clist);

    loadBookmarks(res);

    return res;
}

static void loadBookmarks(Bookmarks h)
{
    gchar buf[BUFSIZ];
    gchar ref[BUFSIZ];
    FILE *f;
    
    if (! h->file) {
	return;
    }

    if (!(f = fopen(h->file, "r"))) {
	return;
    }

    while (fgets(buf, sizeof(buf), f)) {
	sscanf(buf, "%s", ref);
	appendEntry(h, ref);
    }

    fclose(f);
}

void saveBookmarks(Bookmarks h)
{
    struct _bookmarks_entry *entry;
    gint x;
    FILE *f;
    
    if (! h->file) {
	return;
    }

    if (!(f = fopen(h->file, "w"))) {
	return;
    }

    x = 0;
    while (x < GTK_CLIST(h->clist)->rows) {
	entry = gtk_clist_get_row_data(GTK_CLIST(h->clist), x);
	fprintf(f, "%s\n", entry->ref);
	x++;
    }

    fclose(f);
}

static void appendEntry(Bookmarks h, gchar *ref)
{
    struct _bookmarks_entry *entry;
    gint x;
    
    entry = g_new(struct _bookmarks_entry, 1);
    entry->ref = g_strdup(ref);
    g_hash_table_insert(h->table, entry->ref, entry);

    x = gtk_clist_append(GTK_CLIST(h->clist), &ref);
    gtk_clist_set_row_data(GTK_CLIST(h->clist), x, entry);
}

static void freeEntry(gchar *key, struct _bookmarks_entry *val, gpointer bar)
{
    g_free(key);
    g_free(val);
}

void destroyBookmarks(Bookmarks h)
{
    /* Destroy the window */
    g_hash_table_foreach(h->table, (GHFunc)freeEntry, NULL);
    g_hash_table_destroy(h->table);
    if (h->file) {
	g_free(h->file);
    }
    g_free(h);
}

void addToBookmarks(Bookmarks h, gchar *ref)
{
    struct _bookmarks_entry *entry;
    
    entry = g_hash_table_lookup(h->table, ref);
    if (entry) {
	return;
    }

    appendEntry(h, ref);
}

static void mouseDoubleClick(GtkCList *clist, gint row, gint column,
			     gint button, Bookmarks h)
{
    struct _bookmarks_entry *entry;

    entry = gtk_clist_get_row_data(GTK_CLIST(clist), row);
    if (h->callback) {
	(h->callback)(entry->ref);
    }
}

static void removeBookmark(GtkWidget *w, Bookmarks b)
{
    gint row;
    GList *list;

    list = GTK_CLIST (b->clist)->selection;
    while (list)
      {
	row = (gint) list->data;
	list = list->next;
	
	gtk_clist_remove (GTK_CLIST (b->clist), row);
      }
}

void showBookmarks(Bookmarks h)
{
    gtk_widget_show(GTK_WIDGET(h->window));
}

void hideBookmarks(Bookmarks h)
{
    gtk_widget_hide(GTK_WIDGET(h->window));
}

static int hideBookmarksInt(GtkWidget *window)
{
    gtk_widget_hide(GTK_WIDGET(window));

    return FALSE;
}

static void createBookmarksWindow(Bookmarks h, GtkWidget **window,
				GtkWidget **clist)
{
    GtkWidget *box, *button;
    gchar *titles[1] = { "Bookmark" };

    /* Main Window */
    *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(*window), "Gnome Help Bookmarks");
    gtk_widget_set_usize (*window, 500, 200);

    /* Vbox */
    /* Don't need this now but might be used later.  I'll leave it */
    box = gtk_vbox_new(FALSE, 5);
    gtk_container_border_width (GTK_CONTAINER (box), 5);
    gtk_container_add(GTK_CONTAINER(*window), box);
    gtk_widget_show(box);

    /* Buttons */
    button = gtk_button_new_with_label("Remove");
    gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 0);
    gtk_widget_show(button);

    /* The clist */
    *clist = gtk_clist_new_with_titles(1, titles);
    gtk_clist_set_selection_mode(GTK_CLIST(*clist), GTK_SELECTION_SINGLE);
    gtk_clist_set_policy(GTK_CLIST(*clist),
			 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_clist_column_titles_show(GTK_CLIST(*clist));
    gtk_clist_column_titles_passive(GTK_CLIST(*clist));
    gtk_clist_set_column_justification(GTK_CLIST(*clist), 0,
				       GTK_JUSTIFY_LEFT);
    gtk_clist_set_column_width(GTK_CLIST(*clist), 0, 280);

    gtk_box_pack_start(GTK_BOX(box), *clist, TRUE, TRUE, 0);
    gtk_widget_show(*clist);

    /* Set callbacks */
    gtk_signal_connect(GTK_OBJECT (button), "clicked",
		       GTK_SIGNAL_FUNC(removeBookmark), h);
    gtk_signal_connect(GTK_OBJECT (*window), "destroy",
		       GTK_SIGNAL_FUNC(hideBookmarksInt), NULL);
    gtk_signal_connect(GTK_OBJECT (*window), "delete_event",
		       GTK_SIGNAL_FUNC(hideBookmarksInt), NULL);
    gtk_signal_connect_after(GTK_OBJECT(*clist), "select_row",
		       GTK_SIGNAL_FUNC(mouseDoubleClick), h);
}

