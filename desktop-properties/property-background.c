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
#include "imlib-misc.h"


static GnomePropertyConfigurator *config;

static GtkWidget *preview;
static GtkWidget *monitor;

static GtkWidget *fileSel = NULL;
static GtkWidget *wpMenu;
static GtkWidget *wpOMenu;
static gchar *wpFileSelName = NULL;
static gint wpNum;

/* If true, the display is a gradient from color1..color2 */
static gint grad;

/* Direction of the gradient */
static gint vertical;

static gint bgType;
static gint wpType;
static gint fillPreview = TRUE;

static gchar *wpFileName=NULL;

enum {
	WALLPAPER_TILED,
	WALLPAPER_CENTERED,
	WALLPAPER_SCALED,
	WALLPAPER_SCALED_KEEP,
};

enum {
	BACKGROUND_SIMPLE,
	BACKGROUND_WALLPAPER,
};

static GdkColor  bgColor1, bgColor2;

/* The pointers to the color selectors */
static GnomeColorSelector *cs1, *cs2;


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

static void
fill_monitor (int prop_changed)
{
	GdkWindow *rootWindow;
	ImlibImage *pi = NULL;
	ImlibImage *bi = NULL;
	GdkPixmap *screen;
	Pixmap xscreen;
	Pixmap pix;
	Pixmap mask;
	GdkGC *gc;
	GC xgc;
	unsigned char *pdata;
	gint rootWidth, rootHeight;
	gint r, g, b;
	gint cx, cy;
	gint cw, ch;
	GdkCursor *cursor;

	if (prop_changed)
		property_changed ();

	/* Set the cursor to a nice watch.  The main_window may not exist if we are called from the
	 * session manager, though
	 */

	if (main_window) {
		cursor = gdk_cursor_new (GDK_WATCH);
		gdk_window_set_cursor (main_window->window, cursor);
		gdk_cursor_destroy (cursor);
	}

	rootWindow = gdk_window_foreign_new (GDK_ROOT_WINDOW());
	gdk_window_get_size (rootWindow, &rootWidth, &rootHeight);

	if (bgType == BACKGROUND_WALLPAPER) {
		bi = Imlib_load_image (imlib_data, wpFileName);
		if (bi)
			Imlib_render (imlib_data, bi, bi->rgb_width, bi->rgb_height);
		else {
			bgType = BACKGROUND_SIMPLE;
			gtk_option_menu_set_history (GTK_OPTION_MENU (wpOMenu), 0);
		}
	}

	if (main_window) {
		gnome_color_selector_get_color_int(cs1, &r, &g, &b, 0xffff);
		bgColor1.red = r;
		bgColor1.green = g;
		bgColor1.blue = b;
		r >>= 8;
		g >>= 8;
		b >>= 8;
		bgColor1.pixel = gdk_imlib_best_color_match(&r, &g, &b);

		gnome_color_selector_get_color_int(cs2, &r, &g, &b, 0xffff);
		bgColor2.red = r;
		bgColor2.green = g;
		bgColor2.blue = b;
		r >>= 8;
		g >>= 8;
		b >>= 8;
		bgColor2.pixel = gdk_imlib_best_color_match(&r, &g, &b);
	}
	
	/* are we working on root window ? */
	if (!fillPreview) {
		gc = gdk_gc_new (rootWindow);
		xgc = ((GdkGCPrivate *) gc)->xgc;

		if ((!grad || (bi && !bi->shape_mask)) && wpType == WALLPAPER_TILED) {
			if (bgType == BACKGROUND_WALLPAPER) {
				Pixmap bg;

				pix = Imlib_move_image (imlib_data, bi);
				mask = Imlib_move_mask (imlib_data, bi);
				bg = XCreatePixmap (GDK_DISPLAY (),
						    GDK_ROOT_WINDOW (),
						    bi->rgb_width, bi->rgb_height,
						    gdkx_visual_get (Imlib_get_visual (imlib_data)->visualid)->depth);

				gdk_color_alloc (gdk_window_get_colormap (rootWindow), &bgColor1);
				gdk_gc_set_foreground (gc, &bgColor1);

				XFillRectangle (GDK_DISPLAY (), bg, xgc,
						0, 0,
						bi->rgb_width, bi->rgb_height);

				if (mask) {
					XSetClipMask (GDK_DISPLAY (), xgc, mask);
					XSetClipOrigin (GDK_DISPLAY (), xgc, 0, 0);
				}

				XCopyArea (GDK_DISPLAY (),
					   pix, bg,
					   xgc,
					   0, 0,
					   bi->rgb_width, bi->rgb_height,
					   0, 0);

				XSetWindowBackgroundPixmap (GDK_DISPLAY (), GDK_ROOT_WINDOW (), bg);

				Imlib_free_pixmap (imlib_data, pix);
				Imlib_free_pixmap (imlib_data, mask);
				Imlib_destroy_image (imlib_data, bi);
				XClearWindow (GDK_DISPLAY (), GDK_ROOT_WINDOW ());
				XFreePixmap (GDK_DISPLAY (), bg);
			} else {
#ifdef DEBUG
				g_print("Setting background in fill_monitor\n");
#endif
				gdk_window_set_background (rootWindow, &bgColor1);
				gdk_window_clear (rootWindow);
			}
			
			gdk_gc_unref (gc);

			/* Reset the cursor to normal */

			if (main_window)
				gdk_window_set_cursor (main_window->window, NULL);

			return;
		}

		screen = gdk_pixmap_new (rootWindow, rootWidth, rootHeight, -1);
		cx = cy = 0;
		cw = rootWidth;
		ch = rootHeight;
	} else {
		screen = GTK_PIXMAP (GTK_BIN (monitor)->child)->pixmap;
		cw = MONITOR_CONTENTS_WIDTH;
		ch = MONITOR_CONTENTS_HEIGHT;
		cx = MONITOR_CONTENTS_X;
		cy = MONITOR_CONTENTS_Y;

		if (!GTK_WIDGET_REALIZED (monitor))
			gtk_widget_realize (monitor);

		gc = gdk_gc_new (monitor->window);
	}

	xscreen = GDK_WINDOW_XWINDOW (screen);
	xgc = GDK_GC_XGC (gc);

	if (grad) {
	   
	   int render_type;
	
	   pdata = g_new (unsigned char, cw*ch*3);

	   fill_gradient (pdata, cw, ch, &bgColor1, &bgColor2, vertical);
	   
	   pi = Imlib_create_image_from_data (imlib_data, pdata, NULL, cw, ch);
	   
	   g_free (pdata);
	   
	   render_type=Imlib_get_render_type(imlib_data);
	   Imlib_set_render_type(imlib_data,RT_DITHER_TRUECOL);
	   Imlib_render (imlib_data, pi, cw, ch);
	   Imlib_set_render_type(imlib_data,render_type);
	   pix = Imlib_move_image (imlib_data, pi);
	   
	   XCopyArea (GDK_DISPLAY (),
		      pix, xscreen,
		      xgc,
		      0, 0,
		      cw, ch,
		      cx, cy);
	   
	   Imlib_free_pixmap (imlib_data, pix);
	   Imlib_destroy_image (imlib_data, pi);
	} else {
	   gdk_color_alloc (gdk_window_get_colormap (rootWindow), &bgColor1);
	   gdk_gc_set_foreground (gc, &bgColor1);
	   XFillRectangle (GDK_DISPLAY (), xscreen, xgc, cx, cy, cw, ch);
	}

	if (bgType == BACKGROUND_WALLPAPER) {
		gint w, h;
		gint xoff, yoff;

		if (fillPreview &&
		    wpType != WALLPAPER_SCALED && wpType != WALLPAPER_SCALED_KEEP) {
			w = (cw*bi->rgb_width) / rootWidth;
			h = (ch*bi->rgb_height) / rootHeight;
			bi = Imlib_clone_scaled_image (imlib_data, bi, w, h);
			Imlib_render (imlib_data, bi, w, h);
		}
		
		w = bi->rgb_width;
		h = bi->rgb_height;
		
		pix = Imlib_move_image (imlib_data, bi);
		mask = Imlib_move_mask (imlib_data, bi);
		
		if (wpType == WALLPAPER_TILED) {
			for (yoff = 0; yoff < ch;
			     yoff += h)
				for (xoff = 0; xoff < cw;
				     xoff += w) {

					if (mask) {
						XSetClipMask (GDK_DISPLAY (), xgc, mask);
						gdk_gc_set_clip_origin (gc, cx + xoff, cy + yoff);
					}

					XCopyArea (GDK_DISPLAY (),
						   pix, xscreen,
						   xgc,
						   0, 0,
						   (xoff+w > cw) ? cw - xoff : w,
						   (yoff+h > ch) ? ch - yoff : h,
						   cx + xoff,
						   cy + yoff);
				}
		} else {

			if (wpType == WALLPAPER_SCALED || wpType == WALLPAPER_SCALED_KEEP) {

				if (wpType == WALLPAPER_SCALED_KEEP) {

					gdouble asp;
					gint st = 0;

					asp = (gdouble) bi->rgb_width / cw;

					if (asp < (gdouble) bi->rgb_height / ch) {
						asp = (gdouble) bi->rgb_height / ch;
						st = 1;
					}

					if (st) {
						w = bi->rgb_width / asp;
						h = ch;
						xoff = (cw - w) >> 1;
						yoff = 0;
					} else {
						h = bi->rgb_height / asp;
						w = cw;
						xoff = 0;
						yoff = (ch - h) >> 1;
					}
				} else {
					w = cw;
					h = ch;

					xoff = yoff = 0;
				}

				bi = Imlib_clone_scaled_image (imlib_data, bi, w, h);
				Imlib_render (imlib_data, bi, w, h);
		
				w = bi->rgb_width;
				h = bi->rgb_height;
		
				pix = Imlib_move_image (imlib_data, bi);
				mask = Imlib_move_mask (imlib_data, bi);

			} else {
				xoff = (cw - MIN (w, cw)) >> 1;
				yoff = (ch - MIN (h, ch)) >> 1;
			}

			if (mask) {
				XSetClipMask (GDK_DISPLAY (), xgc, mask);
				gdk_gc_set_clip_origin (gc, cx + xoff, cy + yoff);
			}

			XCopyArea (GDK_DISPLAY (),
				   pix, xscreen,
				   xgc,
				   0, 0,
				   (xoff+w > cw) ? cw - xoff : w,
				   (yoff+h > ch) ? ch - yoff : h,
				   cx + xoff,
				   cy + yoff);
		}

		Imlib_free_pixmap (imlib_data, pix);
		Imlib_free_pixmap (imlib_data, mask);
		Imlib_destroy_image (imlib_data, bi);
	}

	gdk_gc_unref (gc);

	if (fillPreview) {
		gtk_widget_queue_draw (monitor);
	} else {
		gdk_window_set_back_pixmap (rootWindow,
					    screen, FALSE);
		gdk_pixmap_unref (screen);
		gdk_window_clear (rootWindow);
	}

	/* Reset the cursor to normal */

	if (main_window)
		gdk_window_set_cursor (main_window->window, NULL);
}

