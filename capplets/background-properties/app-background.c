/* app-background.c - Background configuration application.  */

#include <config.h>
#include <strings.h>
#include "capplet-widget.h"
#include "gnome-desktop.h"

extern void background_init(void);

gint
main (gint argc, char *argv[])
{
  
    gint v=0;
    GnomeClient *client = NULL;
    gint token = 0;
    gchar *new_argv[4];
    gchar *temp = strrchr(argv[0],'/');
    
    int i =0;
    //    while (!i)
    //      ;
    if (!temp)
      temp = argv [0];
    else
      temp++;
    if (strcmp (temp, "background-properties-init")== 0) {
      gnome_init ("background-properties", NULL, argc, argv, 0, NULL);
      background_imlib_init ();
      background_properties_init ();
      return 0;
    }
    if (!argv[1])
      return 1;

    argp_program_version = VERSION;
    bindtextdomain (PACKAGE, GNOMELOCALEDIR);
    textdomain (PACKAGE);
    
    gnome_capplet_init("background-properties", NULL, argc, argv, 0, NULL);
    
    /* setup session management */
    client = gnome_master_client ();
    gnome_client_set_restart_command (client, 1, argv);
    gnome_client_set_clone_command (client, 1, argv);
    
    /* this goes somewhere else eventually */
    background_imlib_init ();
    
    background_init();
    capplet_gtk_main();
    
    return v;
}
