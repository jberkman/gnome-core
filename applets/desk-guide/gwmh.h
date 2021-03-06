/* gwmh.h - GNOME WM interaction helper functions
 * Copyright (C) 1999 Tim Janik
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 * this code is heavily based on the original gnomepager_applet
 * implementation of The Rasterman (Carsten Haitzler) <raster@rasterman.com>
 */
#ifndef __GWMH_H__
#define __GWMH_H__

#include <gtk/gtkwindow.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* --- preinitialized Atoms --- */
extern gulong GWMHA_WIN_SUPPORTING_WM_CHECK;
extern gulong GWMHA_WIN_PROTOCOLS;
extern gulong GWMHA_WIN_LAYER;
extern gulong GWMHA_WIN_STATE;
extern gulong GWMHA_WIN_HINTS;
extern gulong GWMHA_WIN_APP_STATE;
extern gulong GWMHA_WIN_EXPANDED_SIZE;
extern gulong GWMHA_WIN_ICONS;
extern gulong GWMHA_WIN_WORKSPACE;
extern gulong GWMHA_WIN_WORKSPACE_COUNT;
extern gulong GWMHA_WIN_WORKSPACE_NAMES;
extern gulong GWMHA_WIN_CLIENT_LIST;
extern gulong GWMHA_WIN_AREA;
extern gulong GWMHA_WIN_AREA_COUNT;


/* --- enum values --- */
typedef enum
{
  GWMH_LAYER_DESKTOP     = 0,
  GWMH_LAYER_BELOW       = 2,
  GWMH_LAYER_NORMAL      = 4,
  GWMH_LAYER_ONTOP       = 6,
  GWMH_LAYER_DOCK        = 8,
  GWMH_LAYER_ABOVE_DOCK  = 10
} GwmhLayer;
typedef enum
{
  GWMH_STATE_STICKY          = (1<<0), /* window appears on all desktops+areas */
  GWMH_STATE_MINIMIZED       = (1<<1), /* ??? */
  GWMH_STATE_MAXIMIZED_VERT  = (1<<2), /* window vertically maximized */
  GWMH_STATE_MAXIMIZED_HORIZ = (1<<3), /* window horizontally maximized */
  GWMH_STATE_HIDDEN          = (1<<4), /* ??? */
  GWMH_STATE_SHADED          = (1<<5), /* only show window's wm decorations */
  GWMH_STATE_HID_WORKSPACE   = (1<<6), /* ??? */
  GWMH_STATE_HID_TRANSIENT   = (1<<7), /* owner of transient is hidden */
  GWMH_STATE_FIXED_POSITION  = (1<<8), /* don't scroll with area */
  GWMH_STATE_ARRANGE_IGNORE  = (1<<9)  /* ignore for auto arranging */
} GwmhState;
typedef enum
{
  GWMH_HINTS_SKIP_FOCUS      = (1<<0), /* no ALT+Tab focussing */
  GWMH_HINTS_SKIP_WINLIST    = (1<<1), /* not in win list */
  GWMH_HINTS_SKIP_TASKBAR    = (1<<2), /* not on taskbar */
  GWMH_HINTS_GROUP_TRANSIENT = (1<<3), /* ??? */
  GWMH_HINTS_FOCUS_ON_CLICK  = (1<<4), /* app only accepts focus when clicked */
  GWMH_HINTS_DO_NOT_COVER    = (1<<5)  /* attempt to not cover this window */
} GwmhHints;


/* --- GMainLoop priority for update handler --- */
#define	GWMH_PRIORITY_UPDATE	  (GTK_PRIORITY_RESIZE - 1)


