/* GNOME Help Browser */
/* Copyright (C) 1998 Red Hat Software, Inc    */
/* Written by Michael Fulbright and Marc Ewing */

/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2, or (at your option)
  any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
  02111-1307, USA.
*/

#include <config.h>
#include <gnome.h>

#include "window.h"
#include "history.h"
#include "bookmarks.h"
#include "toc2.h"
#include "cache.h"

extern char *program_invocation_name;

#define NAME "GnomeHelp"
#define HELP_VERSION "0.4"


static void aboutCallback(HelpWindow win);
static void newWindowCallback(HelpWindow win);
static void closeWindowCallback(HelpWindow win);
static void setCurrentCallback(HelpWindow win);
static void configCallback(HelpWindow win);

static void historyCallback(gchar *ref);
static void bookmarkCallback(gchar *ref);

void messageHandler(gchar *s);
void warningHandler(gchar *s);
void errorHandler(gchar *s);
void setErrorHandlers(void);
static HelpWindow makeHelpWindow(gint x, gint y, gint w, gint h);
static void initConfig(void);
static void saveConfig(void);
static GnomeClient *newGnomeClient(void);
static error_t parseAnArg (int key, char *arg, struct argp_state *state);

static void configOK(GtkWidget *w, GtkWidget *window);
static void configCancel(GtkWidget *w, GtkWidget *window);

/* MANPATH should probably come from somewhere */
#define DEFAULT_MANPATH "/usr/man:/usr/local/man:/usr/X11R6/man"
#define DEFAULT_INFOPATH "/usr/info:/usr/local/info"
/* GHELPPATH probably needs some automatic additions inside toc */
#define DEFAULT_GHELPPATH "/opt/gnome/share/gnome/help:" \
                          "/usr/local/share/gnome/help:" \
			  "/usr/local/gnome/share/gnome/help:" \
			  "/usr/share/gnome/help"
#define DEFAULT_MEMCACHESIZE "1000000"
#define DEFAULT_HISTORYLENGTH "1000"
#define DEFAULT_HISTORYFILE ".gnome-help-browser/history"
#define DEFAULT_CACHEFILE ".gnome-help-browser/cache"
#define DEFAULT_BOOKMARKFILE ".gnome-help-browser/bookmarks"

/* Argument parsing.  */
static struct argp_option options[] =
{
	{ NULL, 'x', N_("X"), 0, "X position of window", 1 },
	{ NULL, 'y', N_("Y"), 0, "Y position of window", 1 },
	{ NULL, 'w', N_("WIDTH"), 0, "Width of window", 1 },
	{ NULL, 'h', N_("HEIGHT"), 0, "Height of window", 1 },
	{ NULL, 0, NULL, 0, NULL, 0 }
};

static struct argp parser =
{
    options,
    parseAnArg,
    N_("[URL]"),
    NULL,
    NULL,
    NULL,
    NULL
};

/* Config data */			  
static gchar *manPath;			  
static gchar *infoPath;			  
static gchar *ghelpPath;
static gint memCacheSize;
static gint historyLength;
static gchar *historyFile;
static gchar *cacheFile;
static gchar *bookmarkFile;
static GnomeClient *smClient;

/* A few globals */
static Toc toc;
static History historyWindow;
static DataCache cache;
static Bookmarks bookmarkWindow;

GList *windowList = NULL;

/* This is the name of the help URL from the command line.  */
static gchar *helpURL = NULL;

/* requested size/positiion of initial help browser */
static gint defposx=0;
static gint defposy=0;
static gint defwidth =0;
static gint defheight=0;

int
main(gint argc, gchar *argv[])
{
    HelpWindow window;

    argp_program_version = HELP_VERSION;

    /* Initialize the i18n stuff */
    bindtextdomain (PACKAGE, GNOMELOCALEDIR);
    textdomain (PACKAGE);

/* enable session management here */
#if 1
    smClient = NULL;
    g_message("Session management was disabled at compile-time");
#else
    smClient = newGnomeClient();
#endif

    gnome_init(NAME, &parser, argc, argv, 0, NULL);

    initConfig();
    
    setErrorHandlers();
	
    if (smClient && smClient->client_id)
	    g_message("SM client ID is %s", smClient->client_id );
    else
	    g_message("Session Manager not detected");

    historyWindow = newHistory(historyLength, historyCallback, historyFile);
    cache = newDataCache(memCacheSize, 0, (GCacheDestroyFunc)g_free,
			 cacheFile);
    toc = newToc(manPath, infoPath, ghelpPath);
    bookmarkWindow = newBookmarks(bookmarkCallback, NULL, bookmarkFile);

    window = makeHelpWindow(defposx, defposy, defwidth, defheight );

    if (helpURL)
	    helpWindowShowURL(window, helpURL, TRUE, TRUE);
    else {
	    helpWindowShowURL(window, "toc:", TRUE, TRUE);
    }
	 
	
    gtk_main();

    saveHistory(historyWindow);
    saveBookmarks(bookmarkWindow);
    saveCache(cache);
	
    return 0;
}

