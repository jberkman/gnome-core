/* GNOME Help Browser */
/* Copyright (C) 1998 Red Hat Software, Inc    */
/* Written by Michael Fulbright and Marc Ewing */

/*
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <config.h>
#include <gnome.h>

#include "window.h"
#include "history.h"
#include "bookmarks.h"
#include "toc2.h"
#include "cache.h"

#define NAME "GnomeHelp"
#define HELP_VERSION "0.4"


static void aboutCallback(HelpWindow win);
static void newWindowCallback(HelpWindow win);
static void closeWindowCallback(HelpWindow win);
static void setCurrentCallback(HelpWindow win);

static void historyCallback(gchar *ref);
static void bookmarkCallback(gchar *ref);
static void tocCallback(gchar *ref);

void messageHandler(gchar *s);
void warningHandler(gchar *s);
void errorHandler(gchar *s);
void setErrorHandlers(void);
static HelpWindow makeHelpWindow(void);
static void initConfig(void);
static void saveConfig(void);
static GnomeClient *newGnomeClient(void);
static error_t parseAnArg (int key, char *arg, struct argp_state *state);


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

/* Session saving.  */
static struct argp parser =
{
    NULL,
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
static Toc tocWindow;
static History historyWindow;
static DataCache cache;
static Bookmarks bookmarkWindow;

GList *windowList = NULL;

/* This is the name of the help URL from the command line.  */
static char *helpURL = NULL;

int
main(gint argc, gchar *argv[])
{
    HelpWindow window;

    argp_program_version = HELP_VERSION;

    /* Initialize the i18n stuff */
    bindtextdomain (PACKAGE, GNOMELOCALEDIR);
    textdomain (PACKAGE);

    smClient = newGnomeClient();

    gnome_init(NAME, &parser, argc, argv, 0, NULL);

    initConfig();
    
    setErrorHandlers();
	
    historyWindow = newHistory(historyLength, historyCallback, historyFile);

    cache = newDataCache(memCacheSize, 0, (GCacheDestroyFunc)g_free,
			 cacheFile);
    tocWindow = newToc(manPath, infoPath, ghelpPath, tocCallback);
    bookmarkWindow = newBookmarks(bookmarkCallback, NULL, bookmarkFile);

    window = makeHelpWindow();

    if (helpURL)
	helpWindowShowURL(window, argv[1]);
	
    gtk_main();

    saveHistory(historyWindow);
    saveBookmarks(bookmarkWindow);
    saveCache(cache);
	
    return 0;
}

static HelpWindow
makeHelpWindow()
{
    HelpWindow window;
    
    window = helpWindowNew(NAME, aboutCallback, newWindowCallback,
			   closeWindowCallback, setCurrentCallback);
    helpWindowSetHistory(window, historyWindow);
    helpWindowSetCache(window, cache);
    helpWindowSetToc(window, tocWindow);
    helpWindowSetBookmarks(window, bookmarkWindow);

    windowList = g_list_append(windowList, window);
    
    return window;
}

static error_t
parseAnArg (int key, char *arg, struct argp_state *state)
{
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
    
    window = makeHelpWindow();
}

static void
historyCallback (gchar *ref)
{
    g_message("HISTORY: %s", ref);
    helpWindowShowURL((HelpWindow)g_list_last(windowList)->data, ref);
}

static void
bookmarkCallback (gchar *ref)
{
    g_message("BOOKMARKS: %s", ref);
    helpWindowShowURL((HelpWindow)g_list_last(windowList)->data, ref);
}

static void
tocCallback (gchar *ref) 
{
    g_message("TOC: %s", ref);
    helpWindowShowURL((HelpWindow)g_list_last(windowList)->data, ref);
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

	/* blow off any path info in the name of the program */
	s = strrchr(client_data, '/');
	if (!s)
		s = client_data;
        argv[i++] = (char *) s;
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

	s = alloca(80);
	snprintf(s, 80, "restart command is");
	for (j=0; j<i; j++) {
		strcat(s, " ");
		strcat(s, argv[j]);
	}
	g_message("%s", s);
	
        gnome_client_set_restart_command (client, i, argv);
        /* i.e. clone_command = restart_command - '--sm-client-id' */
        gnome_client_set_clone_command (client, 0, NULL);

        return TRUE;
}


/**********************************************************************/

/* Session Management stuff */
static GnomeClient
*newGnomeClient()
{
	GnomeClient *client;

	return NULL; /* cant figure out why below is causing seg faults */

#if 0
        client = gnome_client_new_default();

	g_message("SM client ID is %s", client->client_id);

	if (!client)
		return NULL;

        gtk_object_ref(GTK_OBJECT(client));
        gtk_object_sink(GTK_OBJECT(client));

        gtk_signal_connect (GTK_OBJECT (client), "save_yourself",
                            GTK_SIGNAL_FUNC (save_state), NULL);
        return client;
#endif
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