/* --- GwmhTask macros --- */
#define	GWMH_TASK(t)		     ((GwmhTask*) (t))
#define	GWMH_TASK_GSTATE(t)	     (GWMH_TASK (t)->gstate /*GNOME*/)
#define	GWMH_TASK_GHINTS(t)	     (GWMH_TASK (t)->ghints /*GNOME*/)
#define	GWMH_TASK_APP_STATE(t)	     (GWMH_TASK (t)->app_state /*GNOME*/)
#define	GWMH_TASK_FOCUSED(t)	     (GWMH_TASK (t)->focused)
#define	GWMH_TASK_ICONIFIED(t)	     (GWMH_TASK (t)->iconified)
#define	GWMH_TASK_STICKY(t)	     ((GWMH_TASK_GSTATE (t) & GWMH_STATE_STICKY) != 0)
#define	GWMH_TASK_MINIMIZED(t)	     ((GWMH_TASK_GSTATE (t) & GWMH_STATE_MINIMIZED) != 0)
#define	GWMH_TASK_MAXIMIZED_VERT(t)  ((GWMH_TASK_GSTATE (t) & GWMH_STATE_MAXIMIZED_VERT) != 0)
#define	GWMH_TASK_MAXIMIZED_HORIZ(t) ((GWMH_TASK_GSTATE (t) & GWMH_STATE_MAXIMIZED_HORIZ) != 0)
#define	GWMH_TASK_HIDDEN(t)	     ((GWMH_TASK_GSTATE (t) & GWMH_STATE_HIDDEN) != 0)
#define	GWMH_TASK_SHADED(t)	     ((GWMH_TASK_GSTATE (t) & GWMH_STATE_SHADED) != 0)
#define	GWMH_TASK_HID_WORKSPACE(t)   ((GWMH_TASK_GSTATE (t) & GWMH_STATE_HID_WORKSPACE) != 0)
#define	GWMH_TASK_FIXED_POS(t)	     ((GWMH_TASK_GSTATE (t) & GWMH_STATE_FIXED_POSITION) != 0)
#define	GWMH_TASK_SKIP_FOCUS(t)      ((GWMH_TASK_GHINTS (t) & GWMH_HINTS_SKIP_FOCUS) != 0)
#define	GWMH_TASK_SKIP_WINLIST(t)    ((GWMH_TASK_GHINTS (t) & GWMH_HINTS_SKIP_WINLIST) != 0)
#define	GWMH_TASK_SKIP_TASKBAR(t)    ((GWMH_TASK_GHINTS (t) & GWMH_HINTS_SKIP_TASKBAR) != 0)
#define	GWMH_TASK_GROUP_TRANSIENT(t) ((GWMH_TASK_GHINTS (t) & GWMH_HINTS_GROUP_TRANSIENT) != 0)
#define	GWMH_TASK_FOCUS_ON_CLICK(t)  ((GWMH_TASK_GHINTS (t) & GWMH_HINTS_FOCUS_ON_CLICK) != 0)
#define	GWMH_TASK_DO_NOT_COVER(t)    ((GWMH_TASK_GHINTS (t) & GWMH_HINTS_DO_NOT_COVER) != 0)
#define	GWMH_TASK_UPDATE_QUEUED(t)   ((GWMH_TASK (t)->imask_queued | \
				       GWMH_TASK (t)->imask_notify) != 0)


/* --- typedefs --- */
typedef struct _GwmhTask	GwmhTask;
typedef struct _GwmhDesk	GwmhDesk;
typedef struct _GwmhMiniIcon    GwmhMiniIcon;

typedef enum
{
  GWMH_TASK_INFO_MISC        = 1 <<  0,
  GWMH_TASK_INFO_GSTATE      = 1 <<  1,
  GWMH_TASK_INFO_GHINTS      = 1 <<  2,
  GWMH_TASK_INFO_APP_STATE   = 1 <<  3,
  GWMH_TASK_INFO_FOCUSED     = 1 <<  4,
  GWMH_TASK_INFO_ICONIFIED   = 1 <<  5,

  /* should only be evaluated in notifiers */
  GWMH_TASK_INFO_DESKTOP     = 1 <<  6,
  GWMH_TASK_INFO_AREA        = 1 <<  7,
  GWMH_TASK_INFO_LAYER       = 1 <<  8,
  GWMH_TASK_INFO_FRAME_GEO   = 1 <<  9,
  GWMH_TASK_INFO_WIN_GEO     = 1 << 10,
  GWMH_TASK_INFO_ALLOCATION  = (GWMH_TASK_INFO_FRAME_GEO |
				GWMH_TASK_INFO_WIN_GEO),

  /* use this for updates */
  GWMH_TASK_INFO_GEOMETRY    = (GWMH_TASK_INFO_DESKTOP |
				GWMH_TASK_INFO_AREA |
				GWMH_TASK_INFO_ALLOCATION),

  /* pure user notification flags (not used to update task structure) */
  GWMH_TASK_INFO_WM_HINTS    = 1 <<  11,
  GWMH_TASK_INFO_UNOTIFY     = (GWMH_TASK_INFO_WM_HINTS),

  GWMH_TASK_INFO_ALL	 = (GWMH_TASK_INFO_MISC |
			    GWMH_TASK_INFO_GSTATE |
			    GWMH_TASK_INFO_GHINTS |
			    GWMH_TASK_INFO_APP_STATE |
			    GWMH_TASK_INFO_FOCUSED |
			    GWMH_TASK_INFO_ICONIFIED |
			    GWMH_TASK_INFO_GEOMETRY |
			    GWMH_TASK_INFO_LAYER |
			    GWMH_TASK_INFO_UNOTIFY)
} GwmhTaskInfoMask;