static void
color_sel_set (GnomeColorSelector *gcs, gpointer data)
{
	fill_monitor (TRUE);
}

static gint
idle_fill_monitor (gpointer data)
{
	fill_monitor (FALSE);
	return FALSE;
}

static GtkWidget *radioh, *radiov, *radiof, *radiog, *button2;

static void
set_background_mode (GtkWidget *widget)
{
	if (GTK_TOGGLE_BUTTON (widget)->active) {
		if (widget == radiof) {
			/* Flat color */
			gtk_widget_set_sensitive(button2, FALSE);
			gtk_widget_set_sensitive(radioh, FALSE);
			gtk_widget_set_sensitive(radiov, FALSE);
			grad = FALSE;
		} else if (widget == radiog) {
			/* Gradient fill */
			gtk_widget_set_sensitive(button2, TRUE);
			gtk_widget_set_sensitive(radioh, TRUE);
			gtk_widget_set_sensitive(radiov, TRUE);
			grad = TRUE;
		}

		fill_monitor (TRUE);
	}
}

static void
set_orientation (GtkWidget *widget)
{
	if (GTK_TOGGLE_BUTTON (widget)->active) {
		if (widget == radioh)
			vertical = FALSE;  /* Horizontal gradient */
		else if (widget == radiov)
			vertical = TRUE;   /* Vertical gradient */

		fill_monitor (TRUE);
	}
}

