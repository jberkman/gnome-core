#ifndef _GNOME_HELP_VISIT_H_
#define _GNOME_HELP_VISIT_H_

#include <glib.h>

#include "window.h"

void visitURL( HelpWindow win, gchar *ref );
void visitURL_nohistory( HelpWindow win, gchar *ref );

#endif
