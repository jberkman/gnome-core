#include <gnome.h>
#include "applet-lib.h"
#include "applet-widget.h"

#define IMAGE_FILENAME "test_image"

typedef struct _bussign_properties bussign_properties;

struct _bussign_properties
{
  gchar *url;
};

static const bussign_properties sg_defaults =
{
  "http://www1.netscape.com/fishcam/livefishcamsmall.cgi?livesigncamsmall"
};

static bussign_properties sg_properties = { NULL };

static GtkWidget *
create_bussign_widget(GtkWidget *a_parent);

static int
refresh_imagefile(void);

static void
bussign_refresh_widget_dummy(AppletWidget *a_applet, gpointer a_data);

static int
bussign_refresh(gpointer data);

static gint
destroy_applet(GtkWidget *widget, gpointer data);

static void
about_window(AppletWidget *a_widget, gpointer a_data);

static void
properties_window(AppletWidget *a_widget, gpointer a_data);

static void
load_properties(char *a_cfgpath);

static void
apply_properties(GnomePropertyBox *a_property_box, gint a_page, gpointer a_data);

static gint
session_save(GtkWidget *a_widget,
	     const char *a_cfgpath,
	     const char *a_global_cfgpath);

static void
bussign_refresh_widget_dummy(AppletWidget *a_applet, gpointer a_data)
{
  bussign_refresh(NULL);
}

static gint
session_save(GtkWidget *a_widget,
	     const char *a_cfgpath,
	     const char *a_global_cfgpath)
{
  char     *l_query = NULL;
  
  l_query = g_copy_strings(a_cfgpath, "url", NULL);
  gnome_config_set_string(l_query, sg_properties.url);
  g_free(l_query);
  gnome_config_sync();
  gnome_config_drop_all();
  return FALSE;
}

static void
apply_properties(GnomePropertyBox *a_property_box, gint a_page, gpointer a_data)
{
  gchar *l_new_url = NULL;

  if (a_page != -1)
    return;
  l_new_url = gtk_entry_get_text(GTK_ENTRY(a_data));
  if (l_new_url)
    {
      g_free(sg_properties.url);
      sg_properties.url = g_strdup(l_new_url);
    }
}

static void
load_properties(char *a_cfgpath)
{
  char *l_query = NULL;

  if (sg_properties.url)
    g_free(sg_properties.url);
  l_query = g_copy_strings(a_cfgpath, "bussign", NULL);
  sg_properties.url = gnome_config_get_string(l_query);
  g_free(l_query);
  if (sg_properties.url == NULL)
    sg_properties.url = g_strdup(sg_defaults.url);
  return;
}

static GdkImlibImage *sg_bus = NULL;
static GtkWidget     *sg_pixmap = NULL;

int main(int argc, char **argv)
{
  GtkWidget *l_bussign = NULL;
  GtkWidget *l_applet = NULL;
  
  /* set up the usual stuff */
  panel_corba_register_arguments();
  gnome_init("bussign_applet", NULL, argc, argv, 0, NULL);
  gtk_widget_push_visual(gdk_imlib_get_visual());
  gtk_widget_push_colormap(gdk_imlib_get_colormap());
  l_applet = applet_widget_new(argv[0]);
  if (!l_applet)
    g_error("Can't create applet!\n");
  gtk_widget_realize(l_applet);

  load_properties(APPLET_WIDGET(l_applet)->cfgpath);

  /* set up the bussign widget */
  l_bussign = create_bussign_widget(l_applet);
  gtk_widget_show(l_bussign);

  /* add it */
  applet_widget_add(APPLET_WIDGET(l_applet), l_bussign);
  gtk_widget_show(l_applet);

  /* make sure it will shut down */
  gtk_signal_connect(GTK_OBJECT(l_applet), "destroy",
		     GTK_SIGNAL_FUNC(destroy_applet),
		     NULL);
  /* save the session*/
  gtk_signal_connect(GTK_OBJECT(l_applet), "session_save",
		     GTK_SIGNAL_FUNC(session_save),
		     NULL);
  /* attach the about window */
  applet_widget_register_callback(APPLET_WIDGET(l_applet),
				  "about",
				  _("About..."),
				  about_window,
				  NULL);

  /* attach the properties dialog */
  applet_widget_register_callback(APPLET_WIDGET(l_applet),
				  "properties",
				  _("Properties..."),
				  properties_window,
				  NULL);
  /* attach a refresh button */
  applet_widget_register_callback(APPLET_WIDGET(l_applet),
				  "refresh",
				  _("Refresh Image"),
				  bussign_refresh_widget_dummy,
				  NULL);

  /* do it. */
  applet_widget_gtk_main();
  return 0;
}

static GtkWidget *
create_bussign_widget(GtkWidget *a_parent)
{
  GtkWidget              *l_frame = NULL;
  GtkStyle               *l_style = NULL;

  l_style = gtk_widget_get_style(a_parent);
  
  /* refresh the image */
  if (refresh_imagefile() < 0)
    {
      fprintf(stderr, "Failed to refresh image: %s\n", strerror(errno));
      exit(1);
    }
    
  /* load the file */
  sg_bus = gdk_imlib_load_image(IMAGE_FILENAME);
  /* render it */
  gdk_imlib_render(sg_bus, sg_bus->rgb_width, sg_bus->rgb_height);
  /* get the pixmap */
  sg_pixmap = gtk_pixmap_new(sg_bus->pixmap, sg_bus->shape_mask);
  /* show it */
  gtk_widget_show(sg_pixmap);

  /* set up the timeout to refresh */
  gtk_timeout_add(20000, bussign_refresh, NULL);
  
  /* set the frame up */
  l_frame = gtk_frame_new(NULL);
  gtk_frame_set_shadow_type(GTK_FRAME(l_frame), GTK_SHADOW_IN);
  /* add the pixmap to the frame */
  gtk_container_add(GTK_CONTAINER(l_frame), sg_pixmap);
  return l_frame;
}
  
