/*
 * Background display property module.
 * (C) 1997 the Free Software Foundation
 *
 * Authors: Miguel de Icaza.
 *          Federico Mena.
 *          Radek Doulik
 *          Michael Fulbright
 */

#include <config.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include <gdk/gdkx.h>

#include "capplet-widget.h"
#include <gnome.h>
#include "gnome-desktop.h"
#include "imlib-misc.h"

static GtkWidget *capplet=NULL;

static GtkWidget *preview;
static GtkWidget *monitor;

static GtkWidget *fileSel = NULL;
static GtkWidget *wpMenu;
static GtkWidget *wpOMenu;
static gchar *wpFileSelName = NULL;
static gint wpNum;
static gint fillPreview = TRUE;
static gint ignoreChanges = TRUE;

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

enum {
  TARGET_URI_LIST,
};

struct bgState {
    GdkColor   bgColor1, bgColor2;
    gint       grad;
    gint       vertical;
    gint       bgType;
    gint       wpType;
    gchar      *wpFileName;
    gchar      *wpFileSelName;
};

struct bgState origState, curState;

/* The pointers to the color selectors */
static GtkWidget *cp1, *cp2;

void background_init(void);
void background_properties_init(void);
void background_read(struct bgState *state);
void background_setup(struct bgState *state);


void
printState( struct bgState *state )
{
    printf("\n-------------------------------\n");
    printf("Color 1: #%04x%04x%04x\n",
	      state->bgColor1.red >> 8,
	      state->bgColor1.green >> 8,
	      state->bgColor1.blue >> 8);
    printf("Color 2: #%04x%04x%04x\n",
	      state->bgColor2.red >> 8,
	      state->bgColor2.green >> 8,
	      state->bgColor2.blue >> 8);
    printf("Mode   : %s\n", (state->grad) ? "gradient" : "solid");
    printf("Direct : %s\n", (state->vertical) ? "vertical" : "horizontal");
    printf("bgType : %s\n", (state->bgType == BACKGROUND_SIMPLE) ? "none" : state->wpFileName);
    printf("wpType : %d\n", state->wpType);
    printf("\n-------------------------------\n");
}

