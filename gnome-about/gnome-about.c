#include <gnome.h>
#include "authors.h"
#include "logo.xpm"

gboolean cb_quit      (GtkWidget *widget, gpointer data);
gboolean cb_exposed   (GtkWidget *widget, GdkEventExpose *event);
gboolean cb_configure (GtkWidget *widget, GdkEventConfigure *event);
gboolean cb_keypress (GtkWidget *widget, GdkEventKey *event);
gboolean cb_clicked (GtkWidget *widget, GdkEvent *event);
gint     scroll       (gpointer data);

GtkWidget *area;
GdkPixmap *pixmap=NULL;
GdkFont *font=NULL;
GdkFont *italicfont=NULL;
gint y, y_to_wrap_at;
gint howmuch=0;
GdkImlibImage *im;
GtkWidget *canvas;
GnomeCanvasItem *image;
	


/* Sparkles */
#define NUM_FRAMES 8

struct _Sparkle {
	GnomeCanvasItem *hline;
	GnomeCanvasItem *vline;
	GnomeCanvasItem *hsmall;
	GnomeCanvasItem *vsmall;
	
	GnomeCanvasPoints *hpoints [NUM_FRAMES];
	GnomeCanvasPoints *vpoints [NUM_FRAMES];
	GnomeCanvasPoints *hspoints [NUM_FRAMES];
	GnomeCanvasPoints *vspoints [NUM_FRAMES];

	gint count;
	gboolean up;
};

typedef struct _Sparkle Sparkle;

static void
sparkle_destroy (Sparkle *sparkle)
{
	int i;
	g_return_if_fail (sparkle != NULL);
	
	gtk_object_destroy (GTK_OBJECT (sparkle->hline));
	gtk_object_destroy (GTK_OBJECT (sparkle->vline));
	gtk_object_destroy (GTK_OBJECT (sparkle->hsmall));
	gtk_object_destroy (GTK_OBJECT (sparkle->vsmall));

	for (i=0; i > NUM_FRAMES ; i++) {
		gnome_canvas_points_free (sparkle->hpoints [i]);
		gnome_canvas_points_free (sparkle->vpoints [i]);
		gnome_canvas_points_free (sparkle->hspoints [i]);
		gnome_canvas_points_free (sparkle->vspoints [i]);
	}
	g_free (sparkle);
}

static gboolean
sparkle_timeout (Sparkle *sparkle)
{
	g_return_val_if_fail (sparkle != 0, FALSE);

	if (sparkle->count == -1) {
		sparkle_destroy (sparkle);
		return FALSE;
	}

	gnome_canvas_item_set (sparkle->hline, "points",
			       sparkle->hpoints [sparkle->count], NULL);
	gnome_canvas_item_set (sparkle->vline, "points",
			       sparkle->vpoints [sparkle->count], NULL);
	gnome_canvas_item_set (sparkle->hsmall, "points",
			       sparkle->hspoints [sparkle->count], NULL);
	gnome_canvas_item_set (sparkle->vsmall, "points",
			       sparkle->vspoints [sparkle->count], NULL);

	if (sparkle->count == NUM_FRAMES - 1)
		sparkle->up = FALSE;

	if (sparkle->up)
		sparkle->count++;
	else
		sparkle->count--;

	return TRUE;
}

static void
fill_points (GnomeCanvasPoints *points, double x, double y, double delta,
	     gboolean horizontal, gboolean square)
{
	if (horizontal) {
		if (square) {
			points->coords[0] = x - delta;
			points->coords[1] = y;
			points->coords[2] = x + delta;
			points->coords[3] = y;
		}
		else {
			points->coords[0] = x - delta;
			points->coords[1] = y - delta;
			points->coords[2] = x + delta;
			points->coords[3] = y + delta;
		}
	}
	else {
		if (square) {
			points->coords[0] = x;
			points->coords[1] = y - delta;
			points->coords[2] = x;
			points->coords[3] = y + delta;
		}
		else {
			points->coords[0] = x + delta;
			points->coords[1] = y - delta;
			points->coords[2] = x - delta;
			points->coords[3] = y + delta;
		}
	}  
}

#define DELTA 0.4