/* --- structures --- */
struct _GwmhDesk
{
  guint	  n_desktops;
  guint   n_hareas;
  guint   n_vareas;

  gchar **desktop_names;

  guint   current_desktop;
  guint   current_harea;
  guint   current_varea;

  GList  *client_list;

  guint   detected_gnome_wm : 1;
  guint   unified_area : 1;
};
struct _GwmhTask
{
  gchar     *name;
  gchar     *icon_name;
  
  /* window's state and hints */
  GwmhState        gstate;
  GwmhHints        ghints;
#ifdef GNOME2_CONVERSION_COMPLETE
  GnomeWinAppState app_state;
#endif
  guint            focused : 1;
  guint		   iconified : 1;
  
  /* frame geometry */
  gint       frame_x, frame_y;
  gint       frame_width, frame_height;
  /* window geometry (within frame) */
  gint       win_x, win_y;
  gint       win_width, win_height;

  /* desktop, desktop area, layer */
  guint16    desktop;
  guint16    harea, varea;
  guint16    layer;
  guint16    last_desktop;
  guint16    last_harea, last_varea;
  guint16    last_layer;

  /* GdkWindow proxies */
  GdkWindow *gdkwindow;
  GdkWindow *gdkframe;

  /* private */
  GData           *datalist;
  gulong           xwin;
  gulong           xframe;
  gpointer         sroot;
  GwmhTaskInfoMask imask_queued;
  GwmhTaskInfoMask imask_notify;
};

struct _GwmhMiniIcon
{
  GdkPixmap *pixmap;
  GdkBitmap *mask;
};

/* --- notifications --- */
typedef enum
{
  GWMH_DESK_INFO_DESKTOP_NAMES		= 1 << 0,
  GWMH_DESK_INFO_N_DESKTOPS		= 1 << 1,
  GWMH_DESK_INFO_N_AREAS		= 1 << 2,
  GWMH_DESK_INFO_CURRENT_DESKTOP	= 1 << 3,
  GWMH_DESK_INFO_CURRENT_AREA		= 1 << 4,
  GWMH_DESK_INFO_CLIENT_LIST		= 1 << 5,
  GWMH_DESK_INFO_ALL			= (GWMH_DESK_INFO_DESKTOP_NAMES |
					   GWMH_DESK_INFO_N_DESKTOPS |
					   GWMH_DESK_INFO_N_AREAS |
					   GWMH_DESK_INFO_CURRENT_DESKTOP |
					   GWMH_DESK_INFO_CURRENT_AREA |
					   GWMH_DESK_INFO_CLIENT_LIST),
  GWMH_DESK_INFO_HACK_FLAG		= 1 << 6,
  GWMH_DESK_INFO_FORCE_EVERYTHING	= (GWMH_DESK_INFO_HACK_FLAG |
					   GWMH_DESK_INFO_ALL),
  GWMH_DESK_INFO_BOOTUP			= 1 << 7
} GwmhDeskInfoMask;
typedef enum
{
  GWMH_NOTIFY_INFO_CHANGED, /* features GwmhTaskInfoMask imask */
  GWMH_NOTIFY_NEW,
  GWMH_NOTIFY_DESTROY,
  GWMH_NOTIFY_LAST
} GwmhTaskNotifyType;

typedef gboolean (*GwmhDeskNotifierFunc) (gpointer            func_data,
					  GwmhDesk           *desk,
					  GwmhDeskInfoMask    change_mask);
typedef gboolean (*GwmhTaskNotifierFunc) (gpointer            func_data,
					  GwmhTask	     *task,
					  GwmhTaskNotifyType  ntype,
					  GwmhTaskInfoMask    imask);


/* init GNOME WM Helper code, i.e. watch out for property changes
 * on the root window. returns whether gwmh code is at all usable,
 * depending on whether we have a GNOME compliant window manager.
 */
gboolean	gwmh_init	       		(void);

