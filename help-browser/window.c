/* This file should encapsulate all the HTML widget functionality. */
/* No other files should be accessing HTML widget functions!       */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <gnome.h>
#include <libgnomeui/gnome-app.h>
#include <libgnome/gnome-help.h>

#include "gnome-helpwin.h"

#include "parseUrl.h"
#include "window.h"
#include "history.h"
#include "bookmarks.h"
#include "toc2.h"
#include "docobj.h"
#include "queue.h"
#include "visit.h"
#include "misc.h"

/* Toolbar pixmaps */
#include "close.xpm"
#include "right_arrow.xpm"
#include "left_arrow.xpm"
#include "contents.xpm"
#include "help.xpm"

struct _helpWindow {
    /* Main app widget */
    GtkWidget *app;

    /* Toolbar widgets - needed to set button states */
    GtkWidget *tb_contents, *tb_back, *tb_forw;

    /* The HTML widget */
    GtkWidget *helpWidget;

    /* The forward/backward queue */
    Queue queue;

    /* The current page reference */
    gchar *currentRef;

    /* The entry box that shows the URL */
    GtkWidget *entryBox;

    /* status bar */
    GtkWidget *statusBar;

    /* Passed to us by the main program */
    GtkSignalFunc about_cb;
    GtkSignalFunc new_window_cb;
    GHashFunc close_window_cb;
    GHashFunc set_current_cb;
    History history;
    Toc toc;
    DataCache cache;
    Bookmarks bookmarks;
};


static GtkWidget * makeEntryArea(HelpWindow w);

/* Callbacks */
static void quit_cb(void);
static void about_cb (GtkWidget *w, HelpWindow win);
static void bookmark_cb (GtkWidget *w, HelpWindow win);
static void close_cb (GtkWidget *w, HelpWindow win);
static void delete_cb (GtkWidget *w, void *foo, HelpWindow win);
static void new_window_cb (GtkWidget *w, HelpWindow win);
static void help_forward(GtkWidget *w, HelpWindow win);
static void help_backward(GtkWidget *w, HelpWindow win);
static void help_contents(GtkWidget *w, HelpWindow win);
static void help_onhelp(GtkWidget *w, HelpWindow win);
static void xmhtml_activate(GtkWidget *w, XmHTMLAnchorCallbackStruct *cbs,
			    HelpWindow win);
static void anchorTrack(GtkWidget *w, XmHTMLAnchorCallbackStruct *cbs,
			    HelpWindow win);
static void ghelpShowHistory (GtkWidget *w, HelpWindow win);
static void ghelpShowBookmarks (GtkWidget *w, HelpWindow win);
static void ghelpShowToc (GtkWidget *w, HelpWindow win);
static void entryChanged(GtkWidget *w, HelpWindow win);
static void setCurrent(HelpWindow w);

static void init_toolbar(HelpWindow w);
static void update_toolbar(HelpWindow w);

XmImageInfo *load_image(GtkWidget *html_widget, gchar *ref);



/**********************************************************************/



/**********************************************************************/

/* Menu and toolbar structures */

GnomeUIInfo filemenu[] = {
    GNOMEUIINFO_ITEM("New window", "Open new browser window",
		     new_window_cb, NULL),
    GNOMEUIINFO_ITEM("Add Bookmark", "Add Bookmark", bookmark_cb, NULL),
    GNOMEUIINFO_ITEM("Close", "Close window", close_cb, NULL),
    GNOMEUIINFO_ITEM("Exit", "Exit program", quit_cb, NULL),
    GNOMEUIINFO_END
};

GnomeUIInfo helpmenu[] = {
    GNOMEUIINFO_ITEM("About...", "Info about this program", about_cb, NULL),
    GNOMEUIINFO_SEPARATOR,
    GNOMEUIINFO_HELP("help-browser"),
    GNOMEUIINFO_END
};
 
GnomeUIInfo windowmenu[] = {
    GNOMEUIINFO_ITEM("History", "Show History Window",
		     ghelpShowHistory, NULL),
    GNOMEUIINFO_ITEM("Table of Contents", "Show Table of Contents Window",
		     ghelpShowToc, NULL),
    GNOMEUIINFO_ITEM("Bookmarks", "Show Bookmarks Window",
		     ghelpShowBookmarks, NULL),
    GNOMEUIINFO_END
};

GnomeUIInfo mainmenu[] = {
    GNOMEUIINFO_SUBTREE("File", filemenu),
    GNOMEUIINFO_SUBTREE("Window", windowmenu),
    GNOMEUIINFO_SUBTREE("Help", helpmenu),
    GNOMEUIINFO_END
};

