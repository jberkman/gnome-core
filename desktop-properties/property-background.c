/*
 * Background display property module.
 * (C) 1997 the Free Software Foundation
 *
 * Authors: Miguel de Icaza.
 *          Federico Mena.
 *          Radek Doulik
 */

#include <config.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include <gdk/gdkx.h>

#include <gnome.h>
#include "gnome-desktop.h"


static GnomePropertyConfigurator *config;

static GtkWidget *preview;
static GtkWidget *monitor;
static GdkPixmap *screen;

static GtkWidget *fileSel = NULL;
static GtkWidget *wpMenu;
static GtkWidget *wpOMenu;
static gchar *wpFileSelName = NULL;
static gint wpNum;

static GtkWidget *frame;
static GtkWidget *vbox;

/* If true, the display is a gradient from color1..color2 */
static gint grad;

/* Direction of the gradient */
static gint vertical;

static gint bgType;
static gint wpType;
static gint fillPreview = TRUE;

static gchar *wpFileName;

enum {
	WALLPAPER_TILED,
	WALLPAPER_CENTERED,
};

enum {
	BACKGROUND_SIMPLE,
	BACKGROUND_WALLPAPER,
};

static GdkColor  bgColor1, bgColor2;

/* The pointers to the color selectors */
static GnomeColorSelector *cs1, *cs2;


static void
gtk_widget_show_multi (GtkWidget *first,...)
{
	va_list ap;
	GtkWidget *next;
	
	if (!first)
		return;

	gtk_widget_show (first);
	va_start (ap, first);
	while ((next = va_arg (ap, GtkWidget *)) != 0)
		gtk_widget_show (next);
	va_end (ap);
}

static void
radio_toggle_widget_active(GtkWidget *widget, gpointer data)
{
	GtkWidget *dest;

	dest = GTK_WIDGET(data);

	gtk_widget_set_sensitive(dest, GTK_TOGGLE_BUTTON(widget)->active);
	printf ("%s\n", __FUNCTION__);
	property_changed ();
}

static void
fill_gradient (unsigned char *d, gint w, gint h,
	       GdkColor *c1, GdkColor *c2, int vertical)
{
	gint i, j;
	gint dr, dg, db;
	gint gs1, w3;
	gint vc = (!vertical || (c1 == c2));
	unsigned char *b, *row;

#define R1 c1->red
#define G1 c1->green
#define B1 c1->blue
#define R2 c2->red
#define G2 c2->green
#define B2 c2->blue

	dr = R2 - R1;
	dg = G2 - G1;
	db = B2 - B1;

	gs1 = (vertical) ? h-1 : w-1;
	w3 = w*3;

	row = g_new (unsigned char, w3);

	if (vc) {
		b = row;
		for (j = 0; j < w; j++) {
			*b++ = (R1 + (j * dr) / gs1) >> 8;
			*b++ = (G1 + (j * dg) / gs1) >> 8;
			*b++ = (B1 + (j * db) / gs1) >> 8;
		}
	}

	for (i = 0; i < h; i++) {
		if (!vc) {
			unsigned char cr, cg, cb;
			cr = (R1 + (i * dr) / gs1) >> 8;
			cg = (G1 + (i * dg) / gs1) >> 8;
			cb = (B1 + (i * db) / gs1) >> 8;
			b = row;
			for (j = 0; j < w; j++) {
				*b++ = cr;
				*b++ = cg;
				*b++ = cb;
			}
		}
		memcpy (d, row, w3);
		d += w3;
	}

#undef R1
#undef G1
#undef B1
#undef R2
#undef G2
#undef B2

	g_free (row);
}

