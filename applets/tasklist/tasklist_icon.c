#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk/gdkx.h>
#include <gnome.h>
#include <gwmh.h>
#include "tasklist_applet.h"

static gboolean tasklist_icon_check_mini (TasklistTask *task);
static gboolean tasklist_icon_check_x (TasklistTask *task);
static void tasklist_icon_set_minimized (TasklistTask *task);

extern GtkWidget *area; /* The drawing area used to display tasks */
extern TasklistIcon *unknown_icon; /* The unknown icon */

#define INTENSITY(r, g, b) ((r) * 0.30 + (g) * 0.59 + (b) * 0.11)

/* Shamelessly stolen from gwmh.c by Tim Janik */


static gpointer
get_typed_property_data (Display *xdisplay,
			 Window   xwindow,
			 Atom     property,
			 Atom     requested_type,
			 gint    *size_p,
			 guint    expected_format)
{
	static const guint prop_buffer_lengh = 1024 * 1024;
	unsigned char *prop_data = NULL;
	Atom type_returned = 0;
	unsigned long nitems_return = 0, bytes_after_return = 0;
	int format_returned = 0;
	gpointer data = NULL;
	gboolean abort = FALSE;
	
	g_return_val_if_fail (size_p != NULL, NULL);
	*size_p = 0;
	
	gdk_error_trap_push ();
	
	abort = XGetWindowProperty (xdisplay,
				    xwindow,
				    property,
				    0, prop_buffer_lengh,
				    False,
				    requested_type,
				    &type_returned, &format_returned,
				    &nitems_return,
				    &bytes_after_return,
				    &prop_data) != Success;
	if (gdk_error_trap_pop () ||
	    type_returned == None)
		abort++;
	if (!abort &&
	    requested_type != AnyPropertyType &&
	    requested_type != type_returned) {
		g_warning (G_GNUC_PRETTY_FUNCTION "(): Property has wrong type, probably on crack");
		abort++;
	}
	if (!abort && bytes_after_return) {
			g_warning (G_GNUC_PRETTY_FUNCTION "(): Eeek, property has more than %u bytes, stored on harddisk?",
				   prop_buffer_lengh);
			abort++;
	}
	if (!abort && expected_format && expected_format != format_returned) {
		g_warning (G_GNUC_PRETTY_FUNCTION "(): Expected format (%u) unmatched (%d), programmer was drunk?",
			   expected_format, format_returned);
		abort++;
	}
	if (!abort && prop_data && nitems_return && format_returned) {
		switch (format_returned) {
		case 32:
			*size_p = nitems_return * 4;
			if (sizeof (gulong) == 8) {
				guint32 i, *mem = g_malloc0 (*size_p + 1);
				gulong *prop_longs = (gulong*) prop_data;
				
				for (i = 0; i < *size_p / 4; i++)
					mem[i] = prop_longs[i];
				data = mem;
			}
			break;
		case 16:
			*size_p = nitems_return * 2;
			break;
		case 8:
			*size_p = nitems_return;
			break;
		default:
			g_warning ("Unknown property data format with %d bits (extraterrestrial?)",
				   format_returned);
			break;
		}
		if (!data && *size_p) {
			guint8 *mem = g_malloc (*size_p + 1);
			
			memcpy (mem, prop_data, *size_p);
			mem[*size_p] = 0;
			data = mem;
		}
	}

	if (prop_data)
		XFree (prop_data);
	
	return data;
}

