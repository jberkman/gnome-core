#ifndef GTK_PREVIEW_EXTRA_H
#define GTK_PREVIEW_EXTRA_H

BEGIN_GNOME_DECLS

void
gtk_preview_put_with_offsets (GtkPreview   *preview,
			      GdkWindow    *window,
			      GdkGC        *gc,
			      gint          srcx,
			      gint          srcy,
			      gint          destx,
			      gint          desty,
			      gint          width,
			      gint          height);


END_GNOME_DECLS

#endif