static gint
fill_monitor (void)
{
	GdkWindow *rootWindow;
	GdkImlibImage *pi;
	GdkImlibImage *bi;
	GdkPixmap *screen;
	GdkPixmap *pix;
	GdkBitmap *mask;
	GdkGC *GC;
	unsigned char *pdata;
	gint rootWidth, rootHeight;
	gint r, g, b;
	gint cx, cy;
	gint cw, ch;

	rootWindow = gdk_window_foreign_new (GDK_ROOT_WINDOW());
	gdk_window_get_size (rootWindow, &rootWidth, &rootHeight);

	if (bgType == BACKGROUND_WALLPAPER) {
		bi = gdk_imlib_load_image (wpFileName);
		gdk_imlib_render (bi, bi->rgb_width, bi->rgb_height);
	}

	/* are we working on root window ? */
	if (!fillPreview) {		

		GC = gdk_gc_new (rootWindow);

		if ((!grad || !(bi->shape_mask)) && wpType == WALLPAPER_TILED) {

			gdk_color_alloc (gdk_window_get_colormap (rootWindow),
					 &bgColor1);
			 
			if (bgType == BACKGROUND_WALLPAPER) {
				GdkPixmap *bg;

				pix = gdk_imlib_move_image (bi);
				mask = gdk_imlib_move_mask (bi);
				bg = gdk_pixmap_new (rootWindow,
						     bi->rgb_width, bi->rgb_height,
						     -1);
				gdk_gc_set_foreground (GC, &bgColor1);
				gdk_draw_rectangle (bg, GC, TRUE, 0, 0,
						    bi->rgb_width, bi->rgb_height);
				if (mask) {
					gdk_gc_set_clip_origin (GC, 0, 0);
					gdk_gc_set_clip_mask (GC, mask);
				}
				gdk_window_copy_area
					(bg, GC, 0, 0, pix, 0, 0,
					 bi->rgb_width, bi->rgb_height);

				gdk_window_set_back_pixmap (rootWindow, bg, FALSE);

				gdk_imlib_free_pixmap (pix);
				gdk_imlib_free_bitmap (mask);
				gdk_imlib_destroy_image (bi);
				gdk_window_clear (rootWindow);
				gdk_pixmap_unref (bg);
			} else
				gdk_window_set_background (rootWindow, &bgColor1);
			
			gdk_gc_unref (GC);

			return FALSE;
		}

		screen = gdk_pixmap_new (rootWindow, rootWidth, rootHeight, -1);
		cx = cy = 0;
		cw = rootWidth;
		ch = rootHeight;
	} else {
		screen = GNOME_PIXMAP (monitor)->pixmap;
		cw = MONITOR_CONTENTS_WIDTH;
		ch = MONITOR_CONTENTS_HEIGHT;
		cx = MONITOR_CONTENTS_X;
		cy = MONITOR_CONTENTS_Y;

		if (!GTK_WIDGET_REALIZED (monitor))
			gtk_widget_realize (monitor);

		GC = gdk_gc_new (monitor->window);
	}

	pdata = g_new (unsigned char, cw*ch*3);

	gnome_color_selector_get_color_int(cs1, &r, &g, &b, 0xffff);
	bgColor1.red = r;
	bgColor1.green = g;
	bgColor1.blue = b;
	gnome_color_selector_get_color_int(cs2, &r, &g, &b, 0xffff);
	bgColor2.red = r;
	bgColor2.green = g;
	bgColor2.blue = b;
	
	fill_gradient (pdata, cw, ch,
		       &bgColor1, (grad) ? &bgColor2 : &bgColor1, vertical);

	pi = gdk_imlib_create_image_from_data (pdata, NULL, cw, ch);

	g_free (pdata);

	gdk_imlib_render (pi, cw, ch);
	pix = gdk_imlib_move_image (pi);

	gdk_window_copy_area
		(screen,
		 GC,
		 cx, cy,
		 pix,
		 0, 0,
		 cw, ch);

	gdk_imlib_free_pixmap (pix);
	gdk_imlib_destroy_image (pi);

	if (bgType == BACKGROUND_WALLPAPER) {
		gint w, h;
		gint xoff, yoff;

		if (fillPreview) {
			w = (cw*bi->rgb_width) / rootWidth;
			h = (ch*bi->rgb_height) / rootHeight;
			bi = gdk_imlib_clone_scaled_image (bi, w, h);
			gdk_imlib_render (bi, w, h);
		}
		
		w = bi->rgb_width;
		h = bi->rgb_height;
		
		pix = gdk_imlib_move_image (bi);
		mask = gdk_imlib_move_mask (bi);
		
		if (wpType == WALLPAPER_TILED) {

			for (yoff = 0; yoff < ch;
			     yoff += h)
				for (xoff = 0; xoff < cw;
				     xoff += w) {

					if (mask) {

						gdk_gc_set_clip_mask (GC, mask);
						gdk_gc_set_clip_origin
							(GC, cx + xoff, cy + yoff);
					}

					gdk_window_copy_area
						(screen,
						 GC,
						 cx + xoff,
						 cy + yoff,
						 pix,
						 0, 0,
						 (xoff+w > cw) ? cw - xoff : w,
						 (yoff+h > ch) ? ch - yoff : h);
				}
		} else {
			xoff = (cw - w) >> 1;
			yoff = (ch - h) >> 1;

			if (mask) {
				
				gdk_gc_set_clip_mask (GC, mask);
				gdk_gc_set_clip_origin
					(GC, cx + xoff, cy + yoff);
			}

			gdk_window_copy_area
				(screen,
				 GC,
				 cx + xoff,
				 cy + yoff,
				 pix,
				 0, 0,
				 (xoff+w > cw) ? cw - xoff : w,
				 (yoff+h > ch) ? ch - yoff : h);
		}

		gdk_imlib_free_pixmap (pix);
		gdk_imlib_free_bitmap (mask);
		gdk_imlib_destroy_image (bi);
	}

	gdk_gc_unref (GC);

	if (fillPreview) {
		gtk_widget_queue_draw (monitor);
	} else {
		gdk_window_set_back_pixmap (rootWindow,
					    screen, FALSE);
		gdk_pixmap_unref (screen);
		gdk_window_clear (rootWindow);
	}

	return FALSE;
}

