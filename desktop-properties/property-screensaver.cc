/*
 * Screen Saver property configurator
 * Written by Radek Doulik, 1997 <doulik@karlin.mff.cuni.cz>
 */

#include "property-screensaver.h"
#include "xlockmore.h"
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <gtk/gtk.h>
#ifdef HAVE_LIBINTL
#include <libintl.h>
#define _(String) gettext(String)
#else
#define _(String) (String)
#endif
#include "gnome.h"
#include "gnome-desktop.h"

ConfigScreenSaver *css;

static gint screensaver_action (GnomePropertyRequest req);

struct SelectInfo {
	ConfigScreenSaver *th;
	ScreenSaverMode *m;
	
	SelectInfo (ConfigScreenSaver *t, ScreenSaverMode *p) : th (t), m(p) {}
};

void
ConfigScreenSaver::select_mode (GtkWidget *, GdkEventButton *, SelectInfo *si)
{
	si->th->curMode = si->m;
	si->m->run (SS_PREVIEW,
		    (gint)(GTK_RANGE (si->th->nice)->adjustment->value),
		    si->th->monitor,
		    GNOME_MONITOR_WIDGET_X,
		    GNOME_MONITOR_WIDGET_Y+6,
		    GNOME_MONITOR_WIDGET_WIDTH,
		    GNOME_MONITOR_WIDGET_HEIGHT);
}

void
ConfigScreenSaver::test_mode (GtkWidget *, ConfigScreenSaver *th)
{
	if (th->curMode)
		th->curMode->run (SS_TEST,
				  (gint)(GTK_RANGE (th->nice)->adjustment->value),
				  GTK_TOGGLE_BUTTON (th->lock)->active);
}

void
ConfigScreenSaver::setup_mode (GtkWidget *, ConfigScreenSaver *th)
{
	if (th->curMode)
		th->curMode->setup ();
}

static void
insert_mode_to_list (ScreenSaverMode *m, ConfigScreenSaver *th)
{
	GtkWidget *listItem;

        // printf ("adding %s\n", m->name);
	listItem = gtk_list_item_new_with_label (m->name);
	gtk_signal_connect (GTK_OBJECT (listItem),
			    "button_press_event",
			    (GtkSignalFunc) ConfigScreenSaver::select_mode,
			    (gpointer)new SelectInfo (th, m));
	gtk_container_add (GTK_CONTAINER (th->mlist), listItem);
	gtk_widget_show (listItem);

	m->lp = gtk_list_child_position (GTK_LIST (th->mlist), listItem);
}

static void
insert_screensaver_modes (gpointer key, ScreenSaver *ss, ConfigScreenSaver *th)
{ 
	g_list_foreach (ss->modesL, (GFunc) insert_mode_to_list, th);
}


void
ConfigScreenSaver::add_screensaver (ScreenSaver *ss)
{
	g_hash_table_insert (ssavers, ss->name, ss);
}

void
ConfigScreenSaver::register_screensavers ()
{
	// add all known screensavers (ok, now we have only xlockmore :)
	add_screensaver (new XLockMore);
}


void
ConfigScreenSaver::wait_changed (GtkWidget *entry, ConfigScreenSaver *th)
{
	// printf ("wait changed %s\n", GTK_ENTRY (entry)->text);
	th->waitV = GTK_ENTRY (entry)->text;
}

void
ConfigScreenSaver::nice_changed (GtkWidget *adj, ConfigScreenSaver *th)
{
	// printf ("nice changed %d\n", (gint)GTK_ADJUSTMENT (adj)->value);
	th->nice = (gint)GTK_ADJUSTMENT (adj)->value;
}

void
ConfigScreenSaver::lock_changed (GtkWidget *check, ConfigScreenSaver *th)
{
	// printf ("lock changed %d\n", GTK_TOGGLE_BUTTON (check)->active);
	th->lockV = GTK_TOGGLE_BUTTON (check)->active;
}