static void
sparkle_new (GnomeCanvas *canvas, double x, double y)
{
	int i;
	double delta;

	Sparkle *sparkle = g_new (Sparkle, 1);
	GnomeCanvasPoints *points = gnome_canvas_points_new (2);

	fill_points (points, x, y, 0.1, TRUE, TRUE);
	sparkle->hsmall = gnome_canvas_item_new(GNOME_CANVAS_GROUP(canvas->root),
						gnome_canvas_line_get_type(),
						"points", points,
						"fill_color", "light gray",
						"width_units", 1.0,
						NULL);
	
	gnome_canvas_item_raise_to_top(sparkle->hsmall);
	
	fill_points(points, x, y, 0.1, FALSE, TRUE);
	sparkle->vsmall = gnome_canvas_item_new(GNOME_CANVAS_GROUP(canvas->root),
						gnome_canvas_line_get_type(),
						"points", points,
						"fill_color", "light gray",
						"width_units", 1.0,
						NULL);
	
	gnome_canvas_item_raise_to_top(sparkle->vsmall);
	
	fill_points(points, x, y, DELTA, TRUE, TRUE);
	sparkle->hline = gnome_canvas_item_new(GNOME_CANVAS_GROUP(canvas->root),
					       gnome_canvas_line_get_type(),
					       "points", points,
					       "fill_color", "white",
					       "width_units", 1.0,
					       NULL);
	
	fill_points(points, x, y, DELTA, FALSE, TRUE);
	sparkle->vline = gnome_canvas_item_new(GNOME_CANVAS_GROUP(canvas->root),
					       gnome_canvas_line_get_type(),
					       "points", points,
					       "fill_color", "white",
					       "width_units", 1.0,
					       NULL);
	
	gnome_canvas_points_free(points);
	
	i = 0;
	delta = 0.0;
	while ( i < NUM_FRAMES ) {
		sparkle->hpoints[i] = gnome_canvas_points_new(2);
		sparkle->vpoints[i] = gnome_canvas_points_new(2);
		sparkle->hspoints[i] = gnome_canvas_points_new(2);
		sparkle->vspoints[i] = gnome_canvas_points_new(2);
		
		
		fill_points(sparkle->hspoints[i], x, y, delta, TRUE, FALSE);    
		fill_points(sparkle->vspoints[i], x, y, delta, FALSE, FALSE);    
		
		delta += DELTA;
		fill_points(sparkle->hpoints[i], x, y, delta, TRUE, TRUE);
		fill_points(sparkle->vpoints[i], x, y, delta + delta*.70, FALSE, TRUE);
		++i;
	}
	
	sparkle->count = 0;
	sparkle->up = TRUE;
	
	gtk_timeout_add(75,(GtkFunction)sparkle_timeout, sparkle);
	
}

static gint 
new_sparkles_timeout(GnomeCanvas* canvas)
{
	static gint which_sparkle = 0;

	if (howmuch >= 5)
	        return TRUE;

	switch (which_sparkle) {
	case 0:
		sparkle_new(canvas,50.0,70.0);
		break;
	case 1: 
		sparkle_new(canvas,70.0,130.0);
		break;
	case 2:
		sparkle_new(canvas,100.0,37.0);
		break;
	case 3: 
		sparkle_new(canvas,120.0,110.0);
		break;
	case 4: 
		sparkle_new(canvas,140.0,120.0);
		break;
	case 5: 
		sparkle_new(canvas,110.0,160.0);
		break;
	default:
		which_sparkle = -1;
		break;
	};
	
	++which_sparkle;
	return TRUE;
}

static void
free_imlib_image (GtkObject *object, gpointer data)
{
	gdk_imlib_destroy_image (data);
}

gboolean
cb_clicked (GtkWidget *widget, GdkEvent *event)
{
	if (event->type == GDK_BUTTON_PRESS) {
		if (howmuch >= 5) {
			gchar *filename = gnome_datadir_file ("gnome-about/authors.dat");
			if (filename)
				gnome_sound_play (filename);

			g_free (filename);
		}

	}

	return FALSE;
}