static void
set_background_mode (GtkWidget *widget, gpointer data)
{
	grad = GTK_TOGGLE_BUTTON (widget)->active;

	fill_monitor ();
	printf ("%s\n", __FUNCTION__);
	property_changed ();
}

static void
set_orientation (GtkWidget *widget, gpointer data)
{
	vertical = GTK_TOGGLE_BUTTON (widget)->active;

	fill_monitor ();
	/* printf ("%s\n", __FUNCTION__); */
	property_changed ();
}

static void
set_tiled_wallpaper (GtkWidget *widget, gpointer data)
{
	wpType = (GTK_TOGGLE_BUTTON (widget)->active) ?
		WALLPAPER_TILED : WALLPAPER_CENTERED;

	fill_monitor();
	/* printf ("%s\n", __FUNCTION__); */
	property_changed ();
}

static GtkWidget *
color_setup ()
{
	GtkWidget *frame;
	GtkWidget *table;
	GtkWidget *button1;
	GtkWidget *button2;
	GtkWidget *radiof, *radiog;
	GtkWidget *radiov, *radioh;
	GtkWidget *vb1, *vb2;
	
	frame = gtk_frame_new (_("Color"));
	gtk_widget_show (frame);

	vb1= gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vb1);
	vb2= gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vb2);

	table = gtk_table_new (2, 2, FALSE);
	gtk_container_border_width (GTK_CONTAINER(table), GNOME_PAD);
	gtk_table_set_col_spacings (GTK_TABLE(table), GNOME_PAD);
	gtk_container_add (GTK_CONTAINER(frame), table);
	gtk_widget_show (table);

	cs1 = gnome_color_selector_new ((SetColorFunc) fill_monitor, NULL);
	gnome_color_selector_set_color_int (cs1,
					    bgColor1.red,
					    bgColor1.green,
					    bgColor1.blue, 65535);
	
	button1 = gnome_color_selector_get_button (cs1);
	gtk_table_attach (GTK_TABLE(table), button1, 0, 1, 0, 1, 0, 0, 0, 0);
	gtk_widget_show (button1);

	radiof = gtk_radio_button_new_with_label (NULL, _("Flat"));
	gtk_box_pack_start (GTK_BOX (vb1), radiof, FALSE, FALSE, 0);
	gtk_table_attach_defaults (GTK_TABLE(table), vb1, 1, 2, 0, 1);
	gtk_widget_show (radiof);

	radiog = gtk_radio_button_new_with_label (gtk_radio_button_group(GTK_RADIO_BUTTON(radiof)),
						 _("Gradient"));
	gtk_box_pack_start (GTK_BOX (vb1), radiog, FALSE, FALSE, 0);
	gtk_widget_show(radiog);

	cs2 = gnome_color_selector_new ((SetColorFunc) fill_monitor, NULL);
	gnome_color_selector_set_color_int (cs2,
					    bgColor2.red,
					    bgColor2.green,
					    bgColor2.blue, 65535);

	button2 = gnome_color_selector_get_button (cs2);
	gtk_table_attach (GTK_TABLE(table), button2, 0, 1, 1, 2, 0, 0, 0, 0);
	gtk_widget_set_sensitive (button2, FALSE);
	gtk_widget_show (button2);

	radiov = gtk_radio_button_new_with_label (NULL, _("Vertical"));
	gtk_box_pack_start (GTK_BOX (vb2), radiov, FALSE, FALSE, 0);
	gtk_widget_set_sensitive (radiov, FALSE);
	gtk_widget_show (radiov);

	radioh = gtk_radio_button_new_with_label (gtk_radio_button_group(GTK_RADIO_BUTTON(radiov)),
						 _("Horizontal"));
	gtk_box_pack_start (GTK_BOX (vb2), radioh, FALSE, FALSE, 0);
	gtk_table_attach_defaults (GTK_TABLE(table), vb2, 1, 2, 1, 2);
	gtk_widget_set_sensitive (radioh, FALSE);
	gtk_widget_show(radioh);
	
	gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (radiog), grad);
	gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (radioh), !vertical);

	/* We connect to the signals after setting up the initial
	   state; that way the callbacks can always run
	   property_changed() without worrying about the initial
	   setting.  This is moderately gross, but better than keeping
	   a bunch of state variables around.  */
	gtk_signal_connect (GTK_OBJECT(radiog), "toggled",
			    (GtkSignalFunc) set_background_mode,
			    (gpointer) ((long) TRUE));
	gtk_signal_connect (GTK_OBJECT(radiog), "toggled",
			    (GtkSignalFunc) radio_toggle_widget_active,
			    button2);
	gtk_signal_connect (GTK_OBJECT(radiov), "toggled",
			    (GtkSignalFunc) set_orientation,
			    (gpointer) ((long) TRUE));
	gtk_signal_connect (GTK_OBJECT(radiog), "toggled",
			    (GtkSignalFunc) radio_toggle_widget_active,
			    radioh);
	gtk_signal_connect (GTK_OBJECT(radiog), "toggled",
			    (GtkSignalFunc) radio_toggle_widget_active,
			    radiov);

	gtk_idle_add ((GtkFunction) fill_monitor, NULL);

	return frame;
}

