#include <gtk/gtk.h>
#include <glib.h>

#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>

#include "gnome-helpwin.h"

#include "toc2.h"
#include "toc2-man.h"
#include "toc2-ghelp.h"
#include "toc2-info.h"

static struct _toc_config *toc_config;

struct _toc {
    GList *manTable;
    GList *infoTable;
    GList *ghelpTable;

    GtkWidget *window;
    GtkWidget *htmlWidget;
    TocCB callback;
};

static struct _toc_config *addToConfig(struct _toc_config *index,
				       gchar *paths, gint type);
static gint countChars(gchar *s, gchar ch);
static void buildTocConfig(gchar *manPath, gchar *infoPath, gchar *ghelpPath);
static int hideTocInt(GtkWidget *window);
static GString *generateHTML(Toc res);
static void tocClicked(GtkWidget *w, XmHTMLAnchorCallbackStruct *cbs, Toc toc);
static GList *findFirstEntryByName(GList *table, gchar *name);

void showToc(Toc toc)
{
    gtk_widget_show(GTK_WIDGET(toc->window));
}

void hideToc(Toc toc)
{
    gtk_widget_hide(GTK_WIDGET(toc->window));
}

static int hideTocInt(GtkWidget *window)
{
    gtk_widget_hide(GTK_WIDGET(window));

    return FALSE;
}

static gint countChars(gchar *s, gchar ch)
{
    gint count = 0;
    
    while (s && *s) {
	if (*s == ch)
	    count++;
	s++;
    }
    
    return count;
}

static struct _toc_config *addToConfig(struct _toc_config *index,
				       gchar *paths, gint type)
{
    gchar buf[BUFSIZ];
    gchar *rest, *s;

    if (!paths) {
	return index;
    }

    strcpy(buf, paths);
    rest = buf;

    while ((s = strtok(rest, ":"))) {
	rest = NULL;
	index->path = g_strdup(s);
	index->type = type;
	index++;
    }

    return index;
}

static void buildTocConfig(gchar *manPath, gchar *infoPath, gchar *ghelpPath)
{
    gint count;
    struct _toc_config *index;

    count = 3;
    count += countChars(manPath, ':');
    count += countChars(infoPath, ':');
    count += countChars(ghelpPath, ':');

    toc_config = g_malloc(count * sizeof(*toc_config));
    index = toc_config;
    index = addToConfig(index, manPath, TOC_MAN_TYPE);
    index = addToConfig(index, infoPath, TOC_INFO_TYPE);
    index = addToConfig(index, ghelpPath, TOC_GHELP_TYPE);
    index->path = NULL;
    index->type = 0;
}

Toc newToc(gchar *manPath, gchar *infoPath, gchar *ghelpPath,
	   TocCB callback)
{
    Toc res;
    GString *s;

    buildTocConfig(manPath, infoPath, ghelpPath);
    
    res = g_malloc(sizeof(*res));
    res->manTable = newManTable(toc_config);
    res->ghelpTable = newGhelpTable(toc_config);
    res->infoTable = newInfoTable(toc_config);
    res->callback = callback;

    s = generateHTML(res);

    res->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(res->window), "Gnome Help TOC");
    gtk_widget_set_usize (res->window, 300, 200);

    res->htmlWidget = gnome_helpwin_new();
    gtk_widget_show(res->htmlWidget);

    gtk_signal_connect(GTK_OBJECT(res->htmlWidget), "activate",
		       GTK_SIGNAL_FUNC(tocClicked), res);

    gtk_container_add(GTK_CONTAINER(res->window), res->htmlWidget);
    
    gtk_signal_connect(GTK_OBJECT (res->window), "destroy",
		       GTK_SIGNAL_FUNC(hideTocInt), NULL);
    gtk_signal_connect(GTK_OBJECT (res->window), "delete_event",
		       GTK_SIGNAL_FUNC(hideTocInt), NULL);

    gtk_xmhtml_source(GTK_XMHTML(res->htmlWidget), s->str);
    
    g_string_free(s, TRUE);

    return res;
}

static void
tocClicked(GtkWidget *w, XmHTMLAnchorCallbackStruct *cbs, Toc toc)
{
	/* XXX should also have mime type info */
	if (toc->callback) {
	    (toc->callback)(cbs->href);
	}
}