gboolean
cb_keypress (GtkWidget *widget, GdkEventKey *event)
{
	if (howmuch >= 5)
		return FALSE;

	switch (event->keyval) {
	case GDK_e:
	case GDK_E:
		if (howmuch == 4) {
			howmuch++;
			im = gdk_imlib_create_image_from_xpm_data (magick);
			gnome_canvas_item_set (image,
					       "image", im,
					       NULL);
		}
		else
			howmuch = 0;
		break;
	case GDK_g:
	case GDK_G:
		if (howmuch == 0)
			howmuch++;
		else
			howmuch = 0;
		break;
	case GDK_m:
	case GDK_M:
		if (howmuch == 3)
			howmuch ++;
		else
			howmuch = 0;
		break;
	case GDK_n:
	case GDK_N:
		if (howmuch == 1)
			howmuch++;
		else
			howmuch = 0;
		break;
	case GDK_o:
	case GDK_O:
		if (howmuch == 2)
			howmuch++;
		else
			howmuch = 0;
		break;
	default:
		howmuch = 0;
	}
	
	return FALSE;
}

gboolean
cb_quit (GtkWidget *widget, gpointer data)
{
	gtk_main_quit();
	return FALSE;
}

gint
scroll (gpointer data)
{
	gint cury, i, totalwidth;
	GdkRectangle update_rect;


	i = 0;
	cury = y;
	gdk_draw_rectangle (pixmap, area->style->black_gc,
			    TRUE,
			    0, 0,
			    area->allocation.width,
			    area->allocation.height);

	while (authors[i].name) {
		totalwidth = gdk_string_width (font, authors[i].name);
		if (authors[i].email)
			totalwidth += 4 + gdk_string_width (italicfont, authors[i].email);

		if(cury > -font->descent &&
		   cury < area->allocation.height + font->ascent) {
		        gdk_draw_string (pixmap,
					 font,
					 area->style->white_gc,
					 (area->allocation.width - 
					  totalwidth) / 2,
					 cury,
					 _(authors[i].name));
			gdk_draw_string (pixmap,
					 italicfont,
					 area->style->white_gc,
					 (area->allocation.width -
					  totalwidth) / 2  + 
					 (authors[i].email ? 4 :0) +
					 gdk_string_width (font, authors[i].name),
					 cury,
					 _(authors[i].email));
		}

		i++;
		cury += font->ascent + font->descent;

	}

	y --;
	if(y < y_to_wrap_at)
	        y = area->allocation.height + font->ascent;

	update_rect.x = 0;
	update_rect.y = 0;
	update_rect.width = area->allocation.width;
	update_rect.height = area->allocation.height;
	

	gtk_widget_draw (area, &update_rect);
	return TRUE;
}

gboolean
cb_exposed (GtkWidget *widget, GdkEventExpose *event)
{
	gdk_draw_pixmap (widget->window,
			 widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
			 pixmap,
			 event->area.x, event->area.y,
			 event->area.x, event->area.y,
			 event->area.width, event->area.height);
	return TRUE;
}

gboolean
cb_configure (GtkWidget *widget, GdkEventConfigure *event)
{
	y = widget->allocation.height + font->ascent;

	if (pixmap)
		gdk_pixmap_unref (pixmap);
	pixmap = gdk_pixmap_new (widget->window,
				 widget->allocation.width,
				 widget->allocation.height,
				 -1);

	return TRUE;
}

