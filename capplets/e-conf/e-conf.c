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

GtkWidget *current_capplet;
gchar     e_opt_sound = 1;
gchar     e_opt_slide_cleanup = 1;
gchar     e_opt_slide_map = 1;
gchar     e_opt_slide_desk = 1;
gchar     e_opt_hq_background = 1;

gint      e_opt_focus = 1;
gint      e_opt_move = 0;
gint      e_opt_resize = 2;
gint      e_opt_slide_mode = 0;

gchar     e_opt_tooltips = 1;
gfloat    e_opt_tooltiptime = 1.5;

gfloat    e_opt_shade_speed = 4000.0;
gfloat    e_opt_map_speed = 4000.0;
gfloat    e_opt_cleanup_speed = 4000.0;
gfloat    e_opt_desk_speed = 4000.0;

FILE *eesh = NULL;

static void
e_try (void)
{
  gchar cmd[4096];

  g_snprintf(cmd, sizeof(cmd), "eesh -e \"set_controls"
	     " SOUND: %i"
	     " CLEANUPSLIDE: %i"
	     " MAPSLIDE: %i"
	     " DESKSLIDEIN: %i"
	     " HIQUALITYBG: %i"
	     " FOCUSMODE: %i"
	     " MOVEMODE: %i"
	     " RESIZEMODE: %i"
	     " SLIDEMODE: %i"
	     " TOOLTIPS: %i"
	     " TIPTIME: %f"
	     " SHADESPEED: %i"
	     " SLIDESPEEDMAP: %i"
	     " SLIDESPEEDCLEANUP: %i"
	     " DESKSLIDESPEED: %i"
	     "\""
	     ,
	     e_opt_sound, e_opt_slide_cleanup, e_opt_slide_map, 
	     e_opt_slide_desk, e_opt_hq_background,
	     e_opt_focus, e_opt_move, e_opt_resize, e_opt_slide_mode,
	     e_opt_tooltips, e_opt_tooltiptime,
	     (gint)e_opt_shade_speed,
	     (gint)e_opt_map_speed,
	     (gint)e_opt_cleanup_speed,
	     (gint)e_opt_desk_speed
	     );
  system(cmd);
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
e_cancel (void)
{
}

static void
e_read (void)
{
  gchar cmd[4096];
  gchar buf[10240];

  if (!eesh)
    {
      eesh = popen("eesh -ewait \"get_controls\"", "r");
      if (!eesh)
	return;
    }
  while (fgets(buf, sizeof(buf), eesh))
    {
      sscanf(buf, "%4000s", cmd);
      if (!strcmp(cmd, "SOUND:"))
	{
	  sscanf(buf, "%*s %4000s", cmd);
	  e_opt_sound = atoi(cmd);
	}
      else if (!strcmp(cmd, "CLEANUPSLIDE:"))
	{
	  sscanf(buf, "%*s %4000s", cmd);
	  e_opt_slide_cleanup = atoi(cmd);
	}
      else if (!strcmp(cmd, "MAPSLIDE:"))
	{
	  sscanf(buf, "%*s %4000s", cmd);
	  e_opt_slide_map = atoi(cmd);
	}
      else if (!strcmp(cmd, "DESKSLIDEIN:"))
	{
	  sscanf(buf, "%*s %4000s", cmd);
	  e_opt_slide_desk = atoi(cmd);
	}
      else if (!strcmp(cmd, "HIQUALITYBG:"))
	{
	  sscanf(buf, "%*s %4000s", cmd);
	  e_opt_hq_background = atoi(cmd);
	}
      else if (!strcmp(cmd, "FOCUSMODE:"))
	{
	  sscanf(buf, "%*s %4000s", cmd);
	  e_opt_focus = atoi(cmd);
	}
      else if (!strcmp(cmd, "MOVEMODE:"))
	{
	  sscanf(buf, "%*s %4000s", cmd);
	  e_opt_move = atoi(cmd);
	}
      else if (!strcmp(cmd, "RESIZEMODE:"))
	{
	  sscanf(buf, "%*s %4000s", cmd);
	  e_opt_resize = atoi(cmd);
	}
      else if (!strcmp(cmd, "SLIDEMODE:"))
	{
	  sscanf(buf, "%*s %4000s", cmd);
	  e_opt_resize = atoi(cmd);
	}
      else if (!strcmp(cmd, "TOOLTIPS:"))
	{
	  sscanf(buf, "%*s %4000s", cmd);
	  e_opt_tooltips = atoi(cmd);
	}
      else if (!strcmp(cmd, "TIPTIME:"))
	{
	  sscanf(buf, "%*s %4000s", cmd);
	  e_opt_tooltiptime = atof(cmd);
	}
      else if (!strcmp(cmd, "SHADESPEED:"))
	{
	  sscanf(buf, "%*s %4000s", cmd);
	  e_opt_shade_speed = atof(cmd);
	}
      else if (!strcmp(cmd, "SLIDESPEEDMAP:"))
	{
	  sscanf(buf, "%*s %4000s", cmd);
	  e_opt_map_speed = atof(cmd);
	}
      else if (!strcmp(cmd, "SLIDESPEEDCLEANUP:"))
	{
	  sscanf(buf, "%*s %4000s", cmd);
	  e_opt_cleanup_speed = atof(cmd);
	}
      else if (!strcmp(cmd, "DESKSLIDESPEED:"))
	{
	  sscanf(buf, "%*s %4000s", cmd);
	  e_opt_desk_speed = atof(cmd);
	}
    }
  pclose(eesh);
  eesh = NULL;
}

void
e_cb_multi(GtkWidget *widget, gpointer data)
{
  GtkWidget *c;
  gint *value;
  gint val;
  
  c = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(widget), "capplet");
  capplet_widget_state_changed(CAPPLET_WIDGET(c), FALSE);
  val = (gint)gtk_object_get_data(GTK_OBJECT(widget), "value");
  value = (gint *)data;
  *value = val;
}

