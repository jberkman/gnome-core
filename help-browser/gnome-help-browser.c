/* simple test program for the gnomehelpwin widget (from gtthelp)
 * michael fulbright
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#include <gnome.h>
#include <libgnome/gnome-help.h>

#include "gnome-helpwin.h"

#include "gnome-help-browser.h"

#include "toc.h"
#include "history.h"

#include "docobj.h"
#include "queue.h"
#include "mime.h"

#include "close.xpm"
#include "right_arrow.xpm"
#include "left_arrow.xpm"
#include "contents.xpm"
#include "help.xpm"


/* prototypes */
static void quit_cb (GtkWidget *widget, void *data);
static void about_cb(GtkWidget *widget, void *data);
static void help_forward(GtkWidget *w, GtkWidget *helpwin);
static void help_backward(GtkWidget *w, GtkWidget *helpwin);
static void help_contents(GtkWidget *w, gpointer *data);
static void help_onhelp(GtkWidget *w, GtkWidget *helpwin);

static void prepare_app(char *file);
static void init_toolbar(GnomeApp *app);
static void update_toolbar(void);
static void xmhtml_activate(GtkWidget *w, XmHTMLAnchorCallbackStruct *cbs);
static void tocselectionChanged(gchar *data);
static void ghelpShowHistory (GtkWidget *w);
static void historyCallback (gchar *ref, GtkWidget *w);
static void ghelpRadioMenuCB (GtkWidget *w);

XmImageInfo *load_image(GtkWidget *html_widget, gchar *ref);


#define VERSION "0.3"


/* main GnomeApp widget */
GtkWidget *app;

/* widget for the toolbar */
GtkWidget *tb_contents, *tb_exit, *tb_back, *tb_forw;
GtkWidget *help;
GtkWidget *toc;

GnomeUIInfo filemenu[] = {
	{GNOME_APP_UI_ITEM, "Exit", "Exit program", quit_cb, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL},
	{GNOME_APP_UI_ENDOFINFO, NULL, NULL, NULL, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL}
};

GnomeUIInfo helpmenu[] = {
	{GNOME_APP_UI_ITEM, "About...", "Info about this program", about_cb,
	 NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL},
	{GNOME_APP_UI_SEPARATOR, NULL, NULL, NULL, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL},
	{GNOME_APP_UI_HELP, NULL, NULL, "help-browser", NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL},
	{GNOME_APP_UI_ENDOFINFO, NULL, NULL, NULL, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL}
};
 
GnomeUIInfo radiomenu[] = {
    {GNOME_APP_UI_TOGGLEITEM, "Radio 1", "Radio button 1",
     ghelpRadioMenuCB, (gpointer)1, NULL,
     GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL},
    {GNOME_APP_UI_TOGGLEITEM, "Radio 2", "Radio button 2",
     ghelpRadioMenuCB, (gpointer)2, NULL,
     GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL},
    {GNOME_APP_UI_TOGGLEITEM, "Radio 3", "Radio button 3",
     ghelpRadioMenuCB, (gpointer)3, NULL,
     GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL},
    {GNOME_APP_UI_ENDOFINFO, NULL, NULL, NULL, NULL, NULL,
     GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL}
};

GnomeUIInfo windowmenu[] = {
    {GNOME_APP_UI_TOGGLEITEM, "Show history", "Opens the history window",
     ghelpShowHistory, NULL, NULL, GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL},
#if 0
    {GNOME_APP_UI_RADIOITEMS, NULL, NULL,
     radiomenu, NULL, NULL, GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL},
#endif
    {GNOME_APP_UI_ENDOFINFO, NULL, NULL, NULL, NULL, NULL,
     GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL}
};

GnomeUIInfo mainmenu[] = {
	{GNOME_APP_UI_SUBTREE, "File", NULL, filemenu, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL},
	{GNOME_APP_UI_SUBTREE, "Windows", NULL, windowmenu, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL},
	{GNOME_APP_UI_SUBTREE, "Help", NULL, helpmenu, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL},
	{GNOME_APP_UI_ENDOFINFO, NULL, NULL, NULL}
};