static gint
delete_browse (GtkWidget *w, GdkEvent *e, GtkWidget **f)
{	
	if (wpFileSelName)
		g_free (wpFileSelName);
	wpFileSelName = g_strdup (gtk_file_selection_get_filename
				  (GTK_FILE_SELECTION (*f)));
	*f = NULL;

	return TRUE;
}

static void
browse_activated (GtkWidget *w, gchar *s)
{
	wpFileName = s;
	bgType = (s) ? BACKGROUND_WALLPAPER : BACKGROUND_SIMPLE;
	/* printf ("%s\n", s); */

	fill_monitor ();
	printf ("%s\n", __FUNCTION__);
	property_changed ();
}

static void
wp_selection_cancel (GtkWidget *w, GtkWidget **f)
{
	GtkWidget *cf = *f;
	delete_browse (w, NULL, f);
	gtk_widget_destroy (cf);
}

static void
wp_selection_ok (GtkWidget *w, GtkWidget **f)
{
	GtkWidget *cf = *f;
	GString *gs;
	gchar num[32];
	gint found = -1, i=1;
	GList *child = GTK_MENU_SHELL (wpMenu)->children;
	
	if (w)
		delete_browse (w, NULL, f);
	/* printf ("wp ok\n"); */

	while (child) {
		if (child->data)
			if (GTK_BIN (child->data)->child) {
				/* printf ("%s\n", GTK_LABEL (GTK_BIN (child->data)->child)->label); */
				if (!strcmp (GTK_LABEL (GTK_BIN (child->data)->child)->label, wpFileSelName)) {
					found = i;
					/* printf ("found: %d\n", i); */
				}
			}
		i++;
		child = child->next;
	}

	gtk_widget_destroy (cf);

	if (found < 0) {
		/* printf ("selected %s\n", wpFileSelName); */

		cf = gtk_menu_item_new_with_label (wpFileSelName);
		gtk_signal_connect (GTK_OBJECT (cf),
				    "activate",
				    (GtkSignalFunc) browse_activated, wpFileSelName);
		gtk_menu_append (GTK_MENU (wpMenu), cf);
		gtk_widget_show (cf);
		wpNum++;

		gs = g_string_new ("/Desktop/Background/wallpaper");
		sprintf (num, "%d", wpNum);
		g_string_append (gs, num);
		gnome_config_set_string (gs->str, wpFileSelName);
		g_string_free (gs, TRUE);

		gnome_config_set_int ("/Desktop/Background/wallpapers", wpNum);
		gnome_config_set_string ("/Desktop/Background/wallpapers_dir",
					 wpFileSelName);


		found = wpNum;
		gnome_config_sync ();
	}

	wpFileName = wpFileSelName;
	bgType = BACKGROUND_WALLPAPER;

	gtk_option_menu_set_history (GTK_OPTION_MENU (wpOMenu), found);
	property_changed ();
	fill_monitor ();
}

