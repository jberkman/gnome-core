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

#ifndef GNOME_DATADIR
#error  GNOME_DATADIR must be defined in Makefile
#endif

#define SYSTEM_AD_DIR GNOME_DATADIR "/grdb"

#ifndef PATH_TO_XRDB
#error  PATH_TO_XRDB must be defined in Makefile
#endif

static int   Verbose   = FALSE;     /* output progress as we run */
static int   SkipFonts = FALSE;     /* don't try to do fonts */
static int   DumpConstants = FALSE; /* just print constants to stdout */
static char* FixedFont = NULL;      /* xlfd passed in via command line */

static struct poptOption options[] = {
	{ "constants", 'c', 0, &DumpConstants, 0,
	  "Print the constants grdb defines and exit", NULL },
	{ "fixed", 'f', POPT_ARG_STRING, &FixedFont,
	  0, "Fixed Font", "XLFD" },
	{ "nofonts", 'n', 0, &SkipFonts,
	  0, "Disable setting fonts", NULL },
	{ "verbose", 'V', 0, &Verbose, 0, "Verbose mode", NULL },
	POPT_AUTOHELP
	{ NULL, '\0', 0, NULL, 0 }
};
 
static GtkStyle *get_style (void)
{
	GtkStyle *s;
	GtkWidget *w;

	w = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_widget_ensure_style (w);
	s = gtk_style_copy (gtk_widget_get_style (w));
	gtk_widget_destroy (w);

	return s;
}

static GString* build_prefix_str (void)
{
	GString*  pre;
	GtkStyle* style;

	style = get_style ();

	if (!style)
		g_error (_("style is NULL!"));

	pre = g_string_sized_new (256);

	append_colors_prefix_str (pre, style);

	/* don't even bother trying if user has requested no fonts */
	if (!SkipFonts) {
		SkipFonts = append_fonts_prefix_str (pre,
						     style,
						     FixedFont);
	}

	gtk_style_unref (style);

	if (Verbose)
		g_print (_("--- PREFIX ---\n%s--------------\n"),
			 pre->str);

	return pre;
}

static gint cmp_strings (gconstpointer a, gconstpointer b)
{
	return strcmp (a,b);
}

static gint cmp_basenames (gconstpointer a, gconstpointer b)
{
	return strcmp (g_basename (a),g_basename (b));
}

static GSList* scan_ad_directory (const gchar *dir_path)
{
	DIR*           ad_dir;
	struct dirent* entry;
	GSList*        list    = NULL;

	g_return_val_if_fail ((dir_path != NULL), NULL);

	ad_dir = opendir (dir_path);
	if (ad_dir == NULL)
		return NULL;

	while ((entry = readdir (ad_dir)) != NULL) {
		char* name = entry->d_name;
		int   len  = strlen (name);
		if ((len > 3) && (strcmp (name+len-3,".ad") == 0)) {
			list = g_slist_prepend (list,
				g_strdup_printf ("%s/%s",
						 dir_path,
						 entry->d_name));
		}
	}
	return g_slist_sort (list, cmp_strings);
}

static void free_ad_directory_list (GSList* list)
{
	GSList *p;
	for (p=list; p != NULL; p = g_slist_next (p)) {
		g_free (p->data);
	}
	g_slist_free(list);
}

/* return 0 on successful append */
static gint append_file_contents (FILE* to, const gchar* file)
{
	char  buffer[256];
	FILE* from;

	g_assert (to);
	g_assert (file);

	from = fopen (file, "r");
	if (!from)
		return -1;

	while (fgets (buffer, sizeof (buffer), from)) {
		if (SkipFonts && strstr (buffer, "FONT")) {
			continue;
		}
		fputs (buffer, to);
	}
	fclose (from);
	return 0;
}

int main (int argc, char *argv[])
{
	GString*  pre;
	GSList*   sys_list  = NULL;
	GSList*   user_list = NULL;
	GSList*   p;
	FILE*     fp;
	gchar*    command;
	gchar*    home;
	poptContext ctx;
	int         rc;

	bindtextdomain (PACKAGE, GNOMELOCALEDIR);
	textdomain (PACKAGE);

	gtk_init (&argc, &argv);

	ctx = poptGetContext ("grdb", argc,
			      (const char**)argv, options, 0);
	if ( (rc = poptGetNextOpt (ctx)) < -1 ) {
		g_print ("bad argument %s: %s\n\n",
			 poptBadOption (ctx, POPT_BADOPTION_NOALIAS),
			 poptStrerror (rc));
		poptPrintHelp (ctx, stdout, 0);
		poptFreeContext (ctx);
		exit (-1);
	}
	poptFreeContext (ctx);
	ctx = NULL;

	command = g_strdup_printf ("%s -merge", PATH_TO_XRDB);
	fp = popen (command, "w");
	if (!fp) {
		g_error (_("Cannot open xrdb command.\n"));
	}
	g_free (command);

	pre = build_prefix_str ();

	if (DumpConstants) {
		puts (pre->str);
		g_string_free (pre, TRUE);
		exit (0);
	}

	fputs (pre->str, fp);
	g_string_free (pre, TRUE);

	sys_list = scan_ad_directory (SYSTEM_AD_DIR);
	if (!sys_list) {
		g_warning( _("Cannot scan system directory: %s.\n"),
		            SYSTEM_AD_DIR);
	}

	home = g_get_home_dir (); 
	if (home) {
		gchar* user_ad = g_strdup_printf ("%s/.grdb", home);
		user_list = scan_ad_directory (user_ad);
		g_free (user_ad);
		user_ad = NULL;
	} else {
		g_warning (_("Cannot determine user's home directory."));
	}

	for (p=sys_list; p != NULL; p = g_slist_next (p)) {
		if (g_slist_find_custom (user_list, p->data,
					 cmp_basenames)) {
			if (Verbose)
				g_print (_("skipping %s for user version.\n"),
					 p->data);
			continue; /* we'll just apply user's later */
		}

		if (Verbose)
			g_print (_("Processing %s.\n"), p->data);

		if (append_file_contents (fp, p->data) != 0)
			g_warning (_("Problem appending %s.\n"),
				   (gchar*)p->data);
	}
	free_ad_directory_list (sys_list);

	for (p=user_list; p != NULL; p = g_slist_next (p)) {
		if (Verbose)
			g_print (_("Processing %s.\n"), p->data);

		if (append_file_contents (fp, p->data) != 0)
			g_warning (_("Problem appending %s.\n"),
				   (gchar*)p->data);
	}
	free_ad_directory_list (user_list);

	if (home) {
		gchar* xdflt = g_strdup_printf ("%s/.Xdefaults", home);

		/* might not exist, that's ok */
		(void)append_file_contents (fp, xdflt);

		g_free (xdflt);
		xdflt = NULL;
	}

	if (pclose (fp) != 0)
		g_error (_("Error writing to xrdb."));

	if (Verbose)
		g_print (_("grdb completed successfully.\n"));


	return 0;
}
