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
#include <libgnomeui/gnome-stock.h>
#include <libgnome/gnome-help.h>
#include <gdk/gdkkeysyms.h>
#include "gnome-helpwin.h"

#include "parseUrl.h"
#include "window.h"
#include "history.h"
#include "bookmarks.h"
#include "toc2.h"
#include "transport.h"
#include "docobj.h"
#include "queue.h"
#include "visit.h"
#include "misc.h"

/* Toolbar pixmaps */
#include "contents.xpm"

#define IMAGE_TEMPFILE "/tmp/gnome-help-browser.tmpfile"

struct _helpWindow {
    /* Main app widget */
    GtkWidget *app;

    /* Toolbar widgets - needed to set button states */
    GtkWidget *tb_back, *tb_forw;

    /* The HTML widget */
    GtkWidget *helpWidget;

    /* The forward/backward queue */
    Queue queue;

    /* The current page reference */
    gchar *currentRef;
    gchar *humanRef;
    gchar *Title;

    gboolean useCache;

    /* The entry box that shows the URL */
    GtkWidget *entryBox;

    /* status bar */
    GtkWidget *statusBar;

    /* Bogus widgets used by accel table */
    GtkWidget *accelWidget;
    
    /* Passed to us by the main program */
    HelpWindowCB about_cb;
    HelpWindowCB new_window_cb;
    HelpWindowCB close_window_cb;
    HelpWindowCB set_current_cb;
    HelpWindowCB config_cb;
    History history;
    Toc toc;
    DataCache cache;
    Bookmarks bookmarks;
};


static GtkWidget * makeEntryArea(HelpWindow w);

/* Callbacks */
static void quit_cb(void);
static void about_cb (GtkWidget *w, HelpWindow win);
static void config_cb (GtkWidget *w, HelpWindow win);
static void bookmark_cb (GtkWidget *w, HelpWindow win);
static void close_cb (GtkWidget *w, HelpWindow win);
static void delete_cb (GtkWidget *w, void *foo, HelpWindow win);
static void new_window_cb (GtkWidget *w, HelpWindow win);
static void help_forward(GtkWidget *w, HelpWindow win);
static void help_backward(GtkWidget *w, HelpWindow win);
static void help_onhelp(GtkWidget *w, HelpWindow win);
static void help_gotoindex(GtkWidget *w, HelpWindow win);
static void xmhtml_activate(GtkWidget *w, XmHTMLAnchorCallbackStruct *cbs,
			    HelpWindow win);
static void anchorTrack(GtkWidget *w, XmHTMLAnchorCallbackStruct *cbs,
			    HelpWindow win);
static void formActivate(GtkWidget *w, XmHTMLFormCallbackStruct *cbs,
			    HelpWindow win);
static void reload_page(GtkWidget *w, HelpWindow win);
static void ghelpShowHistory (GtkWidget *w, HelpWindow win);
static void ghelpShowBookmarks (GtkWidget *w, HelpWindow win);
static void entryChanged(GtkWidget *w, HelpWindow win);
static void setCurrent(HelpWindow w);

static void pageUp(GtkWidget *w, HelpWindow win);
static void pageDown(GtkWidget *w, HelpWindow win);
static void focusEnter(GtkWidget *w, HelpWindow win);

static void dndDrop(GtkWidget *widget, GdkEvent *event, HelpWindow win);

static void init_toolbar(HelpWindow w);
static void update_toolbar(HelpWindow w);

XmImageInfo *load_image(GtkWidget *html_widget, gchar *ref);



/**********************************************************************/



/**********************************************************************/

/* Menu and toolbar structures */