static void
browse_wallpapers (GtkWidget *w, gpointer p)
{
	if (!fileSel) {
		
		fileSel = gtk_file_selection_new (_("Wallpaper Selection"));
		if (wpFileSelName)
			gtk_file_selection_set_filename (GTK_FILE_SELECTION (fileSel),
							 wpFileSelName);

		gtk_signal_connect (GTK_OBJECT (fileSel), "delete_event",
				    (GtkSignalFunc) delete_browse,
				    &fileSel);

		gtk_signal_connect (GTK_OBJECT
				    (GTK_FILE_SELECTION (fileSel)->ok_button),
				    "clicked", (GtkSignalFunc) wp_selection_ok,
				    &fileSel);

		gtk_signal_connect (GTK_OBJECT
				    (GTK_FILE_SELECTION (fileSel)->cancel_button),
				    "clicked",
				    (GtkSignalFunc) wp_selection_cancel,
				    &fileSel);

		gtk_widget_show (fileSel);
	}
}

static GtkWidget *
wallpaper_setup ()
{
	GtkWidget *wallp;
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *none;
	GtkWidget *but;
	GtkWidget *rbut;
	gint i;
	gint selectedWp = 0;
	GString *wpName;
	gchar num[32];
	gchar *wpName1;

	wallp = gtk_frame_new (_("Wallpaper"));
	vbox = gtk_vbox_new (FALSE, 0);
	hbox = gtk_hbox_new (FALSE, GNOME_PAD);
	but = gtk_button_new_with_label (_(" Browse... "));
	gtk_signal_connect (GTK_OBJECT (but), "clicked",
			    (GtkSignalFunc) browse_wallpapers, NULL);

	wpMenu = gtk_menu_new ();
	none = gtk_menu_item_new_with_label (_("none"));
	gtk_menu_append (GTK_MENU (wpMenu), none);
	gtk_widget_show (none);
	gtk_signal_connect (GTK_OBJECT (none),
			    "activate",
			    (GtkSignalFunc) browse_activated, NULL);

	wpNum = gnome_config_get_int ("/Desktop/Background/wallpapers=0");

	for (i = 0; i<wpNum; i++) {

		/* printf ("wallpaper%d", i); */
		wpName = g_string_new ("/Desktop/Background/wallpaper");
		sprintf (num, "%d", i+1);
		g_string_append (wpName, num);
		g_string_append (wpName, "=???");

		wpName1 = gnome_config_get_string (wpName->str);
		/* printf (": %s\n", wpName1); */
		if (wpName1) {
			if (wpFileName)
				if (!strcmp (wpName1, wpFileName))
					selectedWp = i + 1;

			none = gtk_menu_item_new_with_label (wpName1);
			gtk_menu_append (GTK_MENU (wpMenu), none);
			gtk_signal_connect (GTK_OBJECT (none),
					    "activate",
					    (GtkSignalFunc) browse_activated, wpName1);
			gtk_widget_show (none);
		}

		g_string_free (wpName, TRUE);
	}

	wpOMenu = gtk_option_menu_new ();
	gtk_option_menu_set_menu (GTK_OPTION_MENU (wpOMenu), wpMenu);	
	gtk_option_menu_set_history (GTK_OPTION_MENU (wpOMenu), selectedWp);
	gtk_widget_set_usize (wpOMenu, 120, -1);

	gtk_box_pack_start (GTK_BOX (hbox), wpOMenu, TRUE, TRUE, 0);
	gtk_box_pack_end (GTK_BOX (hbox), but, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

	rbut = gtk_radio_button_new_with_label (NULL, _("Centered"));
	gtk_box_pack_end (GTK_BOX (vbox), rbut, FALSE, FALSE, 0);
	gtk_widget_show (rbut);

	rbut = gtk_radio_button_new_with_label
		(gtk_radio_button_group (GTK_RADIO_BUTTON (rbut)),
		 _("Tiled"));

	gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (rbut),
				     !(wpType == WALLPAPER_CENTERED));

	gtk_signal_connect (GTK_OBJECT(rbut), "toggled",
			    (GtkSignalFunc) set_tiled_wallpaper,
			    NULL);

	gtk_box_pack_end (GTK_BOX (vbox), rbut, FALSE, FALSE, 0);

	gtk_container_border_width (GTK_CONTAINER (vbox), GNOME_PAD);
	gtk_container_add (GTK_CONTAINER (wallp), vbox);


	gtk_widget_show (wpOMenu);
	gtk_widget_show (but);
	gtk_widget_show (hbox);
	gtk_widget_show (rbut);
	gtk_widget_show (vbox);
	gtk_widget_show (wallp);

	return wallp;
}

