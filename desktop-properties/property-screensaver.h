#ifndef __PROP_SCREENSAVER_H__
#define __PROP_SCREENSAVER_H__

#include "screensaver.h"
#include <gtk/gtk.h>
#include "gnome.h"
#include "gnome-desktop.h"

struct SelectInfo;

struct ConfigScreenSaver {

	GnomePropertyConfigurator *config;

	// list of known screensavers
	GHashTable *ssavers;

	// current(selected) screensaver mode
	ScreenSaverMode *curMode;

	// some important widgets
	GtkWidget *vbox, *monitor, *mlist, *sw;

	ConfigScreenSaver (GnomePropertyConfigurator *);
	void setup ();
	void apply ();

	// global screensaver parameters
	gint lockV;
	gint niceV;
	gchar *waitV;
        gint dpmsV;

	gchar *screensaver_name;
	gchar *mode_name;

	GtkWidget *lock;
        GtkWidget *check_dodpms;
        GtkWidget *ent_dpmsstandby, *ent_dpmssuspend, *ent_dpmsoff;
	GtkWidget *waitMin;
	GtkWidget *nice;

	GtkWidget *settings_frame ();
	GtkWidget *modes_frame ();
	void add_screensaver (ScreenSaver *ss);

	static void select_mode (GtkWidget *, GdkEventButton *,SelectInfo *si);
	static void test_mode (GtkWidget *, ConfigScreenSaver *th);
	static void setup_mode (GtkWidget *, ConfigScreenSaver *th);

	static void wait_changed (GtkWidget *, ConfigScreenSaver *th);
	static void nice_changed (GtkWidget *, ConfigScreenSaver *th);
	static void lock_changed (GtkWidget *, ConfigScreenSaver *th);
	static void check_dodpms_changed (GtkWidget *,
					  ConfigScreenSaver *th);

	void register_screensavers ();
};

#endif
