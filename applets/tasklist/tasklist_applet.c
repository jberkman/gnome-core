#include <config.h>
#include <gnome.h>
#include <libgnomeui/gnome-window-icon.h>

#include "gstc.h"
#include "gwmh.h"

#include <gdk-pixbuf/gdk-pixbuf.h>

#include "tasklist_applet.h"
#include "unknown.xpm"

/* Prototypes */
static void cb_properties (void);
static void cb_about (void);
gchar *fixup_task_label (TasklistTask *task);
gboolean is_task_visible (TasklistTask *task);
void draw_task (TasklistTask *task, GdkRectangle *rect);
TasklistTask *find_gwmh_task (GwmhTask *gwmh_task);
gboolean desk_notifier (gpointer func_data, GwmhDesk *desk, GwmhDeskInfoMask change_mask);
gboolean task_notifier (gpointer func_data, GwmhTask *gwmh_task, GwmhTaskNotifyType ntype, GwmhTaskInfoMask imask);
gboolean cb_button_press_event (GtkWidget *widget, GdkEventButton *event);
gboolean cb_drag_motion (GtkWidget *widget, GdkDragContext *context, int x, int y, guint time);
void cb_drag_leave (GtkWidget *widget, GdkDragContext *context, guint time);
gboolean cb_motion_timeout (gpointer data);
gboolean cb_expose_event (GtkWidget *widget, GdkEventExpose *event);
void create_applet (void);
TasklistTask *task_get_xy (gint x, gint y);
GList *get_visible_tasks (void);
gint get_horz_rows(void);
gboolean show_task (TasklistTask *task);

GNOME_Panel_OrientType tasklist_orient; /* Tasklist orient */
GtkWidget *handle; /* The handle box */
GtkWidget *applet; /* The applet */
GtkWidget *area; /* The drawing area used to display tasks */
GList *tasks = NULL; /* The list of tasks used */

#define MOTION_TIMEOUT 500 /* Show task motion_task if cursor over task for ... msec */
int motion_timeout = 0; /* Show task motion_task after MOTION_TIMEOUT in drag_motion */
TasklistTask *motion_task = NULL; /* Task to show after motion_timeout */

TasklistIcon *unknown_icon = NULL; /* The unknown icon */

gint vert_height=0; /* Vertical height, used for resizing */
gint horz_width=0;  /* Horizontal width, used for resizing */

gint panel_size = 48;

extern TasklistConfig Config;

/* sorting the list... */
static gint
lexographic_compare_func (gconstpointer a, gconstpointer b)
{
       const TasklistTask *la = (const TasklistTask*)a;
       const TasklistTask *lb = (const TasklistTask*)b;

       return strcasecmp (la->gwmh_task->name, lb->gwmh_task->name);
}

static gint
creation_compare_func (gconstpointer a, gconstpointer b)
{
       const TasklistTask *la = (const TasklistTask*)a;
       const TasklistTask *lb = (const TasklistTask*)b;

       return lb->serial_number - la->serial_number;
}

static gint
tasklist_sorting_compare_func (gconstpointer a, gconstpointer b)
{
       /*
        * This is dumb, but here's why:
        * the intent is that there be lots of compare functions
        * (maybe putting the icons at the top, or by desktop,
        * or whatever.  Right now there are just two: the old
        * one and the one I like.  BUT, I don't know enough about
        * how to make gnome_config_xxx map a list of things into
        * functions, so for now I just have a boolean that controls
        * which.  Hopefully the next person who adds sorting functions
        * will know more about gnome than I do and will fix this hack.
        * - johnh@isi.edu, 24-Nov-00
        */
       return Config.sort_tasklist ?
	       lexographic_compare_func (a, b) :
	       creation_compare_func (a, b);
}

/* from gtkhandlebox.c */
#define DRAG_HANDLE_SIZE 10

/* get the horz_rows depending on the configuration settings */
gint
get_horz_rows(void)
{
	int result;

	if (Config.follow_panel_size)
		result = panel_size/ROW_HEIGHT;
	else
		result = Config.horz_rows;

	if (result < 1)
		result = 1;

	return result;
}

