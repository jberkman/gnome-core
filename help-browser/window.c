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

    /* Passed to us by the main program */
    GtkSignalFunc about_cb;
    History history;
};


/* Callbacks */
static void close_cb(void);
static void quit_cb(void);
static void about_cb (GtkWidget *w, HelpWindow win);
static void help_forward(GtkWidget *w, HelpWindow win);
static void help_backward(GtkWidget *w, HelpWindow win);
static void help_contents(GtkWidget *w, HelpWindow win);
static void help_onhelp(GtkWidget *w, HelpWindow win);
static void xmhtml_activate(GtkWidget *w, XmHTMLAnchorCallbackStruct *cbs,
			    HelpWindow win);
static void ghelpShowHistory (GtkWidget *w, HelpWindow win);
static void ghelpShowHistoryTB (GtkWidget *w, HelpWindow win);

static void init_toolbar(HelpWindow w);
static void update_toolbar(HelpWindow w);

XmImageInfo *load_image(GtkWidget *html_widget, gchar *ref);



/**********************************************************************/



/**********************************************************************/

/* Menu and toolbar structures */

GnomeUIInfo filemenu[] = {
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
    GNOMEUIINFO_TOGGLEITEM("Show history", "Toggle History Window",
			   ghelpShowHistory, NULL),
    GNOMEUIINFO_END
};

GnomeUIInfo mainmenu[] = {
    GNOMEUIINFO_SUBTREE("File", filemenu),
    GNOMEUIINFO_SUBTREE("Windows", windowmenu),
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
    GNOMEUIINFO_TOGGLEITEM("History", "Toggle History Window",
			   ghelpShowHistoryTB, contents_xpm),
    GNOMEUIINFO_END
};

/**********************************************************************/


/**********************************************************************/

/* Callbacks */

static void
about_cb (GtkWidget *w, HelpWindow win)
{
    (win->about_cb)();
}

static void
ghelpShowHistory (GtkWidget *w, HelpWindow win)
{
    if (GTK_CHECK_MENU_ITEM(w)->active == TRUE) {
	showHistory(win->history);
    } else {
	hideHistory(win->history);
    }
}

static void
ghelpShowHistoryTB (GtkWidget *w, HelpWindow win)
{
    if (GTK_TOGGLE_BUTTON(w)->active == TRUE) {
	showHistory(win->history);
    } else {
	hideHistory(win->history);
    }
}

static void
close_cb (void)
{
    /* XXX this should just close the current window */
    gtk_main_quit ();
    return;
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
	printf("tag clicked was ->%s<-\n", cbs->href);
	visitURL(win, cbs->href);
	update_toolbar(win);
}

static void help_forward(GtkWidget *w, HelpWindow win) {
	gchar *ref;

	if (!(ref = queue_next(win->queue)))
		return;
	else
		visitURL_nohistory(win, ref);


	update_toolbar(win);
}

static void help_backward(GtkWidget *w, HelpWindow win) {
	gchar *ref;

	if (!(ref = queue_prev(win->queue)))
		return;
	else
		visitURL_nohistory(win, ref);

	update_toolbar(win);
}

static void help_contents(GtkWidget *w, HelpWindow win) {
	return;
}

static void help_onhelp(GtkWidget *w, HelpWindow win) {
	gchar *p, *q;

	p = gnome_help_file_path("help-browser", "help-browser.html");
	if (!p)
		return;
	q = alloca(strlen(p)+10);
	strcpy(q,"file:");
	strcat(q, p);
	printf("Loading help from ->%s<-\n",q);
	g_free(p);
	visitURL(win, q);
	return;
}

/**********************************************************************/



/**********************************************************************/

/* Misc static routines */

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

/**********************************************************************/


/**********************************************************************/

/* Public functions */

void helpWindowQueueAdd(HelpWindow w, gchar *ref)
{
    queue_add(w->queue, ref);
}

gchar *helpWindowCurrentRef(HelpWindow w)
{
    return w->currentRef;
}

void helpWindowHistoryAdd(HelpWindow w, gchar *ref)
{
    addToHistory(w->history, ref);
}

