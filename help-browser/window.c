/* This file should encapsulate all the HTML widget functionality. */
/* No other files should be accessing HTML widget functions!       */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
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

#define DEFAULT_HEIGHT 500
#define DEFAULT_WIDTH  600

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
    GtkWidget *appBar;

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

static void dndDrop(GtkWidget *widget, GdkDragContext *context, gint x, gint y,
		    GtkSelectionData *data, guint info,
		    guint time, HelpWindow win);

static void init_toolbar(HelpWindow w);
static void update_toolbar(HelpWindow w);

XmImageInfo *load_image(GtkWidget *html_widget, gchar *ref);



/**********************************************************************/



/**********************************************************************/

/* Menu and toolbar structures */

GnomeUIInfo filemenu[] = {
        GNOMEUIINFO_MENU_NEW_ITEM(N_("_New Window"),
				     N_("Open new browser window"),
				     new_window_cb, NULL),

	GNOMEUIINFO_SEPARATOR,

	{GNOME_APP_UI_ITEM, 
	 N_("_Add Bookmark"), N_("Add bookmark"),
         bookmark_cb, NULL, NULL, 
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_SAVE,
	 0, 0, NULL},

	GNOMEUIINFO_SEPARATOR,

	GNOMEUIINFO_MENU_CLOSE_ITEM(close_cb, NULL),

	GNOMEUIINFO_MENU_EXIT_ITEM(quit_cb, NULL),

	GNOMEUIINFO_END
};

GnomeUIInfo helpmenu[] = {
  
    GNOMEUIINFO_HELP("help-browser"),

    GNOMEUIINFO_MENU_ABOUT_ITEM(about_cb, NULL),

    GNOMEUIINFO_END
};
 