static GString *generateHTML(Toc toc)
{
    GString *res, *s;
    GList *l;
    gchar *name;
    gchar *link;
    gchar ext, last_ext, last_initial;

    res = g_string_new("<h1>Table of Contents</h1>\n");

    /* Man Pages */
    
    g_string_append(res, "<h2>Man Pages</h2>\n");

    last_ext = ' ';
    last_initial = ' ';
    l = toc->manTable;
    while (l) {
	name = ((struct _big_table_entry *)l->data)->name;
	link = ((struct _big_table_entry *)l->data)->filename;
	ext = ((struct _big_table_entry *)l->data)->ext;

	if (ext != last_ext) {
	    g_string_append(res, "<p><br><h3>");
	    g_string_append(res, getManSection(ext));
	    g_string_append(res, "</h3><p>\n\n");
	} else if (last_initial != *name) {
	    g_string_append(res, "<p>\n");
	}
	
	s = g_string_new(NULL);
	/* XXX should also have mime type info */
	g_string_sprintf(s, "<a href=\"man:%s(%c)\">%s</a> ", name, ext, name);
	g_string_append(res, s->str);
	g_string_free(s, TRUE);

	last_initial = *name;
	last_ext = ext;
	l = g_list_next(l);
    }


    /* Info Pages */

    g_string_append(res, "<br><br><h2>Info Pages</h2>\n");

    l = toc->infoTable;
    while (l) {
	name = ((struct _big_table_entry *)l->data)->name;
	link = ((struct _big_table_entry *)l->data)->filename;

	s = g_string_new(NULL);
	/* XXX should also have mime type info */
	g_string_sprintf(s, "<a href=\"info:%s\">%s</a> ", name, name);
	g_string_append(res, s->str);
	g_string_free(s, TRUE);

	l = g_list_next(l);
    }


    /* Gnome Help */

    g_string_append(res, "<br><br><h2>GNOME Help</h2>\n");

    l = toc->ghelpTable;
    while (l) {
	name = ((struct _big_table_entry *)l->data)->name;
	link = ((struct _big_table_entry *)l->data)->filename;

	s = g_string_new(NULL);
	/* XXX should also have mime type info */
	g_string_sprintf(s, "<a href=\"ghelp:%s\">%s</a> ", name, name);
	g_string_append(res, s->str);
	g_string_free(s, TRUE);

	l = g_list_next(l);
    }


    return res;
}

gchar *tocLookupMan(Toc toc, gchar *name, gchar ext)
{
    GList *p;

    p = toc->manTable;
    
    if (ext != ' ') {
	while (p && ((struct _big_table_entry *)p->data)->ext != ext) {
	    p = p->next;
	}
    }

    if (!p) {
	return NULL;
    }

    while (p) {
	if (!strcmp(((struct _big_table_entry *)p->data)->name, name)) {
	    if ((ext == ' ') ||
		(((struct _big_table_entry *)p->data)->ext == ext)) {
		return ((struct _big_table_entry *)p->data)->filename;
	    }
	    return NULL;
	}
	p = p->next;
    }

    return NULL;
}

gchar *tocLookupGhelp(Toc toc, gchar *name)
{
    GList *p;
    
    p = findFirstEntryByName(toc->ghelpTable, name);
    if (!p) {
	return NULL;
    }
    
    return ((struct _big_table_entry *)p->data)->filename;
}

gchar *tocLookupInfo(Toc toc, gchar *name, gchar *anchor)
{
    GList *p;
    
    p = findFirstEntryByName(toc->infoTable, name);
    if (!p) {
	return NULL;
    }

    if (! ((struct _big_table_entry *)p->data)->expanded) {
	g_message("expanding info table: %s", name);
	if (expandInfoTable(p, name)) {
	    return NULL;
	}
    }

    if (! ((struct _big_table_entry *)p->data)->indirect) {
	return ((struct _big_table_entry *)p->data)->filename;
    }

    /* Yes this is all very inefficient */
    while (p) {
	if (((struct _big_table_entry *)p->data)->section &&
	    !strcmp(((struct _big_table_entry *)p->data)->section, anchor)) {
	    /* Make sure we are still looking at name */
	    if (!strcmp(((struct _big_table_entry *)p->data)->name, name)) {
		return ((struct _big_table_entry *)p->data)->filename;
	    }
	}
	p = p->next;
    }

    return NULL;
}

static GList *findFirstEntryByName(GList *table, gchar *name)
{
    while (table) {
	if (!strcmp(((struct _big_table_entry *)table->data)->name, name)) {
	    return table;
	}
	table = table->next;
    }

    return NULL;
}