void
copyState(struct bgState *dest, struct bgState *src)
{
    memcpy(dest, src, sizeof(struct bgState));
    if (src->wpFileName)
	dest->wpFileName = g_strdup(src->wpFileName);
    if (src->wpFileSelName)
	dest->wpFileSelName = g_strdup(src->wpFileSelName);
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

static void
fill_monitor (int prop_changed, struct bgState *state)
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
    gint r, g, b, a;
    gint cx, cy;
    gint cw, ch;
    GdkCursor *cursor;

    if (prop_changed)
	capplet_widget_state_changed(CAPPLET_WIDGET(capplet), TRUE);
    
    /* Set the cursor to a nice watch.  The main_window may not exist
     * if we are called from the session manager, though
     */
    
    if (capplet) {
	cursor = gdk_cursor_new (GDK_WATCH);
	gdk_window_set_cursor (capplet->window, cursor);
	gdk_cursor_destroy (cursor);
    }
    
    rootWindow = gdk_window_foreign_new (GDK_ROOT_WINDOW());
    gdk_window_get_size (rootWindow, &rootWidth, &rootHeight);
    
    if (state->bgType == BACKGROUND_WALLPAPER) {
	bi = Imlib_load_image (imlib_data, state->wpFileName);
	if (bi)
	    Imlib_render (imlib_data, bi, 
			  bi->rgb_width, bi->rgb_height);
	else {
	    state->bgType = BACKGROUND_SIMPLE;
	    gtk_option_menu_set_history (GTK_OPTION_MENU (wpOMenu), 0);
	}
    }
    
    if (capplet) {
	gnome_color_picker_get_i16(GNOME_COLOR_PICKER(cp1), &r, &g, &b, &a);
	state->bgColor1.red = r;
	state->bgColor1.green = g;
	state->bgColor1.blue = b;
	r >>= 8;
	g >>= 8;
	b >>= 8;
	state->bgColor1.pixel = gdk_imlib_best_color_match(&r, &g, &b);

	gnome_color_picker_get_i16(GNOME_COLOR_PICKER(cp2),  &r, &g, &b, &a);
	state->bgColor2.red = r;
	state->bgColor2.green = g;
	state->bgColor2.blue = b;
	r >>= 8;
	g >>= 8;
	b >>= 8;
	state->bgColor2.pixel = gdk_imlib_best_color_match(&r, &g, &b);
    }
    
    /* are we working on root window ? */
    if (!fillPreview) {
	gc = gdk_gc_new (rootWindow);
	xgc = ((GdkGCPrivate *) gc)->xgc;
	
	if ((!state->grad || (bi && !bi->shape_mask)) && 
	    state->wpType == WALLPAPER_TILED) {
	    if (state->bgType == BACKGROUND_WALLPAPER) {
		Pixmap bg;

		pix = Imlib_move_image (imlib_data, bi);
		mask = Imlib_move_mask (imlib_data, bi);
		bg = XCreatePixmap (GDK_DISPLAY (), GDK_ROOT_WINDOW (),
				    bi->rgb_width, bi->rgb_height,
				    gdkx_visual_get (Imlib_get_visual (imlib_data)->visualid)->depth);
		
		gdk_color_alloc (gdk_window_get_colormap (rootWindow), 
				 &state->bgColor1);
		gdk_gc_set_foreground (gc, &state->bgColor1);

		XFillRectangle (GDK_DISPLAY (), bg, xgc, 0, 0,
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

		XSetWindowBackgroundPixmap(GDK_DISPLAY(), GDK_ROOT_WINDOW(),
					   bg);
		
		Imlib_free_pixmap (imlib_data, pix);
		Imlib_free_pixmap (imlib_data, mask);
		Imlib_destroy_image (imlib_data, bi);
		XClearWindow (GDK_DISPLAY (), GDK_ROOT_WINDOW ());
		XFreePixmap (GDK_DISPLAY (), bg);
	    } else {
#ifdef DEBUG
		g_print("Setting background in fill_monitor\n");
#endif
		gdk_window_set_background (rootWindow, &state->bgColor1);
		gdk_window_clear (rootWindow);
	    }
			
	    gdk_gc_unref (gc);
	    
	    /* Reset the cursor to normal */
	    if (capplet)
		gdk_window_set_cursor (capplet->window, NULL);
	    
	    return;
	}
	
	cx = cy = 0;

	if (state->grad && state->bgType == BACKGROUND_SIMPLE) {
	    if (state->vertical) {
		cw = 32;
		ch = rootHeight;
	    } else {
		cw = rootWidth;
		ch = 32;
	    }
	} else {
	    cw = rootWidth;
	    ch = rootHeight;
	}
/*	screen = gdk_pixmap_new (rootWindow, rootWidth, rootHeight, -1); */
	screen = gdk_pixmap_new (rootWindow, cw, ch, -1); 
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
    
    if (state->grad) {
	   
	int render_type;
	
	pdata = g_new (unsigned char, cw*ch*3);
	
	fill_gradient (pdata, cw, ch, 
		       &state->bgColor1, &state->bgColor2, state->vertical);
	
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
	gdk_color_alloc (gdk_window_get_colormap (rootWindow), 
			 &state->bgColor1);
	gdk_gc_set_foreground (gc, &state->bgColor1);
	XFillRectangle (GDK_DISPLAY (), xscreen, xgc, cx, cy, cw, ch);
    }
    
    if (state->bgType == BACKGROUND_WALLPAPER) {
	gint w, h;
	gint xoff, yoff;
	
	if (fillPreview &&
	    state->wpType != WALLPAPER_SCALED && 
	    state->wpType != WALLPAPER_SCALED_KEEP) {
	    w = (cw*bi->rgb_width) / rootWidth;
	    h = (ch*bi->rgb_height) / rootHeight;
	    bi = Imlib_clone_scaled_image (imlib_data, bi, w, h);
	    Imlib_render (imlib_data, bi, w, h);
	}
		
	w = bi->rgb_width;
	h = bi->rgb_height;
	
	pix = Imlib_move_image (imlib_data, bi);
	mask = Imlib_move_mask (imlib_data, bi);
	
	if (state->wpType == WALLPAPER_TILED) {
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
	    
	    if (state->wpType == WALLPAPER_SCALED || 
		state->wpType == WALLPAPER_SCALED_KEEP) {

		if (state->wpType == WALLPAPER_SCALED_KEEP) {

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
	gdk_window_set_back_pixmap (rootWindow, screen, FALSE);
	gdk_pixmap_unref (screen);
	gdk_window_clear (rootWindow);
    }
    
    /* Reset the cursor to normal */
    
    if (capplet)
	gdk_window_set_cursor (capplet->window, NULL);
}

static void
color_sel_set (GnomeColorPicker *gcp, gpointer data)
{
    fill_monitor (TRUE, &curState);
    if (!ignoreChanges)
	capplet_widget_state_changed(CAPPLET_WIDGET(capplet), TRUE);

#ifdef DEBUG
    printf("Changing color, ignore changes is %d\n",ignoreChanges);
    printState(&curState);
#endif
}

static gint
idle_fill_monitor (gpointer data)
{
	fill_monitor (FALSE, &curState);
	return FALSE;
}

static GtkWidget *radioh, *radiov, *radiof, *radiog;
static GtkWidget *tiledButton, *scaledButton;
static GtkWidget *scaledkeepButton, *centeredButton;

static void
set_background_mode (GtkWidget *widget)
{
    if (GTK_TOGGLE_BUTTON (widget)->active) {
	if (widget == radiof) {
	    /* Flat color */
	    gtk_widget_set_sensitive(cp2, FALSE);
	    gtk_widget_set_sensitive(radioh, FALSE);
	    gtk_widget_set_sensitive(radiov, FALSE);
	    curState.grad = FALSE;
	} else if (widget == radiog) {
	    /* Gradient fill */
	    gtk_widget_set_sensitive(cp2, TRUE);
	    gtk_widget_set_sensitive(radioh, TRUE);
	    gtk_widget_set_sensitive(radiov, TRUE);
	    curState.grad = TRUE;
	}

	fill_monitor (!ignoreChanges, &curState);
    }
    if (!ignoreChanges)
	capplet_widget_state_changed(CAPPLET_WIDGET(capplet), TRUE);

#ifdef DEBUG
    printf("Setting mode to %s\n",(widget == radiof) ? "Solid" : "Gradient");
    printState(&curState);
#endif
}

static void
set_orientation (GtkWidget *widget)
{
    if (GTK_TOGGLE_BUTTON (widget)->active) {
	if (widget == radioh)
	    curState.vertical = FALSE;  /* Horizontal gradient */
	else if (widget == radiov)
	    curState.vertical = TRUE;   /* Vertical gradient */
	
	fill_monitor (!ignoreChanges, &curState);
    }
    if (!ignoreChanges)
	capplet_widget_state_changed(CAPPLET_WIDGET(capplet), TRUE);

#ifdef DEBUG
    printf("Setting orientation to %s\n",(widget == radioh) ? "Horizontal" : "Vertical");
    printState(&curState);
#endif
}

static void
set_wallpaper_type (GtkWidget *widget, gpointer data)
{
    if (GTK_TOGGLE_BUTTON (widget)->active) {
	curState.wpType = (gint) data;
	
	fill_monitor (!ignoreChanges, &curState);
    }
    if (!ignoreChanges)
	capplet_widget_state_changed(CAPPLET_WIDGET(capplet), TRUE);
}

static GtkWidget *
color_setup (struct bgState *state)
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
    gtk_container_set_border_width (GTK_CONTAINER(table), GNOME_PAD);
    gtk_table_set_col_spacings (GTK_TABLE(table), GNOME_PAD);
    gtk_container_add (GTK_CONTAINER(frame), table);
    gtk_widget_show (table);
    
    cp1 = gnome_color_picker_new ();
    gnome_color_picker_set_i16 (GNOME_COLOR_PICKER(cp1), 
				state->bgColor1.red, state->bgColor1.green,
				state->bgColor1.blue, 0xff);
    gtk_signal_connect(GTK_OBJECT(cp1), "color_set", 
		       GTK_SIGNAL_FUNC(color_sel_set), NULL);
	
    gtk_table_attach (GTK_TABLE(table), cp1, 0, 1, 0, 1, 0, 0, 0, 0);
    gtk_widget_show (cp1);

    radiof = gtk_radio_button_new_with_label (NULL, _("Flat"));
    gtk_box_pack_start (GTK_BOX (vb1), radiof, FALSE, FALSE, 0);
    gtk_table_attach_defaults (GTK_TABLE(table), vb1, 1, 2, 0, 1);
    gtk_widget_show (radiof);
    
    radiog = gtk_radio_button_new_with_label (gtk_radio_button_group(GTK_RADIO_BUTTON(radiof)),
					      _("Gradient"));
    gtk_box_pack_start (GTK_BOX (vb1), radiog, FALSE, FALSE, 0);
    gtk_widget_show(radiog);

    cp2 = gnome_color_picker_new ();
    gnome_color_picker_set_i16 (GNOME_COLOR_PICKER(cp2), 
				state->bgColor2.red, state->bgColor2.green,
				state->bgColor2.blue, 0xff);
    gtk_signal_connect(GTK_OBJECT(cp2), "color_set", 
		       GTK_SIGNAL_FUNC(color_sel_set), NULL);
    
    gtk_table_attach (GTK_TABLE(table), cp2, 0, 1, 1, 2, 0, 0, 0, 0);
    gtk_widget_set_sensitive (cp2, FALSE);
    gtk_widget_show (cp2);
    
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
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON((state->grad) ? radiog : radiof), TRUE);

    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON((state->vertical) ? radiov : radioh), TRUE);
    
    gtk_widget_set_sensitive(cp2, state->grad);
    gtk_widget_set_sensitive(radioh, state->grad);
    gtk_widget_set_sensitive(radiov, state->grad);
	
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
	if (curState.wpFileName)
		g_free(curState.wpFileName);
	curState.wpFileName = g_strdup(s);
	curState.bgType = (s) ? BACKGROUND_WALLPAPER : BACKGROUND_SIMPLE;
	/* printf ("%s\n", s); */

	fill_monitor (TRUE, &curState);
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
set_monitor_filename (gchar *str)
{
    GString *gs;
    gchar num[32];
    gint found = -1, i=1;
    GList *child = GTK_MENU_SHELL (wpMenu)->children;
    GtkWidget *cf;
    
/*    printf("searching for %s\n",str); */
    while (child) {
	if (child->data)
	    if (GTK_BIN (child->data)->child) {
/*		printf ("Searching %s\n", GTK_LABEL (GTK_BIN (child->data)->child)->label); */
		if (!strcmp (GTK_LABEL (GTK_BIN (child->data)->child)->label, str)) {
		    found = i;
/*		    printf ("found: %d\n", i);  */
		}
	    }
	i++;
	child = child->next;
    }
    
    /* hack */
    if (!ignoreChanges && found < 0) {
	
	cf = gtk_menu_item_new_with_label (str);
	gtk_signal_connect (GTK_OBJECT (cf),
			    "activate",
			    (GtkSignalFunc) browse_activated, str);
	gtk_menu_append (GTK_MENU (wpMenu), cf);
	gtk_widget_show (cf);
	wpNum++;
	
	gs = g_string_new ("/Background/Default/wallpaper");
	snprintf (num, sizeof(num), "%d", wpNum);
	g_string_append (gs, num);
	gnome_config_set_string (gs->str, str);
	g_string_free (gs, TRUE);
	
	gnome_config_set_int ("/Background/Default/wallpapers", wpNum);
	gnome_config_set_string ("/Background/Default/wallpapers_dir",
				 str);
	
	found = wpNum;
	gnome_config_sync ();
    }
    
    if (curState.wpFileName)
	g_free(curState.wpFileName);
    curState.wpFileName = g_strdup(str);
    curState.bgType = BACKGROUND_WALLPAPER;
    
    gtk_option_menu_set_history (GTK_OPTION_MENU (wpOMenu), found);
    
    fill_monitor (!ignoreChanges, &curState);

    if (!ignoreChanges)
	capplet_widget_state_changed(CAPPLET_WIDGET(capplet), TRUE);
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
	else if (origState.wpFileName)
	    gtk_file_selection_set_filename (GTK_FILE_SELECTION (fileSel),
					     origState.wpFileName);
	
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
wallpaper_setup (struct bgState *state)
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
    
    wpNum = gnome_config_get_int ("/Background/Default/wallpapers=0");
    
    for (i = 0; i<wpNum; i++) {
	
	/* printf ("wallpaper%d", i); */
	wpName = g_string_new ("/Background/Default/wallpaper");
	snprintf (num, sizeof(num),"%d", i+1);
	g_string_append (wpName, num);
	g_string_append (wpName, "=???");
	
	wpName1 = gnome_config_get_string (wpName->str);
	/* printf (": %s\n", wpName1); */
	if (wpName1) {
	    if (state->wpFileName)
		if (!strcmp (wpName1, state->wpFileName))
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
    scaledButton = rbut;

    rbut = gtk_radio_button_new_with_label
	(gtk_radio_button_group (GTK_RADIO_BUTTON (rbut)),
	 _("Scaled (keep aspect)"));
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (rbut),
				 (state->wpType == WALLPAPER_SCALED_KEEP));
    gtk_signal_connect (GTK_OBJECT (rbut), "toggled",
			(GtkSignalFunc) set_wallpaper_type,
			(gpointer) WALLPAPER_SCALED_KEEP);
    gtk_box_pack_end (GTK_BOX (vbox), rbut, FALSE, FALSE, 0);
    gtk_widget_show (rbut);
    scaledkeepButton = rbut;

    rbut = gtk_radio_button_new_with_label
	(gtk_radio_button_group (GTK_RADIO_BUTTON (rbut)),
	 _("Centered"));
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (rbut),
				 (state->wpType == WALLPAPER_CENTERED));
    gtk_signal_connect (GTK_OBJECT (rbut), "toggled",
			(GtkSignalFunc) set_wallpaper_type,
			(gpointer) WALLPAPER_CENTERED);
    gtk_box_pack_end (GTK_BOX (vbox), rbut, FALSE, FALSE, 0);
    gtk_widget_show (rbut);
    centeredButton = rbut;

    rbut = gtk_radio_button_new_with_label
	(gtk_radio_button_group (GTK_RADIO_BUTTON (rbut)),
	 _("Tiled"));
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (rbut),
				 (state->wpType == WALLPAPER_TILED));
    gtk_signal_connect (GTK_OBJECT (rbut), "toggled",
			(GtkSignalFunc) set_wallpaper_type,
			(gpointer) WALLPAPER_TILED);
    gtk_box_pack_end (GTK_BOX (vbox), rbut, FALSE, FALSE, 0);
    gtk_widget_show (rbut);
    tiledButton = rbut;

    gtk_container_set_border_width (GTK_CONTAINER (vbox), GNOME_PAD);
    gtk_container_add (GTK_CONTAINER (wallp), vbox);

    gtk_widget_show (wpOMenu);
    gtk_widget_show (but);
    gtk_widget_show (hbox);
    gtk_widget_show (vbox);
    gtk_widget_show (wallp);

    return wallp;
}

static void
background_apply (struct bgState *state)
{
    GtkWidget *choice;

    /* need to move all this stuff eventually */
    ignoreChanges = TRUE;


    gnome_color_picker_set_i16 (GNOME_COLOR_PICKER(cp1), 
				state->bgColor1.red, state->bgColor1.green,
				state->bgColor1.blue, 0xff);
    gnome_color_picker_set_i16 (GNOME_COLOR_PICKER(cp2), 
				state->bgColor2.red, state->bgColor2.green,
				state->bgColor2.blue, 0xff);
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON((state->grad) ? radiog : radiof), TRUE);

    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON((state->vertical) ? radiov : radioh), TRUE);
    
    gtk_widget_set_sensitive(cp2, state->grad);
    gtk_widget_set_sensitive(radioh, state->grad);
    gtk_widget_set_sensitive(radiov, state->grad);

    switch (state->wpType) {
      case WALLPAPER_SCALED:
	choice = scaledButton;
	break;
      case WALLPAPER_SCALED_KEEP:
	choice = scaledkeepButton;
	break;
      case WALLPAPER_CENTERED:
	choice = centeredButton;
	break;
      case WALLPAPER_TILED:
	choice = tiledButton;
	break;
      default:
	choice = tiledButton;
    }
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (choice), TRUE);

    if (state->bgType!= BACKGROUND_SIMPLE) {
	gchar *tmp;
printf("setting filename back to %s\n",state->wpFileName);
	/* ok this is horrible i should die */
	tmp = g_strdup(state->wpFileName);
	set_monitor_filename(tmp);
	g_free(tmp);
    } else {
	gtk_option_menu_set_history(GTK_OPTION_MENU(wpOMenu), 0);
    }

    fillPreview = FALSE;
    fill_monitor (FALSE, state);
    fillPreview = TRUE;
    ignoreChanges=FALSE;
}

