/* main.c - Main program for desktop properties application.  */

#include <config.h>
#include <stdio.h>

#include <gnome.h>
#include <gdk_imlib.h>
#include "gnome-desktop.h"


GtkWidget *main_window;
GnomePropertyConfigurator *display_config;

/* This is true if we've ever changed the state with this program.  */
static int state_changed = 0;

static GdkImlibImage *monitor_image;


/* True if we are running in initialize-then-exit mode.  */
static int init = 0;

/* Options used by this program.  */
static struct argp_option arguments[] =
{
  { "init", -1, NULL, 0,
    N_("Set parameters from saved state and exit"), 1 },
  { NULL, 0, NULL, 0, NULL, 0 }
};

/* Forward decl of our parsing function.  */
static error_t parse_func (int key, char *arg, struct argp_state *state);

/* The parser used by this program.  */
static struct argp parser =
{
  arguments,
  parse_func,
  NULL,
  NULL,
  NULL,
  NULL
};


GtkWidget *
get_monitor_preview_widget (void)
{
	GtkWidget *pwid;
	char *f;

	f = gnome_pixmap_file ("monitor.xpm");
	pwid = gnome_pixmap_new_from_file (f);
	g_free (f);
	gtk_widget_show (pwid);

	return pwid;
}

/* Enable the Apply button.  */
void
property_changed (void)
{
	gnome_property_box_changed (GNOME_PROPERTY_BOX (display_config->property_box));
}

static gint
deleteFn (GtkWidget *widget, gpointer *data)
{
	if (monitor_image) {
		gdk_imlib_destroy_image (monitor_image);
		monitor_image = NULL;
	}

	gtk_main_quit ();

	return TRUE;
}

/* This is called when the Help button is clicked.  */
static void
help (GtkWidget *w, gint page, gpointer *data)
{
  /* FIXME.  */
}

void
display_properties_setup (void)
{
	gnome_property_configurator_setup (display_config);
	main_window = display_config->property_box;
	gnome_property_configurator_request_foreach (display_config,
						     GNOME_PROPERTY_SETUP);

	gtk_signal_connect (GTK_OBJECT (display_config->property_box),
			    "help", (GtkSignalFunc) help, NULL);
	gtk_signal_connect (GTK_OBJECT (display_config->property_box),
			    "delete_event", (GtkSignalFunc) deleteFn, NULL);
	gtk_signal_connect (GTK_OBJECT (display_config->property_box),
			    "destroy", (GtkSignalFunc) deleteFn, NULL);

	gtk_widget_show (display_config->property_box);
}

static error_t
parse_func (int key, char *arg, struct argp_state *state)
{
  if (key == ARGP_KEY_ARG)
    {
      /* This program has no command-line options.  */
      argp_usage (state);
    }
  else if (key != -1)
    return ARGP_ERR_UNKNOWN;

  init = 1;
  return 0;
}

int
property_main (char *app_id, int argc, char *argv [])
{
        GnomeClient *client = NULL;
	int token = 0;
	char *new_argv[4];

	argp_program_version = VERSION;
	bindtextdomain (PACKAGE, GNOMELOCALEDIR);
	textdomain (PACKAGE);

	client = gnome_client_new_default ();
	gnome_init (app_id, &parser, argc, argv, 0, NULL);

	/* Set this stuff for completeness' sake.  */
	gnome_client_set_restart_command (client, 1, argv);
	gnome_client_set_clone_command (client, 1, argv);

	display_config = gnome_property_configurator_new ();

	application_register (display_config);

	/* If this startup is the result of a previous session, we try
	   to acquire the token that would let us set the current
	   state.  */
	if (gnome_client_get_previous_id (client))
	        token = gnome_startup_acquire_token (application_property (),
						     gnome_client_get_id (client));

	/* If our only purpose is to set the properties and then exit,
	   and we didn't acquire the lock, then we are done.  */
	if (init && ! token)
	        return 0;

	gnome_property_configurator_request_foreach (display_config,
						     GNOME_PROPERTY_READ);

	if (token) {
		/* We have the token, so we set up the state.  */
		gnome_property_configurator_request_foreach (display_config,
							     GNOME_PROPERTY_APPLY);
	}

	if (! init) {
		/* Show the user interface.  */
		display_properties_setup ();
		gtk_main ();
	}

	if (init || state_changed) {
		/* Arrange to be run again next time the session
		   starts.  */
		new_argv[0] = argv[0];
		new_argv[1] = "--init";
		gnome_client_set_restart_command (client, 2, new_argv);
		gnome_client_set_clone_command (client, 2, new_argv);
		gnome_client_set_restart_style (client, GNOME_RESTART_ANYWAY);
	}

	gnome_property_configurator_destroy (display_config);
	return 0;
}