static HelpWindow
makeHelpWindow(gint x, gint y, gint w, gint h)
{
    HelpWindow window;
    
    window = helpWindowNew(NAME, defposx, defposy, defwidth, defheight,
			   aboutCallback, newWindowCallback,
			   closeWindowCallback, setCurrentCallback,
			   configCallback);
    helpWindowSetHistory(window, historyWindow);
    helpWindowSetCache(window, cache);
    helpWindowSetToc(window, toc);
    helpWindowSetBookmarks(window, bookmarkWindow);

    windowList = g_list_append(windowList, window);
    
    return window;
}

static error_t
parseAnArg (int key, char *arg, struct argp_state *state)
{
	gint val;
	
	if (key == 'x' || key == 'y' || key == 'w' || key == 'h') {
		val = strtol(arg, NULL, 10);

		switch (key) {
		case 'x':
			defposx = val;
			break;
		case 'y':
			defposy = val;
			break;
		case 'w':
			defwidth  = val;
			break;
		case 'h':
			defheight = val;
			break;
		default:
			break;
		}
		return 0;
	}

  if (key != ARGP_KEY_ARG)
    return ARGP_ERR_UNKNOWN;
  if (helpURL)
    argp_usage (state);

  helpURL = arg;
  return 0;
}

/********************************************************************/

/* Callbacks */

static void
setCurrentCallback(HelpWindow win)
{
    windowList = g_list_remove(windowList, win);
    windowList = g_list_append(windowList, win);
}

static void
closeWindowCallback(HelpWindow win)
{
    helpWindowClose(win);

    windowList = g_list_remove(windowList, win);

    if (!windowList) {
	gtk_main_quit();
    }
}

static void
newWindowCallback(HelpWindow win)
{
    HelpWindow window;
    
    window = makeHelpWindow(0,0,0,0);
    helpWindowShowURL(window, "toc:", TRUE, TRUE);
}

static void
historyCallback (gchar *ref)
{
    g_message("HISTORY: %s", ref);
    helpWindowShowURL((HelpWindow)g_list_last(windowList)->data, 
		      ref, TRUE, TRUE);
}

static void
bookmarkCallback (gchar *ref)
{
    g_message("BOOKMARKS: %s", ref);
    helpWindowShowURL((HelpWindow)g_list_last(windowList)->data,
		      ref, TRUE, TRUE);
}

static void
aboutCallback (HelpWindow win)
{
	GtkWidget *about;
	gchar *authors[] = {
		"Mike Fulbright",
		"Marc Ewing",
		NULL
	};

	about = gnome_about_new ( "Gnome Help Browser", HELP_VERSION,
				  "Copyright (c) 1998 Red Hat Software, Inc.",
				  authors,
				  "GNOME Help Browser allows easy access to "
				  "various forms of documentation on your "
				  "system",
				  NULL);
	gtk_widget_show (about);
	
	return;
}

/**********************************************************************/

/* Error handlers */

void messageHandler(gchar *s)
{
    printf("M: %s\n", s);
}

void errorHandler(gchar *s)
{
    fprintf(stderr, "E: %s\n", s);
}

void warningHandler(gchar *s)
{
    printf("W: %s\n", s);
}

void setErrorHandlers(void)
{
    g_set_error_handler((GErrorFunc) errorHandler);
    g_set_warning_handler((GErrorFunc) warningHandler);
    g_set_message_handler((GErrorFunc) messageHandler);
    g_set_print_handler((GErrorFunc) printf);
}


/**********************************************************************/

/* Sesssion stuff */