GnomeUIInfo windowmenu[] = {
	{GNOME_APP_UI_ITEM, 
	 N_("_History"), N_("Show History Window"),
         ghelpShowHistory, NULL, NULL, 
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{GNOME_APP_UI_ITEM, 
	 N_("_Bookmarks"), N_("Show Bookmarks Window"),
         ghelpShowBookmarks, NULL, NULL, 
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	 GNOMEUIINFO_END
};

GnomeUIInfo settingsmenu[] = {
        GNOMEUIINFO_MENU_PREFERENCES_ITEM(config_cb, NULL),
	GNOMEUIINFO_END
};

GnomeUIInfo mainmenu[] = {
    GNOMEUIINFO_MENU_FILE_TREE(filemenu),
    GNOMEUIINFO_SUBTREE(N_("_Window"), windowmenu),
    GNOMEUIINFO_MENU_SETTINGS_TREE(settingsmenu),
    GNOMEUIINFO_MENU_HELP_TREE(helpmenu),
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
	gnome_appbar_pop(GNOME_APPBAR(win->appBar));
	if (cbs->href) {
		gnome_appbar_push(GNOME_APPBAR(win->appBar), cbs->href);
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

    hbox = gtk_hbox_new(FALSE, 2);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 3);
    gtk_widget_show(hbox);
    
    label = gtk_label_new(_("Location: "));
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
    /*
      gtk_widget_destroy(win->accelWidget);
    */
    g_free(win);
}

static void init_accel(HelpWindow win)
{
    static gint page_up_signal = 0;
    static gint page_down_signal = 0;
    static gint grab_focus_signal = 0;

    GtkAccelGroup *accel_group = gtk_accel_group_get_default();

    if(!page_up_signal) {
      page_up_signal = gtk_object_class_user_signal_new 
	(GTK_OBJECT(GTK_XMHTML (win->helpWidget)->html.vsb)->klass,
	 "page_up",
	 GTK_RUN_FIRST,
	 gtk_marshal_NONE__NONE,
	 GTK_TYPE_NONE, 0);
      page_down_signal = gtk_object_class_user_signal_new
	(GTK_OBJECT(GTK_XMHTML (win->helpWidget)->html.vsb)->klass,
	 "page_down",
	 GTK_RUN_FIRST,
	 gtk_marshal_NONE__NONE,
	 GTK_TYPE_NONE, 0);
    }
    gtk_signal_connect(GTK_OBJECT(GTK_XMHTML (win->helpWidget)->html.vsb), 
		       "page_up", 
		       GTK_SIGNAL_FUNC(pageUp), win);
    gtk_signal_connect(GTK_OBJECT(GTK_XMHTML (win->helpWidget)->html.vsb), 
		       "page_down", 
		       GTK_SIGNAL_FUNC(pageDown), win);
    gtk_widget_add_accelerator(GTK_XMHTML (win->helpWidget)->html.vsb, 
			       "page_up", accel_group, 
			       'b', 0, 0);
    gtk_widget_add_accelerator(GTK_XMHTML (win->helpWidget)->html.vsb, 
			       "page_down", accel_group, 
			       ' ', 0, 0);

    gtk_widget_add_accelerator(GTK_XMHTML (win->helpWidget)->html.vsb, 
			       "page_up", accel_group, 
			       GDK_Page_Up, 0, 0);
    gtk_widget_add_accelerator(GTK_XMHTML (win->helpWidget)->html.vsb, 
			       "page_down", accel_group, 
			       GDK_Page_Down, 0, 0);
    gtk_widget_add_accelerator(win->entryBox, 
			       "grab_focus", accel_group, 
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

static void dndDrop(GtkWidget *widget, GdkDragContext *context, gint x, gint y,
		    GtkSelectionData *data, guint info,
		    guint time, HelpWindow win)
{
    GList *urls;
    
    if (data->data) {
	GList *urls = gnome_uri_list_extract_uris (data->data);

	if (urls)
	    helpWindowShowURL(win, (char *)urls->data, TRUE, TRUE);
	
	gnome_uri_list_free_strings (urls);
    }
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
	static GtkTargetEntry target_table[] = {
		{ "text/uri-list", 0, 0 },
	};

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

	w->app = gnome_app_new (name, _("Gnome Help Browser"));
	gtk_window_set_wmclass (GTK_WINDOW (w->app), "GnomeHelpBrowser",
				"GnomeHelpBrowser");

	gtk_signal_connect (GTK_OBJECT (w->app), "delete_event",
			    GTK_SIGNAL_FUNC (delete_cb), w);

	gnome_app_create_menus_with_data(GNOME_APP(w->app), mainmenu, w);

	/* do the toolbar */
	init_toolbar(w);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 0);
	gtk_widget_show(vbox);

	entryArea = makeEntryArea(w);
	gtk_widget_show(entryArea);
	gtk_box_pack_start(GTK_BOX(vbox), entryArea, FALSE, FALSE, 0);

	/* make the help window */
	w->helpWidget = gnome_helpwin_new();
	gtk_xmhtml_set_anchor_underline_type(GTK_XMHTML(w->helpWidget),
					    GTK_ANCHOR_SINGLE_LINE);
	gtk_xmhtml_set_anchor_buttons(GTK_XMHTML(w->helpWidget), FALSE);
	gtk_widget_show(w->helpWidget);

	/* add a status bar */
	w->appBar = gnome_appbar_new(FALSE, TRUE,
				     GNOME_PREFERENCES_USER);

	gnome_app_set_statusbar(GNOME_APP(w->app), GTK_WIDGET(w->appBar));
	gnome_app_install_menu_hints(GNOME_APP (w->app), mainmenu);
	
	/* trap clicks on tags so we can stick requested link in browser */
	gtk_signal_connect(GTK_OBJECT(w->helpWidget), "activate",
			   GTK_SIGNAL_FUNC(xmhtml_activate), w);

	gtk_signal_connect(GTK_OBJECT(w->helpWidget), "anchor_track",
					     GTK_SIGNAL_FUNC(anchorTrack), w);

	gtk_signal_connect(GTK_OBJECT(w->helpWidget), "form",
					     GTK_SIGNAL_FUNC(formActivate), w);

	gtk_box_pack_start(GTK_BOX(vbox), w->helpWidget, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), w->appBar, FALSE, FALSE, 0);
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
		gtk_widget_set_usize(GTK_WIDGET(w->app), 
				     DEFAULT_WIDTH, DEFAULT_HEIGHT); 

	if (x != y)
		gtk_widget_set_uposition(GTK_WIDGET(w->app), x, y);

	gtk_window_set_policy(GTK_WINDOW(w->app), TRUE, TRUE, FALSE);

	/* Add accelerators */
	init_accel(w);
					
	gtk_signal_connect(GTK_OBJECT(GTK_XMHTML(w->helpWidget)->html.work_area),
			   "drag_data_received",
			   GTK_SIGNAL_FUNC(dndDrop), w);

	gtk_drag_dest_set (GTK_XMHTML(w->helpWidget)->html.work_area,
			   GTK_DEST_DEFAULT_MOTION |
			   GTK_DEST_DEFAULT_HIGHLIGHT |
			   GTK_DEST_DEFAULT_DROP,
			   target_table, 1,
			   GDK_ACTION_COPY | GDK_ACTION_MOVE);

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

		snprintf(err, sizeof(err), _("Error loading document:\n\n%s"),
			 ref);
		msg = gnome_message_box_new(err, GNOME_MESSAGE_BOX_ERROR,
					   _("Ok"), NULL);
		gnome_message_box_set_modal (GNOME_MESSAGE_BOX (msg));
		gtk_widget_show(msg);

		gtk_entry_set_text(GTK_ENTRY(win->entryBox), win->humanRef);
		return;
	} else {
		/* clear the status bar */
		gnome_appbar_pop(GNOME_APPBAR(win->appBar));
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
/*
 * Among problems this routine has - it leaves temp files in /tmp 
 */
XmImageInfo *
load_image(GtkWidget *html_widget, gchar *ref)
{
        HelpWindow win;
	docObj obj;
	guchar *buf;
	XmImageInfo *info;
	gint buflen;
	gint fd;
	char tmpnam[]="/tmp/GnomeHelpBrowser-tmp.XXXXXX";

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
	
	fd = mkstemp(tmpnam);
	if (fd >= 0) {
	    docObjGetRawData(obj, &buf, &buflen);
	    write(fd, buf, buflen);
	    close(fd);
	}
		
	docObjFree(obj);
	return XmHTMLImageDefaultProc(html_widget, tmpnam, NULL, 0);
}
