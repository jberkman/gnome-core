/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* Copyright (C) 1998 Redhat Software Inc. 
 * Authors: Jonathan Blandford <jrb@redhat.com>
 */
#include "capplet-widget.h"
#include <config.h>
#include <X11/Xlib.h>
#include <assert.h>

#include <gdk/gdkx.h>

#include "gnome.h"

GtkWidget *capplet;

static void
e_try (void)
{
}

static void
e_revert (void)
{
}

static void
e_ok (void)
{
}

static void
e_read (void)
{
}

static void
e_setup (void)
{
  GtkWidget *vbox, *frame, *hbox, *table, *sep;
  
  capplet = capplet_widget_new();
  gtk_widget_show (capplet);  
  
  hbox = gtk_hbox_new (FALSE, GNOME_PAD_SMALL);
  gtk_widget_show (hbox);
  gtk_container_add (GTK_CONTAINER (capplet), hbox);
    
  frame = gtk_frame_new (_("Enlightenment"));
  gtk_widget_show (frame);
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);

  gtk_signal_connect (GTK_OBJECT (capplet), "try",
		      GTK_SIGNAL_FUNC (e_try), NULL);
  gtk_signal_connect (GTK_OBJECT (capplet), "revert",
		      GTK_SIGNAL_FUNC (e_revert), NULL);
  gtk_signal_connect (GTK_OBJECT (capplet), "ok",
		      GTK_SIGNAL_FUNC (e_ok), NULL);
}



void
main (int argc, char **argv)
{
  gnome_capplet_init("Enlightenment Configuration", NULL, argc, argv, 0, NULL);
  
  e_read();
  e_setup();
  capplet_gtk_main();
}