static gboolean
tasklist_icon_check_mini (TasklistTask *task)
{
	GdkGC *gc;
	int x, y, b, width, height, depth;
	guint32 *atomdata;
	Window root;
	GdkImage *image;
	GdkPixmap *pixmap;
	GdkBitmap *mask;
	GdkPixbuf *pixbuf;
	gint size;
	static gulong KWM_WIN_ICON = 0;
	Display *xdisplay;
	guchar *data;
	
	if (!KWM_WIN_ICON) {
		KWM_WIN_ICON = gdk_atom_intern ("KWM_WIN_ICON", FALSE);
	}

	xdisplay = GDK_WINDOW_XDISPLAY (task->gwmh_task->gdkwindow);
	
	atomdata = get_typed_property_data (xdisplay,
					    task->gwmh_task->xwin,
					    KWM_WIN_ICON,
					    KWM_WIN_ICON,
					    &size,
					    32);
	if (!atomdata)
		return FALSE;
	
	if (!atomdata[0])
		return FALSE;

	/* Get icon size and depth */
	XGetGeometry (xdisplay, (Drawable)atomdata[0], &root, &x, &y,
		      &width, &height, &b, &depth);
	
	/* Create a new GdkPixmap and copy the mini icon pixmap to it */
	pixmap = gdk_pixmap_new (NULL, width, height, depth);
	gc = gdk_gc_new (pixmap);
	XCopyArea (GDK_WINDOW_XDISPLAY (pixmap), atomdata[0], GDK_WINDOW_XWINDOW (pixmap),
		   GDK_GC_XGC (gc), 0, 0, width, height, 0, 0);
	gdk_gc_destroy (gc);
	
	pixbuf = gdk_pixbuf_get_from_drawable (NULL,
					       pixmap,
					       gtk_widget_get_colormap (area),
					       0, 0,
					       0, 0,
					       width, height);
	gdk_pixmap_unref (pixmap);
	
	if (atomdata[1]) {
		mask = gdk_pixmap_new (NULL, width, height, depth);
		gc = gdk_gc_new (mask);
		gdk_gc_set_background (gc, &area->style->black);
		gdk_gc_set_foreground (gc, &area->style->white);
		XCopyPlane (GDK_DISPLAY (), atomdata[1], GDK_WINDOW_XWINDOW (mask),
			    GDK_GC_XGC (gc), 0, 0, width, height, 0, 0, 1);
		gdk_gc_unref (gc);
		
		image = gdk_image_get (mask, 0, 0, width, height);
		g_return_val_if_fail (image != NULL, FALSE);
		
		pixbuf = gdk_pixbuf_add_alpha (pixbuf, FALSE, 0, 0, 0);
		
		data = gdk_pixbuf_get_pixels (pixbuf);
		for (y = 0; y < gdk_pixbuf_get_height (pixbuf); y++) 
			for (x = 0; x < gdk_pixbuf_get_width (pixbuf); x++) 
			{
				data += 3;
				*data++ = gdk_image_get_pixel (image, x, y) == 0 ? 0 : 255;
			}
		
		gdk_pixmap_unref (mask);
	}
	
	task->icon->normal = pixbuf;
	
	return TRUE;
}

static gboolean
tasklist_icon_check_x (TasklistTask *task)
{
	XWMHints * wmhints;
	GdkPixmap *pixmap, *mask;
	GdkPixbuf *pixbuf, *scaled;
	GdkImage *image;
	GdkGC *gc;
	Window root;
	int x, y;
	unsigned int width, height;
	unsigned int border_width;
	unsigned int depth;
	guchar *data;

	wmhints = XGetWMHints (GDK_DISPLAY (), task->gwmh_task->xwin);

	if (!wmhints)
		return FALSE;
	
	if (!(wmhints->flags & IconPixmapHint))
		return FALSE;
	
	XGetGeometry (GDK_DISPLAY (), wmhints->icon_pixmap, &root,
		      &x, &y, &width, &height,
		      &border_width, &depth);

	if (width > 65535 || height > 65535)
		return FALSE;
	
	pixmap = gdk_pixmap_new (area->window, width, height, -1);	
	gc = gdk_gc_new (pixmap);

	if (depth == 1) {
		gdk_gc_set_background (gc, &area->style->white);
		gdk_gc_set_foreground (gc, &area->style->black);
		XCopyPlane (GDK_DISPLAY (), wmhints->icon_pixmap, GDK_WINDOW_XWINDOW (pixmap),
			   GDK_GC_XGC (gc), 0, 0, width, height, 0, 0, 1);
	}
	else {
		XCopyArea (GDK_DISPLAY (), wmhints->icon_pixmap, GDK_WINDOW_XWINDOW (pixmap),
			   GDK_GC_XGC (gc), 0, 0, width, height, 0, 0);
	}
	pixbuf = gdk_pixbuf_get_from_drawable (NULL, pixmap, gtk_widget_get_colormap (area), 
					       0, 0,
					       0, 0, width, height);
	gdk_gc_destroy (gc);

	if (depth == 1) {
		pixbuf = gdk_pixbuf_add_alpha (pixbuf, FALSE, 0, 0, 0);
	}
	
	if (wmhints->flags & IconMaskHint) {
	       mask = gdk_pixmap_new (NULL, width, height, depth);
	       gc = gdk_gc_new (mask);
	       gdk_gc_set_background (gc, &area->style->black);
	       gdk_gc_set_foreground (gc, &area->style->white);
	       XCopyPlane (GDK_DISPLAY (), wmhints->icon_mask, GDK_WINDOW_XWINDOW (mask),
			   GDK_GC_XGC (gc), 0, 0, width, height, 0, 0, 1);
	       gdk_gc_unref (gc);

	       image = gdk_image_get (mask, 0, 0, width, height);
	       g_return_val_if_fail (image != NULL, FALSE);

	       scaled = pixbuf;
	       pixbuf = gdk_pixbuf_add_alpha (pixbuf, FALSE, 0, 0, 0);
	       gdk_pixbuf_unref (scaled);

	       data = gdk_pixbuf_get_pixels (pixbuf);
	       for (y = 0; y < gdk_pixbuf_get_height (pixbuf); y++)
		 for (x = 0; x < gdk_pixbuf_get_width (pixbuf); x++) 
		   {
		     data += 3;
		     *data++ = gdk_image_get_pixel (image, x, y) == 0 ? 0 : 255;
		   }

	       gdk_pixmap_unref (mask);
	}

	scaled = gdk_pixbuf_scale_simple (pixbuf,
					  20, 20,
					  GDK_INTERP_BILINEAR);
	gdk_pixbuf_unref (pixbuf);
	gdk_pixmap_unref (pixmap);
	
	task->icon->normal = scaled;

	return TRUE;

}