static void
background_apply ()
{
	fillPreview = FALSE;
	fill_monitor ();
	fillPreview = TRUE;

	/*
	GdkWindow *rootWindow;
	GdkGC *rootGC;
	GdkPixmap *rootBack = NULL;
	GtkWidget *rootPreview;
	gint rootWidth, rootHeight;

	rootWindow = gdk_window_foreign_new (GDK_ROOT_WINDOW());
	rootGC = gdk_gc_new (rootWindow);
	gdk_window_get_size (rootWindow, &rootWidth, &rootHeight);

	if (bgType == BACKGROUND_WALLPAPER && wpType == WALLPAPER_TILED) {
		if (wpFileName) {
			rootBack = gdk_pixmap_create_from_xpm (rootWindow, NULL,
							       &bgColor1, wpFileName);
			if (rootBack) {
				gdk_window_set_back_pixmap (rootWindow,
							    rootBack, FALSE);
				gdk_pixmap_unref (rootBack);
			}
		}
	} else if (grad ||
		   (bgType == BACKGROUND_WALLPAPER && wpType == WALLPAPER_CENTERED)) {
		GdkPixmap *pix, *mask;
		gint xoff, yoff;
		gint w, h;

		rootPreview = gtk_preview_new (GTK_PREVIEW_COLOR);

		gtk_preview_size (GTK_PREVIEW (rootPreview), rootWidth, rootHeight);

		gnome_preview_fill_gradient (GTK_PREVIEW (rootPreview),
					     &bgColor1, (grad) ? &bgColor2 : &bgColor1,
					     vertical);

		rootBack = gdk_pixmap_new (rootWindow, rootWidth, rootHeight, -1);
		gtk_preview_put (GTK_PREVIEW (rootPreview),
				 rootBack,
				 rootGC,
				 0, 0,
				 0, 0,
				 rootWidth, rootHeight);


		if (bgType == BACKGROUND_WALLPAPER) {
			pix = gdk_pixmap_create_from_xpm (rootWindow,
							  &mask,
							  &bgColor1,
							  wpFileName);

			gdk_window_get_size (pix, &w, &h);
			xoff = (rootWidth - w) >> 1;
			yoff = (rootHeight - h) >> 1;
			if (xoff < 0) xoff = 0;
			if (yoff < 0) yoff = 0;
		
			if (mask) {
				gdk_gc_set_clip_mask
					(rootGC,
					 mask);
				gdk_gc_set_clip_origin
					(rootGC,
					 xoff, yoff);
			}

			gdk_window_copy_area (rootBack,
					      rootGC,
					      xoff, yoff,
					      pix,
					      0, 0,
					      (xoff+w > rootWidth) ? rootWidth - xoff : w,
					      (yoff+h > rootHeight) ? rootHeight - yoff : h);
				
			if (mask) {
				gdk_gc_set_clip_mask
					(rootGC,
					 NULL);
				gdk_gc_set_clip_origin
					(rootGC,
					 0, 0);
			}
			gdk_pixmap_unref (pix);
		}

		gdk_window_set_back_pixmap (rootWindow, rootBack, FALSE);
	
		gdk_pixmap_unref (rootBack);
		gtk_widget_destroy (rootPreview);
	} else {
		gdk_color_alloc (gdk_window_get_colormap (rootWindow),
				 &bgColor1);
			 
		gdk_window_set_background (rootWindow, &bgColor1);
	}
	gdk_window_clear (rootWindow);
	gdk_gc_destroy (rootGC);
	*/
}