static void
background_try(GtkWidget *widget, struct bgState *state)
{
    background_apply(state);
}

static void
background_revert ()
{
    ignoreChanges = TRUE;

#ifdef DEBUG
    printf("Current State\n");
    printState(&origState);
#endif

    background_apply(&origState);
    fillPreview = TRUE;
    fill_monitor (FALSE, &origState);

#ifdef DEBUG
    printf("Restored State\n");
    printState(&origState);
#endif

    copyState(&curState, &origState);
    ignoreChanges = FALSE;
}

static void
background_write (struct bgState *state)
{
    char buffer [60];


#ifdef DEBUG
    printf("Saving state to disk\n");
    printState(state);
#endif
    snprintf (buffer, sizeof(buffer), "#%02x%02x%02x",
	      state->bgColor1.red >> 8,
	      state->bgColor1.green >> 8,
	      state->bgColor1.blue >> 8);
    gnome_config_set_string ("/Background/Default/color1", buffer);
    snprintf (buffer, sizeof(buffer), "#%02x%02x%02x",
	      state->bgColor2.red >> 8,
	      state->bgColor2.green >> 8,
	      state->bgColor2.blue >> 8);
    gnome_config_set_string ("/Background/Default/color2", buffer);
    
    gnome_config_set_string ("/Background/Default/simple",
			     (state->grad) ? "gradient" : "solid");
    gnome_config_set_string ("/Background/Default/gradient",
			     (state->vertical) ? "vertical" : "horizontal");
    
    gnome_config_set_string ("/Background/Default/wallpaper",
			     (state->bgType == BACKGROUND_SIMPLE) ? "none" : state->wpFileName);
    gnome_config_set_int ("/Background/Default/wallpaperAlign", state->wpType);
    
    gnome_config_sync ();
#if 0    
    background_apply(state);
#endif
}