GnomeUIInfo filemenu[] = {
	{GNOME_APP_UI_ITEM, 
	 N_("New window"), N_("Open new browser window"),
         new_window_cb, NULL, NULL, 
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_NEW,
	 0, 0, NULL},
	{GNOME_APP_UI_ITEM, 
	 N_("Add Bookmark"), N_("Add bookmark"),
         bookmark_cb, NULL, NULL, 
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_SAVE,
	 0, 0, NULL},
	{GNOME_APP_UI_ITEM, 
	 N_("Configure"), N_("Configure"),
         config_cb, NULL, NULL, 
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_PROP,
	 0, 0, NULL},
	{GNOME_APP_UI_ITEM, 
	 N_("Close"), N_("Close window"),
         close_cb, NULL, NULL, 
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{GNOME_APP_UI_ITEM, 
	 N_("Exit"), N_("Exit all windows"),
         quit_cb, NULL, NULL, 
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_EXIT,
	 0, 0, NULL},
	GNOMEUIINFO_END
};

GnomeUIInfo helpmenu[] = {
    {GNOME_APP_UI_ITEM, 
     N_("About"), N_("Info about this program"),
     about_cb, NULL, NULL, 
     GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_ABOUT,
     0, 0, NULL},
    GNOMEUIINFO_SEPARATOR,
    GNOMEUIINFO_HELP("help-browser"),
    GNOMEUIINFO_END
};
 
GnomeUIInfo windowmenu[] = {
    GNOMEUIINFO_ITEM(N_("History"), N_("Show History Window"),
		     ghelpShowHistory, NULL),
    GNOMEUIINFO_ITEM(N_("Bookmarks"), N_("Show Bookmarks Window"),
		     ghelpShowBookmarks, NULL),
    GNOMEUIINFO_END
};

GnomeUIInfo mainmenu[] = {
    GNOMEUIINFO_SUBTREE(N_("File"), filemenu),
    GNOMEUIINFO_SUBTREE(N_("Window"), windowmenu),
    GNOMEUIINFO_SUBTREE(N_("Help"), helpmenu),
    GNOMEUIINFO_END
};



GnomeUIInfo toolbar[] = {
    GNOMEUIINFO_ITEM_STOCK(N_("Back"), 
			   N_("Go to the previous location in the history list"),
			   help_backward, GNOME_STOCK_PIXMAP_BACK),
    GNOMEUIINFO_ITEM_STOCK(N_("Forward"),
			   N_("Go to the next location in the history list"),
			   help_forward, GNOME_STOCK_PIXMAP_FORWARD),
    GNOMEUIINFO_SEPARATOR,
    GNOMEUIINFO_ITEM_STOCK(N_("Reload"), N_("Reload"), reload_page,
			   GNOME_STOCK_PIXMAP_REFRESH),
    GNOMEUIINFO_SEPARATOR,
    GNOMEUIINFO_ITEM(N_("Index"), N_("Show Documentation Index"), 
		     help_gotoindex, contents_xpm),
    GNOMEUIINFO_SEPARATOR,
    GNOMEUIINFO_ITEM(N_("History"), N_("Show History Window"),
		     ghelpShowHistory, contents_xpm),
    GNOMEUIINFO_ITEM(N_("BMarks"), N_("Show Bookmarks Window"),
		     ghelpShowBookmarks, contents_xpm),
    GNOMEUIINFO_SEPARATOR,
    GNOMEUIINFO_ITEM_STOCK(N_("Help"), N_("Help on Help"), help_onhelp, GNOME_STOCK_PIXMAP_HELP),
    GNOMEUIINFO_END
};

/**********************************************************************/


/**********************************************************************/

/* Callbacks */

static void
about_cb (GtkWidget *w, HelpWindow win)
{
    if (win->about_cb)
	(win->about_cb)(win);
}

static void
config_cb (GtkWidget *w, HelpWindow win)
{
    if (win->config_cb)
	(win->config_cb)(win);
}

static void
bookmark_cb (GtkWidget *w, HelpWindow win)
{
    if (win->bookmarks)
	addToBookmarks(win->bookmarks, win->humanRef, win->Title);
}

static void
new_window_cb (GtkWidget *w, HelpWindow win)
{
    if (win->new_window_cb)
	(win->new_window_cb)(win);
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
quit_cb (void)
{
    gtk_main_quit ();
    return;
}

static void
xmhtml_activate(GtkWidget *w, XmHTMLAnchorCallbackStruct *cbs, HelpWindow win)
{
        g_message("TAG CLICKED: %s", cbs->href);

	helpWindowShowURL(win, cbs->href, TRUE, TRUE);
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
formActivate(GtkWidget *w, XmHTMLFormCallbackStruct *cbs, HelpWindow win)
{
	gint i;

	g_message("Recvieved a GTK_XMHTML_FORM event, enctype = %s",
		  cbs->enctype);

	g_message("There are %d components.", cbs->ncomponents);
	for (i=0; i<cbs->ncomponents; i++) {
		g_message("Component %d: name = %s, value = %s",
			  i,
			  cbs->components[i].name,
			  cbs->components[i].value);
	}
}
static void
help_forward(GtkWidget *w, HelpWindow win)
{
	gchar *ref;
	gint pos;

	if (!(ref = queue_next(win->queue, &pos)))
		return;

	visitURL(win, ref, TRUE, FALSE, FALSE );
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

	visitURL(win, ref, TRUE, FALSE, FALSE);
	queue_move_prev(win->queue);
	
	g_message("jump to line: %d", pos);
	gnome_helpwin_jump_to_line(GNOME_HELPWIN(win->helpWidget), pos);

	update_toolbar(win);
	setCurrent(win);
}

static void
help_onhelp(GtkWidget *w, HelpWindow win)
{
	helpWindowShowURL(win, "ghelp:help-browser", TRUE, TRUE);
}

static void
help_gotoindex(GtkWidget *w, HelpWindow win)
{
	helpWindowShowURL(win, "toc:", TRUE, FALSE);
}

static void
reload_page(GtkWidget *w, HelpWindow win)
{
    gchar *s;
    gchar buf[BUFSIZ];
	
    /* Do a little shorthand processing */
    s = win->humanRef;
    if (!s || *s == '\0') {
	return;
    } else if (*s == '/') {
	snprintf(buf, sizeof(buf), "file:%s", s);
    } else {
	strncpy(buf, s, sizeof(buf));
    }
    
    g_message("RELOAD PAGE: %s", buf);
    /* make html widget believe we want to reload */
    gtk_xmhtml_source(GTK_XMHTML(win->helpWidget), "<BODY>Hi</BODY>");
    helpWindowShowURL(win, buf, FALSE, FALSE);
}	

static void
entryChanged(GtkWidget *w, HelpWindow win)
{
    gchar *s;
    gchar buf[BUFSIZ];
    
    g_message("ENTRY BOX: %s", gtk_entry_get_text(GTK_ENTRY(w)));

    /* Do a little shorthand processing */
    s = gtk_entry_get_text(GTK_ENTRY(w));
    if (*s == '/') {
	snprintf(buf, sizeof(buf), "file:%s", s);
    } else {
	strncpy(buf, s, sizeof(buf));
    }
    
    helpWindowShowURL(win, buf, TRUE, TRUE);
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
	gnome_app_create_toolbar_with_data(GNOME_APP(w->app), toolbar, w);

	w->tb_back = toolbar[0].widget;
	w->tb_forw = toolbar[1].widget;
	
	update_toolbar(w);
}

static void
update_toolbar(HelpWindow w)
{
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
    gtk_signal_disconnect(GTK_OBJECT(GTK_COMBO(entry)->entry),
			  GTK_COMBO(entry)->activate_id);
    
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

gchar
*helpWindowHumanRef(HelpWindow w)
{
    return w->humanRef;
}

void
helpWindowHistoryAdd(HelpWindow w, gchar *ref)
{
    addToHistory(w->history, ref);
}

void
helpWindowHTMLSource(HelpWindow w, gchar *s, gint len,
		     gchar *ref, gchar *humanRef)
{
    gchar *buf=NULL;

    /* First set the current ref (it may be used to load images) */
    if (w->currentRef) {
	g_free(w->currentRef);
    }

    /* It's important to set this first because it used is to */
    /* resolve relative refs for images.                      */
    w->currentRef = g_strdup(ref);
    
    if (w->humanRef) {
	g_free(w->humanRef);
    }

    w->humanRef = g_strdup(humanRef);

    /* Load it up */
    buf = g_malloc(len + 1);
    memcpy(buf, s, len);
    buf[len] = '\0';
    gtk_xmhtml_source(GTK_XMHTML(w->helpWidget), buf);
    g_free(buf);
    gtk_entry_set_text(GTK_ENTRY(w->entryBox), humanRef);

    if (w->Title)
	g_free(w->Title);

    buf = XmHTMLGetTitle((GTK_WIDGET(w->helpWidget)));
    if (!buf)
	w->Title = g_strdup("");
    else
	w->Title = g_strdup(buf);

    g_message("Title is ->%s<-",w->Title);
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
    if (win->humanRef)
	g_free(win->humanRef);
    queue_free(win->queue);
    gtk_widget_destroy(win->accelWidget);
    g_free(win);
}

static void init_accel(HelpWindow win)
{
    GtkAccelGroup *accelGroup;

    /* What a hack this is.  I'm sure there is a better way */
    win->accelWidget = gtk_button_new_with_label("accelWidget");
    gtk_widget_realize(win->accelWidget);
    gtk_widget_ref(win->accelWidget);

    gtk_signal_connect(GTK_OBJECT(win->accelWidget), "pressed",
		       GTK_SIGNAL_FUNC(pageUp), win);
    gtk_signal_connect(GTK_OBJECT(win->accelWidget), "released",
		       GTK_SIGNAL_FUNC(pageDown), win);
    gtk_signal_connect(GTK_OBJECT(win->accelWidget), "enter",
		       GTK_SIGNAL_FUNC(focusEnter), win);
    
    accelGroup = gtk_object_get_data(GTK_OBJECT(win->app),
				     "GtkAccelGroup");
    gtk_widget_add_accelerator(win->accelWidget,
		    "pressed",
		    accelGroup,
		    'b', 0, 0);
    gtk_widget_add_accelerator(win->accelWidget,
		    "released",
		    accelGroup,
		    ' ', 0, 0);
    gtk_widget_add_accelerator(win->accelWidget,
		    "enter",
		    accelGroup,
		    'g', 0, 0);
}

static void pageUp(GtkWidget *w, HelpWindow win)
{
    GtkAdjustment *adj;
    
    adj = GTK_ADJUSTMENT(GTK_XMHTML(win->helpWidget)->vsba);
    gtk_adjustment_set_value(adj, adj->value - (adj->page_size));
}

static void pageDown(GtkWidget *w, HelpWindow win)
{
    GtkAdjustment *adj;
    
    adj = GTK_ADJUSTMENT(GTK_XMHTML(win->helpWidget)->vsba);
    gtk_adjustment_set_value(adj, adj->value + (adj->page_size));
}

static void focusEnter(GtkWidget *w, HelpWindow win)
{
    gtk_widget_grab_focus(GTK_WIDGET(win->entryBox));
}

static void dndDrop(GtkWidget *widget, GdkEvent *event, HelpWindow win)
{
    gchar buf[BUFSIZ];
    gchar *s;
    
    s = (gchar *)event->dropdataavailable.data;
    g_message("DROP: %s", s);

    /* Do a little shorthand processing */
    if (*s == '/') {
	snprintf(buf, sizeof(buf), "file:%s", s);
    } else {
	strncpy(buf, s, sizeof(buf));
    }
    
    helpWindowShowURL(win, buf, TRUE, TRUE);
}

HelpWindow
helpWindowNew(gchar *name,
	      gint x, gint y, gint width, gint height,
	      HelpWindowCB about_callback,
	      HelpWindowCB new_window_callback,
	      HelpWindowCB close_window_callback,
	      HelpWindowCB set_current_callback,
	      HelpWindowCB config_callback)
{
        HelpWindow w;
	GtkWidget *entryArea;
	GtkWidget *vbox;
	char *acceptedDropTypes[] = { "url:ALL" };

	w = (HelpWindow)g_malloc(sizeof(*w));

	w->queue= queue_new();
	w->about_cb = about_callback;
	w->new_window_cb = new_window_callback;
	w->close_window_cb = close_window_callback;
	w->set_current_cb = set_current_callback;
	w->config_cb = config_callback;
	w->history = NULL;
	w->bookmarks = NULL;
	w->cache = NULL;
	w->currentRef = NULL;
	w->humanRef = NULL;
	w->Title    = NULL;

	w->app = gnome_app_new (name, "Gnome Help Browser");
	gtk_window_set_wmclass (GTK_WINDOW (w->app), "GnomeHelpBrowser",
				"GnomeHelpBrowser");
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

	/* Add accelerators */
	init_accel(w);
					
	/* add a status bar */
	w->statusBar = gtk_statusbar_new();
	gtk_widget_show(w->statusBar);

	/* trap clicks on tags so we can stick requested link in browser */
	gtk_signal_connect(GTK_OBJECT(w->helpWidget), "activate",
			   GTK_SIGNAL_FUNC(xmhtml_activate), w);

	gtk_signal_connect(GTK_OBJECT(w->helpWidget), "anchor_track",
					     GTK_SIGNAL_FUNC(anchorTrack), w);

	gtk_signal_connect(GTK_OBJECT(w->helpWidget), "form",
					     GTK_SIGNAL_FUNC(formActivate), w);

	gtk_box_pack_start(GTK_BOX(vbox), w->helpWidget, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), w->statusBar, FALSE, FALSE, 0);
	gnome_app_set_contents(GNOME_APP(w->app), vbox);

	/* HACKHACKHACK this will grab images via http */
	gtk_object_set_data(GTK_OBJECT(w->helpWidget), "HelpWindow", w);
	gtk_xmhtml_set_image_procs(GTK_XMHTML(w->helpWidget),
				   (XmImageProc)load_image,
				   NULL,NULL,NULL);

	/* size should be auto-determined, or read from gnomeconfig() */
	if (width && height)
		gtk_widget_set_usize(GTK_WIDGET(w->app), width, height); 
	else
		gtk_widget_set_usize(GTK_WIDGET(w->app), 600, 450); 

	if (x != y)
		gtk_widget_set_uposition(GTK_WIDGET(w->app), x, y);

	gtk_window_set_policy(GTK_WINDOW(w->app), TRUE, TRUE, FALSE);
	gtk_widget_show(w->app);

	gtk_widget_realize(w->helpWidget);
	gtk_signal_connect(GTK_OBJECT(GTK_XMHTML(w->helpWidget)->html.work_area),
			   "drop_data_available_event",
			   GTK_SIGNAL_FUNC(dndDrop), w);
	gtk_widget_dnd_drop_set(GTK_XMHTML(w->helpWidget)->html.work_area,
				TRUE, acceptedDropTypes, 1, FALSE);

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
helpWindowShowURL(HelpWindow win, gchar *ref, 
		  gboolean useCache, gboolean addToQueue)
{
	gchar err[1024];

	win->useCache = useCache;
	if (visitURL(win, ref, useCache, addToQueue, TRUE)) {
		GtkWidget *msg;

		snprintf(err, sizeof(err), "Error loading document:\n\n%s",
			 ref);
		msg = gnome_message_box_new(err, GNOME_MESSAGE_BOX_ERROR,
					   "Ok", NULL);
		gnome_message_box_set_modal (GNOME_MESSAGE_BOX (msg));
		gtk_widget_show(msg);

		gtk_entry_set_text(GTK_ENTRY(win->entryBox), win->humanRef);
		return;
	} else {
		/* clear the status bar */
		gtk_statusbar_pop(GTK_STATUSBAR(win->statusBar), 1);
	}
	update_toolbar(win);
	setCurrent(win);
	win->useCache = TRUE;

	/* XXX This should work, but it doesn't */
	{
	  const char *title = XmHTMLGetTitle(GTK_WIDGET(win->helpWidget));
	  if (!title) title = "";
	}

	gtk_widget_grab_focus(GTK_XMHTML(win->helpWidget)->html.vsb);
}

GtkWidget 
*helpWindowGetAppWindow(HelpWindow w)
{
	g_return_val_if_fail( w != NULL, NULL );

	return w->app;
}

/**********************************************************************/


/* HACK HACK HACK */
XmImageInfo *
load_image(GtkWidget *html_widget, gchar *ref)
{
        HelpWindow win;
	docObj obj;
	guchar *buf;
	XmImageInfo *info;
	gint buflen;
	gint fd;

	win = gtk_object_get_data(GTK_OBJECT(html_widget), "HelpWindow");
	obj = docObjNew(ref, win->useCache);
	docObjResolveURL(obj, helpWindowCurrentRef(win));
	if (strstr(docObjGetAbsoluteRef(obj), "file:")) {
	    info = XmHTMLImageDefaultProc(html_widget,
					  docObjGetAbsoluteRef(obj) + 5,
					  NULL, 0);
	    docObjFree(obj);
	    return info;
	}
	if (transport(obj, helpWindowGetCache(win))) {
	    docObjFree(obj);
	    return NULL;
	}
	
	fd = open(IMAGE_TEMPFILE, O_WRONLY | O_CREAT, 0666);
	if (fd >= 0) {
	    docObjGetRawData(obj, &buf, &buflen);
	    write(fd, buf, buflen);
	    close(fd);
	}
		
	docObjFree(obj);
	return XmHTMLImageDefaultProc(html_widget, IMAGE_TEMPFILE,
				      NULL, 0);
}