GnomeUIInfo toolbar[] = {
    GNOMEUIINFO_ITEM("Contents", "Show Contents", help_contents, contents_xpm),
    GNOMEUIINFO_SEPARATOR,
    GNOMEUIINFO_ITEM("Back", "Go to the previous location in the history list",
		     help_backward, right_arrow_xpm),
    GNOMEUIINFO_ITEM("Forward", "Go to the next location in the history list",
		     help_forward, left_arrow_xpm),
    GNOMEUIINFO_SEPARATOR,
    GNOMEUIINFO_ITEM("Help", "Help on Help", help_onhelp, help_xpm),
    GNOMEUIINFO_ITEM("Close", "Close Help Window", close_cb, close_xpm),
    GNOMEUIINFO_SEPARATOR,
    GNOMEUIINFO_ITEM("History", "Show History Window",
		     ghelpShowHistory, contents_xpm),
    GNOMEUIINFO_ITEM("TOC", "Show Table of Contents Window",
		     ghelpShowToc, contents_xpm),
    GNOMEUIINFO_ITEM("BMarks", "Show Bookmarks Window",
		     ghelpShowBookmarks, contents_xpm),
    GNOMEUIINFO_END
};

/**********************************************************************/


/**********************************************************************/

/* Callbacks */

static void
about_cb (GtkWidget *w, HelpWindow win)
{
    if (win->about_cb)
	(win->about_cb)();
}

static void
bookmark_cb (GtkWidget *w, HelpWindow win)
{
    if (win->bookmarks)
	addToBookmarks(win->bookmarks, win->currentRef);
}

static void
new_window_cb (GtkWidget *w, HelpWindow win)
{
    if (win->new_window_cb)
	(win->new_window_cb)();
}

static void
delete_cb (GtkWidget *w, void *foo, HelpWindow win)
{
    if (win->close_window_cb)
	(win->close_window_cb)(win);
}

static void
close_cb (GtkWidget *w, HelpWindow win)
{
    if (win->close_window_cb)
	(win->close_window_cb)(win);
}

static void
ghelpShowHistory (GtkWidget *w, HelpWindow win)
{
    showHistory(win->history);
}

static void
ghelpShowBookmarks (GtkWidget *w, HelpWindow win)
{
    showBookmarks(win->bookmarks);
}

static void
ghelpShowToc (GtkWidget *w, HelpWindow win)
{
    showToc(win->toc);
}

static void
quit_cb (void)
{
    gtk_main_quit ();
    return;
}

static void
xmhtml_activate(GtkWidget *w, XmHTMLAnchorCallbackStruct *cbs, HelpWindow win)
{
        g_message("TAG CLICKED: %s", cbs->href);

	helpWindowShowURL(win, cbs->href);

	update_toolbar(win);
	setCurrent(win);
}

static void
anchorTrack(GtkWidget *w, XmHTMLAnchorCallbackStruct *cbs, HelpWindow win)
{
	gtk_statusbar_pop(GTK_STATUSBAR(win->statusBar), 1);
	if (cbs->href) {
		gtk_statusbar_push(GTK_STATUSBAR(win->statusBar),1,cbs->href);
	}
}

static void
help_forward(GtkWidget *w, HelpWindow win)
{
	gchar *ref;
	gint pos;

	if (!(ref = queue_next(win->queue, &pos)))
		return;

	visitURL_nohistory(win, ref);
	queue_move_next(win->queue);
	
	g_message("jump to line: %d", pos);
	gnome_helpwin_jump_to_line(GNOME_HELPWIN(win->helpWidget), pos);

	update_toolbar(win);
	setCurrent(win);
}

static void
help_backward(GtkWidget *w, HelpWindow win)
{
	gchar *ref;
	gint pos;

	if (!(ref = queue_prev(win->queue, &pos)))
		return;

	visitURL_nohistory(win, ref);
	queue_move_prev(win->queue);
	
	g_message("jump to line: %d", pos);
	gnome_helpwin_jump_to_line(GNOME_HELPWIN(win->helpWidget), pos);

	update_toolbar(win);
	setCurrent(win);
}

static void
help_contents(GtkWidget *w, HelpWindow win)
{
	return;
}

static void
help_onhelp(GtkWidget *w, HelpWindow win)
{
	gchar *p, *q;

	p = gnome_help_file_path("help-browser", "help-browser.html");
	if (!p)
		return;
	q = alloca(strlen(p)+10);
	strcpy(q,"file:");
	strcat(q, p);
	g_free(p);
	helpWindowShowURL(win, q);

	setCurrent(win);
}

static void
entryChanged(GtkWidget *w, HelpWindow win)
{
    g_message("ENTRY BOX: %s", gtk_entry_get_text(GTK_ENTRY(w)));
    helpWindowShowURL(win, gtk_entry_get_text(GTK_ENTRY(w)));

    setCurrent(win);
}

/**********************************************************************/



/**********************************************************************/

/* Misc static routines */

