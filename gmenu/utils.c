/*
 * This file is a part of gmenu, the GNOME panel menu editor.
 *
 * File: gmenu.c
 *
 * This file contains main() for gmenu.  It handles creating the gmenu
 * window and widgets, and the 'sort menu' toolbar callbacks are here
 * too.
 *
 * Authors: John Ellis <johne@bellatlantic.net>
 *          Nat Friedman <nat@nat.org>
 */

#include "gmenu.h"
#include "top.xpm"
#include "unknown.xpm"

/*
 *-----------------------------------------------------------------------------
 * Utility functions (public)
 *-----------------------------------------------------------------------------
 */

gchar *check_for_dir(char *d)
{
	if (!g_file_exists(d))
		{
		g_print(_("creating user directory: %s\n"), d);
		if (mkdir( d, 0755 ) < 0)
			{
			g_print(_("unable to create user directory: %s\n"), d);
			g_free(d);
			d = NULL;
			}
		}
	return d;
}

gint isfile(gchar *s)
{
	struct stat st;

	if ((!s)||(!*s)) return 0;
	if (stat(s,&st)<0) return 0;
	if (S_ISREG(st.st_mode)) return 1;
	return 0;
}

gint isdir(gchar *s)
{
	struct stat st;
   
	if ((!s)||(!*s)) return 0;
	if (stat(s,&st)<0) return 0;
	if (S_ISDIR(st.st_mode)) return 1;
	return 0;
}



gint file_is_editable(gchar *path)
{
	if (!g_file_exists(path)) return FALSE;

	if (isdir(path))
		{
		gchar *dirpath = g_strconcat (path, ".directory", NULL);
		if (g_file_exists(dirpath))
			{
			if (!access(dirpath, W_OK))
				{
				g_free(dirpath);
				return !access(path, W_OK);
				}
			else
				{
				g_free(dirpath);
				return FALSE;
				}
			}
		g_free(dirpath);
		}

	return !access(path, W_OK);
}

gchar *remove_level_from_path(gchar *path)
{
	gint p;

	p = strlen(path);
	while(p > 0 && path[p] != '/') p--;
	if (p == 0) p++;
	return g_strndup(path, p);
}

/* returns a g_strduped string with filesystem reserved chars replaced */
gchar *validate_filename(gchar *file)
{
	gchar *ret;
	gchar *ptr;

	if (!file) return NULL;

	ret = g_strdup(file);
	ptr = ret;
	while (*ptr != '\0')
		{
		if (*ptr == '/') *ptr = '_';
		ptr++;
		}

	return ret;
}

/*
 *-----------------------------------------------------------------------------
 * Gnome pixmap functions (public)
 *-----------------------------------------------------------------------------
 */

GtkWidget *pixmap_top(void)
{
	return gnome_pixmap_new_from_xpm_d (top_xpm);
}

GtkWidget *pixmap_unknown(void)
{
	return gnome_pixmap_new_from_xpm_d (unknown_xpm);
}

GtkWidget *pixmap_load(gchar *path)
{
	GtkWidget *pixmap;

	if (!g_file_exists(path)) return pixmap_unknown();

	pixmap = gnome_stock_pixmap_widget_at_size (NULL, path, 20, 20);
	if (!pixmap) pixmap = pixmap_unknown();

	return pixmap;
}
