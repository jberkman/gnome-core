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
gchar     e_opt_sound = 1;
gchar     e_opt_slide_cleanup = 1;
gchar     e_opt_slide_map = 1;
gchar     e_opt_slide_desk = 1;
gchar     e_opt_hq_background = 1;
gchar     e_opt_autosave = 1;

gint      e_opt_focus = 1;
gint      e_opt_move = 0;
gint      e_opt_resize = 2;
gint      e_opt_slide_mode = 0;

gchar     e_opt_tooltips = 1;
gfloat    e_opt_tooltiptime = 1.5;

gfloat    e_opt_button_move_resistance = 5.0;
gfloat    e_opt_shade_speed = 4000.0;

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

void
e_cb_multi(GtkWidget *widget, gpointer data)
{
  gint *value;
  gint val;
  
  val = (gint)gtk_object_get_data(GTK_OBJECT(widget), "value");
  value = (gint *)data;
  *value = val;
}

void
e_cb_onoff(GtkWidget *widget, gpointer data)
{
  gchar *value;
  
  value = (gchar *)data;
  if (GTK_TOGGLE_BUTTON(widget)->active)
    *value = 1;
  else
    *value = 0;
}

void
e_cb_range(GtkWidget *widget, gpointer data)
{
  GtkWidget          *w;
  GtkAdjustment      *adj;
  gfloat             *value;
  
  w = GTK_WIDGET(data);
  adj = gtk_object_get_data(GTK_OBJECT(w), "adj");
  value = (gfloat *)gtk_object_get_data(GTK_OBJECT(w), "value");
  *value = adj->value;
  
  widget = NULL;
}

void
e_cb_rangeonoff_toggle(GtkWidget *widget, gpointer data)
{
  GtkWidget          *w, *ww;
  GtkAdjustment      *adj;
  gfloat             *value, offvalue;
  gchar              *onoff;
  
  w = GTK_WIDGET(data);
  adj = gtk_object_get_data(GTK_OBJECT(w), "adj");
  value = (gfloat *)gtk_object_get_data(GTK_OBJECT(w), "value");
  onoff = (gchar *)gtk_object_get_data(GTK_OBJECT(w), "onoff");
  offvalue = *((gfloat *)gtk_object_get_data(GTK_OBJECT(w), "offvalue"));
  *value = adj->value;
  
  if (onoff)
    {
      if (GTK_TOGGLE_BUTTON(widget)->active)
	{
	  ww = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(w), "l1");
	  gtk_widget_set_sensitive(ww, TRUE);
	  ww = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(w), "l2");
	  gtk_widget_set_sensitive(ww, TRUE);
	  gtk_widget_set_sensitive(w, TRUE);
	  *onoff = 1;
	}
      else
	{
	  ww = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(w), "l1");	
	  gtk_widget_set_sensitive(ww, FALSE);
	  ww = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(w), "l2");
	  gtk_widget_set_sensitive(ww, FALSE);
	  gtk_widget_set_sensitive(w, FALSE);
	  *onoff = 0;
	}
    }
  else
    {    
      if (GTK_TOGGLE_BUTTON(widget)->active)
	{
	  ww = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(w), "l1");
	  gtk_widget_set_sensitive(ww, TRUE);
	  ww = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(w), "l2");
	  gtk_widget_set_sensitive(ww, TRUE);
	  gtk_widget_set_sensitive(w, TRUE);
	  *value = 0.0;
	}
      else
	{
	  ww = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(w), "l1");
	  gtk_widget_set_sensitive(ww, FALSE);
	  ww = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(w), "l2");
	  gtk_widget_set_sensitive(ww, FALSE);
	  gtk_widget_set_sensitive(w, FALSE);
	  *value = offvalue;
	}
    }
  widget = NULL;
}