GtkWidget *
ConfigScreenSaver::settings_frame ()
{
	GtkWidget *vbox, *hb1;
	GtkWidget *f, *l1, *l2;
	GtkObject *adjustment;
	GtkWidget *vb1, *l3, *hb2, *l4, *l5;
	
	f  = gtk_frame_new (_("Settings"));

	l1 = gtk_label_new (_("Wait"));
	l2 = gtk_label_new (_("min"));
	waitMin  = gtk_entry_new ();
	gtk_entry_set_text (GTK_ENTRY (waitMin),
			    waitV);
	gtk_signal_connect (GTK_OBJECT (waitMin),
			    "changed",
			    (GtkSignalFunc) ConfigScreenSaver::wait_changed,
			    this);

	gtk_widget_set_usize (waitMin, 50, -1);
	hb1 = gtk_hbox_new (FALSE, 0);

	lock = gtk_check_button_new_with_label (_("requires password"));
	gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (lock),
				     lockV);
	gtk_signal_connect (GTK_OBJECT (lock),
			    "toggled",
			    (GtkSignalFunc) ConfigScreenSaver::lock_changed,
			    this);

	vb1 = gtk_vbox_new (FALSE, 0);
	l3 = gtk_label_new (_("Priority"));
	gtk_misc_set_alignment (GTK_MISC (l3), 0, 0.5);

	adjustment = gtk_adjustment_new (niceV,
					 0.0, 19.0, 1.0, 1.0, 0.0);
	nice = gtk_hscale_new (GTK_ADJUSTMENT (adjustment));
	gtk_scale_set_digits (GTK_SCALE (nice), 0);
	gtk_scale_set_draw_value (GTK_SCALE (nice), FALSE);
	l4 = gtk_label_new (_("normal"));
	l5 = gtk_label_new (_("low"));
	hb2 = gtk_hbox_new (FALSE, 0);
	gtk_signal_connect (GTK_OBJECT (adjustment),
			    "value_changed",
			    (GtkSignalFunc) ConfigScreenSaver::nice_changed,
			    this);
     
	vbox = gtk_vbox_new (FALSE, GNOME_PAD);
	gtk_container_border_width (GTK_CONTAINER (vbox), GNOME_PAD);

	gtk_box_pack_start (GTK_BOX (hb1), l1, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hb1), waitMin, TRUE, TRUE, GNOME_PAD);
	gtk_box_pack_start (GTK_BOX (hb1), l2, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), hb1, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), lock, FALSE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (vb1), l3, FALSE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (vb1), nice, FALSE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (hb2), l4, FALSE, FALSE, 0);
	gtk_box_pack_end (GTK_BOX (hb2), l5, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vb1), hb2, FALSE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), vb1, FALSE, TRUE, 0);
	gtk_container_add (GTK_CONTAINER (f), vbox);

	gtk_widget_show (l1);
	gtk_widget_show (l2);
	gtk_widget_show (waitMin);
	gtk_widget_show (lock);
	gtk_widget_show (l3);
	gtk_widget_show (nice);
	gtk_widget_show (l4);
	gtk_widget_show (l5);
	gtk_widget_show (hb1);
	gtk_widget_show (hb2);
	gtk_widget_show (vb1);
	gtk_widget_show (vbox);

	return f;
}

GtkWidget *
ConfigScreenSaver::modes_frame ()
{
	GtkWidget *f, *hb1, *vb1, *b1, *b2;

	f  = gtk_frame_new (_("Screen savers"));

	hb1 = gtk_hbox_new (FALSE, GNOME_PAD);
	gtk_container_border_width (GTK_CONTAINER (hb1), GNOME_PAD);

	sw = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
					GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	mlist = gtk_list_new ();
	gtk_list_set_selection_mode (GTK_LIST (mlist), GTK_SELECTION_BROWSE);
	gtk_widget_set_usize (f, 240, -1);
	vb1 = gtk_vbox_new (FALSE, GNOME_PAD);
	b1 = gtk_button_new_with_label (_("Setup"));
	gtk_signal_connect (GTK_OBJECT (b1), "clicked",
			    (GtkSignalFunc) setup_mode, (gpointer)this);

	b2 = gtk_button_new_with_label (_("Test"));
	gtk_signal_connect (GTK_OBJECT (b2), "clicked",
			    (GtkSignalFunc) test_mode, (gpointer)this);



	gtk_container_add (GTK_CONTAINER (sw), mlist);
	gtk_box_pack_start (GTK_BOX (hb1), sw, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (vb1), b1, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vb1), b2, FALSE, FALSE, 0);
	gtk_box_pack_end (GTK_BOX (hb1), vb1, FALSE, FALSE, 0);
	gtk_container_add (GTK_CONTAINER (f), hb1);

	g_hash_table_foreach (ssavers, (GHFunc) insert_screensaver_modes, this);

	gtk_widget_show (sw);
	gtk_widget_show (mlist);
	gtk_widget_show (b1);
	gtk_widget_show (b2);
	gtk_widget_show (vb1);
	gtk_widget_show (hb1);

	return f;
}

static void
runPreviewXLock (GtkWidget *w, ConfigScreenSaver *c)
{
	// printf ("map\n");
	if (c->curMode)
		c->curMode->run (SS_PREVIEW,
				 (gint)(GTK_RANGE (c->nice)->adjustment->value),
				 c->monitor,
				 GNOME_MONITOR_WIDGET_X,
				 GNOME_MONITOR_WIDGET_Y+6,
				 GNOME_MONITOR_WIDGET_WIDTH,
				 GNOME_MONITOR_WIDGET_HEIGHT);
}

static void
killPreviewXLock (GtkWidget *w, ConfigScreenSaver *c)
{
	// printf ("unmap\n");
	if (c->curMode)
		c->curMode->stop (SS_PREVIEW);
}