void
e_cb_onoff(GtkWidget *widget, gpointer data)
{
  GtkWidget *c;
  gchar *value;
  
  c = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(widget), "capplet");
  capplet_widget_state_changed(CAPPLET_WIDGET(c), FALSE);
  value = (gchar *)data;
  if (GTK_TOGGLE_BUTTON(widget)->active)
    *value = 1;
  else
    *value = 0;
}

void
e_cb_range(GtkWidget *widget, gpointer data)
{
  GtkWidget          *c;
  GtkWidget          *w;
  GtkAdjustment      *adj;
  gfloat             *value;
  
  w = GTK_WIDGET(data);
  c = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(w), "capplet");
  capplet_widget_state_changed(CAPPLET_WIDGET(c), FALSE);
  adj = gtk_object_get_data(GTK_OBJECT(w), "adj");
  value = (gfloat *)gtk_object_get_data(GTK_OBJECT(w), "value");
  *value = adj->value;
  
  widget = NULL;
}

void
e_cb_rangeonoff_toggle(GtkWidget *widget, gpointer data)
{
  GtkWidget          *c;
  GtkWidget          *w, *ww;
  GtkAdjustment      *adj;
  gfloat             *value, offvalue;
  gchar              *onoff;
  
  w = GTK_WIDGET(data);
  c = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(w), "capplet");
  capplet_widget_state_changed(CAPPLET_WIDGET(c), FALSE);
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
  gtk_object_set_data(GTK_OBJECT(hscale), "capplet", (gpointer)current_capplet);  
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
  gtk_object_set_data(GTK_OBJECT(hscale), "capplet", (gpointer)current_capplet);  
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
      gtk_object_set_data(GTK_OBJECT(mi), "capplet", (gpointer)current_capplet);  
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

  gtk_object_set_data(GTK_OBJECT(check), "capplet", (gpointer)current_capplet);  
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

void
e_add_space_to_frame(GtkWidget *w, gint size)
{
  GtkWidget *table, *align, *child;
  gint rows;
  
  if (!w)
    return;
  table = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(w), "table");
  if (!table)
    return;
  rows = (gint)gtk_object_get_data(GTK_OBJECT(w), "rows");
  
  align = gtk_alignment_new(0.5, 0.5, 1.0, 1.0);
  gtk_widget_show(align);
  child = gtk_fixed_new();
  gtk_widget_show(child);
  gtk_widget_set_usize(child, 1, size);
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
e_init_capplet (GtkWidget *c, GtkSignalFunc try, GtkSignalFunc revert, 
		GtkSignalFunc ok, GtkSignalFunc cancel)
{
  gtk_widget_show (c);  
    
  gtk_signal_connect (GTK_OBJECT (c), "try",
		      GTK_SIGNAL_FUNC (try), NULL);
  gtk_signal_connect (GTK_OBJECT (c), "revert",
		      GTK_SIGNAL_FUNC (revert), NULL);
  gtk_signal_connect (GTK_OBJECT (c), "ok",
		      GTK_SIGNAL_FUNC (ok), NULL);
  gtk_signal_connect (GTK_OBJECT (c), "cancel",
		      GTK_SIGNAL_FUNC (cancel), NULL);
}