static void
set_wallpaper_type (GtkWidget *widget, gpointer data)
{
	if (GTK_TOGGLE_BUTTON (widget)->active) {
		wpType = (gint) data;

		fill_monitor (TRUE);
	}
}

static GtkWidget *
color_setup ()
{
	GtkWidget *frame;
	GtkWidget *table;
	GtkWidget *button1;
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

	cs1 = gnome_color_selector_new ((SetColorFunc) color_sel_set, NULL);
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

	cs2 = gnome_color_selector_new ((SetColorFunc) color_sel_set, NULL);
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

	/* BEFORE our signals are connected, set initial states */
	gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (radiog), grad);
	gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (radioh), !vertical);

	gtk_widget_set_sensitive(button2, grad);
	gtk_widget_set_sensitive(radioh, grad);
	gtk_widget_set_sensitive(radiov, grad);
	
	/* When people change the background mode controls, update */
	gtk_signal_connect (GTK_OBJECT(radiog), "toggled",
			    GTK_SIGNAL_FUNC(set_background_mode),
			    NULL);
	gtk_signal_connect (GTK_OBJECT(radiof), "toggled",
			    (GtkSignalFunc) set_background_mode,
			    NULL);

	gtk_signal_connect (GTK_OBJECT(radiov), "toggled",
			    (GtkSignalFunc) set_orientation,
			    (gpointer) ((long) TRUE));
	gtk_signal_connect (GTK_OBJECT(radioh), "toggled",
			    (GtkSignalFunc) set_orientation,
			    (gpointer) ((long) FALSE));

	gtk_idle_add ((GtkFunction) idle_fill_monitor, NULL);

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
#ifdef DEBUG
	g_print("delete_browse\n");