/* Shorten a label that is too long */
gchar *
fixup_task_label (TasklistTask *task)
{
	gchar *str, *tempstr;
	gint len, label_len;

	label_len = gdk_string_width (area->style->font,
				      task->gwmh_task->name);
	
	if (GWMH_TASK_ICONIFIED (task->gwmh_task))
		label_len += gdk_string_width (area->style->font,
					       "[]");

	if (label_len > task->width - ROW_HEIGHT) {
		GdkWChar *wstr;

		len = strlen (task->gwmh_task->name);
		wstr = g_new (GdkWChar, len + 3);
		len = gdk_mbstowcs (wstr, task->gwmh_task->name, len);
		if ( len < 0 ) { /* if the conversion is failed */
			wstr[0] = wstr[1] = wstr[2] = '?'; 
			wstr[3] = '\0'; /* wcscpy(wstr,"???");*/
			len = 3;
			label_len = gdk_text_width_wc(area->style->font,
                                                      wstr, len);
			if (label_len <= task->width 
			    - (Config.show_mini_icons ? 24:6)) {
				str = gdk_wcstombs(wstr);
				g_free(wstr);
				return str;
			}
		}
		wstr[len] = wstr[len+1] = '.';
		wstr[len+2] = '\0'; /*wcscat(wstr,"..");*/
		len--;

		for (; len > 0; len--) {
			wstr[len] = '.';
			wstr[len + 3] = '\0';
			
			label_len = gdk_text_width_wc (area->style->font,
						       wstr, len + 3);
			
			if (GWMH_TASK_ICONIFIED (task->gwmh_task))
				label_len += gdk_string_width (area->style->font,
							       "[]");
			if (label_len <= task->width - (Config.show_mini_icons ? 24:6))
				break;
		}
		str = gdk_wcstombs (wstr);
		g_free (wstr);
	}
	else
		str = g_strdup (task->gwmh_task->name);

	if (GWMH_TASK_ICONIFIED (task->gwmh_task)) {
		tempstr = g_strdup_printf ("[%s]", str);
		g_free(str);
		str = tempstr;
	}

	return str;
}

/* Check what task (if any) is at position x,y on the tasklist */
TasklistTask *
task_get_xy (gint x, gint y)
{
	GList *temp_tasks, *temp;
	TasklistTask *task;

	temp_tasks = get_visible_tasks ();

	for (temp = temp_tasks; temp != NULL; temp = temp->next) {
		task = (TasklistTask *)temp->data;
		if (x > task->x &&
		    x < task->x + task->width &&
		    y > task->y &&
		    y < task->y + task->height) {
			g_list_free (temp_tasks);
			return task;
		}
	}

	if (temp_tasks != NULL)
		g_list_free (temp_tasks);

	return NULL;
}


/* Check which tasks are "visible",
   if they should be drawn onto the tasklist */
GList *
get_visible_tasks (void)
{
	GList *temp_tasks;
	GList *visible_tasks = NULL;

	temp_tasks = tasks;
	while (temp_tasks) {
		if (is_task_visible ((TasklistTask *) temp_tasks->data))
                        visible_tasks = g_list_insert_sorted (visible_tasks, temp_tasks->data, tasklist_sorting_compare_func);
		temp_tasks = temp_tasks->next;
	}
	return visible_tasks;
}

/* Check if a task is "visible", 
   if it should be drawn onto the tasklist */
gboolean
is_task_visible (TasklistTask *task)
{
	GwmhDesk *desk_info;

	desk_info = gwmh_desk_get_config ();

	if (GWMH_TASK_SKIP_TASKBAR (task->gwmh_task))
		return FALSE;
	

	if (task->gwmh_task->desktop != desk_info->current_desktop ||
	    task->gwmh_task->harea != desk_info->current_harea ||
	    task->gwmh_task->varea != desk_info->current_varea) {
		if (!GWMH_TASK_STICKY (task->gwmh_task)) {
			if (!Config.all_desks_minimized && 
			    !Config.all_desks_normal)
				return FALSE;
				
			else if (Config.all_desks_minimized && 
				 !Config.all_desks_normal) {
				if (!GWMH_TASK_ICONIFIED (task->gwmh_task))
					return FALSE;
			}
			else if (Config.all_desks_normal && 
				 !Config.all_desks_minimized) {
				if (GWMH_TASK_ICONIFIED (task->gwmh_task))
					return FALSE;
			}
		}
	}			

	if (GWMH_TASK_ICONIFIED (task->gwmh_task)) {
		if (!Config.show_minimized)
			return FALSE;
	} else {
		if (!Config.show_normal)
			return FALSE;
	}
		
	return TRUE;
}

