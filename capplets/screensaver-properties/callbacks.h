/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* Copyright (C) 1998 Redhat Software Inc. 
 * Authors: Jonathan Blandford <jrb@redhat.com>
 */
#ifndef __CALLBACKS_H__
#define __CALLBACKS_H__
#include <gtk/gtk.h>
#include "parser.h"
void screensaver_load ();
GtkWidget *get_and_set_min_entry ();
GtkWidget *get_and_set_pword ();
GtkWidget *get_and_set_dpmsmin ();
GtkWidget *get_and_set_dpmscheck (GtkWidget *box);
GtkAdjustment *get_and_set_nice ();
GtkWidget *get_and_set_mode();
void launch_miniview (screensaver_data *sd);
void dialog_callback (GtkWidget *dialog, gint button, screensaver_data *sd);
void dialog_destroy_callback (GtkWidget *widget, screensaver_data *sd);
void list_expose_callback (GtkWidget *list, GdkEventButton *event, void *data);
void list_click_callback (GtkWidget *list, void *data);
void insert_text_callback (GtkEditable *editable, const gchar *text, gint length, gint *position, void *data);
void insert_text_callback2 (GtkEditable *editable, const gchar *text, gint length, gint *position, void *data);
void delete_text_callback (GtkEditable *editable, gint length, gint *position, void *data);
void dpms_callback (GtkWidget *check, void *data);
void password_callback (GtkWidget *password, void *data);
void password_callback (GtkWidget *password, void *data);
void nice_callback (GtkObject *adj, void *data);
void setup_callback (GtkWidget *widget, gpointer data);
void destroy_callback (GtkWidget *widget, void *data);
void ok_callback ();
void try_callback ();
void revert_callback ();
void sig_child(int sig);

#endif