#endif
	return TRUE;
}

static void
browse_activated (GtkWidget *w, gchar *s)
{
	if (wpFileName)
		g_free(wpFileName);
	wpFileName = g_strdup(s);
	bgType = (s) ? BACKGROUND_WALLPAPER : BACKGROUND_SIMPLE;
	/* printf ("%s\n", s); */

	fill_monitor (TRUE);
	/* printf ("%s\n", __FUNCTION__); */
}

static void
wp_selection_cancel (GtkWidget *w, GtkWidget **f)
{
	GtkWidget *cf = *f;
	delete_browse (w, NULL, f);
	gtk_widget_destroy (cf);
}

static void
set_monitor_filename (char *str)
{
	GString *gs;
	gchar num[32];
	gint found = -1, i=1;
	GList *child = GTK_MENU_SHELL (wpMenu)->children;
	GtkWidget *cf;
	
	while (child) {
		if (child->data)
			if (GTK_BIN (child->data)->child) {
				/* printf ("%s\n", GTK_LABEL (GTK_BIN (child->data)->child)->label); */
				if (!strcmp (GTK_LABEL (GTK_BIN (child->data)->child)->label, str)) {
					found = i;
					/* printf ("found: %d\n", i); */
				}
			}
		i++;
		child = child->next;
	}


	if (found < 0) {

		cf = gtk_menu_item_new_with_label (str);
		gtk_signal_connect (GTK_OBJECT (cf),
				    "activate",
				    (GtkSignalFunc) browse_activated, str);
		gtk_menu_append (GTK_MENU (wpMenu), cf);
		gtk_widget_show (cf);
		wpNum++;

		gs = g_string_new ("/Desktop/Background/wallpaper");
		sprintf (num, "%d", wpNum);
		g_string_append (gs, num);
		gnome_config_set_string (gs->str, str);
		g_string_free (gs, TRUE);

		gnome_config_set_int ("/Desktop/Background/wallpapers", wpNum);
		gnome_config_set_string ("/Desktop/Background/wallpapers_dir",
					 str);


		found = wpNum;
		gnome_config_sync ();
	}

	if (wpFileName)
		g_free(wpFileName);
	wpFileName = g_strdup(str);
	bgType = BACKGROUND_WALLPAPER;

	gtk_option_menu_set_history (GTK_OPTION_MENU (wpOMenu), found);

	fill_monitor (TRUE);
}

