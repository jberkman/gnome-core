/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */

#ifndef __CONTROL_PANEL_LIB_H__
#define __CONTROL_PANEL_LIB_H__
#include <gnome.h>

void capplet_corba_gtk_main (void);
void capplet_corba_gtk_main_quit (void);
void capplet_widget_corba_init(gint *argc, char **argv, gchar *cc_ior, gint id);
gint get_new_id();
#endif