void
e_add_range_to_frame(GtkWidget *w, gchar *text, gfloat *value, gfloat lower,
		     gfloat upper, gchar *lower_text, gchar *upper_text)
{
  GtkObject *adj;
  GtkWidget *frame, *hscale, *hbox, *label, *align, *table, *align2;
  gint rows;
  
  if (!w)
    return;
  if (!text)
    return;
  table = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(w), "table");
  if (!table)
    return;
  rows = (gint)gtk_object_get_data(GTK_OBJECT(w), "rows");
  
  frame = gtk_frame_new(text);
  gtk_widget_show(frame);
  align = gtk_alignment_new(0.5, 0.5, 1.0, 0.0);
  gtk_widget_show(align);
  gtk_container_add(GTK_CONTAINER(align), frame);
  gtk_table_attach(GTK_TABLE(table), align, 0, 10, rows, rows + 1,
		   GTK_EXPAND | GTK_FILL,
		   GTK_EXPAND | GTK_FILL, 0, 0);
  
  align = gtk_alignment_new(0.5, 0.5, 1.0, 0.0);
  gtk_widget_show(align);
  gtk_container_add(GTK_CONTAINER(frame), align);
  hbox = gtk_hbox_new(FALSE, 2);
  gtk_widget_show(hbox);
  gtk_container_add(GTK_CONTAINER(align), hbox);
  
  label = gtk_label_new(lower_text);
  gtk_widget_show(label);
  align2 = gtk_alignment_new(0.5, 1.0, 0.0, 0.0);
  gtk_widget_show(align2);
  gtk_container_add(GTK_CONTAINER(align2), label);
  gtk_box_pack_start(GTK_BOX(hbox), align2, FALSE, FALSE, 0);
  adj = gtk_adjustment_new(*value, lower, upper, 0.1, .1, 0.1);
  hscale = gtk_hscale_new(GTK_ADJUSTMENT(adj));
  gtk_widget_show(hscale);
  gtk_signal_connect(GTK_OBJECT(adj), "value_changed",
		     GTK_SIGNAL_FUNC(e_cb_range), (gpointer)hscale);
  gtk_object_set_data(GTK_OBJECT(hscale), "adj", (gpointer)adj);
  gtk_object_set_data(GTK_OBJECT(hscale), "value", (gpointer)value);  
  gtk_box_pack_start(GTK_BOX(hbox), hscale, TRUE, TRUE, 0);
  label = gtk_label_new(upper_text);
  gtk_widget_show(label);
  align2 = gtk_alignment_new(0.5, 1.0, 0.0, 0.0);
  gtk_widget_show(align2);
  gtk_container_add(GTK_CONTAINER(align2), label);
  gtk_box_pack_start(GTK_BOX(hbox), align2, FALSE, FALSE, 0);
  
  gtk_object_set_data(GTK_OBJECT(w), "rows", (gpointer)(rows + 1));
}

