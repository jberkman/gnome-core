#ifndef APPLET_CMDS_H
#define APPLET_CMDS_H

#include "panel_cmds.h"
#include "panel-widget.h"
#include "panel.h"

BEGIN_GNOME_DECLS


typedef enum {
	APPLET_CMD_QUERY,
	APPLET_CMD_INIT_MODULE,
	APPLET_CMD_DESTROY_MODULE,
	APPLET_CMD_GET_DEFAULT_PARAMS,
	APPLET_CMD_CREATE_INSTANCE,
	APPLET_CMD_GET_INSTANCE_PARAMS,
	APPLET_CMD_ORIENTATION_CHANGE_NOTIFY,
	APPLET_CMD_PROPERTIES
} AppletCommandType;

typedef struct {
	AppletCommandType cmd;

	PanelWidget  *panel;
	GtkWidget    *applet;

	union {
		/* Init module parameters */
		struct {
			PanelCmdFunc cmd_func;
		} init_module;
		
		/* Create instance parameters */
		struct {
			char *params;
			int   pos;
			int   panel;
		} create_instance;

		/* Orientation change notify parameters */
		struct {
			PanelSnapped     snapped;
			PanelOrientation orient;
		} orientation_change_notify;
	} params;
} AppletCommand;

typedef gpointer (*AppletCmdFunc) (AppletCommand *cmd);


END_GNOME_DECLS

#endif