static int
save_state (GnomeClient        *client,
            gint                phase,
            GnomeRestartStyle   save_style,
            gint                shutdown,
            GnomeInteractStyle  interact_style,
            gint                fast,
            gpointer            client_data)
{
        gchar *argv[20];
	gchar *s;
        gint i = 0, j;
	HelpWindow win;
	GtkWidget  *appwin;
        gint xpos, ypos;
	gint xsize, ysize;


	/* for now we worry about first window in window list */
	/* but we want to save state for all window browsers  */
	/* in the future                                      */
	if (!windowList)
		return FALSE;
	win = windowList->data;
	if (!win)
		return FALSE;
	appwin = helpWindowGetAppWindow(win);
	if (!appwin)
		return FALSE;

	g_message("Saving myself");
        gdk_window_get_origin(appwin->window, &xpos, &ypos);
	gdk_window_get_size(appwin->window, &xsize, &ysize);

        argv[i++] = program_invocation_name;
        argv[i++] = (char *) "-x";
	s = alloca(20);
	snprintf(s, 20, "%d", xpos);
        argv[i++] = s;
        argv[i++] = (char *) "-y";
	s = alloca(20);
	snprintf(s, 20, "%d", ypos);
        argv[i++] = s;
        argv[i++] = (char *) "-w";
	s = alloca(20);
	snprintf(s, 20, "%d", xsize);
        argv[i++] = s;
        argv[i++] = (char *) "-h";
	s = alloca(20);
	snprintf(s, 20, "%d", ysize);
        argv[i++] = s;
	s = alloca(512);
	snprintf(s, 512, "%s", helpWindowHumanRef(win));
	argv[i++] = s;

	s = alloca(512);
	snprintf(s, 512, "restart command is");
	for (j=0; j<i; j++) {
		strncat(s, " ", 511);
		strncat(s, argv[j], 511);
	}
	g_message("%s", s);
	
        gnome_client_set_restart_command (client, i, argv);
        /* i.e. clone_command = restart_command - '--sm-client-id' */
        gnome_client_set_clone_command (client, 0, NULL);

        return TRUE;
}


/**********************************************************************/

static void
session_die (gpointer client_data)
{
	gtk_main_quit ();
}

/* Session Management stuff */
static GnomeClient
*newGnomeClient()
{
	gchar *buf[1024];

	GnomeClient *client;

        client = gnome_client_new_default();

	if (!client)
		return NULL;

	getcwd((char *)buf,sizeof(buf));
	gnome_client_set_current_directory(client, (char *)buf);

        gtk_object_ref(GTK_OBJECT(client));
        gtk_object_sink(GTK_OBJECT(client));

        gtk_signal_connect (GTK_OBJECT (client), "save_yourself",
                            GTK_SIGNAL_FUNC (save_state), NULL);
        gtk_signal_connect (GTK_OBJECT (client), "die",
                            GTK_SIGNAL_FUNC (session_die), NULL);
        return client;
}

/**********************************************************************/

/* Configure stuff */

static void initConfig(void)
{
    manPath = gnome_config_get_string("/" NAME "/paths/manpath="
				      DEFAULT_MANPATH);
    infoPath = gnome_config_get_string("/" NAME "/paths/infopath="
				       DEFAULT_INFOPATH);
    ghelpPath = gnome_config_get_string("/" NAME "/paths/ghelppath="
					DEFAULT_GHELPPATH);

    memCacheSize = gnome_config_get_int("/" NAME "/cache/memsize="
					DEFAULT_MEMCACHESIZE);
    cacheFile = gnome_config_get_string("/" NAME "/cache/file="
					DEFAULT_CACHEFILE);
    historyLength = gnome_config_get_int("/" NAME "/history/length="
					 DEFAULT_HISTORYLENGTH);
    historyFile = gnome_config_get_string("/" NAME "/history/file="
					  DEFAULT_HISTORYFILE);
    bookmarkFile = gnome_config_get_string("/" NAME "/bookmarks/file="
					   DEFAULT_BOOKMARKFILE);

    saveConfig();
}

static void saveConfig(void)
{
    gnome_config_set_string("/" NAME "/paths/manpath", manPath);
    gnome_config_set_string("/" NAME "/paths/infopath", infoPath);
    gnome_config_set_string("/" NAME "/paths/ghelppath", ghelpPath);
    gnome_config_set_int("/" NAME "/cache/memsize", memCacheSize);
    gnome_config_set_string("/" NAME "/cache/file", cacheFile);
    gnome_config_set_int("/" NAME "/history/length", historyLength);
    gnome_config_set_string("/" NAME "/history/file", historyFile);
    gnome_config_set_string("/" NAME "/bookmarks/file", bookmarkFile);
    
    gnome_config_sync();
}

/* Given the current config settings, reset all the widgets and stuff */
static void setConfig(void)
{
    reconfigHistory(historyWindow, historyLength,
		    historyCallback, historyFile);
    reconfigDataCache(cache, memCacheSize, 0, (GCacheDestroyFunc)g_free,
		      cacheFile);
    /*toc = newToc(manPath, infoPath, ghelpPath, tocCallback);*/
    reconfigBookmarks(bookmarkWindow, bookmarkCallback, NULL, bookmarkFile);
}

/* XXX This stuff should all be abstracted somewhere else. */
/* There are globals around where there shouldn't be.      */

#define CONFIG_INT  1
#define CONFIG_TEXT 2

struct _config_entry {
    gchar *name;
    gint type;
    gpointer var;
    GtkWidget *entry;
};

GtkWidget *configWindow = NULL;
struct _config_entry configs[] = {
    { "History size", CONFIG_INT, &historyLength, NULL },
    { "History file", CONFIG_TEXT, &historyFile, NULL },
    { "Cache size", CONFIG_INT, &memCacheSize, NULL },
    { "Cache file", CONFIG_TEXT, &cacheFile, NULL },
    { "Bookmark file", CONFIG_TEXT, &bookmarkFile, NULL },