static gint
destroy_applet(GtkWidget *widget, gpointer data)
{
	gtk_exit(0);
	return FALSE;
}

static int
refresh_imagefile(void)
{
  int   l_return = 0;
  FILE *l_file = NULL;
  
  if ((l_file = fopen(IMAGE_FILENAME, "w+")) == NULL)
    {
      fprintf(stderr, "Failed to open file \"%s\": %s\n", IMAGE_FILENAME, strerror(errno));
      l_return = -1;
      goto ec;
    }
  if (http_get_to_file("www1.netscape.com", 80, "/fishcam/livefishcamsmall.cgi?livesigncamsmall",
		       l_file) < 1)
    {
      l_return = -1;
      goto ec;
    }
  fclose(l_file);
  /*
<<<<<<< bussign.c
    l_image_location = g_malloc(strlen(sg_properties.url) + 5 + 2);
    strcpy(l_image_location, "wget ");
    strcat(l_image_location, sg_properties.url);
=======
    l_return = system("wget -q http://www1.netscape.com/fishcam/livefishcamsmall.cgi?livesigncamsmall");
>>>>>>> 1.5
  */
<<<<<<< bussign.c
  
  unlink (IMAGE_FILENAME);
  l_return = system("wget -q http://www1.netscape.com/fishcam/livefishcamsmall.cgi?livesigncamsmall");
  /* 
     g_free(l_image_location);
  */
=======
 ec:
>>>>>>> 1.5
  return l_return;
}

static int
bussign_refresh(gpointer data)
{
  if (refresh_imagefile() < 0)
    {
      fprintf(stderr, "Failed to refresh image: %s\n", strerror(errno));
      goto ec;
    }
  /* kill the image and flush it */
  gdk_imlib_kill_image(sg_bus);
  gdk_imlib_changed_image(sg_bus);
  /* reload the image */
  sg_bus = gdk_imlib_load_image(IMAGE_FILENAME);
  /* render it */
  gdk_imlib_render(sg_bus, sg_bus->rgb_width, sg_bus->rgb_height);
  /* set the pixmap */
  gtk_pixmap_set(GTK_PIXMAP(sg_pixmap), sg_bus->pixmap, sg_bus->shape_mask);
  /* redraw that sucker. */
  gtk_widget_queue_draw(sg_pixmap);
 ec:
  return TRUE;
}

static void
about_window(AppletWidget *a_widget, gpointer a_data)
{
  GtkWidget *l_about = NULL;
  gchar *l_author[2];
  
  l_author[0] = "Christopher Blizzard";
  l_author[1] = NULL;

  l_about = gnome_about_new ( _("The Bus Sign Applet", "1.0"),
			      _("(c) 1998 the Free Software Foundation"),
			      l_author,
			      _("This applet is a total waste of time. "
				"Get back to work!\n\n"
				"To fill in the sign please see:\n\n"
				"http://people.netscape.com/mtoy/sign/index.html"),
			      NULL);
  gtk_widget_show(l_about);
  return;
}

static void
properties_window(AppletWidget *a_widget, gpointer a_data)
{
  GtkWidget       *l_property_box = NULL;
  GtkWidget       *l_vbox = NULL;
  GtkWidget       *l_entry = NULL;
  GtkWidget       *l_label = NULL;

  /* new box.. */
  l_property_box = gnome_property_box_new();
  /* set the title */
  gtk_window_set_title(GTK_WINDOW(l_property_box), _("GNOME Bus Sign Applet Properties"));
  /* set up a new vbox */
  l_vbox = gtk_vbox_new(GNOME_PAD, FALSE);
  gtk_container_border_width(GTK_CONTAINER(l_vbox), GNOME_PAD);
  /* set up the labels and entries */
  l_label = gtk_label_new("URL: ");
  l_entry = gtk_entry_new();
  /* get the old text */
  gtk_entry_set_text(GTK_ENTRY(l_entry), sg_properties.url);
  /* if it changes */
  gtk_signal_connect_object(GTK_OBJECT(l_entry), "changed",
			    GTK_SIGNAL_FUNC(gnome_property_box_changed),
			    GTK_OBJECT(l_property_box));
  /* pack everything in there */
  gtk_box_pack_start(GTK_BOX(l_vbox), l_label, FALSE, FALSE, GNOME_PAD);
  gtk_box_pack_start(GTK_BOX(l_vbox), l_entry, TRUE, TRUE, GNOME_PAD);
  gnome_property_box_append_page(GNOME_PROPERTY_BOX(l_property_box), l_vbox,
				 gtk_label_new("URL"));
  /* make sure you can apply */
  gtk_signal_connect(GTK_OBJECT(l_property_box), "apply",
		     GTK_SIGNAL_FUNC(apply_properties), l_entry);
  /* show your gams! */
  gtk_widget_show_all(l_property_box);
  return;
}
		     
  
