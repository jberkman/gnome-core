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

#include <gnome.h>

#include "window.h"
#include "history.h"
#include "bookmarks.h"
#include "toc2.h"
#include "cache.h"

#define NAME "GnomeHelp"
#define VERSION "0.4"

static void aboutCallback(void);
static void newWindowCallback(void);
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

/* Config data */			  
static gchar *manPath;			  
static gchar *infoPath;			  
static gchar *ghelpPath;
static gint memCacheSize;
static gint historyLength;
static gchar *historyFile;
static gchar *cacheFile;
static gchar *bookmarkFile;

/* A few globals */
static Toc tocWindow;
static History historyWindow;
static DataCache cache;
static Bookmarks bookmarkWindow;

GList *windowList = NULL;

int
main(int argc, char *argv[])
{
    HelpWindow window;
    
    gnome_init(NAME, &argc, &argv);

    initConfig();
    
    setErrorHandlers();
	
    historyWindow = newHistory(historyLength, (GSearchFunc)historyCallback,
			       NULL, historyFile);
    cache = newDataCache(memCacheSize, 0, (GCacheDestroyFunc)g_free,
			 cacheFile);
    tocWindow = newToc(manPath, infoPath, ghelpPath,
		       (GtkSignalFunc)tocCallback);
    bookmarkWindow = newBookmarks((GSearchFunc)bookmarkCallback, NULL,
				  bookmarkFile);

    window = makeHelpWindow();

    if (argc > 1)
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
newWindowCallback(void)
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
tocCallback(gchar *ref) 
{
    g_message("TOC: %s", ref);
    helpWindowShowURL((HelpWindow)g_list_last(windowList)->data, ref);
}

static void
aboutCallback (void)
{
	GtkWidget *about;
	gchar *authors[] = {
		"Mike Fulbright",
		"Marc Ewing",
		NULL
	};

	about = gnome_about_new ( "Gnome Help Browser", VERSION,
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