static void
wp_selection_ok (GtkWidget *w, GtkWidget **f)
{
	GtkWidget *cf = *f;
	
	if (w)
		delete_browse (w, NULL, f);
	/* printf ("wp ok\n"); */

	gtk_widget_destroy (cf);
	set_monitor_filename (wpFileSelName);
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

	rbut = gtk_radio_button_new_with_label (NULL, _("Scaled"));
	gtk_signal_connect (GTK_OBJECT (rbut), "toggled",
			    (GtkSignalFunc) set_wallpaper_type,
			    (gpointer)WALLPAPER_SCALED);
	gtk_box_pack_end (GTK_BOX (vbox), rbut, FALSE, FALSE, 0);
	gtk_widget_show (rbut);

	rbut = gtk_radio_button_new_with_label
		(gtk_radio_button_group (GTK_RADIO_BUTTON (rbut)),
		 _("Scaled (keep ascpect)"));
	gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (rbut),
				     (wpType == WALLPAPER_SCALED_KEEP));
	gtk_signal_connect (GTK_OBJECT (rbut), "toggled",
			    (GtkSignalFunc) set_wallpaper_type,
			    (gpointer) WALLPAPER_SCALED_KEEP);
	gtk_box_pack_end (GTK_BOX (vbox), rbut, FALSE, FALSE, 0);
	gtk_widget_show (rbut);

	rbut = gtk_radio_button_new_with_label
		(gtk_radio_button_group (GTK_RADIO_BUTTON (rbut)),
		 _("Centered"));
	gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (rbut),
				     (wpType == WALLPAPER_CENTERED));
	gtk_signal_connect (GTK_OBJECT (rbut), "toggled",
			    (GtkSignalFunc) set_wallpaper_type,
			    (gpointer) WALLPAPER_CENTERED);
	gtk_box_pack_end (GTK_BOX (vbox), rbut, FALSE, FALSE, 0);
	gtk_widget_show (rbut);

	rbut = gtk_radio_button_new_with_label
		(gtk_radio_button_group (GTK_RADIO_BUTTON (rbut)),
		 _("Tiled"));
	gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (rbut),
				     (wpType == WALLPAPER_TILED));
	gtk_signal_connect (GTK_OBJECT (rbut), "toggled",
			    (GtkSignalFunc) set_wallpaper_type,
			    (gpointer) WALLPAPER_TILED);
	gtk_box_pack_end (GTK_BOX (vbox), rbut, FALSE, FALSE, 0);
	gtk_widget_show (rbut);

	gtk_container_border_width (GTK_CONTAINER (vbox), GNOME_PAD);
	gtk_container_add (GTK_CONTAINER (wallp), vbox);


	gtk_widget_show (wpOMenu);
	gtk_widget_show (but);
	gtk_widget_show (hbox);
	gtk_widget_show (vbox);
	gtk_widget_show (wallp);

	return wallp;
}

static void
background_apply ()
{
	fillPreview = FALSE;
	fill_monitor (FALSE);
	fillPreview = TRUE;
	property_applied ();
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
	gnome_config_set_int ("/Desktop/Background/wallpaperAlign", wpType);

	gnome_config_sync ();
}

