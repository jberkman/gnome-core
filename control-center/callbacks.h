/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */

/* Copyright (C) 1998 Redhat Software Inc. 
 * Authors: Jonathan Blandford <jrb@redhat.com>
 */
#ifndef __CALLBACKS_H__
#define __CALLBACKS_H__

#include <gnome.h>

#ifdef __cplusplus
extern "C" {
#pragma }
#endif /* __cplusplus */

/* menubar callbacks */
void exit_row_callback(GtkWidget *widget, gint row, gint, GdkEventButton *, gpointer);
void exit_dialog_ok_callback(GtkWidget *widget, gpointer data);
void exit_dialog_cancel_callback(GtkWidget *widget, gpointer data);
void exit_callback(GtkWidget *widget, gpointer data);
void help_callback(GtkWidget *widget, gpointer data);
void about_callback(GtkWidget *widget, gpointer data);
void try_button_callback(GtkWidget *widget, gpointer data);
void revert_button_callback(GtkWidget *widget, gpointer data);
void ok_button_callback(GtkWidget *widget, gpointer data);
void cancel_button_callback(GtkWidget *widget, gpointer data);
void help_button_callback(GtkWidget *widget, gpointer data);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

