/* Miscellaneous Imlib-based functions for desktop-properties */

/* This is a very bad hack to allow having a second instance of imlib
 * in addition to the "normal" one initialized by the Gnome libraries.
 * We need a second instance when setting the root window's background
 * because the root window may be running in a different visual than
 * the one imlib normally uses.
 *
 * In this modified version of Imlib_init, we set the visual and
 * colormap to those which are being used by the root window.
 *
 * I know this is ugly, so sue me.  I don't want to copy the user's
 * .imrc to a temporary file and write a new one with the changed
 * visual and colormap
 *
 * - Federico
 */

#include <gdk/gdkx.h>
#include "imlib-misc.h"
#include <gnome.h>

ImlibData *imlib_data;

void
background_imlib_init(void)
{
	ImlibInitParams params;

	params.visualid = XVisualIDFromVisual (DefaultVisual (GDK_DISPLAY (), DefaultScreen (GDK_DISPLAY ())));
	params.flags = PARAMS_VISUALID;

	imlib_data = Imlib_init_with_params (GDK_DISPLAY (), &params);
}

GtkWidget *
get_monitor_preview_widget (void)
{
	GdkVisual *visual;
	GdkColormap *cmap;
	GtkWidget *eb;
	GtkWidget *p;
	ImlibImage *im;
	Pixmap xpixmap;
	Pixmap xmask;
	GdkPixmap *pixmap;
	GdkBitmap *mask;
	GdkGC     *gc, *mgc;
	char *f;

	visual = gdkx_visual_get (Imlib_get_visual (imlib_data)->visualid);
	cmap = gdkx_colormap_get (Imlib_get_colormap (imlib_data));

	gtk_widget_push_visual (visual);
	gtk_widget_push_colormap (cmap);

	f = gnome_pixmap_file ("monitor.png");
	im = Imlib_load_image (imlib_data, f);
	g_free (f);

	Imlib_render (imlib_data, im, im->rgb_width, im->rgb_height);

	xpixmap = Imlib_copy_image (imlib_data, im);
	xmask = Imlib_copy_mask (imlib_data, im);

	Imlib_destroy_image (imlib_data, im);

	gc = gdk_gc_new (GDK_ROOT_PARENT ());

	if (xpixmap) {
		pixmap = gdk_pixmap_new (GDK_ROOT_PARENT (),
					 im->rgb_width,
					 im->rgb_height,
					 visual->depth);
		XCopyArea (GDK_DISPLAY (),
			   xpixmap,
			   ((GdkWindowPrivate *) pixmap)->xwindow,
			   ((GdkGCPrivate *) gc)->xgc,
			   0, 0,
			   im->rgb_width, im->rgb_height,
			   0, 0);
		XFreePixmap (GDK_DISPLAY (), xpixmap);
	} else
		pixmap = NULL;

	if (xmask) {
		mask = gdk_pixmap_new (GDK_ROOT_PARENT (),
				       im->rgb_width,
				       im->rgb_height,
				       1);
		mgc = gdk_gc_new (mask);
		XCopyArea (GDK_DISPLAY (),
			   xmask,
			   ((GdkWindowPrivate *) mask)->xwindow,
			   ((GdkGCPrivate *) mgc)->xgc,
			   0, 0,
			   im->rgb_width, im->rgb_height,
			   0, 0);
		XFreePixmap (GDK_DISPLAY (), xmask);
		gdk_gc_destroy (mgc);
	} else
		mask = NULL;

	gdk_gc_destroy (gc);

	eb = gtk_event_box_new ();
	p = gtk_pixmap_new (pixmap, mask);
	gtk_container_add (GTK_CONTAINER (eb), p);
	gtk_widget_show (p);
	gtk_widget_show (eb);

	gtk_widget_pop_colormap ();
	gtk_widget_pop_visual ();

	return eb;
}
