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
static void prepare_app(char *file);
static void about_cb(GtkWidget *widget, void *data);
static void help_forward(GtkWidget *w, gpointer *data);
static void help_backward(GtkWidget *w, gpointer *data);
static void help_contents(GtkWidget *w, gpointer *data);
static void help_onhelp(GtkWidget *w, gpointer *data);
static void init_toolbar(GnomeHelpWin *help);
static void update_toolbar(void);
static void xmhtml_activate(GtkWidget *w, XmHTMLAnchorCallbackStruct *cbs);
static void tocselectionChanged(gchar *data);
static void ghelpShowHistory (void);
static void ghelpHideHistory (void);
static void historyCallback (gchar *ref, GtkWidget *w);

XmImageInfo *load_image(GtkWidget *html_widget, gchar *ref);


#define VERSION "0.3"


/* main GnomeApp widget */
GtkWidget *app;

/* widget for the toolbar */
GtkWidget *tb_contents, *tb_exit, *tb_back, *tb_forw;
GtkToolbar *toolbar;
GtkWidget *help;
GtkWidget *toc;

GnomeUIInfo filemenu[] = {
	{GNOME_APP_UI_ITEM, "Exit", "Exit program", quit_cb, 
	 GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL},
	{GNOME_APP_UI_ENDOFINFO, NULL, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL}
};

GnomeUIInfo helpmenu[] = {
	{GNOME_APP_UI_ITEM, "About...", "Info about this program", about_cb,
	 GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL},
	{GNOME_APP_UI_SEPARATOR, NULL, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL},
	{GNOME_APP_UI_HELP, NULL, NULL, "help-browser", 
	 GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL},
	{GNOME_APP_UI_ENDOFINFO, NULL, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL}
};
 
GnomeUIInfo windowmenu[] = {
    {GNOME_APP_UI_ITEM, "Show history", "Opens the history window",
     ghelpShowHistory, GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL},
    {GNOME_APP_UI_ITEM, "Hide history", "Hides the history window",
     ghelpHideHistory, GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL},
    {GNOME_APP_UI_ENDOFINFO, NULL, NULL, NULL,
     GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL}

};

GnomeUIInfo mainmenu[] = {
	{GNOME_APP_UI_SUBTREE, "File", NULL, filemenu, 
	 GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL},
	{GNOME_APP_UI_SUBTREE, "Windows", NULL, windowmenu,
	 GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL},
	{GNOME_APP_UI_SUBTREE, "Help", NULL, helpmenu,
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
ghelpShowHistory (void)
{
    showHistory(history);
}

static void
ghelpHideHistory (void)
{
    hideHistory(history);
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
	init_toolbar(GNOME_HELPWIN(help));

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
				  /* copyrigth notice */
				  "(C) 1998 the Free Software Foundation",
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

/* grabbed partially from the old gtthelp.c */
static void
init_toolbar(GnomeHelpWin *help)
{
        GtkWidget *w, *pixmap;
        GtkToolbar *tbar;

        tbar = (GtkToolbar *)gtk_toolbar_new(GTK_ORIENTATION_HORIZONTAL,
                                             GTK_TOOLBAR_BOTH);

        pixmap = gnome_create_pixmap_widget_d(GTK_WIDGET(app),
                                              GTK_WIDGET(tbar),
                                              contents_xpm);
        w = gtk_toolbar_append_item(tbar, "Contents", "Show Contents", NULL,
                                    pixmap, NULL, NULL);
        tb_contents = w;
        gtk_signal_connect_object(GTK_OBJECT(w), "clicked",
                                  GTK_SIGNAL_FUNC(help_contents),
                                  GTK_OBJECT(help));

        gtk_toolbar_append_space(tbar);

        pixmap = gnome_create_pixmap_widget_d(GTK_WIDGET(app),
                                              GTK_WIDGET(tbar),
                                              right_arrow_xpm);
        w = gtk_toolbar_append_item(tbar, "Back",
                                    "Go to the previouse location "
                                    "in history list", NULL,
                                    pixmap, NULL, NULL);
        tb_back = w;
        gtk_signal_connect_object(GTK_OBJECT(w), "clicked",
                                  GTK_SIGNAL_FUNC(help_backward),
                                  GTK_OBJECT(help));

        pixmap = gnome_create_pixmap_widget_d(GTK_WIDGET(app),
                                              GTK_WIDGET(tbar),
                                              left_arrow_xpm);
        w = gtk_toolbar_append_item(tbar, "Forward",
                                    "Go to the next location in history list", NULL,
                                    pixmap, NULL, NULL);
        tb_forw = w;
        gtk_signal_connect_object(GTK_OBJECT(w), "clicked",
                                  GTK_SIGNAL_FUNC(help_forward),
                                  GTK_OBJECT(help));

        gtk_toolbar_append_space(tbar);

        pixmap = gnome_create_pixmap_widget_d(GTK_WIDGET(app),
                                              GTK_WIDGET(tbar),
                                              help_xpm);
        w = gtk_toolbar_append_item(tbar, "Help", "Help on Help", NULL,
                                    pixmap, NULL, NULL);
        gtk_signal_connect_object(GTK_OBJECT(w), "clicked",
                                  GTK_SIGNAL_FUNC(help_onhelp),
                                  GTK_OBJECT(help));

        pixmap = gnome_create_pixmap_widget_d(GTK_WIDGET(app),
                                              GTK_WIDGET(tbar),
                                              close_xpm);
        w = gtk_toolbar_append_item(tbar, "Close", "Close Help Window", NULL,
                                    pixmap, NULL, NULL);
        gtk_signal_connect_object(GTK_OBJECT(w), "clicked",
                                  GTK_SIGNAL_FUNC(quit_cb),
                                  GTK_OBJECT(help));

        gnome_app_set_toolbar(GNOME_APP(app), tbar);

        toolbar = tbar;

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


static void help_forward(GtkWidget *w, gpointer *data) {
	gchar *ref;

	if (!(ref=queue_next(queue)))
		return;
	else
		visitURL_nohistory(GNOME_HELPWIN(w),ref);


	update_toolbar();
}

static void help_backward(GtkWidget *w, gpointer *data) {
	gchar *ref;

	if (!(ref=queue_prev(queue)))
		return;
	else
		visitURL_nohistory(GNOME_HELPWIN(w),ref);

	update_toolbar();
}

static void help_contents(GtkWidget *w, gpointer *data) {
	return;
}

static void help_onhelp(GtkWidget *w, gpointer *data) {
	gchar *p, *q;

	p = gnome_help_file_path("help-browser", "help-browser.html");
	if (!p)
		return;
	q = alloca(strlen(p)+10);
	strcpy(q,"file:");
	strcat(q, p);
	printf("Loading help from ->%s<-\n",q);
	g_free(p);
	visitURL(GNOME_HELPWIN(w), q);
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
