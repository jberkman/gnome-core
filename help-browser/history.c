#include <time.h>

#include <gtk/gtk.h>
#include <glib.h>

#include "history.h"

struct _history_struct {
    GtkWidget *window;
    GtkWidget *clist;
    GHashTable *table;
    GSearchFunc callback;
    gpointer data;
    gint length;
    gint internalSelectSkipThis;
};

struct _history_entry {
    gchar *ref;
    guint count;
    gint timestamp;
};

static void createHistoryWindow(History h,
				GtkWidget **window, GtkWidget **clist);
static void mouseDoubleClick(GtkCList *clist, gint row, gint column,
			     gint button, History h);
static void freeEntry(gchar *key, struct _history_entry *val, gpointer bar);
static int hideHistoryInt(GtkWidget *window);

History newHistory(gint length, GSearchFunc callback, gpointer data)
{
    History res;

    res = g_new(struct _history_struct, 1);
    res->length = length;
    res->table = g_hash_table_new(g_str_hash, g_str_equal);
    res->callback = callback;
    res->data = data;
    res->internalSelectSkipThis = 0;

    createHistoryWindow(res, &res->window, &res->clist);

    return res;
}

static void freeEntry(gchar *key, struct _history_entry *val, gpointer bar)
{
    g_free(key);
    g_free(val);
}

void destroyHistory(History h)
{
    /* XXX Save the history? */

    /* Destroy the window */
    g_hash_table_foreach(h->table, (GHFunc)freeEntry, NULL);
    g_hash_table_destroy(h->table);
    g_free(h);
}

void addToHistory(History h, gchar *ref)
{
    struct _history_entry *entry;
    gint row;
    gchar *text[3];
    gchar buf0[BUFSIZ];
    gchar buf1[BUFSIZ];
    gchar buf2[BUFSIZ];

    struct tm *tstruct;
    time_t timet;

    gtk_clist_freeze(GTK_CLIST(h->clist));
    
    entry = g_hash_table_lookup(h->table, ref);
    if (entry) {
	entry->count++;
    } else {
	entry = g_new(struct _history_entry, 1);
	entry->ref = g_strdup(ref);
	entry->count = 1;
	g_hash_table_insert(h->table, entry->ref, entry);
    }
    entry->timestamp = time(NULL);

    sprintf(buf0, "%s", ref);
    timet = (time_t)entry->timestamp;
    tstruct = localtime(& timet);
    strftime(buf1, sizeof(buf1), "%b %d, %Y %H:%M", tstruct);
    sprintf(buf2, "%d", entry->count);

    text[0] = buf0;
    text[1] = buf1;
    text[2] = buf2;
    
    row = gtk_clist_find_row_from_data(GTK_CLIST(h->clist), entry);
    if (row >= 0) {
	gtk_clist_remove(GTK_CLIST(h->clist), row);
    }
    gtk_clist_insert(GTK_CLIST(h->clist), 0, text);
    gtk_clist_set_row_data(GTK_CLIST(h->clist), 0, entry);
    h->internalSelectSkipThis = 1;
    gtk_clist_select_row(GTK_CLIST(h->clist), 0, 0);

    gtk_clist_thaw(GTK_CLIST(h->clist));
}

static void mouseDoubleClick(GtkCList *clist, gint row, gint column,
			     gint button, History h)
{
    struct _history_entry *entry;

    if (h->internalSelectSkipThis) {
	h->internalSelectSkipThis = 0;
	return;
    }
    
    entry = gtk_clist_get_row_data(GTK_CLIST(clist), row);
    if (h->callback) {
	(h->callback)(entry->ref, h->data);
    }
}

void showHistory(History h)
{
    gtk_widget_show(GTK_WIDGET(h->window));
}

void hideHistory(History h)
{
    gtk_widget_hide(GTK_WIDGET(h->window));
}

static int hideHistoryInt(GtkWidget *window)
{
    gtk_widget_hide(GTK_WIDGET(window));

    return FALSE;
}

static void createHistoryWindow(History h, GtkWidget **window,
				GtkWidget **clist)
{
    GtkWidget *box;
    gchar *titles[3] = { "URL", "Last", "Count" };

    /* Main Window */
    *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(*window), "Gnome Help History");
    gtk_widget_set_usize (*window, 500, 200);

    /* Vbox */
    /* Don't need this now but might be used later.  I'll leave it */
    box = gtk_vbox_new(FALSE, 5);
    gtk_container_border_width (GTK_CONTAINER (box), 5);
    gtk_container_add(GTK_CONTAINER(*window), box);
    gtk_widget_show(box);

    /* The clist */
    *clist = gtk_clist_new_with_titles(3, titles);
    gtk_clist_set_selection_mode(GTK_CLIST(*clist), GTK_SELECTION_SINGLE);
    gtk_clist_set_policy(GTK_CLIST(*clist),
			 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_clist_column_titles_show(GTK_CLIST(*clist));
    gtk_clist_column_titles_passive(GTK_CLIST(*clist));
    gtk_clist_set_column_justification(GTK_CLIST(*clist), 0,
				       GTK_JUSTIFY_LEFT);
    gtk_clist_set_column_justification(GTK_CLIST(*clist), 1,
				       GTK_JUSTIFY_CENTER);
    gtk_clist_set_column_justification(GTK_CLIST(*clist), 2,
				       GTK_JUSTIFY_RIGHT);
    gtk_clist_set_column_width(GTK_CLIST(*clist), 0, 280);
    gtk_clist_set_column_width(GTK_CLIST(*clist), 1, 120);
    gtk_clist_set_column_width(GTK_CLIST(*clist), 2, 50);

    gtk_box_pack_start(GTK_BOX(box), *clist, TRUE, TRUE, 0);
    gtk_widget_show(*clist);

    /* Set callbacks */
    gtk_signal_connect(GTK_OBJECT (*window), "destroy",
		       GTK_SIGNAL_FUNC(hideHistoryInt), NULL);
    gtk_signal_connect(GTK_OBJECT (*window), "delete_event",
		       GTK_SIGNAL_FUNC(hideHistoryInt), NULL);
    gtk_signal_connect_after(GTK_OBJECT(*clist), "select_row",
		       GTK_SIGNAL_FUNC(mouseDoubleClick), h);
}

