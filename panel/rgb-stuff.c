/*
 * RGB code for drawing stuff on the panel
 *
 * Code taken from gdk-pixbuf, so parts of it are authored by:
 *   Cody Russell <bratsche@dfw.net>
 * Rest is:
 *   George Lebl <jirka@5z.com>
 */
#include <config.h>
#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <gmodule.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "rgb-stuff.h"

GdkPixBuf *
my_gdk_pixbuf_rgb_from_drawable (GdkWindow *window)
{
	GdkImage *image;
	ArtPixBuf *art_pixbuf;
	GdkColormap *colormap;
	art_u8 *buff;
	art_u8 *pixels;
	gulong pixel;
	gint rowstride;
	art_u8 r, g, b;
	gint xx, yy;
	int width, height;

	g_return_val_if_fail (window != NULL, NULL);
	
	gdk_window_get_size(window, &width, &height);

	image = gdk_image_get (window, 0, 0, width, height);
	colormap = gdk_rgb_get_cmap ();

	rowstride = width * 3;

	buff = art_alloc (rowstride * height);
	pixels = buff;

	switch (image->depth)
	{
	case 0:
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
	case 6:
	case 7:
	case 8:
		for (yy = 0; yy < height; yy++)
		{
			for (xx = 0; xx < width; xx++)
			{
				pixel = gdk_image_get_pixel (image, xx, yy);
				pixels[0] = colormap->colors[pixel].red;
				pixels[1] = colormap->colors[pixel].green;
				pixels[2] = colormap->colors[pixel].blue;
				pixels += 3;
           
			}
		}
		break;

	case 16:
	case 15:
		for (yy = 0; yy < height; yy++)
		{
			for (xx = 0; xx < width; xx++)
			{
				pixel = gdk_image_get_pixel (image, xx, yy);
				r =  (pixel >> 8) & 0xf8;
				g =  (pixel >> 3) & 0xfc;
				b =  (pixel << 3) & 0xf8;
				pixels[0] = r;
				pixels[1] = g;
				pixels[2] = b;
				pixels += 3;
			}
		}
		break;

	case 24:
	case 32:
		for (yy = 0; yy < height; yy++)
		{
			for (xx = 0; xx < width; xx++)
			{
				pixel = gdk_image_get_pixel (image, xx, yy);
				r =  (pixel >> 16) & 0xff;
				g =  (pixel >> 8)  & 0xff;
				b = pixel & 0xff;
				pixels[0] = r;
				pixels[1] = g;
				pixels[2] = b;
				pixels += 3;
			}
		}
		break;

	default:
		g_error ("art_pixbuf_from_drawable_core (): Unknown depth.");
	}

	art_pixbuf = art_pixbuf_new_rgb (buff, width, height, rowstride);

	return gdk_pixbuf_new(art_pixbuf, NULL);
}

void
combine_rgb_rgba(guchar *dest, int dx, int dy, int dw, int dh, int drs,
		 guchar *rgba, int rw, int rh, int rrs)
{
	int r, g, b, a;
	int i,j;
	guchar *imgrow;
	guchar *dstrow;

	for(j=0;j<dh;j++) {
		imgrow = rgba + j * rrs;
		dstrow = dest + (dy+j) * drs + (dx*3);
		for(i=0;i<dw;i++) {
			r = *(imgrow++);
			g = *(imgrow++);
			b = *(imgrow++);
			a = *(imgrow++);
			*dstrow = (r*a + *dstrow * (256-a))>>8;
			dstrow++;
			*dstrow = (g*a + *dstrow * (256-a))>>8;
			dstrow++;
			*dstrow = (b*a + *dstrow * (256-a))>>8;
			dstrow++;
		}
	}
}

void
tile_rgb(guchar *dest, int dw, int dh, int offx, int offy, int drs,
	 guchar *tile, int w, int h, int rowstride, int has_alpha)
{
	guchar *p;
	int i,j,x,y,a;
	guchar *imgrow;
	int off;

	p = dest;
	y = offy % h;
	off = drs - (dw*3); /*the space after width ends until next row*/
	for(j=0;j<dh;j++) {
		x = offx % w;
		imgrow = tile + y * rowstride + x * (has_alpha?4:3);
		for(i=0;i<dw;i++) {
			*(p++) = *(imgrow++);
			*(p++) = *(imgrow++);
			*(p++) = *(imgrow++);
			if(has_alpha) {
				a = *(imgrow++);
				if(a!=255) {
					guchar *pp = p-3;
					pp[0] = (pp[0]*a)>>8;
					pp[1] = (pp[1]*a)>>8;
					pp[2] = (pp[2]*a)>>8;
				}
			}
			x++;
			if(x>w) {
				x = 0;
				imgrow = tile + y * rowstride;
			}
		}
		p += off;
		y++;
		if(y>h)
			y = 0;
	}
}

void
make_scale_affine(double affine[], int w, int h, int size)
{
	int oh, ow;
	oh = h;
	ow = w;
	if(w>h) {
		h = h*((double)size/w);
		w = size;
	} else {
		w = w*((double)size/h);
		h = size;
	}
	w = w>0?w:1;
	h = h>0?h:1;

	affine[1] = affine[2] = affine[4] = affine[5] = 0;

	affine[0] = w / (double)(ow);
	affine[3] = h / (double)(oh);
}

void
cutout_rgb(guchar *dest, int drs, guchar *src, int x, int y, int w, int h, int srs)
{
	int j;
	guchar *srcrow;
	guchar *dstrow;

	for(j=0;j<h;j++) {
		srcrow = src + (j+y) * srs + (x*3);
		dstrow = dest + j * drs;
		memcpy(dstrow,srcrow,3*w);
	}
}

void
place_rgb(guchar *dest, int drs, guchar *src, int x, int y, int w, int h, int srs)
{
	int j;
	guchar *srcrow;
	guchar *dstrow;

	for(j=0;j<h;j++) {
		srcrow = src + j * srs;
		dstrow = dest + (j+y) * drs + (x*3);
		memcpy(dstrow,srcrow,3*w);
	}
}