void
background_read ( struct bgState *state )
{
	GdkColor bgColor;
	gint r, g, b;

	gdk_color_parse
		(gnome_config_get_string ("/Background/Default/color1=#808080"),
		 &state->bgColor1);
	r = state->bgColor1.red >> 8;
	g = state->bgColor1.green >> 8;
	b = state->bgColor1.blue >> 8;
	state->bgColor1.pixel = gdk_imlib_best_color_match(&r, &g, &b);
 
	gdk_color_parse
		(gnome_config_get_string ("/Background/Default/color2=#0000ff"),
		 &state->bgColor2);

	r = state->bgColor2.red >> 8;
	g = state->bgColor2.green >> 8;
	b = state->bgColor2.blue >> 8;
	state->bgColor2.pixel = gdk_imlib_best_color_match(&r, &g, &b);
  
	state->bgType = (strcasecmp
		  (gnome_config_get_string
		   ("/Background/Default/type=simple"),
		   "simple"));
	state->grad = (strcasecmp
		(gnome_config_get_string
		 ("/Background/Default/simple=solid"),
		 "solid"));
	state->vertical = !(strcasecmp
		     (gnome_config_get_string
		      ("/Background/Default/gradient=vertical"),
		      "vertical"));
	state->wpType = gnome_config_get_int ("/Background/Default/wallpaperAlign=0");

	state->wpFileName = gnome_config_get_string ("/Background/Default/wallpaper=none");
	state->wpFileSelName = gnome_config_get_string 
		("/Background/Default/wallpapers_dir=./");

	if (!strcasecmp (state->wpFileName, "none")) {
		g_free(state->wpFileName);
		state->wpFileName = NULL;
		state->bgType = BACKGROUND_SIMPLE;
	} else
		state->bgType = BACKGROUND_WALLPAPER;
}