int
main(int argc, char *argv[]) {
	gnome_init("gnome_help_browser", &argc, &argv);
	prepare_app((argc > 1) ? argv[1] : NULL);
	gtk_main();
	return 0;
}

static void
ghelpShowHistory (GtkWidget *w)
{
    if (GTK_CHECK_MENU_ITEM(w)->active == TRUE) {
	showHistory(history);
    } else {
	hideHistory(history);
    }
}

static void
ghelpShowHistoryTB (GtkWidget *w)
{
    if (GTK_TOGGLE_BUTTON(w)->active == TRUE) {
	showHistory(history);
    } else {
	hideHistory(history);
    }
}

static void
ghelpShowHistoryTBR (GtkWidget *w, char *s)
{
    if (GTK_TOGGLE_BUTTON(w)->active == TRUE)
	printf("Got radio = %s\n", s);
}

static void
ghelpRadioMenuCB (GtkWidget *w)
{
    if (GTK_CHECK_MENU_ITEM(w)->active == TRUE) {
	printf("Radio selected!  %d\n",
	       (gint)gtk_object_get_data(GTK_OBJECT(w), "MenuData"));
    }
}

static void
historyCallback (gchar *ref, GtkWidget *w)
{
    visitURL(GNOME_HELPWIN(w), ref);
    update_toolbar();
}

static void
quit_cb (GtkWidget *widget, void *data)
{
	gtk_main_quit ();
	return;
}

static void
xmhtml_activate(GtkWidget *w, XmHTMLAnchorCallbackStruct *cbs)
{
	printf("tag clicked was ->%s<-\n",cbs->href);
	visitURL(GNOME_HELPWIN(w), cbs->href);
	update_toolbar();
}

static void
prepare_app(char *file)
{
	/* Make the main window and binds the delete event so you can close
	   the program from your WM */
	app = gnome_app_new ("GnomeHelp", "Gnome Help Browser");
	gtk_widget_realize (app);
	gtk_signal_connect (GTK_OBJECT (app), "delete_event",
			    GTK_SIGNAL_FUNC (quit_cb),
			    NULL);

	gnome_app_create_menus(GNOME_APP(app), mainmenu);

	/* make the help window */
	help = gnome_helpwin_new();
	
	/* trap clicks on tags so we can stick requested link in browser */
	gtk_signal_connect(GTK_OBJECT(help), "activate",
			   GTK_SIGNAL_FUNC(xmhtml_activate), GTK_OBJECT(help));

	/* size should be auto-determined, or read from gnomeconfig() */
	gtk_widget_set_usize(GTK_WIDGET(help), 800, 600);
	gnome_app_set_contents( GNOME_APP(app), help );
	gtk_widget_show(help);

	/* HACKHACKHACK this will grab images via http */
	gtk_xmhtml_set_image_procs(GTK_XMHTML(help),(XmImageProc)load_image,
				   NULL,NULL,NULL);

	/* do the toolbar */
	init_toolbar(GNOME_APP(app));

	/* make the toc browser */
	toc = createToc((GtkSignalFunc)tocselectionChanged);
	gtk_widget_show(toc);

	gtk_widget_show (app);

	history = newHistory(0, (GSearchFunc)historyCallback, help); 

	queue= queue_new();
	if (file)
		visitURL(GNOME_HELPWIN(help), file);

}

/* Callbacks functions */
static void
about_cb (GtkWidget *widget, void *data)
{
	GtkWidget *about;
	gchar *authors[] = {
/* Here should be your names */
		"Mike Fulbright",
		"Marc Ewing",
		NULL
	};

	about = gnome_about_new ( "Gnome Help Browser", VERSION,
				  /* copyright notice */
				  "Copyright (c) 1998 Red Hat Software, Inc.",
				  authors,
				  /* another comments */
				  "GNOME Help Browser allows easy access to "
				  "various forms of documentation on your "
				  "system",
				  NULL);
	gtk_widget_show (about);
	
	return;
}