static void
setCurrent(HelpWindow w)
{
    if (w->set_current_cb) {
	(w->set_current_cb)(w);
    }
}

static void
init_toolbar(HelpWindow w)
{
	toolbar[5].user_data = w->helpWidget;
	toolbar[2].user_data = w->helpWidget;
	toolbar[3].user_data = w->helpWidget;
	
	gnome_app_create_toolbar_with_data(GNOME_APP(w->app), toolbar, w);

	w->tb_contents = toolbar[0].widget;
	w->tb_back = toolbar[2].widget;
	w->tb_forw = toolbar[3].widget;
	
	update_toolbar(w);
}

static void
update_toolbar(HelpWindow w)
{
	/* we dont have mapping for 'contents' button yet */
	if (w->tb_contents)
		gtk_widget_set_sensitive(w->tb_contents, 0);
	if (w->tb_back)
		gtk_widget_set_sensitive(w->tb_back, queue_isprev(w->queue));
	if (w->tb_forw)
		gtk_widget_set_sensitive(w->tb_forw, queue_isnext(w->queue));
}

static GtkWidget *
makeEntryArea(HelpWindow w)
{
    GtkWidget *handleBox, *hbox, *label, *entry;
    
    handleBox = gtk_handle_box_new();

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_widget_show(hbox);
    
    label = gtk_label_new("URL: ");
    gtk_widget_show(label);
    
    entry = gnome_entry_new(NULL);
    gtk_widget_show(entry);
    gtk_signal_connect(GTK_OBJECT(GTK_COMBO(entry)->entry),
		       "activate", (GtkSignalFunc)entryChanged, w);
    
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(handleBox), hbox);

    w->entryBox = GTK_COMBO(entry)->entry;
    
    return handleBox;
}


/**********************************************************************/


/**********************************************************************/

/* Public functions */

void
helpWindowQueueMark(HelpWindow w)
{
    gint pos;

    pos = gnome_helpwin_get_line(GNOME_HELPWIN(w->helpWidget));
    g_message("get_line = %d", pos);
    
    queue_mark_current(w->queue, pos ? pos : 1);
}

void
helpWindowQueueAdd(HelpWindow w, gchar *ref)
{
    queue_add(w->queue, ref, 1);
}

gchar
*helpWindowCurrentRef(HelpWindow w)
{
    return w->currentRef;
}

void
helpWindowHistoryAdd(HelpWindow w, gchar *ref)
{
    addToHistory(w->history, ref);
}

void
helpWindowHTMLSource(HelpWindow w, gchar *s, gchar *ref)
{
    /* First set the current ref (it may be used to load images) */
    if (w->currentRef) {
	g_free(w->currentRef);
    }
    w->currentRef = g_strdup(ref);
    gtk_entry_set_text(GTK_ENTRY(w->entryBox), ref);

    /* Load it up */
    gtk_xmhtml_source(GTK_XMHTML(w->helpWidget), s);
}

void
helpWindowJumpToAnchor(HelpWindow w, gchar *s)
{
    gnome_helpwin_jump_to_anchor(GNOME_HELPWIN(w->helpWidget), s);
}

void
helpWindowJumpToLine(HelpWindow w, gint n)
{
    gnome_helpwin_jump_to_line(GNOME_HELPWIN(w->helpWidget), n);
}

void
helpWindowClose(HelpWindow win)
{
    gtk_widget_destroy(win->app);

    if (win->currentRef)
	g_free(win->currentRef);
    queue_free(win->queue);
    g_free(win);
}

