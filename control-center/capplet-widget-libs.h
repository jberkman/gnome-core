/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */

#ifndef __CONTROL_PANEL_LIB_H__
#define __CONTROL_PANEL_LIB_H__
#include <gnome.h>

void capplet_corba_gtk_main (void);
void capplet_corba_gtk_main_quit (void);
void capplet_corba_state_changed (gint id, gboolean undoable);
guint32 get_xid (gint cid);
gint get_ccid (gint cid);
gint get_capid ();
void *capplet_widget_corba_init(const char *app_id,
                               const char *app_version,
                               int *argc, char **argv,
                               struct poptOption *options,
                               unsigned int flags,
                               poptContext *return_ctx);
gint get_new_id();
#endif
