/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */

#ifndef __CONTROL_PANEL_LIB_H__
#define __CONTROL_PANEL_LIB_H__
#include <gnome.h>

void capplet_corba_gtk_main (void);
void capplet_corba_gtk_main_quit (void);
void capplet_corba_state_changed (gint id, gboolean undoable);
guint32 get_xid (gint cid);
gint get_ccid (gint cid);
gint capplet_widget_corba_init(char *app_id,
                               struct argp *app_parser,
                               int *argc, char **argv,
                               unsigned int flags,
                               int *arg_index);
gint get_new_id();
#endif
