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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "gnome-helpwin.h"

static GtkXmHTMLClass *parent_class;

/****************/
/* Filter stuff */
/****************/

/*
 * gnome_helpwin_loadfilters -
 *
 * very simple for the moment, just loads a list of filter types
 *
 * returns non-zero on error
 *
 */
static int
gnome_helpwin_loadfilters(GnomeHelpWin *help) {
    gint i;
    
    /* lets just hard code for now */
    help->filters = g_realloc(help->filters,
			      (help->numfilters+4)*sizeof(HTMLFilter));

    i = help->numfilters;
    
    /* HTML */
    help->filters[i].type = i;
    help->filters[i].descr   = strdup("HTML");
    help->filters[i].exec    = strdup("");
    i++;

    /* MAN */
    help->filters[i].type = i;
    help->filters[i].descr   = strdup("MAN");
    help->filters[i].exec    = strdup("man2html {}");
    i++;

    /* info */
    help->filters[i].type = i;
    help->filters[i].descr   = strdup("INFO");
    help->filters[i].exec    = strdup("info2html {}");
    i++;

    /* other */
    help->filters[i].type = i;
    help->filters[i].descr   = strdup("UNKNOWN");
    help->filters[i].exec    = strdup("");
    i++;

    help->numfilters += 4;

    return 0;
}
    
    
	

/****************/
/* widget stuff */
/****************/

static void
gnome_helpwin_destroy(GtkObject *object)
{
    GnomeHelpWin *help;
    gint i;
    
    g_return_if_fail (object != NULL);
    g_return_if_fail (GNOME_HELPWIN_IS_HELP (object));
    
    help = GNOME_HELPWIN (object);
    
    /* cleanup any structures here */
    for (i=0; i < help->numfilters; i++)
    {
	if (help->filters[i].descr)
	    g_free(help->filters[i].descr);
	if (help->filters[i].exec)
	    g_free(help->filters[i].exec);
    }

    if (help->filters)
	g_free(help->filters);

    if (help->html_source)
	g_free(help->html_source);
    
	/* probably want to turn this on at some point */
/* 	if (GTK_OBJECT_CLASS(parent_class)->destroy)
 		(* GTK_OBJECT_CLASS(parent_class)->destroy)(GTK_OBJECT(help));
*/		
}


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

/* this is done by calling program, not in the widget */
#if 0
static void
xmhtml_activate(GtkWidget *w, XmHTMLAnchorCallbackStruct *cbs)
{

    printf("In activate with ref = |%s|\n",cbs->href);
    fflush(stdout);
#if 1
	gnome_helpwin_goto(GNOME_HELPWIN(w), cbs->href);
#else	
	gnome_helpwin_goto(GNOME_HELPWIN(w), "<help>");
#endif
}
#endif

static int
my_false(GtkWidget *w, gpointer *data)
{
	return FALSE;
}


static void
gnome_helpwin_init(GnomeHelpWin *help)
{
	/* init struct _GnomeHelpWin */
	help->document_path[0] = 0;
	help->html_source = NULL;
	
	help->filters = NULL;
	help->numfilters = 0;

/*	gnome_helpwin_loadfilters(help); */
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
			(GtkArgSetFunc) NULL,
			(GtkArgGetFunc) NULL
		};

		GnomeHelpWin_type = gtk_type_unique(gtk_xmhtml_get_type(),
						    &GnomeHelpWin_info);
	}
	return GnomeHelpWin_type;
}



GtkWidget *
gnome_helpwin_new(void)
{
	GtkWidget *w;
	GnomeHelpWin *help;

	help = gtk_type_new(gnome_helpwin_get_type());

/* let application trap this signal */
#if 0
	gtk_signal_connect_object(GTK_OBJECT(help), "activate",
		   GTK_SIGNAL_FUNC(xmhtml_activate), GTK_OBJECT(help));
#endif

#if 0	
	if (filename) {
		gnome_helpwin_goto(help, filename);
		help->home = g_strdup(filename);
	}
#endif

	return GTK_WIDGET(help);
}




guint
gnome_helpwin_close(GnomeHelpWin *w)
{
        gtk_widget_hide(GTK_WIDGET(w));
        return FALSE;
}

static void
jump_to_anchor(GtkXmHTML *w, char *html, char *anchor)
{
        /* Okay, after reading much of the sources of gtk-XmHTML, I
           found the right function to call for this. But it seems not
           to be implemented yet. To let the user see, that we're
           trying to do something, I use my improper set_topline */
#if 1
        XmHTMLAnchorScrollToName(GTK_WIDGET(w), anchor);
#else
	/* TODO: this doesn't seem to work properly */
	char *p1, *p2, *s;
	int line = 0;
	
	if (!anchor) return;
	p1 = html;
	p2 = NULL;
	s = g_malloc(strlen(anchor) + 10);
	sprintf(s, "name=\"%s\"", anchor);
	while (*p1) {
		if (*p1 == '\n') {
			line++;
			p1++;
			continue;
		}
		if (*p1 == '<') {
			p1++;
			if ((*p1 == 'a') || (*p1 == 'A')) {
				p2 = s;
			}
		} else if (*p1 == '>') {
			p2 = NULL;
		}
		if (p2) {
			if (*p1 == *p2) {
				p2++;
				if (*p2 == 0) {
					/* found the line */
					gtk_xmhtml_set_topline(w, line);
					break;
				}
			} else {
				p2 = s;
			}
		}
		p1++;
	}
	g_free(s);
#endif
}