static void
background_write ()
{
	char buffer [60];
	
	sprintf (buffer, "#%02x%02x%02x",
		 bgColor1.red >> 8,
		 bgColor1.green >> 8,
		 bgColor1.blue >> 8);
	gnome_config_set_string ("/Desktop/Background/color1", buffer);
	sprintf (buffer, "#%02x%02x%02x",
		 bgColor2.red >> 8,
		 bgColor2.green >> 8,
		 bgColor2.blue >> 8);
	gnome_config_set_string ("/Desktop/Background/color2", buffer);

	gnome_config_set_string ("/Desktop/Background/simple",
				 (grad) ? "gradient" : "solid");
	gnome_config_set_string ("/Desktop/Background/gradient",
				 (vertical) ? "vertical" : "horizontal");

	gnome_config_set_string ("/Desktop/Background/wallpaper",
				 (bgType == BACKGROUND_SIMPLE) ? "none" : wpFileName);
	gnome_config_set_string ("/Desktop/Background/wallpaperAlign",
				 (wpType == WALLPAPER_TILED) ? "tiled" : "centered");
}

static void
background_read ()
{
	gdk_color_parse
		(gnome_config_get_string ("/Desktop/Background/color1=#808080"),
		 &bgColor1);
	gdk_color_parse
		(gnome_config_get_string ("/Desktop/Background/color2=#0000ff"),
		 &bgColor2);

	bgType = (strcasecmp
		  (gnome_config_get_string
		   ("/Desktop/Background/type=simple"),
		   "simple"));
	grad = (strcasecmp
		(gnome_config_get_string
		 ("/Desktop/Background/simple=solid"),
		 "solid"));
	vertical = !(strcasecmp
		     (gnome_config_get_string
		      ("/Desktop/Background/gradient=vertical"),
		      "vertical"));
	if ((strcasecmp
	      (gnome_config_get_string
	       ("/Desktop/Background/wallpaperAlign=tiled"),
	       "tiled")))
		wpType = WALLPAPER_CENTERED;
	else
		wpType = WALLPAPER_TILED;

	wpFileName = gnome_config_get_string ("/Desktop/Background/wallpaper=none");
	wpFileSelName = gnome_config_get_string
		("/Desktop/Background/wallpapers_dir=./");

	if (!strcasecmp (wpFileName, "none")) {
		wpFileName = NULL;
		bgType = BACKGROUND_SIMPLE;
	} else
		bgType = BACKGROUND_WALLPAPER;
	/* printf ("%s\n", wpFileName); */
}