/* Draw a single task */
void
draw_task (TasklistTask *task, GdkRectangle *rect)
{
	gchar *tempstr;
	gint text_height, text_width;

	/* For mini icons */
	TasklistIcon *icon;
	GdkPixbuf *pixbuf;

	if (!is_task_visible (task))
		return;

	gtk_paint_box (area->style, area->window,
		       GWMH_TASK_FOCUSED (task->gwmh_task) ?
		       GTK_STATE_ACTIVE : GTK_STATE_NORMAL,
		       GWMH_TASK_FOCUSED (task->gwmh_task) ?
		       GTK_SHADOW_IN : GTK_SHADOW_OUT,
		       rect, area, "button",
		       task->x, task->y,
		       task->width, task->height);

	if (task->gwmh_task->name) {
		tempstr = fixup_task_label (task);
		text_height = gdk_string_height (area->style->font, "1");
		text_width = gdk_string_width (area->style->font, tempstr);
		gdk_draw_string (area->window,
				 area->style->font,
				 GWMH_TASK_FOCUSED (task->gwmh_task) ?
				 area->style->fg_gc[GTK_STATE_ACTIVE] :
				 area->style->fg_gc[GTK_STATE_NORMAL],
				 task->x +
				 (Config.show_mini_icons ? 10 : 0) +
				 ((task->width - text_width) / 2),
				 task->y + ((task->height - text_height) / 2) + text_height,
				 tempstr);

		g_free (tempstr);
	}

	if (Config.show_mini_icons) {
		icon = task->icon;
		
		if (GWMH_TASK_ICONIFIED (task->gwmh_task))
			pixbuf = icon->minimized;
		else
			pixbuf = icon->normal;

		gdk_pixbuf_render_to_drawable_alpha (pixbuf,
						     area->window,
						     0, 0,
						     task->x + 3 + (16 - gdk_pixbuf_get_width (pixbuf)) / 2,
						     task->y + (task->height - gdk_pixbuf_get_height (pixbuf)) / 2,
						     gdk_pixbuf_get_width (pixbuf),
						     gdk_pixbuf_get_height (pixbuf),
						     GDK_PIXBUF_ALPHA_BILEVEL,
						     127,
						     GDK_RGB_DITHER_NORMAL,
						     gdk_pixbuf_get_width (pixbuf),
						     gdk_pixbuf_get_height (pixbuf));

	}
}

static int
max_width (GList *temp_tasks)
{
	GList *li;
	int maxwidth = 0;

	for (li = temp_tasks; li != NULL; li = li->next) {
		int width;
		TasklistTask *task = li->data;
		const char *name = task->gwmh_task->name != NULL ? 
			task->gwmh_task->name : "";

		if (task->fullwidth < 0) {
			width = gdk_string_width (area->style->font, name);

			if (GWMH_TASK_ICONIFIED (task->gwmh_task))
				width += gdk_string_width (area->style->font,
							   "[]");

			task->fullwidth = width;
		} else {
			width = task->fullwidth;
		}

		if (Config.show_mini_icons)
			width += 10;

		width += ROW_HEIGHT;

		if (width > maxwidth)
			maxwidth = width;
	}

	return maxwidth;
}

