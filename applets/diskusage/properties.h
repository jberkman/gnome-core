#ifndef PROPERTIES_H__
#define PROPERTIES_H__

#include <applet-widget.h>

typedef struct 
{
	gchar *ucolor;	/* color for used space */
	gchar *fcolor;	/* color for free space */
	guint speed, height, width;
	gboolean look;
} diskusage_properties;

void properties(AppletWidget *applet, gpointer data);
void load_properties( char *path, diskusage_properties *prop );
void save_properties( char *path, diskusage_properties *prop );

#endif