/*
 * gnome_helpwin_goto()
 *
 * Loads contents of file 'filename' in the help widget object 'help'
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
	anchor = NULL;
	
	/* TODO: parse filename for '..' */
	if (filename[0] == '/') {
		strcpy(path, filename);
	} else if (strrchr(path, '/')) {
		strcpy(strrchr(path, '/') + 1, filename);
	} else {
		strcpy(path, filename);
	}
	/* check for '#' */

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
		jump_to_anchor( GTK_XMHTML(help), help->html_source, anchor);
		return;
	}

	errno = 0;
	f = fopen(path, "rt");
	if ((errno) || (!f)) {
		if (f) fclose(f);

		if (help->html_source)
		    g_free(help->html_source);

		help->html_source= g_strdup("<body><h2>Error: file not "
					    "found</h2></body>");
		gtk_xmhtml_source( GTK_XMHTML(help), help->html_source );
		return;
	}

	str = NULL;
	while ((!feof(f)) && (!errno)) {
		if (!fgets(s, 1023, f))
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
	jump_to_anchor( GTK_XMHTML(help), str, anchor);

	if (help->html_source)
	    g_free(help->html_source);

	help->html_source = str;

}


/*
 * gnome_helpwin_load()
 *
 * Load the specified file into the help widget, possibly running a filter
 * on the file contents and sending the output to the help widget.
 * 'type' is a string, corresponding to one of the filters descr field.
 *
 */
void
gnome_helpwin_load(GnomeHelpWin *help,
		   const char *filename,
		   const char *type)
{
    FILE *f;
    char *str, *tmp, s[1024], *anchor, *path;
    int  i, havefilter, match;
    
    g_return_if_fail(help != NULL);
    g_return_if_fail(GNOME_HELPWIN_IS_HELP(help));
    g_return_if_fail(filename != NULL);
    g_return_if_fail(type != NULL);
    
    /* find the document type */
    for (i=0; i<help->numfilters; i++)
    {
	if (!strcmp(help->filters[i].descr, type))
	    break;
    }
    
    havefilter = (i != help->numfilters);
    if (havefilter)
	match = i;
    
    /* now resolve path to the source document */
    path = help->document_path;

    /* TODO: parse filename for '..' */
    if (filename[0] == '/') {
	strcpy(path, filename);
    } else if (strrchr(path, '/')) {
	strcpy(strrchr(path, '/') + 1, filename);
    } else {
	strcpy(path, filename);
    }
    
    /* check for '#' */
    if (NULL != (anchor = strrchr(path, '#'))) {
	*anchor = 0;
	anchor++;
    }
    
    /* TODO: jump to a "#anchor" in the same document should work */
    if ((path[strlen(path) - 1] == '/') ||
	(path[0] == 0)) {
	if (!help->html_source)
	    return;

	/* just jump to the anchor */
	jump_to_anchor(GTK_XMHTML(help), help->html_source, anchor);
	return;
    }

    /* ok we have a path so lets see how to filter it */
    if (!havefilter || *help->filters[match].exec == '\0') {
	errno = 0;
	f = fopen(path, "rt");
	if ((errno) || (!f)) {
	    if (f) fclose(f);
	    if (help->html_source)
		free(help->html_source);
	    help->html_source = g_strdup("<body><h2>Error: file not "
					 "found</h2></body>");
	    gtk_xmhtml_source( GTK_XMHTML(help), help->html_source);
	    return;
	}

	if (help->html_source)
	    g_free(help->html_source);

	/* if no filter, assume its some kind of ASCII text */
	if (!havefilter) {
	    str = g_malloc(12);
	    strcpy(str, "<BODY><PRE>");
	} else {
	    str  = g_malloc(1);
	    *str = '\0';
	}
	
	while ((!feof(f)) && (!errno)) {
	    if (!fgets(s, 1023, f))
		continue;
	    str = g_realloc(str, strlen(str) + strlen(s) + 1);
	    strcat(str, s);
	}
	if (!havefilter) {
	    str = g_realloc(str, strlen(str) + 16);
	    strcat(str,"</PRE></BODY>");
	}
	fclose(f);
	if (errno) {
	   g_warning("gnome_helpwin_goto: error reading file \"%s\".\n", path);
	   errno = 0;
	}
    } else {
	/* exec code here */
	str = g_malloc(80);
	g_snprintf(str, 80, "<body><h2>Error: file type %s not "
		   "implemented</h2></body>", type);
    }
    
    gtk_xmhtml_source( GTK_XMHTML(help), str);
    jump_to_anchor( GTK_XMHTML(help), str, anchor);
    if (help->html_source)
	g_free(help->html_source);
    help->html_source = str;
}


