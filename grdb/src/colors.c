/*
 * grdb: prime xrdb settings with gtk theme colors
 *
 * This is a public release of grdb, a tool to apply gtk theme
 * color schemes to non-gtk programs.
 * 
 * (C) Copyright 2000 Samuel Hunter <shunter@bigsky.net>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
 *
 * Author: Sam Hunter <shunter@bigsky.net>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include <popt.h>

#include <config.h>

#include "grdb.h"

static void append_color_prefix (GString *str, const gchar* name,
				 GdkColor* col)
{
	g_string_sprintfa(str, "#define %s #%2.2hx%2.2hx%2.2hx\n",
			  name, col->red>>8, col->green>>8, col->blue>>8);
}

/*
 * quick and dirty guess at what the shaded colors should be
 */
static GdkColor* style_shade (GdkColor *a,
			      gboolean true_is_light, GdkColor *b)
{
	gulong   red;
	gulong   green;
	gulong   blue;

	if(true_is_light) {
		red   = (a->red + 2*0xffff)/3;
		green = (a->green + 2*0xffff)/3;
		blue  = (a->blue + 2*0xffff)/3;
	} else {
		red   = ((gulong)a->red)*2/3;
		green = ((gulong)a->green)*2/3;
		blue  = ((gulong)a->blue)*2/3;
	}

	b->red = (gshort)red;
	b->green = (gshort)green;
	b->blue = (gshort)blue;

	return b;
}

void append_colors_prefix_str (GString* pre, GtkStyle* style)
{
	GdkColor tmp;

	append_color_prefix(pre, "BACKGROUND",
			    &style->bg[GTK_STATE_NORMAL]);
	append_color_prefix(pre, "FOREGROUND",
			    &style->fg[GTK_STATE_NORMAL]);
	append_color_prefix(pre, "SELECT_BACKGROUND",
			    &style->bg[GTK_STATE_SELECTED]);
	append_color_prefix(pre, "SELECT_FOREGROUND",
			    &style->text[GTK_STATE_SELECTED]);
	append_color_prefix(pre, "WINDOW_BACKGROUND",
			    &style->base[GTK_STATE_NORMAL]);
	append_color_prefix(pre, "WINDOW_FOREGROUND",
			    &style->text[GTK_STATE_NORMAL]);

	append_color_prefix(pre, "INACTIVE_BACKGROUND",
			    &style->bg[GTK_STATE_INSENSITIVE]);
	append_color_prefix(pre, "INACTIVE_FOREGROUND",
			    &style->text[GTK_STATE_INSENSITIVE]);

	append_color_prefix(pre, "ACTIVE_BACKGROUND",
			    &style->bg[GTK_STATE_SELECTED]);
	append_color_prefix(pre, "ACTIVE_FOREGROUND",
			    &style->text[GTK_STATE_SELECTED]);

	append_color_prefix(pre, "HIGHLIGHT",
			    style_shade(&style->bg[GTK_STATE_NORMAL],
					TRUE, &tmp));
	append_color_prefix(pre, "LOWLIGHT",
			    style_shade(&style->bg[GTK_STATE_NORMAL],
					FALSE, &tmp));
}
