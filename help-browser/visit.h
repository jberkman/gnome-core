#ifndef _GNOME_HELP_VISIT_H_
#define _GNOME_HELP_VISIT_H_

#include <glib.h>

#include "window.h"

gint visitURL( HelpWindow win, gchar *ref );
gint visitURL_nohistory( HelpWindow win, gchar *ref );

#endif
