#ifndef __SCREENSAVER_H__
#define __SCREENSAVER_H__

#include "ghash.h"
#include <gtk/gtk.h>

enum {
	SS_PREVIEW,
	SS_SETUP,
	SS_TEST,
	SS_CMDLINE,
};

class ScreenSaver;

struct ScreenSaverMode {
	gchar *name;
	gchar *comment;
	gint lp;
	ScreenSaver *parent;

	virtual void run (gint type, ...)=0;
	virtual void stop (gint type, ...)=0;
	virtual void setup ()=0;

	ScreenSaverMode (ScreenSaver *p, gchar *n, gchar *c) {
		parent = p;
		name = g_strdup (n);
		comment = g_strdup (c);
		lp = 0;
	}

	virtual ~ScreenSaverMode () {
		g_free (name);
		g_free (comment);
	}
};

struct ScreenSaver {

	gchar *name;
	gchar *comment;
	GHashTable *modes;
	GList *modesL;

	ScreenSaver (gchar *n, gchar *c) {
		name = g_strdup (n);
		comment = g_strdup (c);
		modes = g_hash_table_new (g_hash_function_gcharp,
					  g_hash_compare_gcharp);
		modesL = NULL;
	}

	virtual ~ScreenSaver () {
		g_free (name);
		g_free (comment);
		g_hash_table_destroy (modes);
		g_list_free (modesL);
	}

	void addMode (ScreenSaverMode *m) {
		g_hash_table_insert (modes, m->name, m);
		modesL = g_list_append (modesL, (gpointer) m);
	}

};

#endif