void
e_add_rangeonoff_to_frame(GtkWidget *w, gchar *text, gfloat *value, 
			  gfloat lower, gfloat upper, gchar *lower_text, 
			  gchar *upper_text, gchar *onoff, gfloat offvalue)
{
  GtkObject *adj;
  GtkWidget *frame, *hscale, *hbox, *label, *align, *table, *align2, *check;
  gint rows;
  gfloat *offval;
  
  if (!w)
    return;
  if (!text)
    return;
  table = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(w), "table");
  if (!table)
    return;
  rows = (gint)gtk_object_get_data(GTK_OBJECT(w), "rows");
  
  frame = gtk_frame_new(text);
  gtk_widget_show(frame);
  align = gtk_alignment_new(0.5, 0.5, 1.0, 0.0);
  gtk_widget_show(align);
  gtk_container_add(GTK_CONTAINER(align), frame);
  gtk_table_attach(GTK_TABLE(table), align, 0, 10, rows, rows + 1,
		   GTK_EXPAND | GTK_FILL,
		   GTK_EXPAND | GTK_FILL, 0, 0);
  
  align = gtk_alignment_new(0.5, 0.5, 1.0, 0.0);
  gtk_widget_show(align);
  gtk_container_add(GTK_CONTAINER(frame), align);
  hbox = gtk_hbox_new(FALSE, 2);
  gtk_widget_show(hbox);
  gtk_container_add(GTK_CONTAINER(align), hbox);
  
  check = gtk_check_button_new();
  gtk_widget_show(check);
  align2 = gtk_alignment_new(1.0, 1.0, 0.0, 0.0);
  gtk_widget_show(align2);
  gtk_container_add(GTK_CONTAINER(align2), check);
  gtk_box_pack_start(GTK_BOX(hbox), align2, FALSE, FALSE, 0);

  if (onoff)
    {
      if (*onoff)
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(check), 1);
      else
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(check), 0);
    }
  else
    {
      if (*value == offvalue)
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(check), 0);
      else
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(check), 1);
    }
  
  adj = gtk_adjustment_new(*value, lower, upper, 0.1, .1, 0.1);
  hscale = gtk_hscale_new(GTK_ADJUSTMENT(adj));
  gtk_widget_show(hscale);
  
  gtk_signal_connect(GTK_OBJECT(check), "toggled",
		     GTK_SIGNAL_FUNC(e_cb_rangeonoff_toggle), (gpointer)hscale);
  
  label = gtk_label_new(lower_text);
  gtk_widget_show(label);
  align2 = gtk_alignment_new(0.5, 1.0, 0.0, 0.0);
  gtk_widget_show(align2);
  gtk_container_add(GTK_CONTAINER(align2), label);
  gtk_box_pack_start(GTK_BOX(hbox), align2, FALSE, FALSE, 0);
  gtk_signal_connect(GTK_OBJECT(adj), "value_changed",
		     GTK_SIGNAL_FUNC(e_cb_range), (gpointer)hscale);
  gtk_object_set_data(GTK_OBJECT(hscale), "adj", (gpointer)adj);
  gtk_object_set_data(GTK_OBJECT(hscale), "value", (gpointer)value);  
  gtk_object_set_data(GTK_OBJECT(hscale), "onoff", (gpointer)onoff);
  gtk_object_set_data(GTK_OBJECT(hscale), "l1", (gpointer)label);
  offval = g_malloc(sizeof(gfloat));
  *offval = offvalue;
  gtk_object_set_data(GTK_OBJECT(hscale), "offvalue", (gpointer)offval);
  
  gtk_box_pack_start(GTK_BOX(hbox), hscale, TRUE, TRUE, 0);
  label = gtk_label_new(upper_text);
  gtk_object_set_data(GTK_OBJECT(hscale), "l2", (gpointer)label);
  gtk_widget_show(label);
  align2 = gtk_alignment_new(0.5, 1.0, 0.0, 0.0);
  gtk_widget_show(align2);
  gtk_container_add(GTK_CONTAINER(align2), label);
  gtk_box_pack_start(GTK_BOX(hbox), align2, FALSE, FALSE, 0);
  
  gtk_object_set_data(GTK_OBJECT(w), "rows", (gpointer)(rows + 1));
}