static void
background_read ()
{
	GdkColor bgColor;
	gint r, g, b;

	gdk_color_parse
		(gnome_config_get_string ("/Desktop/Background/color1=#808080"),
		 &bgColor1);
	r = bgColor1.red >> 8;
	g = bgColor1.green >> 8;
	b = bgColor1.blue >> 8;
	bgColor1.pixel = gdk_imlib_best_color_match(&r, &g, &b);
 
	gdk_color_parse
		(gnome_config_get_string ("/Desktop/Background/color2=#0000ff"),
		 &bgColor2);

	r = bgColor2.red >> 8;
	g = bgColor2.green >> 8;
	b = bgColor2.blue >> 8;
	bgColor2.pixel = gdk_imlib_best_color_match(&r, &g, &b);
  
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
	wpType = gnome_config_get_int ("/Desktop/Background/wallpaperAlign=0");

	wpFileName = gnome_config_get_string ("/Desktop/Background/wallpaper=none");
	wpFileSelName = gnome_config_get_string
		("/Desktop/Background/wallpapers_dir=./");

	if (!strcasecmp (wpFileName, "none")) {
		g_free(wpFileName);
		wpFileName = NULL;
		bgType = BACKGROUND_SIMPLE;
	} else
		bgType = BACKGROUND_WALLPAPER;
	/* printf ("%s\n", wpFileName); */
}

/*
 * Invoked when a filename is dropped on the monitor widget
 */
static void
image_dnd_drop(GtkWidget *widget, GdkEventDropDataAvailable *event, gpointer data)
{
	/* Test for the type that was dropped */
	if (strcmp (event->data_type, "url:ALL") != 0)
		return;
	set_monitor_filename (event->data);
	return;
}

static int
connect_dnd (void)
{
	char *image_drop_types[] = {"url:ALL"};

	/* Configure drag and drop on the monitor image */
	gtk_signal_connect (GTK_OBJECT (monitor),
			    "drop_data_available_event",
			    GTK_SIGNAL_FUNC (image_dnd_drop),
			    NULL);
	
	gtk_widget_dnd_drop_set (GTK_WIDGET(monitor), TRUE,
				 image_drop_types, 1, FALSE);
	
	return TRUE;
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
	GtkWidget *vbox, *hbox;
	GtkWidget *fill, *wallp;
	GtkWidget *align;
	
	vbox = gtk_vbox_new (FALSE, 0);

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_container_border_width (GTK_CONTAINER(hbox), GNOME_PAD);

	align = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);

	monitor = get_monitor_preview_widget ();
	gtk_signal_connect (GTK_OBJECT (monitor),
			    "realize",
			    GTK_SIGNAL_FUNC (connect_dnd),
			    NULL);
	gdk_null_window_warnings = 0;
#if 0
	preview = gtk_preview_new(GTK_PREVIEW_COLOR);
	gtk_preview_size(GTK_PREVIEW(preview),
			 MONITOR_CONTENTS_WIDTH,
			 MONITOR_CONTENTS_HEIGHT);
#endif
	gtk_container_add (GTK_CONTAINER (align), monitor);
	gtk_box_pack_start (GTK_BOX(hbox), align, TRUE, TRUE, 0);

	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
	
	settings = gtk_hbox_new (FALSE, GNOME_PAD);
	gtk_container_border_width (GTK_CONTAINER(settings), GNOME_PAD);
	gtk_box_pack_end (GTK_BOX (vbox), settings, TRUE, TRUE, 0);

	fill = color_setup ();
	gtk_box_pack_start (GTK_BOX (settings), fill, FALSE, FALSE, 0);
	
	wallp  = wallpaper_setup ();
	gtk_box_pack_end (GTK_BOX (settings), wallp, TRUE, TRUE, 0);
	
	gtk_widget_show (align);
	gtk_widget_show (monitor);
	gtk_widget_show (settings);
	gtk_widget_show (hbox);
	gtk_widget_show (vbox);

        gnome_property_box_append_page (GNOME_PROPERTY_BOX (config->property_box),
				  vbox,
				  gtk_label_new (_(" Background ")));
}

