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

#ifndef __GNOME_HELPWIN_H__
#define __GNOME_HELPWIN_H__


#include <gnome.h>
#include <gtk-xmhtml/gtk-xmhtml.h>

BEGIN_GNOME_DECLS


/* filters convert a particular file type to HTML               */
/* it is assumed that the filter takes the file from stdin, and */
/* sends the converted data to stdout                           */

typedef gint DocType;

struct _HTMLFilter {
    DocType type;        /* key for type, assigned when created */
    gchar   *descr;      /* string describing type of file */
    gchar   *exec;       /* path of executable to run , a '{}' is */
                         /* replaced by the name of the file */
};

typedef struct _HTMLFilter   HTMLFilter;

/* widget related macros and structures */
#define GNOME_HELPWIN(obj) GTK_CHECK_CAST(obj, gnome_helpwin_get_type(), GnomeHelpWin)
#define GNOME_HELPWIN_CLASS(klass) GTK_CHECK_CAST_CLASS(klass, gnome_helpwin_get_type(), GnomeHelpWinClass)
#define GNOME_HELPWIN_IS_HELP(obj) GTK_CHECK_TYPE(obj, gnome_helpwin_get_type())

typedef struct _GnomeHelpWin GnomeHelpWin;
typedef struct _GnomeHelpWinClass GnomeHelpWinClass;

struct _GnomeHelpWin {
    GtkXmHTML parent;

    gchar document_path[1024];
    gchar *html_source;
    
    gint       numfilters;
    HTMLFilter *filters;
};

struct _GnomeHelpWinClass {
    GtkXmHTMLClass parent_class;
};




guint gnome_helpwin_get_type(void);
GtkWidget *gnome_helpwin_new(void);
guint gnome_helpwin_close(GnomeHelpWin *help);

/* load file straight into the HTML widget */
void gnome_helpwin_goto(GnomeHelpWin *help,  const char *filename);

/* load file into HTML widget, using filter for file type 'type' */
void gnome_helpwin_load(GnomeHelpWin *help,  const char *filename,
			const char *type);

END_GNOME_DECLS


#endif /* __GNOME_HELPWIN_H__ */