/* Layout the tasklist */
void
layout_tasklist (gboolean call_change_size)
{
	gint j = 0, k = 0, num = 0, p = 0;
	GList *temp_tasks, *temp;
	TasklistTask *task;
	/* gint extra_space; */
	gint num_rows = 0, num_cols = 0;
	gint curx = 0, cury = 0, curwidth = 0, curheight = 0;
	
	temp_tasks = get_visible_tasks ();
	num = g_list_length (temp_tasks);
	
	switch (applet_widget_get_panel_orient (APPLET_WIDGET (applet))) {
	case ORIENT_UP:
	case ORIENT_DOWN:
		if (num == 0) {
			if (Config.horz_fixed)
				horz_width = Config.horz_width;
			else
				horz_width = 4;
			
			change_size (FALSE, -1);
			
			gtk_widget_draw (area, NULL);
			return;
		}

		while (p < num) {
			if (num < get_horz_rows())
				num_rows = num;
			
			j++;
			if (num_cols < j)
				num_cols = j;
			if (num_rows < k + 1)
				num_rows = k + 1;
			
			if (get_horz_rows () == 0 || j >= ((num + get_horz_rows() - 1) / get_horz_rows())) {
				j = 0;
				k++;
			}
			p++;
		}
		
		if (Config.horz_fixed) {
			curheight = (ROW_HEIGHT * get_horz_rows() - 0) / num_rows;
			curwidth = (Config.horz_width - 0) / num_cols;

		} else {
			curheight = (ROW_HEIGHT * get_horz_rows() - 0) / num_rows;
			curwidth = Config.horz_taskwidth;

			/* If the total width is higher than allowed, 
			   we use the "fixed" way instead */
			if ((curwidth * num_cols) > Config.horz_width)
				curwidth = (Config.horz_width - 0) / num_cols;
		}


		curx = 0;
		cury = 0;


		for (temp = temp_tasks; temp != NULL; temp = temp->next) {
			task = (TasklistTask *) temp->data;
			
			task->x = curx;
			task->y = cury;
			task->width = curwidth;
			task->height = curheight;
			
			if (Config.horz_fixed) {
				curx += curwidth;
			
				if (curx >= Config.horz_width ||
				    curx + curwidth > Config.horz_width) {
					cury += curheight;
					curx = 0;
				}
			} else {

				curx += curwidth;

				if (curx >= num_cols * curwidth) {
					cury += curheight;
					curx = 0;
				}
			}
		}

		if (Config.horz_fixed)
			horz_width = Config.horz_width;
		else
			horz_width = num_cols * curwidth + 4;

		if (call_change_size)
			change_size (FALSE, -1);

		break;

	case ORIENT_LEFT:
	case ORIENT_RIGHT:

		if (num == 0) {
			if (Config.vert_fixed)
				vert_height = Config.vert_height;
			else
				vert_height = 4;
			
			if (call_change_size)
				change_size (FALSE, -1);
			
			gtk_widget_draw (area, NULL);
			return;
		}

		curheight = ROW_HEIGHT;
		if (Config.follow_panel_size)
			curwidth = panel_size - 0;
		else
			curwidth = Config.vert_width - 0;

		if (Config.vert_width_full) {
			int fullwidth = max_width (temp_tasks);

			if (fullwidth > curwidth)
				curwidth = fullwidth;
		}
		
		num_cols = 1;
		num_rows = num;
		
		curx = 0;
		cury = 0;

		if (Config.vert_fixed)
			vert_height = Config.vert_height;
		else
			vert_height = curheight * num_rows + 4;
		
		if (call_change_size)
			change_size (FALSE, curwidth);

		for (temp = temp_tasks; temp != NULL; temp = temp->next) {
			task = (TasklistTask *) temp->data;
			
			task->x = curx;
			task->y = cury;
			task->width = curwidth;
			task->height = curheight;
			
			curx += curwidth;

			if (curx >= (Config.follow_panel_size?
				     panel_size:
				     Config.vert_width) - 0) {
				cury += curheight;
				curx = 0;
			}
		}

		break;
	}

	if (temp_tasks != NULL)
		g_list_free (temp_tasks);

	
	gtk_widget_draw (area, NULL);
}

/* Get a task from the list that has got the given gwmh_task */
TasklistTask *
find_gwmh_task (GwmhTask *gwmh_task)
{
	GList *temp_tasks;
	TasklistTask *task;

	temp_tasks = tasks;

	while (temp_tasks) {
		task = (TasklistTask *)temp_tasks->data;
		if (task->gwmh_task == gwmh_task)
			return task;
		temp_tasks = temp_tasks->next;
	}
	
	return NULL;
}

