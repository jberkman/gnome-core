/*  GemVt - GNU Emulator of a Virtual Terminal
 *  Copyright (C) 1997	Tim Janik
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifndef __GVTVT_H__
#define __GVTVT_H__

#include	<gtktty/libgtktty.h>
#include	<gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


typedef	enum
{
  GVT_STATE_NONE,
  GVT_STATE_RUNNING,
  GVT_STATE_DEAD
} GvtStateType;

typedef enum
{
  GVT_VT_SIMPLE,
  GVT_VT_NOTEBOOK,
  GVT_VT_WINDOW
} GvtVtMode;


/* --- type macros --- */
#define GVT_VT(object)		(GTK_CHECK_CAST ((object), gvt_vt_get_type (), GvtVt))
#define GVT_VT_CLASS(klass)	(GTK_CHECK_CLASS_CAST ((klass), gvt_vt_get_type (), GvtVtClass))
#define GVT_IS_VT(object)	(GTK_CHECK_TYPE ((object), gvt_vt_get_type ()))


/* --- typedefs --- */
typedef struct	_GvtVt		GvtVt;
typedef struct	_GvtVtClass	GvtVtClass;
typedef	struct	_GvtColorEntry	GvtColorEntry;


/* --- structures --- */
struct	_GvtVt
{
  GtkObject	object;

  GvtVtMode	mode;

  guint		nth;

  GSList	*col_queue;

  GtkWidget	*window;
  GtkWidget	*vbox;
  GtkWidget	*status_box;
  GtkWidget	*tty;
  GtkWidget	*gem;
  GtkWidget	*label_box;
  GtkWidget	*label_program;
  GtkWidget	*label_status;
  GtkWidget	*label_time;
  GtkWidget	*led_box;
};

struct	_GvtVtClass
{
  GtkObjectClass	parent_class;

  GdkColorContext	*color_context;

  GvtVt		**vts;
  guint		n_vts;

  guint		realized : 1;

  GdkBitmap	*gem_red_bit;
  GdkPixmap	*gem_red_pix;
  GdkBitmap	*gem_green_bit;
  GdkPixmap	*gem_green_pix;
  GdkBitmap	*gem_blue_bit;
  GdkPixmap	*gem_blue_pix;
};

struct	_GvtColorEntry
{
  guint       back_val : 24;
  guint       fore_val : 24;
  guint       dim_val  : 24;
  guint       bold_val : 24;
};


GtkType		gvt_vt_get_type		(void);
GtkObject*	gvt_vt_new		(GvtVtMode	mode,
					 GtkContainer	*parent);
void		gvt_vt_status_update	(GvtVt		*vt);
void		gvt_vt_set_gem_state	(GvtVt		*vt,
					 GvtStateType	state);
void		gvt_vt_execute		(GvtVt		*vt,
					 const gchar	*exec_string);
void		gvt_vt_set_color	(GvtVt		*vt,
					 guint		 index,
					 GvtColorEntry	*c_entry);
void		gvt_vt_do_color_queue	(GvtVt		*vt);
     
     


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GVTVT_H__ */