void
background_ok(GtkWidget *widget, struct bgState *state)
{
    background_apply(state);
    background_write(state);
}

void
background_init() {
    background_read(&origState);
    copyState(&curState, &origState);
    ignoreChanges = TRUE;
    background_setup(&origState);
    ignoreChanges = FALSE;

#ifdef DEBUG
    printState(&origState);
#endif
}

void
background_properties_init() {
    background_read(&origState);
    copyState(&curState, &origState);
    ignoreChanges = TRUE;
    fillPreview = FALSE;
    fill_monitor (FALSE, &origState);
    fillPreview = TRUE;
    ignoreChanges = FALSE;

#ifdef DEBUG
    printState(&origState);
#endif
}

/*
 * Invoked when a filename is dropped on the monitor widget
 */
static void
img_dnd_drop (GtkWidget *widget, GdkDragContext *context, gint x, gint y,
	      GtkSelectionData *selection_data, guint info,
	      guint time, gpointer data)
{
        GList *names;
  
	switch (info) {
	case TARGET_URI_LIST:
		names = gnome_uri_list_extract_filenames (selection_data->data);
		if (names) {
			set_monitor_filename ((gchar *)names->data);
			gnome_uri_list_free_strings (names);
		}
		break;
	default:
	}
}

/*
 * background_setup: creates the dialog box for configuring the
 * display background and registers it with the display property
 * configurator
 */