gint
main (gint argc, gchar *argv[])
{
	GtkWidget *separator;
	GtkWidget *window;
	GtkWidget *button_box;
	GtkWidget *hbox;
	GtkWidget *okbutton;
	GtkWidget *vbox;
	GdkPixmap *logo_pixmap;
	GdkBitmap *logo_mask;
	GtkWidget *frame;
	GtkWidget *gtkpixmap;
	GtkWidget *href;
	
	gnome_init ("gnome-about","1.0", argc, argv);
	
	window = gtk_window_new (GTK_WINDOW_DIALOG);
	gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER);

	gtk_signal_connect (GTK_OBJECT (window), "delete_event",
			    GTK_SIGNAL_FUNC (cb_quit), NULL);
	gtk_signal_connect (GTK_OBJECT (window), "key_press_event",
			    GTK_SIGNAL_FUNC (cb_keypress), NULL);
	gtk_window_set_title (GTK_WINDOW (window), "About GNOME");
	gtk_widget_realize (window);

	/* Load the fonts */
	font = gdk_font_load ("-adobe-helvetica-medium-r-normal--*-120-*-*-*-*-*-*");
	italicfont = gdk_font_load("-adobe-helvetica-medium-o-normal--*-120-*-*-*-*-*-*");
	if (!font)
		font = window->style->font;

	if (!italicfont)
	        italicfont = window->style->font;

	y_to_wrap_at = -(sizeof(authors)/sizeof(authors[0]))*
	                (font->ascent+font->descent);

	logo_pixmap = gdk_pixmap_create_from_xpm_d (window->window, &logo_mask, NULL,
					       logo_xpm);
	gtkpixmap = gtk_pixmap_new (logo_pixmap, logo_mask);

	im = gdk_imlib_create_image_from_xpm_data (logo_xpm);

	canvas = gnome_canvas_new ();
	gtk_widget_set_usize (canvas, im->rgb_width, im->rgb_height);
	gnome_canvas_set_scroll_region (GNOME_CANVAS (canvas), 0, 0,
					im->rgb_width, im->rgb_height);
	image = gnome_canvas_item_new (GNOME_CANVAS_GROUP (GNOME_CANVAS (canvas)->root),
				       gnome_canvas_image_get_type (),
				       "image", im,
				       "x", 0.0,
				       "y", 0.0,
				       "width", (double) im->rgb_width,
				       "height", (double) im->rgb_height,
				       "anchor", GTK_ANCHOR_NW,
				       NULL);
	gtk_signal_connect (GTK_OBJECT (image), "destroy",
			    GTK_SIGNAL_FUNC (free_imlib_image), im);
	gtk_signal_connect (GTK_OBJECT (image), "event",
			    GTK_SIGNAL_FUNC (cb_clicked), NULL);
	gtk_timeout_add (1300, (GtkFunction) new_sparkles_timeout, canvas);

	gtk_container_border_width (GTK_CONTAINER (window), 10);
	hbox = gtk_hbox_new (FALSE, 10);
	vbox = gtk_vbox_new (FALSE, 10);
	frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (frame), canvas);
	gtk_box_pack_start_defaults (GTK_BOX (hbox), frame);
	gtk_box_pack_start_defaults (GTK_BOX (vbox), hbox);
	gtk_container_add (GTK_CONTAINER (window), vbox);
	area = gtk_drawing_area_new ();
	gtk_drawing_area_size (GTK_DRAWING_AREA (area), 320, 160);
	gtk_widget_draw (area, NULL);
	frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (frame), area);
	gtk_box_pack_start_defaults (GTK_BOX (hbox), frame);
	gtk_widget_show (area);
	gtk_signal_connect (GTK_OBJECT (area), "expose_event",
			    GTK_SIGNAL_FUNC (cb_exposed), NULL);
	gtk_signal_connect (GTK_OBJECT (area), "configure_event",
			    GTK_SIGNAL_FUNC (cb_configure), NULL);

	hbox = gtk_hbox_new (TRUE, 10);

	href = gnome_href_new ("http://gnotices.gnome.org/gnome-news/",
			       _("GNOME News Site"));
	gtk_box_pack_start (GTK_BOX (hbox), href,
			    TRUE, FALSE, 0);

	href = gnome_href_new ("http://www.gnome.org/",
			       _("GNOME Main Site"));
	gtk_box_pack_start (GTK_BOX (hbox), href,
			    TRUE, FALSE, 0);

	href = gnome_href_new ("http://developer.gnome.org/",
			       _("GNOME Developers' Site"));
	gtk_box_pack_start (GTK_BOX (hbox), href,
			    TRUE, FALSE, 0);
	
	gtk_box_pack_start (GTK_BOX (vbox), hbox,
			    FALSE, TRUE, 0);

	button_box = gtk_hbutton_box_new ();
	gtk_box_pack_end (GTK_BOX (vbox), button_box,
			  FALSE, TRUE, 0);
	separator = gtk_hseparator_new ();
	gtk_box_pack_end (GTK_BOX (vbox), separator,
			  FALSE, TRUE,
			  GNOME_PAD_SMALL);
	gtk_button_box_set_layout (GTK_BUTTON_BOX (button_box),
				   gnome_preferences_get_button_layout ());
	gtk_button_box_set_spacing (GTK_BUTTON_BOX (button_box),
				    GNOME_PAD);
	okbutton = gnome_stock_or_ordinary_button (GNOME_STOCK_BUTTON_OK);
	gtk_signal_connect (GTK_OBJECT (okbutton), "clicked",
			    GTK_SIGNAL_FUNC (cb_quit), NULL);
	gtk_box_pack_start (GTK_BOX (button_box), okbutton, TRUE, TRUE, 0);
	gtk_timeout_add (50, scroll, NULL);

	gtk_widget_show_all (window);
	gtk_main ();

	return 0;
}


