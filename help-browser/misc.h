#ifndef _GNOME_HELP_MISC_H_
#define _GNOME_HELP_MISC_H_

#include <glib.h>

gchar *getOutputFrom(gchar *argv[], gchar *writePtr, gint writeBytesLeft);
gint  getOutputFromBin(gchar *argv[], gchar *writePtr, gint writeBytesLeft,
	         guchar **outbuf, gint *outbuflen);
void  map_spaces_to_underscores( gchar *str );
guchar *loadFileToBuf( gchar *file );
#endif