void
tasklist_icon_set (TasklistTask *task)
{
	task->icon = g_new (TasklistIcon, 1);

	task->icon->normal = unknown_icon->normal;
	task->icon->minimized = unknown_icon->minimized;

	if (!tasklist_icon_check_mini (task))
		tasklist_icon_check_x (task);

	tasklist_icon_set_minimized (task);
}

static void
tasklist_icon_set_minimized (TasklistTask *task)
{
	if (task->icon->minimized &&
		task->icon->minimized != unknown_icon->minimized)
		gdk_pixbuf_unref (task->icon->minimized);

	task->icon->minimized = tasklist_icon_create_minimized_icon (task->icon->normal);
}

void
tasklist_icon_destroy (TasklistTask *task)
{
	if (task->icon->normal != unknown_icon->normal)
		gdk_pixbuf_unref (task->icon->normal);
	
	if (task->icon->minimized != unknown_icon->minimized)
		gdk_pixbuf_unref (task->icon->minimized);

	g_free (task->icon);
}

/* Stolen from gnome-pixmap.c */
GdkPixbuf *
tasklist_icon_create_minimized_icon (GdkPixbuf *pixbuf)
{
	GdkPixbuf *target;
	gint i, j;
	gint width, height, has_alpha, rowstride;
	guchar *target_pixels;
	guchar *original_pixels;
	guchar *dest_pixel, *src_pixel;
	gint32 red, green, blue;
	GdkColor color;
	
	color = area->style->bg[GTK_STATE_NORMAL];
	red = color.red / 255;
	blue = color.blue / 255;
	green = color.green / 255;
	
	has_alpha = gdk_pixbuf_get_has_alpha (pixbuf);
	width = gdk_pixbuf_get_width (pixbuf);
	height = gdk_pixbuf_get_height (pixbuf);
	rowstride = gdk_pixbuf_get_rowstride (pixbuf);

	target = gdk_pixbuf_new (GDK_COLORSPACE_RGB,
				 has_alpha,
				 gdk_pixbuf_get_bits_per_sample (pixbuf),
				 width, height);

	target_pixels = gdk_pixbuf_get_pixels (target);
	original_pixels = gdk_pixbuf_get_pixels (pixbuf);

	for (i = 0; i < height; i++) {
		for (j = 0; j < width; j++) {
				src_pixel = original_pixels + i*rowstride + j*(has_alpha?4:3);
				dest_pixel = target_pixels + i*rowstride + j*(has_alpha?4:3);

				dest_pixel[0] = ((src_pixel[0] - red) >> 2) + red;
				dest_pixel[1] = ((src_pixel[1] - green) >> 2) + green;
				dest_pixel[2] = ((src_pixel[2] - blue) >> 2) + blue;
				
				if (has_alpha)
					dest_pixel[3] = src_pixel[3];

		}
	}
	
	return target;
}