/* Gnome panel: Initialization routines
 * (C) 1997 the Free Software Foundation
 *
 * Authors: Federico Mena
 *          Miguel de Icaza
 */

#include <string.h>
#include "gnome.h"
#include "applet_files.h"
#include "panel_cmds.h"
#include "applet_cmds.h"
#include "panel.h"


static void
load_applet(char *id, char *params, int xpos, int ypos)
{
	PanelCommand  cmd;

	cmd.cmd = PANEL_CMD_CREATE_APPLET;
	cmd.params.create_applet.id     = id;
	cmd.params.create_applet.params = params;
	cmd.params.create_applet.xpos   = xpos;
	cmd.params.create_applet.ypos   = ypos;

	panel_command(&cmd);
}


static void
load_default_applets(void)
{
	/* XXX: the IDs for these applets are hardcoded here. */

	/* Here we use NULL to request querying of default applet parameters */

#if 0
	load_applet("Menu", NULL, PANEL_UNKNOWN_APPLET_POSITION, PANEL_UNKNOWN_APPLET_POSITION);
	load_applet("Mail check", NULL, PANEL_UNKNOWN_APPLET_POSITION, PANEL_UNKNOWN_APPLET_POSITION);
	load_applet("Clock", NULL, PANEL_UNKNOWN_APPLET_POSITION, PANEL_UNKNOWN_APPLET_POSITION);
#endif
	/* FIXME: we are not using the code above because automatic
	 * positioning of applets is not working right now.  So we use
	 * explicit coordinates :-(
	 */

	/*we set the x value rediculously high for right align*/
	load_applet("Menu", NULL, 0, 0);
	load_applet("Clock", NULL, 0, 0);
	load_applet("Mail check", NULL, 9999, 0);
}


static void
init_user_applets(void)
{
	void *iterator;
	char *key;
	char *realkey;
	char *value;
	char *applet_name;
	char *applet_params;
	int   xpos, ypos;

	iterator = gnome_config_init_iterator("/panel/Applets");

	if (!iterator)
		load_default_applets();
	
	while (iterator) {
		iterator = gnome_config_iterator_next(iterator, &key, &value);
		realkey = strchr(key, ',') + 1; /* Skip over number-for-unique-keys hack and go to applet id */
		applet_params = strchr(realkey, ','); /* Everything after first comma is parameters to the applet */

		if (applet_params)
			*applet_params++ = '\0'; /* Terminate string at comma and skip over it */

		/* If applet_params is NULL, then the applet will be queried for default params */

		applet_name = realkey;

		if (sscanf(value, "%d%d", &xpos, &ypos) != 2) {
			fprintf(stderr,
				"init_user_applets: using unknown applet position for \"%s\"\n",
				applet_name);

			xpos = ypos = PANEL_UNKNOWN_APPLET_POSITION;
		}

		load_applet(applet_name, applet_params, xpos, ypos);
		
		g_free (key);
		g_free (value);
	}
}


/* FIXME: session management not complete.  In particular, we should:
   1. Actually save state in a useful way.
   2. Parse argv to get our session management key.  */
static void
init_session_management (int argc, char *argv[])
{
  char *previous_id = NULL;
  char *session_id;

  session_id = gnome_session_init (panel_session_save, NULL, NULL, NULL,
				   previous_id);
}

int
main(int argc, char **argv)
{
	gnome_init(&argc, &argv);

	init_session_management (argc, argv);

	applet_files_init();
	panel_init();
	panel_init_applet_modules();
	init_user_applets();

	gtk_widget_show(the_panel->window);

	gtk_main();
	return 0;
}