HelpWindow
helpWindowNew(gchar *name,
	      GtkSignalFunc about_callback,
	      GtkSignalFunc new_window_callback,
	      GtkSignalFunc close_window_callback,
	      GHashFunc set_current_callback)
{
        HelpWindow w;
	GtkWidget *entryArea;
	GtkWidget *vbox;

	w = (HelpWindow)g_malloc(sizeof(*w));

	w->queue= queue_new();
	w->about_cb = about_callback;
	w->new_window_cb = new_window_callback;
	w->close_window_cb = (GHashFunc)close_window_callback;
	w->set_current_cb = set_current_callback;
	w->history = NULL;
	w->bookmarks = NULL;
	w->cache = NULL;
	w->currentRef = NULL;

	w->app = gnome_app_new (name, "Gnome Help Browser");
	gtk_widget_realize (w->app);

	gtk_signal_connect (GTK_OBJECT (w->app), "delete_event",
			    GTK_SIGNAL_FUNC (delete_cb), w);

	gnome_app_create_menus_with_data(GNOME_APP(w->app), mainmenu, w);

	/* do the toolbar */
	init_toolbar(w);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_border_width (GTK_CONTAINER (vbox), 0);
	gtk_widget_show(vbox);

	entryArea = makeEntryArea(w);
	gtk_widget_show(entryArea);
	gtk_box_pack_start(GTK_BOX(vbox), entryArea, FALSE, FALSE, 0);

	/* make the help window */
	w->helpWidget = gnome_helpwin_new();
	gtk_widget_show(w->helpWidget);

	/* add a status bar */
	w->statusBar = gtk_statusbar_new();
	gtk_widget_show(w->statusBar);

	/* trap clicks on tags so we can stick requested link in browser */
	gtk_signal_connect(GTK_OBJECT(w->helpWidget), "activate",
			   GTK_SIGNAL_FUNC(xmhtml_activate), w);

	gtk_signal_connect(GTK_OBJECT(w->helpWidget), "anchor_track",
					     GTK_SIGNAL_FUNC(anchorTrack), w);

	/* size should be auto-determined, or read from gnomeconfig() */
	gtk_widget_set_usize(GTK_WIDGET(w->helpWidget), 800, 600);

	gtk_box_pack_start(GTK_BOX(vbox), w->helpWidget, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), w->statusBar, FALSE, FALSE, 0);
	gnome_app_set_contents(GNOME_APP(w->app), vbox);

	/* HACKHACKHACK this will grab images via http */
	gtk_object_set_data(GTK_OBJECT(w->helpWidget), "HelpWindow", w);
	gtk_xmhtml_set_image_procs(GTK_XMHTML(w->helpWidget),
				   (XmImageProc)load_image,
				   NULL,NULL,NULL);

	gtk_widget_show(w->app);

	return w;
}

void
helpWindowSetHistory(HelpWindow win, History history)
{
    win->history = history;
}

void
helpWindowSetBookmarks(HelpWindow win, Bookmarks bookmarks)
{
    win->bookmarks = bookmarks;
}

void
helpWindowSetToc(HelpWindow win, Toc toc)
{
    win->toc = toc;
}

Toc
helpWindowGetToc(HelpWindow win)
{
    return win->toc;
}

void
helpWindowSetCache(HelpWindow win, DataCache cache)
{
    win->cache = cache;
}

DataCache
helpWindowGetCache(HelpWindow win)
{
    return win->cache;
}

void
helpWindowShowURL(HelpWindow win, gchar *ref)
{
	gchar err[1024];

	if (visitURL(win, ref)) {
		GtkWidget *msg;

		snprintf(err, sizeof(err), "Error loading document:\n\n%s",
			 ref);
		msg = gnome_messagebox_new(err, GNOME_MESSAGEBOX_ERROR,
					   "Ok", NULL);
		gnome_messagebox_set_modal (GNOME_MESSAGEBOX (msg));
		gtk_widget_show(msg);

		gtk_entry_set_text(GTK_ENTRY(win->entryBox), win->currentRef);
		return;
	} else {
		/* clear the status bar */
		gtk_statusbar_pop(GTK_STATUSBAR(win->statusBar), 1);
	}
	update_toolbar(win);
	setCurrent(win);
}

/**********************************************************************/


/* HACK HACK HACK */
XmImageInfo *
load_image(GtkWidget *html_widget, gchar *ref)
{
        HelpWindow win;
	gchar *tmpfile;
	gchar *argv[4];

	guchar *buf=NULL;
	gint   buflen;
	gint   fd;
	gint   cached = 0;

	DecomposedUrl du;
	
	gchar  theref[1024];
	gchar  *p;

	win = gtk_object_get_data(GTK_OBJECT(html_widget), "HelpWindow");
	du = decomposeUrlRelative(ref, win->currentRef, &p);
	g_message("%s + %s = %s", ref, win->currentRef, p);
	freeDecomposedUrl(du);
	strcpy(theref, p);
	g_message("loading image: %s", theref);

	if (strstr(theref, "file:")) {
		return XmHTMLImageDefaultProc(html_widget, theref+5, NULL, 0);
	} else {
		tmpfile = "/tmp/gnome-help-browser.tmpfile";
		
		if (win->cache) {
		    buf = lookupInDataCacheWithLen(win->cache, theref,
						   &buflen);
		}

		if (buf) {
		    g_message("cache hit: %s", theref);
		    cached = 1;
		} else {
		    argv[0] = "lynx";
		    argv[1] = "-source";
		    argv[2] = theref;
		    argv[3] = NULL;
		
		    getOutputFromBin(argv, NULL, 0, &buf, &buflen);
		}
		
		fd = open(tmpfile, O_WRONLY | O_CREAT, 0666);
		if (fd >= 0) {
		    write(fd, buf, buflen);
		    close(fd);
		}
		
		if (win->cache && !cached) {
		    addToDataCache(win->cache, theref, buf, buflen);
		}

		return XmHTMLImageDefaultProc(html_widget, 
					      tmpfile, NULL, 0);
	}
		
	return NULL;
}