    { "Man Path", CONFIG_TEXT, &manPath, NULL },
    { "Info Path", CONFIG_TEXT, &infoPath, NULL },
    { "GNOME Help Path", CONFIG_TEXT, &ghelpPath, NULL },
    
    { NULL, 0, NULL }
};

static void
generateConfigWidgets(GtkWidget *parentBox, struct _config_entry *configs)
{
    GtkWidget *table, *label, *entry;
    struct _config_entry *p;
    gchar buf[BUFSIZ];
    gint rows;
    
    if (! configs)
	return;

    rows = 0;
    p = configs;
    while (p->name) {
	rows++;
	p++;
    }

    table = gtk_table_new(rows, 2, FALSE);
    gtk_widget_show(table);
    gtk_box_pack_start(GTK_BOX(parentBox), table, TRUE, TRUE, 0);

    rows = 0;
    p = configs;
    while (p->name) {
	label = gtk_label_new(p->name);
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT);
	gtk_widget_show(label);
	
	entry = gtk_entry_new();
	gtk_widget_show(entry);
	p->entry = entry;

	if (p->type == CONFIG_INT) {
	    sprintf(buf, "%d", *(gint *)(p->var));
	    gtk_entry_set_text(GTK_ENTRY(entry), buf);
	} else if (p->type == CONFIG_TEXT) {
	    gtk_entry_set_text(GTK_ENTRY(entry), *(gchar **)(p->var));
	}

	gtk_table_attach(GTK_TABLE(table), label, 0, 1, rows, rows + 1,
			 GTK_FILL, GTK_FILL,
			 5, 0);
	gtk_table_attach(GTK_TABLE(table), entry, 1, 2, rows, rows + 1,
			 GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL,
			 0, 0);

	rows++;
	p++;
    }
}

static void
configCallback(HelpWindow win)
{
    GtkWidget *window, *box, *buttonBox, *ok, *cancel;

    if (configWindow) {
	return;
    }
    
    /* Main Window */
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Gnome Help Configure");
    gtk_widget_set_usize (window, 500, -1);
    configWindow = window;

    /* Vbox */
    box = gtk_vbox_new(FALSE, 5);
    gtk_container_border_width (GTK_CONTAINER (box), 5);
    gtk_container_add(GTK_CONTAINER(window), box);
    gtk_widget_show(box);

    /* Entries */

    generateConfigWidgets(box, configs);

    /* Buttons */

    buttonBox = gtk_table_new(1, 2, TRUE);
    gtk_widget_show(buttonBox);

    ok = gnome_stock_button(GNOME_STOCK_BUTTON_OK);
    GTK_WIDGET_SET_FLAGS (GTK_WIDGET (ok), GTK_CAN_DEFAULT);
    gtk_widget_show(ok);

    cancel = gnome_stock_button(GNOME_STOCK_BUTTON_CANCEL);
    GTK_WIDGET_SET_FLAGS (GTK_WIDGET (cancel), GTK_CAN_DEFAULT);
    gtk_widget_show(cancel);

    gtk_table_attach(GTK_TABLE(buttonBox), ok, 0, 1, 0, 1,
		     GTK_EXPAND, 0, 0, 0);
    gtk_table_attach(GTK_TABLE(buttonBox), cancel, 1, 2, 0, 1,
		     GTK_EXPAND, 0, 0, 0);
    gtk_box_pack_start(GTK_BOX(box), buttonBox, TRUE, TRUE, 0);

    gtk_signal_connect(GTK_OBJECT(ok), "clicked",
		       (GtkSignalFunc)configOK, window);
    gtk_signal_connect(GTK_OBJECT(cancel), "clicked",
		       (GtkSignalFunc)configCancel, window);

    gtk_widget_show(window);
}

static void
configOK(GtkWidget *w, GtkWidget *window)
{
    struct _config_entry *p = configs;
    gchar *s;
    gint x;
    
    while (p->name) {
	s = gtk_entry_get_text(GTK_ENTRY(p->entry));

	if (p->type == CONFIG_INT) {
	    sscanf(s, "%d", &x);
	    *(gint *)(p->var) = x;
	} else if (p->type == CONFIG_TEXT) {
	    if (*(gchar **)(p->var)) {
		g_free(*(gchar **)(p->var));
	    }
	    *(gchar **)(p->var) = g_strdup(s);;
	}
	
	p++;
    }
    
    gtk_widget_destroy(window);
    configWindow = NULL;
    saveConfig();
    setConfig();
}

static void
configCancel(GtkWidget *w, GtkWidget *window)
{
    gtk_widget_destroy(window);
    configWindow = NULL;
}
