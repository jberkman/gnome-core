/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */

#ifndef __CONTROL_PANEL_LIB_H__
#define __CONTROL_PANEL_LIB_H__
#include <gnome.h>

void control_center_corba_gtk_main (char *str);
void control_center_corba_gtk_main_quit (void);
gboolean control_center_init_corba (gchar *ior);
#endif
