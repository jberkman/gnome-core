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

#include <gtk/gtk.h>
#include <gdk/gdk.h>

/* needed to get font xlfd via XA_FONT */
#include <gdk/gdkx.h>
#include <X11/Xatom.h>

#include <config.h>

#include "grdb.h"

/* since gtk themes don't have a standard fixed font,
 * this will be used if the user doesn't specify one.
 */
#ifndef FIXED_FONT_NAME
#define FIXED_FONT_NAME "fixed"
#endif

#ifndef DEFAULT_GTK_FONT
#define DEFAULT_GTK_FONT \
  "-adobe-helvetica-medium-r-normal--*-120-*-*-*-*-*-*"
#endif

/* used to hold the various font styles we derive from
 * theme's default font. 
 */
typedef struct xlfd_variants {
	gchar* normal;
	gchar* bold;
	gchar* italic;
} xlfd_variants;

/* quick and dirty guess if str could be an xlfd */
static gboolean str_is_xlfd (const gchar* str)
{
	g_return_val_if_fail ((str != NULL), FALSE);

	return (str[0] == '-') && (strlen(str) > 30);
}

static gchar* get_xlfd_from_style (const GtkStyle* style)
{
	char *cp = NULL;

	/* this fails with truetype fonts and xfstt */
	if (style->font->type == GDK_FONT_FONT) {
		XFontStruct*  xfont = GDK_FONT_XFONT (style->font);
		char *tmp;
		unsigned long value = 0;
		if (XGetFontProperty (xfont, XA_FONT, &value)) {
			tmp = XGetAtomName (GDK_DISPLAY (), value);
			if (str_is_xlfd (tmp)) {
				cp = g_strdup (tmp);
			}
			XFree (tmp);
		}
	}


	if (!cp && style->rc_style) {
		if (str_is_xlfd (style->rc_style->font_name)) {
			cp = g_strdup (style->rc_style->font_name);
		}
	}

	/* last resort */
	if (!cp) {
		cp = g_strdup (DEFAULT_GTK_FONT);
	}

	return cp;
}

static gboolean xlfd_exists (const gchar* xlfd)
{
	GdkFont* font;
	font = gdk_font_load (xlfd);
	if (font) {
		gdk_font_unref (font);
		return TRUE;
	}
	return FALSE;
}

static gchar* make_bold_xlfd (gchar** xlfd_pieces)
{
	gchar* cp;
	gchar* bold;

	cp = xlfd_pieces[3];
	xlfd_pieces[3] = "bold";
	bold = g_strjoinv ("-", xlfd_pieces);
	xlfd_pieces[3] = cp;

	if (xlfd_exists (bold)) {
		return bold;
	}

	g_free (bold);
	return NULL;
}

static gchar* make_italic_xlfd (gchar** xlfd_pieces)
{
	gchar* cp;
	gchar* italic;

	cp = xlfd_pieces[4];
	xlfd_pieces[4] = "i";
	italic = g_strjoinv ("-", xlfd_pieces);
	xlfd_pieces[4] = cp;

	if (xlfd_exists (italic)) {
		return italic;
	}

	g_free (italic);
        /* cp = xlfd_pieces[4];
	 * (from above)
	 */
	xlfd_pieces[4] = "o";
	italic = g_strjoinv ("-", xlfd_pieces);
	xlfd_pieces[4] = cp;

	if (xlfd_exists (italic)) {
		return italic;
	}
	
	g_free (italic);
	return NULL;
}

static xlfd_variants* get_xlfd_variants (const gchar* xlfd)
{
	xlfd_variants* fonts;
	gchar** xlfd_pieces;
	gchar*  cp;

	xlfd_pieces = g_strsplit (xlfd, "-", 14);

	/* The average width field is troublesome because
	 * bold/italic fonts tend to be wider than medium fonts.
	 *
	 * We wildcard here and hope the xserver finds the best match.
	 */
	cp = xlfd_pieces[12];
	xlfd_pieces[12] = "*";

	fonts = g_new (xlfd_variants, 1);
	fonts->normal = g_strdup (xlfd);

	fonts->bold = make_bold_xlfd (xlfd_pieces);
	if (!fonts->bold) {
		/* fallback to original */
		fonts->bold = g_strdup (xlfd);
	}

	fonts->italic = make_italic_xlfd (xlfd_pieces);
	if (!fonts->italic) {
		/* fallback to original */
		fonts->italic = g_strdup (xlfd);
	}

	xlfd_pieces[12] = cp;

	g_strfreev (xlfd_pieces);
	return fonts;
}

static void free_xlfd_variants (xlfd_variants* fonts)
{
	g_free (fonts->normal);
	g_free (fonts->bold);
	g_free (fonts->italic);
	g_free (fonts);
}

static gchar* get_fixed_font_xlfd (void)
{
	char *cp = NULL;


	/* Did the user set a default via env var? */
	cp = g_getenv ("GRDB_FIXED_FONT");
	if (cp != NULL) {
		return g_strdup (cp);
	}

	/* use the hardcoded default */
	return g_strdup (FIXED_FONT_NAME);
}

/* returns non-zero if we couldn't get an xlfd from the GdkFont,
 * this means we don't want to do any font processing at all */
int append_fonts_prefix_str (GString*     pre,
			     GtkStyle*    style,
			     const gchar* fixed_xlfd)
{
	gchar*         xlfd;
	gchar*         fixed_font_xlfd;
	xlfd_variants* fonts;

	xlfd = get_xlfd_from_style (style);
	if (!xlfd) {
		return 1;
	}

	if (fixed_xlfd) {
		fixed_font_xlfd = g_strdup (fixed_xlfd);
	} else {
		fixed_font_xlfd = get_fixed_font_xlfd ();
	}

	fonts = get_xlfd_variants (xlfd);
	
	g_string_sprintfa (pre, "#define FIXED_FONT   %s\n",
			   fixed_font_xlfd);
	
	g_string_sprintfa (pre, "#define FONT         %s\n",
			   fonts->normal);
	g_string_sprintfa (pre, "#define BOLD_FONT    %s\n",
			   fonts->bold);
	g_string_sprintfa (pre, "#define ITALIC_FONT  %s\n",
			   fonts->italic);
	g_string_sprintfa (pre, "#define FONTLIST "
			   "FONT,BOLD_FONT=BOLD,"
			   "ITALIC_FONT=ITALIC\n");
	
	free_xlfd_variants (fonts);
	g_free (fixed_font_xlfd);
	g_free (xlfd);
	xlfd = NULL;

	return 0;
}