/* This routine gets called when desktops are switched etc */
gboolean
desk_notifier (gpointer func_data, GwmhDesk *desk,
	       GwmhDeskInfoMask change_mask)
{
	if (Config.all_desks_minimized && 
	    Config.all_desks_normal)
		return TRUE;

	layout_tasklist (TRUE);

	return TRUE;
}

/* This routine gets called when tasks are created/destroyed etc */
gboolean
task_notifier (gpointer func_data, GwmhTask *gwmh_task,
	       GwmhTaskNotifyType ntype,
	       GwmhTaskInfoMask imask)
{
	gboolean que_draw, que_layout;
        static gint master_serial_number = 0;
	TasklistTask *task;
	
	switch (ntype)
	{
	case GWMH_NOTIFY_INFO_CHANGED:
		que_draw = FALSE;
		que_layout = FALSE;
		task = find_gwmh_task (gwmh_task);

		if (imask & GWMH_TASK_INFO_WM_HINTS) {
			task->fullwidth = -1;
			if (tasklist_icon_get_pixmap (task) !=
			    task->wmhints_icon) {
				tasklist_icon_destroy (task);
				tasklist_icon_set (task);
				que_draw = TRUE;
			}
		}
		if (imask & GWMH_TASK_INFO_GSTATE)
			que_layout = TRUE;
		if (imask & GWMH_TASK_INFO_ICONIFIED) {
			task->fullwidth = -1;
			que_layout = TRUE;
		}
		if (imask & GWMH_TASK_INFO_FOCUSED)
			que_draw = TRUE;
		if (imask & GWMH_TASK_INFO_MISC) {
			task->fullwidth = -1;
			que_draw = TRUE;

			/* this might change the size,
			 * if width full is on. */
			if (Config.vert_width_full &&
			    (tasklist_orient == ORIENT_LEFT ||
			     tasklist_orient == ORIENT_RIGHT))
				que_layout = TRUE;
		}
		if (imask & GWMH_TASK_INFO_DESKTOP) {
			if ( ! Config.all_desks_minimized ||
			     ! Config.all_desks_normal)
				que_layout = TRUE;
		}

		if (que_layout)
			/* Redraw entire tasklist */
			layout_tasklist (TRUE);
		else if (que_draw)
			/* We can get away with redrawing the task only */
			draw_task (task, NULL);
		break;
	case GWMH_NOTIFY_NEW:
		task = g_malloc0 (sizeof (TasklistTask));
		task->gwmh_task = gwmh_task;
		task->wmhints_icon = tasklist_icon_get_pixmap (task);
		tasklist_icon_set (task);
                task->serial_number = master_serial_number++; /* wrapping seems unlikely :-) */
                tasks = g_list_insert_sorted (tasks, task, tasklist_sorting_compare_func);
	        layout_tasklist (TRUE);
		break;
	case GWMH_NOTIFY_DESTROY:
		task = find_gwmh_task (gwmh_task);
		if(task) {
			tasks = g_list_remove (tasks, task);
			if (task == motion_task) {
				motion_task = NULL;
			}
			tasklist_icon_destroy (task);
			if(task->menu)
				gtk_widget_destroy(task->menu);
			g_free (task);
			layout_tasklist (TRUE);
		}
		break;
	default:
		g_print ("Unknown ntype: %d\n", ntype);
	}

	return TRUE;
}

/* Show the task if need. Return TRUE if so */
gboolean
show_task (TasklistTask *task)
{
	if (GWMH_TASK_ICONIFIED (task->gwmh_task) || !GWMH_TASK_FOCUSED (task->gwmh_task)) {
	
		if (!(Config.move_to_current && GWMH_TASK_ICONIFIED (task->gwmh_task))) {
			GwmhDesk *desk_info;
			desk_info = gwmh_desk_get_config ();
					
			if (task->gwmh_task->desktop != desk_info->current_desktop ||
			    task->gwmh_task->harea != desk_info->current_harea ||
			    task->gwmh_task->varea != desk_info->current_varea) {
				gwmh_desk_set_current_area (task->gwmh_task->desktop,
							    task->gwmh_task->harea,
							    task->gwmh_task->varea);
			}
		}
	
		gwmh_task_show (task->gwmh_task);
		/* Why is a focus needed here?
		   gwmh_task_show is supposed to give focus */
		gwmh_task_focus (task->gwmh_task);
		gwmh_task_focus (task->gwmh_task);
		
		
		return TRUE;
	}
	
	return FALSE;
}