ConfigScreenSaver::ConfigScreenSaver (GnomePropertyConfigurator *c)
{
	config = c;
	curMode = NULL;
	ssavers = g_hash_table_new (g_hash_function_gcharp,
				    g_hash_compare_gcharp);
	

	register_screensavers ();
	gnome_property_configurator_register (config, screensaver_action);
}

void
ConfigScreenSaver::setup ()
{
	GtkWidget *hbox, *bottom;
	GtkWidget *settings, *modes;
	
	vbox = gtk_vbox_new (TRUE, 0);
	hbox = gtk_hbox_new (TRUE, 0);
	gtk_container_border_width (GTK_CONTAINER (hbox), GNOME_PAD);
	bottom = gtk_hbox_new (FALSE, GNOME_PAD);
	gtk_container_border_width (GTK_CONTAINER (bottom), GNOME_PAD);

	this->monitor = get_monitor_preview_widget (config->notebook);
	gtk_signal_connect (GTK_OBJECT (monitor), "map",
			    (GtkSignalFunc) runPreviewXLock, this);
	gtk_signal_connect (GTK_OBJECT (monitor), "unmap",
			    (GtkSignalFunc) killPreviewXLock, this);
	gtk_signal_connect (GTK_OBJECT (monitor), "destroy",
			    (GtkSignalFunc) killPreviewXLock, this);


	settings = settings_frame ();
	modes = modes_frame ();

	gtk_box_pack_start (GTK_BOX(hbox), monitor, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX(bottom), settings, FALSE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX(bottom), modes, TRUE, TRUE, 0);
	gtk_box_pack_end (GTK_BOX (vbox), bottom, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

	gtk_widget_show (monitor);
	gtk_widget_show (settings);
	gtk_widget_show (modes);
	gtk_widget_show (hbox);
	gtk_widget_show (bottom);
	gtk_widget_show (vbox);

	gtk_list_select_item (GTK_LIST (mlist), curMode->lp);

	gtk_notebook_append_page (GTK_NOTEBOOK (config->notebook),
				  vbox,
				  gtk_label_new (_(" Screensaver ")));
}


void
ConfigScreenSaver::apply ()
{
	gchar *cmdLine;

	if (curMode) {
		gint pid;

		curMode->run (SS_CMDLINE, niceV, lockV, &cmdLine);

		/* pid = gnome_config_get_int ("/Desktop/ScreenSaver/xautolock_pid=0");
	        if (pid)
			::kill (pid, SIGTERM); */

		pid = fork ();

		if (!pid) {
			execlp ("xautolock",
				"xautolock",
				"-corners",
				"++++",
				"-time",
				waitV,
				"-locker",
				cmdLine,
				NULL);
		} else {
			gnome_config_set_int ("/Desktop/ScreenSaver/xautolock_pid",
					      pid);
			gnome_config_sync ();
		}
	}
}

static gint
screensaver_read ()
{
	css->niceV = gnome_config_get_int ("/Desktop/ScreenSaver/nice=12");
	css->lockV = gnome_config_get_int ("/Desktop/ScreenSaver/lock=1");
	css->waitV = g_strdup
		(gnome_config_get_string ("/Desktop/ScreenSaver/waitMin=5"));

	css->screensaver_name = g_strdup
		(gnome_config_get_string ("/Desktop/ScreenSaver/screensaver"
					  "=xlockmore"));
	css->mode_name = g_strdup
		(gnome_config_get_string ("/Desktop/ScreenSaver/mode=blank"));

	ScreenSaver *ss =
		(ScreenSaver *) g_hash_table_lookup (css->ssavers,
						     css->screensaver_name);

	if (ss)
		css->curMode = (ScreenSaverMode *)
			g_hash_table_lookup (ss->modes, css->mode_name);
	return 1;
}

static gint
screensaver_write ()
{
	gnome_config_set_int ("/Desktop/ScreenSaver/nice", css->niceV);
	gnome_config_set_int ("/Desktop/ScreenSaver/lock", css->lockV);
	gnome_config_set_string ("/Desktop/ScreenSaver/waitMin", css->waitV);

	gnome_config_set_string ("/Desktop/ScreenSaver/screensaver",
				 css->screensaver_name);
	gnome_config_set_string ("/Desktop/ScreenSaver/mode", css->mode_name);

	return 1;
}

static gint
screensaver_apply ()
{
	css->apply ();
}

static gint
screensaver_setup ()
{
	css->setup ();
}

static gint
screensaver_action (GnomePropertyRequest req)
{
	switch (req) {
	case GNOME_PROPERTY_READ:
		screensaver_read ();
		break;
	case GNOME_PROPERTY_WRITE:
		screensaver_write ();
		break;
	case GNOME_PROPERTY_APPLY:
		screensaver_apply ();
		break;
	case GNOME_PROPERTY_SETUP:
		screensaver_setup ();
		break;
	default:
		return 0;
	}

	return 1;
}

extern "C" {
	void
		screensaver_register (GnomePropertyConfigurator *c)
		{
			css = new ConfigScreenSaver (c);
		}
}