void
background_setup (struct bgState *state)
{
     static GtkTargetEntry drop_types [] = { 
	     { "text/uri-list", 0, TARGET_URI_LIST },
     };
     static gint n_drop_types = sizeof (drop_types) / sizeof(drop_types[0]);

    GtkWidget *settings;
    GtkWidget *vbox, *hbox;
    GtkWidget *fill, *wallp;
    GtkWidget *align;

    capplet = capplet_widget_new();

    vbox = gtk_vbox_new (FALSE, 0);
    hbox = gtk_hbox_new (FALSE, 0);
    gtk_container_set_border_width (GTK_CONTAINER(hbox), GNOME_PAD);

    align = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);

    monitor = get_monitor_preview_widget ();

    gtk_drag_dest_set (GTK_WIDGET (monitor),
		       GTK_DEST_DEFAULT_MOTION |
		       GTK_DEST_DEFAULT_HIGHLIGHT |
		       GTK_DEST_DEFAULT_DROP,
		       drop_types, n_drop_types,
		       GDK_ACTION_COPY);

    gtk_signal_connect (GTK_OBJECT (monitor), "drag_data_received",
			GTK_SIGNAL_FUNC (img_dnd_drop), NULL);
			    
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
    gtk_container_set_border_width (GTK_CONTAINER(settings), GNOME_PAD);
    gtk_box_pack_end (GTK_BOX (vbox), settings, TRUE, TRUE, 0);

    fill = color_setup (state);
    gtk_box_pack_start (GTK_BOX (settings), fill, FALSE, FALSE, 0);
	
    wallp  = wallpaper_setup (state);
    gtk_box_pack_end (GTK_BOX (settings), wallp, TRUE, TRUE, 0);
	
    gtk_widget_show (align);
    gtk_widget_show (monitor);
    gtk_widget_show (settings);
    gtk_widget_show (hbox);
    gtk_widget_show (vbox);


    gtk_signal_connect (GTK_OBJECT (capplet), "try",
			GTK_SIGNAL_FUNC (background_try), &curState);
    gtk_signal_connect (GTK_OBJECT (capplet), "revert",
			GTK_SIGNAL_FUNC (background_revert), NULL);
    gtk_signal_connect (GTK_OBJECT (capplet), "ok",
			GTK_SIGNAL_FUNC (background_ok), &curState);

    gtk_container_add(GTK_CONTAINER(capplet), vbox);
    gtk_widget_show(capplet);
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

