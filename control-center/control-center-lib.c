/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
#include "control-center-lib.h"
#include <orb/orbit.h>


void control_center_corba_gtk_main (char *str)
{
        gtk_main();
}
void control_center_corba_gtk_main_quit (void)
{
          CORBA_ORB_shutdown(orb, CORBA_FALSE, &ev);
          gtk_main_quit();

}

gboolean
control_center_init_corba ( gchar *ior)
{
        int n = 1;

        CORBA_exception_init(&ev);
        
        control_center = CORBA_ORB_string_to_object(orb, ior, &ev);
        return n;
}
