#ifndef IMLIB_MISC_H
#define IMLIB_MISC_H

#include <Imlib.h>
#include <gtk/gtk.h>
#include <libgnome/gnome-defs.h>

BEGIN_GNOME_DECLS

extern ImlibData *imlib_data;

/* Inits a custom-configured imlib with the visual explicitly set to
 * the default root window visual.
 */
void background_imlib_init(void);

/* Returns an GtkEventBox with a GtkPixmap in it.  The pixmap is
 * inside an eventbox because we need it to have an X window.  It will
 * the use the visual and colormap that the global imlib_data carry
 * (i.e. the default visual and colormap).
 */
#define MONITOR_CONTENTS_X 20
#define MONITOR_CONTENTS_Y 10
#define MONITOR_CONTENTS_WIDTH 157
#define MONITOR_CONTENTS_HEIGHT 111

GtkWidget *get_monitor_preview_widget (void);

END_GNOME_DECLS
#endif