static void
e_setup (GtkWidget *c)
{
  GtkWidget *frame, *frame2, *hbox;

  current_capplet = c;
  hbox = gtk_hbox_new (FALSE, GNOME_PAD_SMALL);
  gtk_widget_show (hbox);
  gtk_container_add (GTK_CONTAINER (c), hbox);
    
  frame = e_create_frame (_("Options"));
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);
  
  frame2 = e_create_frame (_("Window Activites"));
  e_add_widget_to_frame(frame, frame2);
  e_add_multi_to_frame(frame2, _("Focus style"), 
		       _("Pointer\nSloppy\nClick to focus"), &e_opt_focus);
  e_add_multi_to_frame(frame2, _("Move mode"), 
		       _("Opaque\nLines\nBox\nShaded\nSemi-solid"), &e_opt_move);
  e_add_multi_to_frame(frame2, _("Resize mode"), 
		       _("Opaque\nLines\nBox\nShaded\nSemi-solid"), &e_opt_resize);

  e_add_space_to_frame(frame, 20);
  
  e_add_rangeonoff_to_frame(frame, 
			    _("Tooltip timeout (sec)"),
			    &e_opt_tooltiptime,
			    0.0, 10.0, 
			    _("Shorter"), _("Longer"), 
			    &e_opt_tooltips, 0.0);
  e_add_rangeonoff_to_frame(frame, 
			    _("Shading speed (pixels / sec)"),
			    &e_opt_shade_speed,
			    16.0, 20000.0, 
			    _("Slower"), _("Faster"), 
			    NULL, 99999.0);

  e_add_space_to_frame(frame, 20);
  
  frame = e_create_frame (_("Effects"));
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);
  
  e_add_multi_to_frame(frame, _("Slide mode"), 
		       _("Opaque\nLines\nBox\nShaded\nSemi-solid"), &e_opt_slide_mode);
  e_add_rangeonoff_to_frame(frame, 
			    _("Windows slide in on Map (pixels / sec)"),
			    &e_opt_map_speed,
			    16.0, 20000.0, 
			    _("Slower"), _("Faster"), 
			    &e_opt_slide_map, 0.0);
  e_add_rangeonoff_to_frame(frame, 
			    _("Windows slide in on Cleanup (pixels / sec)"),
			    &e_opt_cleanup_speed,
			    16.0, 20000.0, 
			    _("Slower"), _("Faster"), 
			    &e_opt_slide_cleanup, 0.0);
  e_add_rangeonoff_to_frame(frame, 
			    _("Desktops slide in on change (pixels / sec)"),
			    &e_opt_desk_speed,
			    16.0, 20000.0, 
			    _("Slower"), _("Faster"), 
			    &e_opt_slide_desk, 0.0);

  capplet_widget_state_changed(CAPPLET_WIDGET(c), FALSE);
}

static void
e_setup_desktops (GtkWidget *c)
{
  GtkWidget *frame, *frame2, *hbox;
  
  current_capplet = c;
  hbox = gtk_hbox_new (FALSE, GNOME_PAD_SMALL);
  gtk_widget_show (hbox);
  gtk_container_add (GTK_CONTAINER (c), hbox);
    
  frame = e_create_frame (_("Options"));
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);

  e_add_onoff_to_frame(frame, _("Dither backgrounds"), &e_opt_hq_background);

  capplet_widget_state_changed(CAPPLET_WIDGET(c), FALSE);
}

static void
e_setup_fonts (GtkWidget *c)
{
}

static void
e_setup_sound (GtkWidget *c)
{
  GtkWidget *frame, *frame2, *hbox;
  
  current_capplet = c;
  hbox = gtk_hbox_new (FALSE, GNOME_PAD_SMALL);
  gtk_widget_show (hbox);
  gtk_container_add (GTK_CONTAINER (c), hbox);
    
  frame = e_create_frame (_("Options"));
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);

  e_add_onoff_to_frame(frame, _("Sound Effects"), &e_opt_sound);

  capplet_widget_state_changed(CAPPLET_WIDGET(c), FALSE);
}

static void
e_setup_colors (GtkWidget *c)
{
}

void 
e_setup_multi(GtkWidget *old, GtkWidget *c)
{
  printf("new capplet? %i\n", CAPPLET_WIDGET(c)->capid);

  gtk_signal_connect (GTK_OBJECT (c), "new_multi_capplet",
		      GTK_SIGNAL_FUNC (e_setup_multi), NULL);
  switch (CAPPLET_WIDGET(c)->capid)
    {
     default:
     case -1:
     case 0:
      e_setup(c);
      e_init_capplet(c, e_try, e_revert, e_ok, e_cancel);
      break;
     case 1:
      e_setup_desktops(c);
      e_init_capplet(c, e_try, e_revert, e_ok, e_cancel);
      break;
     case 2:
      e_setup_fonts(c);
      e_init_capplet(c, e_try, e_revert, e_ok, e_cancel);
      break;
     case 3:
      e_setup_sound(c);
      e_init_capplet(c, e_try, e_revert, e_ok, e_cancel);
     case 4:
      e_setup_colors(c);
      e_init_capplet(c, e_try, e_revert, e_ok, e_cancel);
      break;
    }
}

int
main (int argc, char **argv)
{
  GtkWidget *capplet;
  
  e_read();

  gnome_capplet_init("Enlightenment Configuration", NULL, argc, argv, 0, NULL);
  capplet = capplet_widget_new();
  
  e_setup_multi(NULL, capplet);
  capplet_gtk_main();
  return 0;
}
