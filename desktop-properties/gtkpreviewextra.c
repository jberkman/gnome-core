#include <string.h>
#include <gtk/gtk.h>
#include "gnome.h"
#include "gtkpreviewextra.h"

/* FIXME FIXME FIXME FIXME PLEASE FOR YOUR LIFE
 *
 * This file is extremely ugly.  It copies an important part of
 * gtkpreview.c to add a single function that is missing in Gtk and
 * that would be nice to have.  We don't want to change the GTK API
 * yet, so this is why this mess is here.
 */

#define IMAGE_SIZE            256
#define COLOR_COMPOSE(r,g,b)  (lookup_red[r] | lookup_green[g] | lookup_blue[b])

typedef void (*GtkTransferFunc) (guchar *dest, guchar *src, gint count);

static GdkImage *preview_class_image;

static void
gtk_lsbmsb_1_1 (guchar *dest,
		guchar *src,
		gint    count)
{
  memcpy (dest, src, count);
}

static void
gtk_lsb_2_2 (guchar *dest,
	     guchar *src,
	     gint    count)
{
  memcpy (dest, src, count * 2);
}

static void
gtk_msb_2_2 (guchar *dest,
	     guchar *src,
	     gint    count)
{
  while (count--)
    {
      dest[0] = src[1];
      dest[1] = src[0];
      dest += 2;
      src += 2;
    }
}

static void
gtk_lsb_3_3 (guchar *dest,
	     guchar *src,
	     gint    count)
{
  memcpy (dest, src, count * 3);
}

static void
gtk_msb_3_3 (guchar *dest,
	     guchar *src,
	     gint    count)
{
  while (count--)
    {
      dest[0] = src[2];
      dest[1] = src[1];
      dest[2] = src[0];
      dest += 3;
      src += 3;
    }
}

static void
gtk_lsb_3_4 (guchar *dest,
	     guchar *src,
	     gint    count)
{
  while (count--)
    {
      dest[0] = src[0];
      dest[1] = src[1];
      dest[2] = src[2];
      dest += 4;
      src += 3;
    }
}

static void
gtk_msb_3_4 (guchar *dest,
	     guchar *src,
	     gint    count)
{
  while (count--)
    {
      dest[1] = src[2];
      dest[2] = src[1];
      dest[3] = src[0];
      dest += 4;
      src += 3;
    }
}

void
gtk_preview_put_with_offsets (GtkPreview   *preview,
			      GdkWindow    *window,
			      GdkGC        *gc,
			      gint          srcx,
			      gint          srcy,
			      gint          destx,
			      gint          desty,
			      gint          width,
			      gint          height)
{
  GtkWidget *widget;
  GdkImage *image;
  gint w, h;
  GdkRectangle origin, rt;
  GdkRectangle r1, r2, r3;
  GtkTransferFunc transfer_func;
  guchar *image_mem;
  guchar *src, *dest;
  gint x, xe, x2;
  gint y, ye, y2;
  guint dest_rowstride;
  guint src_bpp;
  guint dest_bpp;
  gint i;

  g_return_if_fail (preview != NULL);
  g_return_if_fail (GTK_IS_PREVIEW (preview));
  g_return_if_fail (window != NULL);

  if (!preview->buffer)
    return;

  widget = GTK_WIDGET (preview);

  if ((srcx < 0) || (srcy < 0) ||
      (srcx >= preview->buffer_width) || (srcy >= preview->buffer_height))
    return;

  origin.x = 0;
  origin.y = 0;
  origin.width = preview->buffer_width;
  origin.height = preview->buffer_height;

  r1.x = srcx;
  r1.y = srcy;
  r1.width = width;
  r1.height = height;

  if (!gdk_rectangle_intersect(&origin, &r1, &rt))
    return;

  rt.x += destx;
  rt.y += desty;

  gdk_window_get_size(window, &w, &h);

  r2.x = 0;
  r2.y = 0;
  r2.width = w;
  r2.height = h;

  if (!gdk_rectangle_intersect(&rt, &r2, &r3))
    return;

  x2 = r3.x + r3.width;
  y2 = r3.y + r3.height;

  if (!preview_class_image)
    preview_class_image = gdk_image_new (GDK_IMAGE_FASTEST,
					 gtk_preview_get_visual(),
					 IMAGE_SIZE, IMAGE_SIZE);
  image = preview_class_image;
  src_bpp = gtk_preview_get_info()->bpp;

  image_mem = image->mem;
  dest_bpp = image->bpp;
  dest_rowstride = image->bpl;

  transfer_func = NULL;

  switch (dest_bpp)
    {
    case 1:
      switch (src_bpp)
	{
	case 1:
	  transfer_func = gtk_lsbmsb_1_1;
	  break;
	}
      break;
    case 2:
      switch (src_bpp)
	{
	case 2:
	  if (image->byte_order == GDK_MSB_FIRST)
	    transfer_func = gtk_msb_2_2;
	  else
	    transfer_func = gtk_lsb_2_2;
	  break;
	case 3:
	  break;
	}
      break;
    case 3:
      switch (src_bpp)
	{
	case 3:
	  if (image->byte_order == GDK_MSB_FIRST)
	    transfer_func = gtk_msb_3_3;
	  else
	    transfer_func = gtk_lsb_3_3;
	  break;
	}
      break;
    case 4:
      switch (src_bpp)
	{
	case 3:
	  if (image->byte_order == GDK_MSB_FIRST)
	    transfer_func = gtk_msb_3_4;
	  else
	    transfer_func = gtk_lsb_3_4;
	  break;
	}
      break;
    }

  if (!transfer_func)
    {
      g_warning ("unsupported byte order/src bpp/dest bpp combination: %s:%d:%d",
		 (image->byte_order == GDK_MSB_FIRST) ? "msb" : "lsb", src_bpp, dest_bpp);
      return;
    }

  for (y = r3.y; y < y2; y += IMAGE_SIZE)
    {
      for (x = r3.x; x < x2; x += IMAGE_SIZE)
	{
	  xe = x + IMAGE_SIZE;
	  if (xe > x2)
	    xe = x2;

	  ye = y + IMAGE_SIZE;
	  if (ye > y2)
	    ye = y2;

	  for (i = y; i < ye; i++)
	    {
	      src = preview->buffer + (((gulong) (i - r3.y + r1.y) * (gulong) preview->buffer_width) +
				       (x - r3.x + r1.x)) * (gulong) src_bpp;
	      dest = image_mem + ((gulong) (i - y) * dest_rowstride);

	      if (xe > x)
		(* transfer_func) (dest, src, xe - x);
	    }

	  gdk_draw_image (window, gc,
			  image, 0, 0, x, y,
			  xe - x, ye - y);
	  gdk_flush ();
	}
    }
}
