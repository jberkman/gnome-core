/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 1998 Redhat Software Inc. 
 * Authors: Elliot Lee <sopwith@redhat.com>
 */
#include "capplet-widget.h"
#include <config.h>
#include <X11/Xlib.h>
#include <assert.h>

#include <gdk/gdkx.h>

#include "gnome.h"

/* The property configurator we're associated with.  */
static GnomePropertyConfigurator *config;

void
main (int argc, char **argv)
{
        gchar *temp = strrchr(argv[0],'/');
        gboolean init = FALSE;

        if (argv[1] && strcmp (argv[1], "--init")== 0) 
                init = TRUE;
        if (!temp)
                temp = argv [0];
        else
                temp++;
        if (strcmp (temp, "mouse-properties-init")== 0) 
                init = TRUE;
        if (init) {
                argv[1] = NULL;
                gnome_init ("sound-properties", NULL, 1, argv, 0, NULL);
                mouse_read ();
                mouse_apply ();
                return;
        }

        gnome_capplet_init ("sound-properties", NULL, argc, argv, 0, NULL);
        
        capplet_gtk_main ();
}