/* This routine gets called when the mouse is pressed */
gboolean
cb_button_press_event (GtkWidget *widget, GdkEventButton *event)
{
	TasklistTask *task;
	
	task = task_get_xy ((gint)event->x, (gint)event->y);

	if (!task)
		return FALSE;

	if (event->button == 1) {

		if (! show_task (task)) {
			gwmh_task_iconify (task->gwmh_task);
		}

		return TRUE;
	}

	if (event->button == 3) {
		gtk_signal_emit_stop_by_name (GTK_OBJECT (widget),
					      "button_press_event");
		menu_popup (task, event->button, event->time);
		return TRUE;
	}

	return FALSE;
}

void
cb_drag_leave (GtkWidget *widget, GdkDragContext *context, guint time)
{
	if (motion_timeout) {
		gtk_timeout_remove (motion_timeout);
		motion_timeout = 0;
		motion_task = NULL;
	}
}

/* This routine gets called when user drag something over the tasklist */
gboolean 
cb_drag_motion (GtkWidget *widget, GdkDragContext *context, int x, int y, guint time)
{
	TasklistTask *task;
	
	gdk_drag_status (context, 0, time);
	
	task = task_get_xy (x, y);
	
	if (task != motion_task) {
	
		if (motion_timeout) {
			gtk_timeout_remove (motion_timeout);
		}
	
		motion_task = task;

		if (task) {	
			motion_timeout = gtk_timeout_add (MOTION_TIMEOUT, cb_motion_timeout, widget);		
		} else {
			motion_timeout = 0;
		}
	}
	
	return TRUE;
}

gboolean 
cb_motion_timeout (gpointer data)
{
	if (motion_task) {
		show_task (motion_task);
	}

	motion_timeout = 0;
	motion_task = NULL;
	
	return FALSE;	
}

/* This routine gets called when the tasklist is exposed */
gboolean
cb_expose_event (GtkWidget *widget, GdkEventExpose *event)
{
	GList *temp_tasks, *temp;
	TasklistTask *task;

	temp_tasks = get_visible_tasks ();

	gtk_paint_flat_box (area->style, area->window,
			    area->state, GTK_SHADOW_NONE,
			    &event->area, area, "button",
			    0, 0, -1, -1);
	
	for (temp = temp_tasks; temp != NULL; temp = temp->next) {
		GdkRectangle rect, dest;
		task = (TasklistTask *)temp->data;

		rect.x = task->x;
		rect.y = task->y;
		rect.width = task->width;
		rect.height = task->height;

		if(gdk_rectangle_intersect(&event->area, &rect, &dest))
			draw_task (task, &dest);
	}

	if (temp_tasks != NULL)
		g_list_free (temp_tasks);

	return FALSE;
}

/* This routine gets called when the user selects "properties" */
static void
cb_properties (void)
{
	display_properties ();
}

/* This routine gets called when the user selects "about" */
static void
cb_about (void)
{
	static GtkWidget *dialog = NULL;

	const char *authors[] = {
		"Anders Carlsson (andersca@gnu.org)",
		NULL
	};

	/* Stop the about box from being shown twice at once */
	if (dialog != NULL)
	{
		gdk_window_show (dialog->window);
		gdk_window_raise (dialog->window);
		return;
	}
	
	dialog = gnome_about_new ("Gnome Tasklist",
				  VERSION,
				  "Copyright (C) 1999 Anders Carlsson",
				  authors,
				  "A tasklist for the GNOME desktop environment.\nIcons by Tuomas Kuosmanen (tigert@gimp.org).",
				  NULL);
	gtk_signal_connect (GTK_OBJECT(dialog), "destroy",
			    GTK_SIGNAL_FUNC(gtk_widget_destroyed), &dialog);

	gtk_widget_show (dialog);
	gdk_window_raise (dialog->window);
}