#if 0
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
		gnome_config_set_string ("/Background/Default/wallpaper", arg);
		init = need_sync = 1;
		return 0;

	case COLOR_KEY:
		gnome_config_set_string ("/Background/Default/color1", arg);
		init = need_sync = 1;
		return 0;

	case ENDCOLOR_KEY:
		gnome_config_set_string ("/Background/Default/color2", arg);
		gnome_config_set_string ("/Background/Default/simple", "gradient");
		init = need_sync = 1;
		return 0;

	case ORIENT_KEY:
		if (strcasecmp (arg, "vertical") == 0 || strcasecmp (arg, "horizontal") == 0){
			gnome_config_set_string ("/Background/Default/gradient", arg);
			init = need_sync = 1;
			return 0;
		} else
			return ARGP_ERR_UNKNOWN;
	case SOLID_KEY:
		gnome_config_set_string ("/Background/Default/simple", "solid");
		init = need_sync = 1;
		return 0;

	case GRADIENT_KEY:
		gnome_config_set_string ("/Background/Default/simple", "gradient");
		init = need_sync = 1;
		return 0;

	case ALIGN_KEY:
		for (i = 0; i < 4; i++)
			if (strcasecmp (align_keys [i], arg) == 0){
				gnome_config_set_int ("/Background/Default/wallpaperAlign", i);
				init = need_sync = 1;
				return 0;
			}
		return ARGP_ERR_UNKNOWN;

	default:
		return ARGP_ERR_UNKNOWN;
	}
}

#endif

