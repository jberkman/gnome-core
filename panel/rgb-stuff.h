#ifndef _RGB_STUFF_H_
#define _RGB_STUFF_H_

#include <gdk/gdk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

/* combine rgba onto the dest (given the dest has something in it already) */
void combine_rgb_rgba(guchar *dest, int dx, int dy, int dw, int dh, int drs,
		      guchar *rgba, int rw, int rh, int rrs);
/* tile an rgb onto dest */
void tile_rgb(guchar *dest, int dw, int dh, int offx, int offy, int drs,
	      guchar *tile, int w, int h, int rowstride, int has_alpha);

/* just copied from pixbuf source */
GdkPixBuf *my_gdk_pixbuf_rgb_from_drawable(GdkWindow *window);

/* scale a w by h pixmap into a square of size 'size' */
void make_scale_affine(double affine[], int w, int h, int size);

void cutout_rgb(guchar *dest, int drs, guchar *src, int x, int y, int w, int h, int srs);
void place_rgb(guchar *dest, int drs, guchar *src, int x, int y, int w, int h, int srs);

#endif /* _RGB_STUFF_H_ */