void
e_add_multi_to_frame(GtkWidget *w, gchar *text, gchar *opts, gint *value)
{
  GtkWidget *table, *label, *om, *m, *mi, *align;
  gint i, j, rows;
  gchar s[1024], tmpbuf[2];
  
  if (!w)
    return;
  if (!text)
    return;
  table = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(w), "table");
  if (!table)
    return;
  rows = (gint)gtk_object_get_data(GTK_OBJECT(w), "rows");
  
  label = gtk_label_new(text);
  gtk_widget_show(label);
  align = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
  gtk_widget_show(align);
  gtk_container_add(GTK_CONTAINER(align), label);
  gtk_table_attach_defaults(GTK_TABLE(table), align, 0, 1, rows, rows + 1);
  
  om = gtk_option_menu_new();
  align = gtk_alignment_new(1.0, 0.5, 0.0, 0.0);
  gtk_widget_show(align);
  gtk_container_add(GTK_CONTAINER(align), om);
  gtk_table_attach(GTK_TABLE(table), align, 9, 10, rows, rows + 1,
		   GTK_EXPAND | GTK_FILL,
		   GTK_EXPAND | GTK_FILL, 0, 0);
  
  m = gtk_menu_new();
  gtk_widget_show(m);
  
  i = 0;
  j = 0;
  tmpbuf[1] = 0;
  while (opts[j])
    {
      s[0] = 0;
      if (opts[j] == '\n')
	j++;
      while ((opts[j]) && (opts[j] != '\n'))
	{
	  tmpbuf[0] = opts[j];
	  strcat(s, tmpbuf);
	  j++;
	}
      mi = gtk_menu_item_new_with_label(s);
      gtk_object_set_data(GTK_OBJECT(mi), "value", (gpointer)i);
      gtk_signal_connect(GTK_OBJECT(mi),
			 "activate", GTK_SIGNAL_FUNC(e_cb_multi),
			 (gpointer)value);
      gtk_menu_append(GTK_MENU(m), mi);
      gtk_widget_show(mi);
      i++;
    }

  gtk_option_menu_set_menu(GTK_OPTION_MENU(om), m);
    
  gtk_option_menu_set_history(GTK_OPTION_MENU(om), *value);
  gtk_widget_show (om);
  
  gtk_object_set_data(GTK_OBJECT(w), "rows", (gpointer)(rows + 1));
}

void
e_add_onoff_to_frame(GtkWidget *w, gchar *text, gchar *value)
{
  GtkWidget *table, *label, *check, *align;
  gint rows;
  
  if (!w)
    return;
  if (!text)
    return;
  table = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(w), "table");
  if (!table)
    return;
  rows = (gint)gtk_object_get_data(GTK_OBJECT(w), "rows");
  
  label = gtk_label_new(text);
  gtk_widget_show(label);
  align = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
  gtk_widget_show(align);
  gtk_container_add(GTK_CONTAINER(align), label);
  gtk_table_attach_defaults(GTK_TABLE(table), align, 0, 1, rows, rows + 1);
  
  check = gtk_check_button_new();
  gtk_widget_show(check);
  align = gtk_alignment_new(1.0, 0.5, 0.0, 0.0);
  gtk_widget_show(align);
  gtk_container_add(GTK_CONTAINER(align), check);
  gtk_table_attach_defaults(GTK_TABLE(table), align, 9, 10, rows, rows + 1);

  if (*value)
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(check), 1);
  else
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(check), 0);

  gtk_signal_connect(GTK_OBJECT(check), "toggled",
		     GTK_SIGNAL_FUNC(e_cb_onoff), (gpointer)value);
  
  gtk_object_set_data(GTK_OBJECT(w), "rows", (gpointer)(rows + 1));
}

void
e_add_widget_to_frame(GtkWidget *w, GtkWidget *child)
{
  GtkWidget *table, *align;
  gint rows;
  
  if (!w)
    return;
  if (!child)
    return;
  table = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(w), "table");
  if (!table)
    return;
  rows = (gint)gtk_object_get_data(GTK_OBJECT(w), "rows");
  
  align = gtk_alignment_new(0.5, 0.5, 1.0, 1.0);
  gtk_widget_show(align);
  gtk_container_add(GTK_CONTAINER(align), child);
  gtk_table_attach(GTK_TABLE(table), align, 0, 10, rows, rows + 1,
		   GTK_EXPAND | GTK_FILL,
		   GTK_EXPAND | GTK_FILL, 0, 0);
  
  gtk_object_set_data(GTK_OBJECT(w), "rows", (gpointer)(rows + 1));
}

GtkWidget *
e_create_frame(gchar *title)
{
  GtkWidget *frame, *table, *align;
  
  frame = gtk_frame_new(title);
  gtk_widget_show(frame);
  
  align = gtk_alignment_new(0.0, 0.0, 1.0, 0.0);
  gtk_widget_show(align);
  gtk_container_add(GTK_CONTAINER(frame), align);
  
  table = gtk_table_new(1, 1, FALSE);
  gtk_widget_show(table);
  gtk_container_border_width(GTK_CONTAINER(table), GNOME_PAD_SMALL);
  gtk_container_add(GTK_CONTAINER(align), table);
  
  gtk_object_set_data(GTK_OBJECT(frame), "table", (gpointer)table);
  gtk_object_set_data(GTK_OBJECT(frame), "rows", (gpointer)0);
  return frame;    
} 

