/* main.c - Main program for desktop properties application.  */

#include <stdio.h>

#include "gnome.h"
#include "gnome-desktop.h"
#include "libgnomeui/gnome-session.h"

GtkWidget *main_window;
GnomePropertyConfigurator *display_config;

/* The Apply button.  */
static GtkWidget *apply_button;

GtkWidget *
get_monitor_preview_widget (GtkWidget *window)
{
	GtkWidget *pwid;
	GdkPixmap *pixmap, *mask;
	GtkStyle  *style;
	char *f;
	
	style = gtk_widget_get_style (window);
	f = gnome_datadir_file ("pixmaps/monitor.xpm");
	/* FIXME if f is 0, alert () */
	pixmap = gdk_pixmap_create_from_xpm (window->window, &mask,
					     &style->bg [GTK_STATE_NORMAL], f);
	free (f);

	pwid = gtk_pixmap_new (pixmap, mask);
	gtk_widget_show (pwid);

	return pwid;
}

/* Enable the Apply button.  */
void
property_changed (void)
{
  gtk_widget_set_sensitive (apply_button, TRUE);
}

static gint
deleteFn (GtkWidget *widget, gpointer *data)
{
	gtk_widget_destroy (main_window);
	gtk_main_quit ();

	return TRUE;
}

/* This is called when the Help button is clicked.  */
static gint
help (GtkWidget *w, gpointer *data)
{
  /* FIXME.  */
}

static void
display_properties_action (GtkWidget *w, gint close)
{
  gnome_property_configurator_request_foreach (display_config,
					       GNOME_PROPERTY_APPLY);

  gtk_widget_set_sensitive (apply_button, FALSE);

  if (close)
    {
      gnome_property_configurator_request_foreach (display_config,
						   GNOME_PROPERTY_WRITE);
      gnome_config_sync ();
      deleteFn (NULL, NULL);
    }
}

void
display_properties_setup (void)
{
	GtkWidget *vbox = gtk_vbox_new (FALSE, 0),
		*hbox = gtk_hbox_new (FALSE, GNOME_PAD),
		*bf = gtk_frame_new (NULL),
		*bok = gtk_button_new_with_label (_("  OK  ")),
		*bapl = gtk_button_new_with_label (_(" Apply ")),
		*bcl = gtk_button_new_with_label (_(" Cancel ")),
	        *bhelp = gtk_button_new_with_label (_("Help"));

	apply_button = bapl;
	gtk_widget_set_sensitive (apply_button, FALSE);

	main_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (main_window), application_title ());
	gtk_window_set_policy (GTK_WINDOW (main_window), FALSE, FALSE, TRUE);
	gtk_signal_connect (GTK_OBJECT (main_window), "delete_event",
			    GTK_SIGNAL_FUNC (deleteFn), NULL);	

	gtk_signal_connect (GTK_OBJECT (bhelp), "clicked",
			    GTK_SIGNAL_FUNC (help), NULL);

	gtk_signal_connect (GTK_OBJECT (bcl), "clicked",
			    GTK_SIGNAL_FUNC (deleteFn), NULL);	

	gtk_signal_connect (GTK_OBJECT (bok), "clicked",
			    GTK_SIGNAL_FUNC (display_properties_action), (gpointer) 1);

	gtk_signal_connect (GTK_OBJECT (bapl), "clicked",
			    GTK_SIGNAL_FUNC (display_properties_action), (gpointer) 0);

	gtk_container_border_width (GTK_CONTAINER (hbox), GNOME_PAD);
	gtk_frame_set_shadow_type (GTK_FRAME (bf), GTK_SHADOW_OUT);

	/* configurators_setup (); */
	gnome_property_configurator_setup (display_config);
	gnome_property_configurator_request_foreach (display_config,
						     GNOME_PROPERTY_SETUP);
	
	gtk_container_add (GTK_CONTAINER(main_window), vbox);
	gtk_box_pack_start (GTK_BOX (vbox), display_config->notebook, FALSE, FALSE, 0);
	
	gtk_box_pack_end (GTK_BOX (hbox), bhelp, FALSE, FALSE, 0);
	gtk_box_pack_end (GTK_BOX (hbox), bcl, FALSE, FALSE, 0);
	gtk_box_pack_end (GTK_BOX (hbox), bapl, FALSE, FALSE, 0);
	gtk_box_pack_end (GTK_BOX (hbox), bok, FALSE, FALSE, 0);
	gtk_container_add (GTK_CONTAINER(bf), hbox);
	gtk_box_pack_start (GTK_BOX (vbox), bf, FALSE, FALSE, 0);

	gtk_widget_show (bok);
	gtk_widget_show (bapl);
	gtk_widget_show (bcl);
	gtk_widget_show (bhelp);
	gtk_widget_show (hbox);
	gtk_widget_show (bf);
	gtk_widget_show (display_config->notebook);
	gtk_widget_show (vbox);
	gtk_widget_show (main_window);
}

int
property_main (int argc, char *argv [])
{
	int init = 0;
	int i;
	char *session_id;
	char *previous_id = NULL;
	char *new_argv[4];

	gtk_init (&argc, &argv);
	gnome_init (&argc, &argv);

	display_config = gnome_property_configurator_new ();
	application_register (display_config);
	
	/* FIXME: look for session management token.  */
	for (i=1; i<argc; i++)
		if (!strcmp (argv [i], "-init"))
			init = 1;

	/* FIXME: actually save state.
	   FIXME: does it make sense to contact the session manager in
	   -init mode?  */
	session_id = gnome_session_init (NULL, NULL,
					 NULL, NULL,
					 previous_id);

	/* Tell session manager to run us in init mode next time the
	   session starts up.  */
	new_argv[0] = argv[0];
	new_argv[1] = "-init";
	gnome_session_set_initialization_command (2, new_argv);

	new_argv[1] = "-previous-id";
	new_argv[2] = session_id;
	gnome_session_set_restart_command (3, new_argv);
	gnome_session_set_clone_command (1, new_argv);
	gnome_session_set_program (argv[0]);

	/* FIXME: set session's notion of pwd.  */

	gnome_property_configurator_request_foreach (display_config,
						     GNOME_PROPERTY_READ);

	if (!init) {
		display_properties_setup ();
		gtk_main ();
	} else {
		gnome_property_configurator_request_foreach (display_config,
							     GNOME_PROPERTY_APPLY);
	}

	gnome_property_configurator_destroy (display_config);
	return 0;
}
