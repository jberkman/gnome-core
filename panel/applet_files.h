#ifndef APPLET_FILE_H
#define APPLET_FILE_H

#include "gnome.h"
#include "libgnome/gnome-dl.h"
#include "applet_cmds.h"

BEGIN_GNOME_DECLS


typedef struct {
	GnomeLibHandle *dl_handle;
	char           *filename;
	AppletCmdFunc   cmd_func;
} AppletFile;


extern GHashTable *applet_files_ht;


void applet_files_init(void);
void applet_files_destroy(void);

AppletCmdFunc get_applet_cmd_func(char *id);


END_GNOME_DECLS

#endif