static void
e_setup (void)
{
  GtkWidget *frame, *frame2, *hbox;
  
  capplet = capplet_widget_new();
  gtk_widget_show (capplet);  
  
  hbox = gtk_hbox_new (FALSE, GNOME_PAD_SMALL);
  gtk_widget_show (hbox);
  gtk_container_add (GTK_CONTAINER (capplet), hbox);
    
  frame = e_create_frame (_("Options"));
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);
  
  frame2 = e_create_frame (_("Window Display"));
  e_add_widget_to_frame(frame, frame2);
  e_add_multi_to_frame(frame2, _("Focus style"), 
		       _("Pointer\nSloppy\nClick to focus"), &e_opt_focus);
  e_add_multi_to_frame(frame2, _("Move mode"), 
		       _("Opaque\nLines\nBox\nShaded\nSemi-solid"), &e_opt_move);
  e_add_multi_to_frame(frame2, _("Resize mode"), 
		       _("Opaque\nLines\nBox\nShaded\nSemi-solid"), &e_opt_resize);

  frame2 = e_create_frame (_("Miscellaneous"));
  e_add_widget_to_frame(frame, frame2);
  e_add_range_to_frame(frame2, _("Button move resistance (pixels)"), 
		       &e_opt_button_move_resistance,
		       1.0, 20.0, _("Less"), _("More"));
  e_add_rangeonoff_to_frame(frame2, 
			    _("Tooltip timeout (sec)"),
			    &e_opt_tooltiptime,
			    0.0, 10.0, 
			    _("Shorter"), _("Longer"), 
			    &e_opt_tooltips, 0.0);
  e_add_rangeonoff_to_frame(frame2, 
			    _("Shading speed (pixels / sec)"),
			    &e_opt_shade_speed,
			    1.0, 20000.0, 
			    _("Slower"), _("Faster"), 
			    NULL, 99999.0);
  e_add_onoff_to_frame(frame2, _("Autosave"), &e_opt_autosave);
  
  frame = e_create_frame (_("Effects"));
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);
  

  frame2 = e_create_frame (_("Sliding"));
  e_add_widget_to_frame(frame, frame2);
  e_add_multi_to_frame(frame2, _("Slide mode"), 
		       _("Opaque\nLines\nBox\nShaded\nSemi-solid"), &e_opt_slide_mode);
  e_add_onoff_to_frame(frame2, _("Windows slide on cleanup"), &e_opt_slide_cleanup);
  e_add_onoff_to_frame(frame2, _("Windows slide on map"), &e_opt_slide_map);
  e_add_onoff_to_frame(frame2, _("Desktops slide on change"), &e_opt_slide_desk);

  frame2 = e_create_frame (_("Miscellaneous"));
  e_add_widget_to_frame(frame, frame2);
  e_add_onoff_to_frame(frame2, _("Sound Effects"), &e_opt_sound);
  e_add_onoff_to_frame(frame2, _("Dither backgrounds"), &e_opt_hq_background);

  
  gtk_signal_connect (GTK_OBJECT (capplet), "try",
		      GTK_SIGNAL_FUNC (e_try), NULL);
  gtk_signal_connect (GTK_OBJECT (capplet), "revert",
		      GTK_SIGNAL_FUNC (e_revert), NULL);
  gtk_signal_connect (GTK_OBJECT (capplet), "ok",
		      GTK_SIGNAL_FUNC (e_ok), NULL);
}



int
main (int argc, char **argv)
{
  gnome_capplet_init("Enlightenment Configuration", NULL, argc, argv, 0, NULL);
  
  e_read();
  e_setup();
  capplet_gtk_main();
  
  return 0;
}