static gint
background_action (GnomePropertyRequest req)
{
#ifdef DEBUG
  g_print("Doing background_action %d\n", req);
#endif
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
	background_imlib_init ();
	gnome_property_configurator_register (config, background_action);
}

enum {
	INIT_KEY      = -1,
	WALLPAPER_KEY = -2,
	COLOR_KEY     = -3,
	ENDCOLOR_KEY  = -4,
	ORIENT_KEY    = -5,
	SOLID_KEY     = -6,
	GRADIENT_KEY  = -7,
	ALIGN_KEY     = -8
};

/* Options used by background properties.  */
static struct argp_option arguments[] =
{
  { "init",                    -1,  NULL,        0, N_("Set parameters from saved state and exit"), 1 },
  { "setwallpaper",  WALLPAPER_KEY, N_("IMAGE"), 0, N_("Sets the wallpaper to the value specified"), 1 },
  { "color",         COLOR_KEY,     N_("COLOR"), 0, N_("Specifies the background color"), 1 },
  { "endcolor",      ENDCOLOR_KEY,  N_("COLOR"), 0, N_("Specifies end background color for gradient"), 1 },
  { "orient",        ORIENT_KEY,    N_("ORIENT"),0, N_("Gradient orientation: vertical or horizontal"), 1 },
  { "solid",         SOLID_KEY,     NULL,        0, N_("Use a solid fill for the background"), 1 },
  { "gradient",      GRADIENT_KEY,  NULL,        0, N_("Use a gradient fill for the background"), 1 },
  { "wallpapermode", ALIGN_KEY,     N_("MODE"),  0, N_("Display wallpaper: tiled, centered, scaled or ratio"), 1 },
  { NULL, 0, NULL, 0, NULL, 0 }
};

/* Forward decl of our parsing function.  */
static error_t parse_func (int key, char *arg, struct argp_state *state);

/* The parser used by this program.  */
struct argp parser =
{
  arguments,
  parse_func,
  NULL,
  NULL,
  NULL,
  NULL
};

static error_t
parse_func (int key, char *arg, struct argp_state *state)
{
	char *align_keys [] = { "tiled", "centered", "scaled", "ratio" };
	int i;
	
	switch (key){
	case ARGP_KEY_ARG:
		argp_usage (state);
		return 0;

	case INIT_KEY:
		init = 1;
		return 0;

	case WALLPAPER_KEY:
		gnome_config_set_string ("/Desktop/Background/wallpaper", arg);
		init = need_sync = 1;
		return 0;

	case COLOR_KEY:
		gnome_config_set_string ("/Desktop/Background/color1", arg);
		init = need_sync = 1;
		return 0;

	case ENDCOLOR_KEY:
		gnome_config_set_string ("/Desktop/Background/color2", arg);
		gnome_config_set_string ("/Desktop/Background/simple", "gradient");
		init = need_sync = 1;
		return 0;

	case ORIENT_KEY:
		if (strcasecmp (arg, "vertical") == 0 || strcasecmp (arg, "horizontal") == 0){
			gnome_config_set_string ("/Desktop/Background/gradient", arg);
			init = need_sync = 1;
			return 0;
		} else
			return ARGP_ERR_UNKNOWN;
	case SOLID_KEY:
		gnome_config_set_string ("/Desktop/Background/simple", "solid");
		init = need_sync = 1;
		return 0;

	case GRADIENT_KEY:
		gnome_config_set_string ("/Desktop/Background/simple", "gradient");
		init = need_sync = 1;
		return 0;

	case ALIGN_KEY:
		for (i = 0; i < 4; i++)
			if (strcasecmp (align_keys [i], arg) == 0){
				gnome_config_set_int ("/Desktop/Background/wallpaperAlign", i);
				init = need_sync = 1;
				return 0;
			}
		return ARGP_ERR_UNKNOWN;

	default:
		return ARGP_ERR_UNKNOWN;
	}
}