/* Ignore mouse button 1 clicks */
static gboolean
ignore_1st_click (GtkWidget *widget, GdkEvent *event)
{
	GdkEventButton *buttonevent = (GdkEventButton *)event;

	if (event->type == GDK_BUTTON_PRESS &&
	    buttonevent->button == 1) {
		if (buttonevent->window != area->window)
			buttonevent->button = 2;
	}
	if (event->type == GDK_BUTTON_RELEASE &&
	    buttonevent->button == 1) {
		if (buttonevent->window != area->window)
			buttonevent->button = 2;
	}
	 
	return FALSE;
}
 
/*
 * Resort the task list.
 * (This is only here because tasks and tasklist_sorting_compare_func
 * are not globally defined.)
 */
void
resort_tasklist (void)
{
       tasks = g_list_sort (tasks, tasklist_sorting_compare_func);
}

/* Changes size of the applet */
void
change_size (gboolean layout, int fullwidth)
{
	gboolean force_change = FALSE;
	static int old_handle_width = -1;
	static int old_handle_height = -1;
	static int old_width = -1;
	static int old_height = -1;
	int handle_width = 0;
	int handle_height = 0;
	int width = 0;
	int height = 0;

	switch (applet_widget_get_panel_orient (APPLET_WIDGET (applet))) {
	case ORIENT_UP:
	case ORIENT_DOWN:
/*		if (Config.horz_fixed)
			horz_width = Config.horz_width;
		else
		horz_width = 4;*/

		width = horz_width;
		handle_width = DRAG_HANDLE_SIZE + horz_width;
		height = handle_height = get_horz_rows() * ROW_HEIGHT;

		if (GTK_HANDLE_BOX (handle)->handle_position != GTK_POS_LEFT) {
			GTK_HANDLE_BOX (handle)->handle_position = GTK_POS_LEFT;
			force_change = TRUE;
		}
		break;
	case ORIENT_LEFT:
	case ORIENT_RIGHT:
/*		if (Config.vert_fixed)
			vert_height = Config.vert_height;
		else
		vert_height = 4;*/

		if (Config.follow_panel_size)
			width = panel_size;
		else
			width = Config.vert_width;

		if (Config.vert_width_full) {
			if (fullwidth < 0) {
				GList *temp_tasks = get_visible_tasks ();

				fullwidth = max_width (temp_tasks);

				g_list_free (temp_tasks);
			}

			if (fullwidth > width)
				width = fullwidth;
		}

		if (GTK_HANDLE_BOX (handle)->handle_position != GTK_POS_TOP) {
			GTK_HANDLE_BOX (handle)->handle_position = GTK_POS_TOP;
			force_change = TRUE;
		}

		handle_width = width;
		handle_height = vert_height + DRAG_HANDLE_SIZE;
		height = vert_height;
	}

	if (force_change ||
	    old_width != width ||
	    old_height != height ||
	    old_handle_width != handle_width ||
	    old_handle_height != handle_height) {
		gtk_widget_set_usize (handle,
				      handle_width,
				      handle_height);
		gtk_drawing_area_size (GTK_DRAWING_AREA (area), 
				       width, height);
		old_width = width;
		old_height = height;
		old_handle_width = handle_width;
		old_handle_height = handle_height;
	}

	if (layout)
		layout_tasklist (FALSE);
}

/* Called when the panel's orient changes */
static void
cb_change_orient (GtkWidget *widget, GNOME_Panel_OrientType orient)
{
	
	tasklist_orient = orient;

	/* Change size accordingly */
	change_size (TRUE, -1);
}

static void
cb_change_pixel_size (GtkWidget *widget, int size)
{
	panel_size = size;
	
	/* Change size accordingly */
	if(Config.follow_panel_size)
		change_size (TRUE, -1);
}

static void
cb_help (GtkWidget *w, gpointer data)
{
	GnomeHelpMenuEntry help_entry = { "tasklist_applet",
					  "index.html" };
	gnome_help_display(NULL, &help_entry);
}