/* --- notifiers --- */
guint		gwmh_desk_notifier_add		(GwmhDeskNotifierFunc func,
						 gpointer             func_data);
void		gwmh_desk_notifier_remove	(guint                id);
void		gwmh_desk_notifier_remove_func	(GwmhDeskNotifierFunc func,
						 gpointer             func_data);
guint		gwmh_task_notifier_add		(GwmhTaskNotifierFunc func,
						 gpointer             func_data);
void		gwmh_task_notifier_remove	(guint                id);
void		gwmh_task_notifier_remove_func	(GwmhTaskNotifierFunc func,
						 gpointer             func_data);

/* --- GwmhDesk --- */
void		gwmh_desk_queue_update		(GwmhDeskInfoMask imask);
GwmhDesk*	gwmh_desk_get_config		(void);
void		gwmh_desk_set_current_desktop	(guint		 desktop);
void		gwmh_desk_set_current_area	(guint		 desktop,
						 guint		 harea,
						 guint		 varea);
void		gwmh_desk_set_desktop_name	(guint		 desktop,
						 const gchar	*name);
void		gwmh_desk_guess_desktop_area	(guint		 desktop,
						 guint		*harea,
						 guint		*varea);
void		gwmh_desk_set_hack_values	(gboolean	 unified_areas,
						 gboolean        violate_client_msg);


/* --- GwmhTask --- */
void		gwmh_task_queue_update		(GwmhTask	 *task,
						 GwmhTaskInfoMask imask);
GwmhTask*	gwmh_task_from_window		(GdkWindow	 *window);
void            gwmh_task_set_qdata_full	(GwmhTask	*task,
						 GQuark          quark,
						 gpointer        data,
						 GDestroyNotify  destroy);
gpointer	gwmh_task_get_qdata		(GwmhTask	*task,
						 GQuark          quark);
gpointer	gwmh_task_steal_qdata		(GwmhTask	*task,
						 GQuark		 quark);
void		gwmh_task_get_frame_area_pos	(GwmhTask        *task,
						 gint            *x_p,
						 gint            *y_p);
gboolean	gwmh_task_close			(GwmhTask	 *task);
void		gwmh_task_kill			(GwmhTask	 *task);
void		gwmh_task_iconify		(GwmhTask	 *task);
void		gwmh_task_deiconify		(GwmhTask	 *task);
void		gwmh_task_focus			(GwmhTask	 *task);
void		gwmh_task_raise			(GwmhTask	 *task);
void		gwmh_task_show			(GwmhTask	 *task);
void		gwmh_task_set_gstate_flags	(GwmhTask	 *task,
						 GwmhState	  flags);
void		gwmh_task_unset_gstate_flags	(GwmhTask	 *task,
						 GwmhState	  flags);
void		gwmh_task_set_ghint_flags	(GwmhTask	 *task,
						 GwmhHints        flags);
void		gwmh_task_unset_ghint_flags	(GwmhTask	 *task,
						 GwmhHints	  flags);
#ifdef GNOME2_CONVERSION_COMPLETE
void		gwmh_task_set_app_state		(GwmhTask	 *task,
						 GnomeWinAppState app_state);
#endif
void		gwmh_task_set_layer		(GwmhTask	 *task,
						 GwmhLayer        layer);
void		gwmh_task_set_desktop		(GwmhTask	 *task,
						 guint		  desktop);
void		gwmh_task_set_area		(GwmhTask	 *task,
						 guint		  desktop,
						 guint		  harea,
						 guint            varea);
void            gwmh_task_get_mini_icon         (GwmhTask        *task,
						 GdkPixmap      **pixmap,
						 GdkBitmap      **mask);

/* task list functions, task list managing needs to be explicitely
 * enabled since it causes a lot of extra traffic that not all
 * applications want to deal with. task will automatically be
 * added/removed from the list as they appear/disappear on the
 * screen.
 */

GList*		gwmh_task_list_get		(void);
GList*		gwmh_task_list_stack_sort	(GList		*task_list);


/* --- internal --- */
void		gwmh_window_send_client_message (GdkWindow *window,
						 gulong     atom,
						 gulong	    long1,
						 gulong	    long2,
						 gulong	    long3,
						 gulong	    long4,
						 gulong	    long5);
GdkWindow*	gwmh_root_put_atom_window	(const gchar   *atom_name,
						 GdkWindowType  window_type,
						 GdkWindowClass window_class,
						 GdkEventMask   event_mask);




#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GWMH_H__ */





