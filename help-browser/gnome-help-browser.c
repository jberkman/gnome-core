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
#include "toc.h"
#include "cache.h"

#define VERSION "0.3"

static void about_cb(void);
static void historyCallback(gchar *ref);
static void tocCallback(gchar *ref);
void messageHandler(gchar *s);
void warningHandler(gchar *s);
void errorHandler(gchar *s);
void setErrorHandlers(void);

static HelpWindow currentHelpWindow;
static History historyWindow;
static GtkWidget *tocWindow;
static DataCache cache;

int
main(int argc, char *argv[])
{
    HelpWindow window;
    
    /* Global application history should be here as well          */
    
    gnome_init("gnome_help_browser", &argc, &argv);

    setErrorHandlers();
	
    window = helpWindowNew(about_cb);
    currentHelpWindow = window;

    historyWindow = newHistory(0, (GSearchFunc)historyCallback, NULL); 
    helpWindowSetHistory(window, historyWindow);

    cache = newDataCache(10000000, 0, (GCacheDestroyFunc)g_free);
    helpWindowSetCache(window, cache);

    /* make the toc browser */
    tocWindow = createToc((GtkSignalFunc)tocCallback);
    gtk_widget_show(tocWindow);

    if (argc > 1)
	helpWindowShowURL(window, argv[1]);
	
    gtk_main();
	
    return 0;
}

static void
historyCallback (gchar *ref)
{
    helpWindowShowURL(currentHelpWindow, ref);
}

static void
tocCallback(gchar *ref) 
{
    helpWindowShowURL(currentHelpWindow, ref);
}

static void
about_cb (void)
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