/*
 * background_setup: creates the dialog box for configuring the
 * display background and registers it with the display property
 * configurator
 */
static void
background_setup ()
{
	GtkWidget *settings;
	GtkWidget *hbox;
	GtkWidget *fill, *wallp;
	
	vbox = gtk_vbox_new (TRUE, 0);

	hbox = gtk_hbox_new (TRUE, 0);
	gtk_container_border_width (GTK_CONTAINER(hbox), GNOME_PAD);

	monitor = get_monitor_preview_widget ();

	preview = gtk_preview_new(GTK_PREVIEW_COLOR);
	gtk_preview_size(GTK_PREVIEW(preview),
			 MONITOR_CONTENTS_WIDTH,
			 MONITOR_CONTENTS_HEIGHT);
	
	gtk_box_pack_start (GTK_BOX(hbox), monitor, FALSE, FALSE, 0);

	gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
	
	settings = gtk_hbox_new (FALSE, GNOME_PAD);
	gtk_container_border_width (GTK_CONTAINER(settings), GNOME_PAD);
	gtk_box_pack_end (GTK_BOX (vbox), settings, TRUE, TRUE, 0);

	fill = color_setup ();
	gtk_box_pack_start (GTK_BOX (settings), fill, FALSE, FALSE, 0);
	
	wallp  = wallpaper_setup ();
	gtk_box_pack_end (GTK_BOX (settings), wallp, TRUE, TRUE, 0);
	
	gtk_widget_show (monitor);
	gtk_widget_show (settings);
	gtk_widget_show (hbox);
	gtk_widget_show (vbox);

        gtk_notebook_append_page (GTK_NOTEBOOK (config->notebook),
				  vbox,
				  gtk_label_new (_(" Background ")));
}

static gint
background_action (GnomePropertyRequest req)
{
	switch (req) {
	case GNOME_PROPERTY_READ:
		background_read ();
		break;
	case GNOME_PROPERTY_WRITE:
		background_write ();
		break;
	case GNOME_PROPERTY_APPLY:
		background_apply ();
		break;
	case GNOME_PROPERTY_SETUP:
		background_setup ();
		break;
	default:
		return 0;
	}

	return 1;
}

void
background_register (GnomePropertyConfigurator *c)
{
	config = c;
	gnome_property_configurator_register (config, background_action);
}