/* Create the applet */
void
create_applet (void)
{
	GtkWidget *hbox;

	applet = applet_widget_new ("tasklist_applet");
	
	hbox = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (hbox);

	/* we must bind signals BEFORE applet_widget_add to avoid
	 * a race */
	gtk_signal_connect (GTK_OBJECT (applet), "change-orient",
			    GTK_SIGNAL_FUNC (cb_change_orient), NULL);
	gtk_signal_connect (GTK_OBJECT (applet), "save-session",
			    GTK_SIGNAL_FUNC (write_config), NULL);
	gtk_signal_connect (GTK_OBJECT (applet), "change-pixel-size",
			    GTK_SIGNAL_FUNC (cb_change_pixel_size), NULL);

	applet_widget_add (APPLET_WIDGET (applet), hbox);
	
	handle = gtk_handle_box_new ();
	gtk_signal_connect (GTK_OBJECT (handle), "event",
			    GTK_SIGNAL_FUNC (ignore_1st_click), NULL);

	area = gtk_drawing_area_new ();

	gtk_widget_show (area);
	gtk_widget_show (handle);
	gtk_container_add (GTK_CONTAINER (handle), area);
	gtk_container_add (GTK_CONTAINER (hbox), handle);

	gtk_widget_set_events (area, GDK_EXPOSURE_MASK | 
			       GDK_BUTTON_PRESS_MASK |
			       GDK_BUTTON_RELEASE_MASK);
	gtk_signal_connect (GTK_OBJECT (area), "expose_event",
			    GTK_SIGNAL_FUNC (cb_expose_event), NULL);
	gtk_signal_connect (GTK_OBJECT (area), "button_press_event",
			    GTK_SIGNAL_FUNC (cb_button_press_event), NULL);
	gtk_signal_connect (GTK_OBJECT (area), "drag_motion",
			    GTK_SIGNAL_FUNC (cb_drag_motion), NULL);
	gtk_signal_connect (GTK_OBJECT (area), "drag_leave",
			    GTK_SIGNAL_FUNC (cb_drag_leave), NULL);			    
			    
			    
	gtk_drag_dest_set (GTK_WIDGET (area), 0,
	 		   NULL, 0, GDK_ACTION_COPY);  

	applet_widget_register_stock_callback (APPLET_WIDGET (applet),
					       "properties",
					       GNOME_STOCK_MENU_PROP,
					       _("Properties..."),
					       (AppletCallbackFunc) cb_properties,
					       NULL);
	applet_widget_register_stock_callback (APPLET_WIDGET (applet),
					       "help",
					       GNOME_STOCK_PIXMAP_HELP,
					       _("Help"),
					       (AppletCallbackFunc) cb_help,
					       NULL);

	applet_widget_register_stock_callback (APPLET_WIDGET (applet),
					       "about",
					       GNOME_STOCK_MENU_ABOUT,
					       _("About..."),
					       (AppletCallbackFunc) cb_about,
					       NULL);
}

gint
main (gint argc, gchar *argv[])
{
	/* Initialize i18n */
	bindtextdomain (PACKAGE, GNOMELOCALEDIR);
	textdomain (PACKAGE);

	applet_widget_init ("tasklist_applet",
			    VERSION,
			    argc, argv,
			    NULL, 0, NULL);

	gdk_rgb_init ();

	gnome_window_icon_set_default_from_file (GNOME_ICONDIR"/gnome-tasklist.png");

	gtk_widget_set_default_colormap (gdk_rgb_get_cmap ());
	gtk_widget_set_default_visual (gdk_rgb_get_visual ());

	gwmh_init ();
	gwmh_task_notifier_add (task_notifier, NULL);
	gwmh_desk_notifier_add (desk_notifier, NULL);
	
	create_applet ();

	read_config ();
	panel_size = applet_widget_get_panel_pixel_size(APPLET_WIDGET(applet));
	tasklist_orient = applet_widget_get_panel_orient(APPLET_WIDGET(applet));

	change_size (TRUE, -1);

	gtk_widget_show (applet);

	unknown_icon = g_new (TasklistIcon, 1);
	unknown_icon->normal = gdk_pixbuf_new_from_xpm_data (unknown_xpm);
	unknown_icon->minimized = tasklist_icon_create_minimized_icon (unknown_icon->normal);

	applet_widget_gtk_main ();

	return 0;
}
