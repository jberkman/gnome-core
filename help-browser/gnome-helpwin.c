/*   Gnome Help Window - Michael Fulbright <msf@redhat.com>
 *   A help widget based on a help widget from:
 *
 *   GTimeTracker - a time tracker
 *   Copyright (C) 1997,98 Eckehard Berns
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <config.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/wait.h>

#include <glib.h>
#include <gnome.h>
#include "gnome-helpwin.h"

static GtkXmHTMLClass *parent_class;

/****************/
/* widget stuff */
/****************/

#if 0
static void
gnome_helpwin_destroy(GtkObject *object)
{
	GnomeHelpWin *help;
	
	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_HELPWIN_IS_HELP (object));
	
	help = GNOME_HELPWIN (object);
	
	if (help->html_source)
		g_free(help->html_source);
	
	/* probably want to turn this on at some point */
/* 	if (GTK_OBJECT_CLASS(parent_class)->destroy)
 		(* GTK_OBJECT_CLASS(parent_class)->destroy)(GTK_OBJECT(help));
*/		
}
#endif

static void
gnome_helpwin_class_init(GnomeHelpWinClass *helpclass)
{
    /* may need to enable this */
/*	GtkObjectClass *object_class = GTK_OBJECT_CLASS(helpclass);
	object_class->destroy = gnome_helpwin_destroy;
*/

	/* ugly C magic, doubt its required */
	parent_class = gtk_type_class (gtk_xmhtml_get_type ()); 
}

static void
gnome_helpwin_init(GnomeHelpWin *help)
{
	/* init struct _GnomeHelpWin */
	help->document_path[0] = 0;
	help->html_source = NULL;
}

guint
gnome_helpwin_get_type(void)
{
	static guint GnomeHelpWin_type = 0;
	if (!GnomeHelpWin_type) {
		GtkTypeInfo GnomeHelpWin_info = {
			"GnomeHelpWin",
			sizeof(GnomeHelpWin),
			sizeof(GnomeHelpWinClass),
			(GtkClassInitFunc) gnome_helpwin_class_init,
			(GtkObjectInitFunc) gnome_helpwin_init,
			NULL,
			NULL,
			(GtkClassInitFunc) NULL
		};

		GnomeHelpWin_type = gtk_type_unique(gtk_xmhtml_get_type(),
						    &GnomeHelpWin_info);
	}
	return GnomeHelpWin_type;
}

GtkWidget *
gnome_helpwin_new(void)
{
	return GTK_WIDGET(gtk_type_new(gnome_helpwin_get_type()));
}




guint
gnome_helpwin_close(GnomeHelpWin *w)
{
        gtk_widget_hide(GTK_WIDGET(w));
        return FALSE;
}

void
gnome_helpwin_jump_to_anchor(GnomeHelpWin *w, gchar *anchor)
{
	gchar *a;

	g_return_if_fail( w != NULL );
	g_return_if_fail( anchor != NULL );

	if (*anchor != '#') {
		a = alloca(strlen(anchor)+5);
		strcpy(a, "#");
		strcat(a, anchor);
	} else {
		a = anchor;
	}
		
        XmHTMLAnchorScrollToName(GTK_WIDGET(w), a);
}

void
gnome_helpwin_jump_to_line(GnomeHelpWin *w, gint line)
{
	g_return_if_fail( w != NULL );

	/*        XmHTMLTextScrollToLine(GTK_WIDGET(w), line); */
	gtk_xmhtml_set_topline(GTK_XMHTML(w), line);
}

gint
gnome_helpwin_get_line(GnomeHelpWin *w)
{
        return gtk_xmhtml_get_topline(GTK_XMHTML(w));
}

/*
 * gnome_helpwin_goto()
 *
 * Loads contents of file 'filename' in the help widget object 'help'
 *
 * NOTE - fairly obsolete, left around for reference mostly
 *        wouldn't recommend using it
 *      
 *        Still figuring out what goes into the widget and
 *        what needs to be external routines (like filtering) 
 *
 */
void
gnome_helpwin_goto(GnomeHelpWin *help, const char *filename)
{
	FILE *f;
	char *str, *tmp, s[1024], *anchor, *path;

	g_return_if_fail(help != NULL);
	g_return_if_fail(GNOME_HELPWIN_IS_HELP(help));
	g_return_if_fail(filename != NULL);

	path = help->document_path;
	str = help->html_source;
	
	/* TODO: parse filename for '..' */
	if (filename[0] == '/') {
		strcpy(path, filename);
	} else if (strrchr(path, '/')) {
		strcpy(strrchr(path, '/') + 1, filename);
	} else {
		strcpy(path, filename);
	}
	/* check for '#' */
	anchor = NULL;
	if (NULL != (tmp = strrchr(path, '#'))) {
		anchor = alloca(strlen(tmp)+2);
		strcpy(anchor, tmp);
		*tmp = '\0';
	}


        /* TODO: jump to a "#anchor" in the same document should work */
	if ((path[strlen(path) - 1] == '/') ||
	    (path[0] == 0)) {
		if (!help->html_source)
		    return;

		/* just jump to the anchor */
		gnome_helpwin_jump_to_anchor( help, anchor);
		return;
	}

	errno = 0;
	f = fopen(path, "rt");
	if ((errno) || (!f)) {
		if (f) fclose(f);

		if (help->html_source)
		    g_free(help->html_source);

		help->html_source= g_strdup(_("<body><h2>Error: file not "
					    "found</h2></body>"));
		gtk_xmhtml_source( GTK_XMHTML(help), help->html_source );
		return;
	}

	str = NULL;
	while ((!feof(f)) && (!errno)) {
		if (!fgets(s, sizeof (s)-1, f))
		    continue;
		if (str)
		    str = g_realloc(str, strlen(str) + strlen(s) + 1);
		else {
		    str = g_malloc(strlen(s) + 1);
		    *str = 0;
		}
		strcat(str, s);
	}
	fclose(f);
	if (errno) {
		g_warning("gnome_helpwin_goto: error reading file "
			  "\"%s\".\n", path);
		errno = 0;
	}

	gtk_xmhtml_source( GTK_XMHTML(help), str);
	gnome_helpwin_jump_to_anchor( help, anchor);

	if (help->html_source)
	    g_free(help->html_source);

	help->html_source = str;

}