static void
tocselectionChanged(gchar *file) 
{
	if (file != NULL) {
		visitURL(GNOME_HELPWIN(help), file);
		update_toolbar();
	}
}


/* toolbar routines... */

GnomeUIInfo toolbar_radiolist[] = {
    GNOMEUIINFO_RADIOITEM_DATA("Radio A", "Radio Button A",
			      ghelpShowHistoryTBR, "A", help_xpm),
    GNOMEUIINFO_RADIOITEM_DATA("Radio B", "Radio Button B",
			      ghelpShowHistoryTBR, "B", help_xpm),
    GNOMEUIINFO_RADIOITEM_DATA("Radio C", "Radio Button C",
			      ghelpShowHistoryTBR, "C", help_xpm),
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
    GNOMEUIINFO_ITEM("Close", "Close Help Window", quit_cb, close_xpm),
    GNOMEUIINFO_SEPARATOR,
    GNOMEUIINFO_TOGGLEITEM("History", "Toggle History Window",
			   ghelpShowHistoryTB, contents_xpm),
    GNOMEUIINFO_SEPARATOR,
#if 0
    GNOMEUIINFO_RADIOLIST(toolbar_radiolist),
#endif
    GNOMEUIINFO_END
};

/* grabbed partially from the old gtthelp.c */
static void
init_toolbar(GnomeApp *app)
{
        GtkWidget *w, *pixmap;
        GtkToolbar *tbar;

	toolbar[5].user_data = help;
	toolbar[2].user_data = help;
	toolbar[3].user_data = help;
	
	gnome_app_create_toolbar(app, toolbar);

	tb_contents = toolbar[0].widget;
	tb_back = toolbar[2].widget;
	tb_forw = toolbar[3].widget;
	
	update_toolbar();
}

static void
update_toolbar(void)
{
	/* we dont have mapping for 'contents' button yet */
	if (tb_contents)
		gtk_widget_set_sensitive(tb_contents, 0);
	if (tb_back)
		gtk_widget_set_sensitive(tb_back, queue_isprev(queue));
	if (tb_forw)
		gtk_widget_set_sensitive(tb_forw, queue_isnext(queue));
}


static void help_forward(GtkWidget *w, GtkWidget *helpwin) {
	gchar *ref;

	if (!(ref=queue_next(queue)))
		return;
	else
		visitURL_nohistory(GNOME_HELPWIN(helpwin),ref);


	update_toolbar();
}

static void help_backward(GtkWidget *w, GtkWidget *helpwin) {
	gchar *ref;

	if (!(ref=queue_prev(queue)))
		return;
	else
		visitURL_nohistory(GNOME_HELPWIN(helpwin),ref);

	update_toolbar();
}

static void help_contents(GtkWidget *w, gpointer *data) {
	return;
}

static void help_onhelp(GtkWidget *w, GtkWidget *helpwin) {
	gchar *p, *q;

	p = gnome_help_file_path("help-browser", "help-browser.html");
	if (!p)
		return;
	q = alloca(strlen(p)+10);
	strcpy(q,"file:");
	strcat(q, p);
	printf("Loading help from ->%s<-\n",q);
	g_free(p);
	visitURL(GNOME_HELPWIN(helpwin), q);
	return;
}


/* HACK HACK HACK */

XmImageInfo *
load_image(GtkWidget *html_widget, gchar *ref)
{
	gchar *tmpfile;
	gchar *argv[4];

	guchar *buf=NULL;
	gint   buflen;
	gint   fd;

	gchar  theref[1024];
	gchar  *p;

	strcpy(theref, LoadingRef);
	p = theref + strlen(theref) - 1;
	for (; p >= theref && *p != '/'; p--);
	if (*p == '/')
		*p = '\0';
	strcat(theref ,"/");
	strcat(theref, ref);
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
