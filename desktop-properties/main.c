#ifdef HAVE_LIBINTL
#include <libintl.h>
#define _(String) gettext(String)
#else
#define _(String) (String)
#endif
#include <stdio.h>
#include <gtk/gtk.h>

#include "gnome.h"
#include "gnome-desktop.h"

GtkWidget *main_window;
GnomePropertyConfigurator *display_config;

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

static gint
deleteFn (GtkWidget *widget, gpointer *data)
{
	gtk_widget_destroy (main_window);
	gtk_main_quit ();

	return TRUE;
}

static void
display_properties_action (GtkWidget *w, gint close)
{
	if (close) {
		gnome_property_configurator_request_foreach (display_config,
							     GNOME_PROPERTY_APPLY);
		gnome_property_configurator_request_foreach (display_config,
							     GNOME_PROPERTY_WRITE);
		gnome_config_sync ();
	} else
		gnome_property_configurator_request (display_config,
						     GNOME_PROPERTY_APPLY);
	if (close)
		deleteFn (NULL, NULL);
}


void
display_properties_register ()
{
	background_register (display_config);
	screensaver_register (display_config);
	keyboard_register (display_config);
	mouse_register (display_config);
}

void
display_properties_setup (void)
{
	GtkWidget *vbox = gtk_vbox_new (FALSE, 0),
		*hbox = gtk_hbox_new (FALSE, GNOME_PAD),
		*bf = gtk_frame_new (NULL),
		*bok = gtk_button_new_with_label (_("  Ok  ")),
		*bapl = gtk_button_new_with_label (_(" Apply ")),
		*bcl = gtk_button_new_with_label (_(" Cancel "));

	main_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_policy (GTK_WINDOW (main_window), FALSE, FALSE, TRUE);
	gtk_signal_connect (GTK_OBJECT (main_window), "delete_event",
			    GTK_SIGNAL_FUNC (deleteFn), NULL);	

	gtk_signal_connect (GTK_OBJECT (bcl), "clicked",
			    GTK_SIGNAL_FUNC (deleteFn), NULL);	

	gtk_signal_connect (GTK_OBJECT (bok), "clicked",
			    GTK_SIGNAL_FUNC (display_properties_action), (gpointer) 1);

	gtk_signal_connect (GTK_OBJECT (bapl), "clicked",
			    GTK_SIGNAL_FUNC (display_properties_action), (gpointer) 0);

	gtk_container_border_width (GTK_CONTAINER (hbox), GNOME_PAD);
	gtk_frame_set_shadow_type (GTK_FRAME (bf), GTK_SHADOW_OUT);

	// configurators_setup ();
	gnome_property_configurator_setup (display_config);
	gnome_property_configurator_request_foreach (display_config,
						     GNOME_PROPERTY_SETUP);
	
	gtk_container_add (GTK_CONTAINER(main_window), vbox);
	gtk_box_pack_start (GTK_BOX (vbox), display_config->notebook, FALSE, FALSE, 0);
	
	gtk_box_pack_end (GTK_BOX (hbox), bcl, FALSE, FALSE, 0);
	gtk_box_pack_end (GTK_BOX (hbox), bapl, FALSE, FALSE, 0);
	gtk_box_pack_end (GTK_BOX (hbox), bok, FALSE, FALSE, 0);
	gtk_container_add (GTK_CONTAINER(bf), hbox);
	gtk_box_pack_start (GTK_BOX (vbox), bf, FALSE, FALSE, 0);

	gtk_widget_show (bok);
	gtk_widget_show (bapl);
	gtk_widget_show (bcl);
	gtk_widget_show (hbox);
	gtk_widget_show (bf);
	gtk_widget_show (display_config->notebook);
	gtk_widget_show (vbox);
	gtk_widget_show (main_window);
}

int
main (int argc, char *argv [])
{
	int init = 0;
	int i;

	gtk_init (&argc, &argv);
	gnome_init (&argc, &argv);
	
	display_config = gnome_property_configurator_new ();
	display_properties_register ();
	
	for (i=1; i<argc; i++)
		if (!strcmp (argv [i], "-init"))
			init = 1;
	
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
