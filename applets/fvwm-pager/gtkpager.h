/* fvwm-pager
 *
 * Copyright (C) 1998 Michael Lausch
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifndef __GTK_PAGER_H__
#define __GTK_PAGER_H__ 1
#include <stdio.h>

#include <gnome.h>

#ifdef __cpluplus
extern "C" {
#endif
  
#define GTKPAGER_WINDOW_ICONIFIED  0x00000001
#define GTKPAGER_WINDOW_FOCUS      0x00000002
  
#define GTK_FVWMPAGER(obj)         (GTK_CHECK_CAST ((obj), gtk_fvwmpager_get_type (), GtkFvwmPager))
#define GTK_FVWMPAGER_CLASS(klass) (GTK_CHECK_CLASS_CAST ((klass), gtk_fvwmpager_get_type (), GtkFvwmPagerClass))
#define GTK_IS_FVWMPAGER(obj)      (GTK_CHECK_TYPE ((obj), gtk_fvwmpager_get_type ()))

typedef struct _GtkFvwmPager       GtkFvwmPager;
typedef struct _GtkFvwmPagerClass  GtkFvwmPagerClass;

typedef struct _Desktop
{
  gint              idx;
  gint              x1;
  gint              x2;
  gint              y1;
  gint              y2;
  double            x_scale;
  double            y_scale;
  gchar*            title;
  GnomeCanvasItem*  dt_group;
  GnomeCanvasItem*  dt;
  GList*            windows;
}Desktop;
  
typedef struct _Window
{
  gint             xid;
  gint             ixid;
  gint             x;
  gint             y;
  gint             w;
  gint             h;
  gint             ix;
  gint             iy;
  gint             iw;
  gint             ih;
  gint             bw;
  gint             th;
  Desktop*         desk;
  gulong           flags;
  gchar*           title;
  GnomeCanvasItem* rect;
}PagerWindow;



struct _GtkFvwmPager
{
  GnomeCanvas  canvas;
  GList*       desktops;
  GHashTable*  windows;
  gint         num_of_desks;
  gint         num_of_wins;
  gint         width;
  gint         height;
  
  int*         fd;
  Desktop*     current_desktop;
  PagerWindow* current_window;
};


struct _GtkFvwmPagerClass
{
  GnomeCanvas parent_class;

  void (* switch_to_desk) (GtkWidget*, guint desktop_offset);
  
};


guint         gtk_fvwmpager_get_type          (void);
GtkWidget*    gtk_fvwmpager_new               (gint* fd, gint width, gint height);
#if 0
void          gtk_fvwmpager_add_desk          (GtkFvwmPager*, gchar*);
#endif
void          gtk_fvwmpager_set_current_desk  (GtkFvwmPager* pager, gint desktop);
void          gtk_fvwmpager_set_desktops      (GtkFvwmPager* pager, GList* desktops);
void          gtk_fvwmpager_display_desks     (GtkFvwmPager* pager);
void          gtk_fvwmpager_label_desk        (GtkFvwmPager* pager, gint idx, gchar* label);
void          gtk_fvwmpager_set_current_desk  (GtkFvwmPager*, gint idx);

void          gtk_fvwmpager_display_window    (GtkFvwmPager* pager, PagerWindow* old_window, gint xid);
void          gtk_fvwmpager_deiconify_window  (GtkFvwmPager* pager, gint xid);
void          gtk_fvwmpager_raise_window      (GtkFvwmPager* pager, gint xid);
void          gtk_fvwmpager_lower_window      (GtkFvwmPager* pager, gint xid);
void          gtk_fvwmpager_destroy_window    (GtkFvwmPager* pager, gint xid);
void          gtk_fvwmpager_set_current_window(GtkFvwmPager*, gint idx);


void          gtk_fvwmpager_prop_changed      (GtkFvwmPager* pager);

#endif