void helpWindowHTMLSource(HelpWindow w, gchar *s, gchar *ref)
{
    /* First set the current ref (it may be used to load images) */
    if (w->currentRef) {
	g_free(w->currentRef);
    }
    w->currentRef = g_strdup(ref);

    /* Load it up */
    gtk_xmhtml_source(GTK_XMHTML(w->helpWidget), s);
}

void helpWindowJumpToAnchor(HelpWindow w, gchar *s)
{
    gnome_helpwin_jump_to_anchor(GNOME_HELPWIN(w->helpWidget), s);
}

void helpWindowJumpToLine(HelpWindow w, gint n)
{
    gnome_helpwin_jump_to_line(GNOME_HELPWIN(w->helpWidget), n);
}

HelpWindow
helpWindowNew(GtkSignalFunc about_cb)
{
        HelpWindow w;

	w = (HelpWindow)g_malloc(sizeof(*w));

	w->queue= queue_new();
	w->about_cb = about_cb;
	w->history = NULL;
	w->currentRef = NULL;

	/* XXX this needs to change.  The "app" needs to be */
	/* Somehow distinct from the windows.  Or at least, only */
	/* One of the windows can be the "app". */
	/* Make the main window and binds the delete event so you can close
	   the program from your WM */
	w->app = gnome_app_new ("GnomeHelp", "Gnome Help Browser");
	gtk_widget_realize (w->app);
	gtk_signal_connect (GTK_OBJECT (w->app), "delete_event",
			    GTK_SIGNAL_FUNC (close_cb),
			    NULL);

	gnome_app_create_menus_with_data(GNOME_APP(w->app), mainmenu, w);

	/* make the help window */
	w->helpWidget = gnome_helpwin_new();
	
	/* trap clicks on tags so we can stick requested link in browser */
	gtk_signal_connect(GTK_OBJECT(w->helpWidget), "activate",
			   GTK_SIGNAL_FUNC(xmhtml_activate), w);

	/* size should be auto-determined, or read from gnomeconfig() */
	gtk_widget_set_usize(GTK_WIDGET(w->helpWidget), 800, 600);
	gnome_app_set_contents(GNOME_APP(w->app), w->helpWidget);
	gtk_widget_show(w->helpWidget);

	/* HACKHACKHACK this will grab images via http */
	gtk_object_set_data(GTK_OBJECT(w->helpWidget), "HelpWindow", w);
	gtk_xmhtml_set_image_procs(GTK_XMHTML(w->helpWidget),
				   (XmImageProc)load_image,
				   NULL,NULL,NULL);

	/* do the toolbar */
	init_toolbar(w);

	gtk_widget_show(w->app);

	return w;
}

void
helpWindowSetHistory(HelpWindow win, History history)
{
    win->history = history;
}

void
helpWindowShowURL(HelpWindow win, gchar *ref)
{
    visitURL(win, ref);
    update_toolbar(win);
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

	DecomposedUrl du;
	
	gchar  theref[1024];
	gchar  *p;

	win = gtk_object_get_data(GTK_OBJECT(html_widget), "HelpWindow");
	du = decomposeUrlRelative(ref, win->currentRef, &p);
	freeDecomposedUrl(du);
	strcpy(theref, p);
	printf("Loading image ->%s<-\n", theref);

	if (strstr(theref, "file:")) {
		return XmHTMLImageDefaultProc(html_widget, theref+5, NULL, 0);
	} else {
		tmpfile = "/tmp/gnome-help-browser.tmpfile";
		
		argv[0] = "lynx";
		argv[1] = "-source";
		argv[2] = theref;
		argv[3] = NULL;
		
		getOutputFromBin(argv, NULL, 0, &buf, &buflen);

		fd = open(tmpfile, O_WRONLY | O_CREAT, 0666);
		if (fd >= 0) {
			write(fd, buf, buflen);
			close(fd);
			if (buf)
				g_free(buf);
			return XmHTMLImageDefaultProc(html_widget, 
						      tmpfile, NULL, 0);
		}
	}
		
	if (buf)
		g_free(buf);

	return NULL;

}
