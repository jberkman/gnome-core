#include <config.h>
#include "tasklist_applet.h"
#include "unknown.xpm"

/* Prototypes */
static void cb_properties (void);
static void cb_about (void);
gchar *fixup_task_label (TasklistTask *task);
gboolean is_task_visible (TasklistTask *task);
void draw_task (TasklistTask *task);
void layout_tasklist (void);
TasklistTask *find_gwmh_task (GwmhTask *gwmh_task);
gboolean desk_notifier (gpointer func_data, GwmhDesk *desk, GwmhDeskInfoMask change_mask);
gboolean task_notifier (gpointer func_data, GwmhTask *gwmh_task, GwmhTaskNotifyType ntype, GwmhTaskInfoMask imask);
gboolean cb_button_press_event (GtkWidget *widget, GdkEventButton *event);
gboolean cb_expose_event (GtkWidget *widget, GdkEventExpose *event);
void create_applet (void);
TasklistTask *task_get_xy (gint x, gint y);
GList *get_visible_tasks (void);

GtkWidget *handle; /* The handle box */
GtkWidget *applet; /* The applet */
GtkWidget *area; /* The drawing area used to display tasks */
GList *tasks; /* The list of tasks used */
GdkPixmap *unknown_icon; /* Unknown icon */
GdkBitmap *unknown_mask; /* Unknown mask */

extern TasklistConfig Config;

/* from gtkhandlebox.c */
#define DRAG_HANDLE_SIZE 10

/* Shorten a label that is too long */
gchar *
fixup_task_label (TasklistTask *task)
{
	gchar *str;
	gint len, label_len;

	label_len = gdk_string_width (area->style->font,
				      task->gwmh_task->name);

	if (label_len > task->width - ROW_HEIGHT) {
		len = strlen (task->gwmh_task->name);
		len--;
		str = g_malloc (len + 4);
		strcpy (str, task->gwmh_task->name);
		strcat (str, "..");
		for (; len > 0; len--) {
			str[len] = '.';
			str[len + 3] = '\0';
			
			label_len = gdk_string_width (area->style->font, str);
			
			if (label_len <= task->width - (Config.show_pixmaps ? 24:6))
				break;
		}
	}
	else
		str = g_strdup (task->gwmh_task->name);

	return str;
}

/* Check what task (if any) is at position x,y on the tasklist */
TasklistTask *
task_get_xy (gint x, gint y)
{
	GList *temp_tasks;
	TasklistTask *task;

	temp_tasks = get_visible_tasks ();

	while (temp_tasks) {
		task = (TasklistTask *)temp_tasks->data;
		if (x > task->x &&
		    x < task->x + task->width &&
		    y > task->y &&
		    y < task->y + task->height) {
			g_list_free (temp_tasks);
			return task;
		}
		temp_tasks = temp_tasks->next;
	}

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
			visible_tasks = g_list_append (visible_tasks, temp_tasks->data);
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
	
	if (!GWMH_TASK_STICKY (task->gwmh_task))
		if (task->gwmh_task->desktop != desk_info->current_desktop)
			return FALSE;

	return TRUE;
}

/* Draw a single task */
void
draw_task (TasklistTask *task)
{
	gchar *tempstr;
	gint text_height, text_width;
	
	if (!is_task_visible (task))
		return;

	gtk_paint_box (area->style, area->window,
		       GWMH_TASK_FOCUSED (task->gwmh_task) ?
		       GTK_STATE_ACTIVE : GTK_STATE_NORMAL,
		       GWMH_TASK_FOCUSED (task->gwmh_task) ?
		       GTK_SHADOW_IN : GTK_SHADOW_OUT,
		       NULL, area, "button",
		       task->x, task->y,
		       task->width, task->height);

	if (task->gwmh_task->name) {
		tempstr = fixup_task_label (task);
		text_height = gdk_string_height (area->style->font, "1");
		text_width = gdk_string_width (area->style->font, tempstr);
		gdk_draw_string (area->window,
				 area->style->font, area->style->black_gc,
				 task->x +
				 (Config.show_pixmaps ? 8 : 0) +
				 ((task->width - text_width) / 2),
				 task->y + ((task->height - text_height) / 2) + text_height,
				 tempstr);

#if 0
		gdk_draw_rectangle (area->window,
				    area->style->black_gc,
				    FALSE,
				    task->x + 
				    (Config.show_pixmaps ? 8 : 0) +
				    ((task->width - text_width) / 2),
				    task->y + ((task->height - text_height) / 2),
				    text_width,
				    text_height);
#endif 
		g_free (tempstr);
	}

	if (Config.show_pixmaps) {
		if (task->pixmap) {
			if (task->mask) {
				gdk_gc_set_clip_mask (area->style->black_gc, 
						      task->mask);

				gdk_gc_set_clip_origin (area->style->black_gc,
							task->x + 3,
							task->y + (task->height - 16) / 2);
			}
			
			gdk_draw_pixmap (area->window,
					 area->style->black_gc,
					 task->pixmap,
					 0, 0,
					 task->x + 3, 
					 task->y + (task->height - 16) / 2,
					 16, 16);
			if (task->mask)
				gdk_gc_set_clip_mask (area->style->black_gc, NULL);
		}
		else {
			gdk_gc_set_clip_mask (area->style->black_gc, unknown_mask);
			gdk_gc_set_clip_origin (area->style->black_gc,
						task->x + 3,
						task->y + (task->height - 16) / 2);
			
			gdk_draw_pixmap (area->window,
					 area->style->black_gc,
					 unknown_icon,
					 0, 0,
					 task->x + 3, task->y + (task->height - 16) / 2,
					 16, 16);
			gdk_gc_set_clip_mask (area->style->black_gc, NULL);
		}
	}
}

/* Layout the tasklist */
void
layout_tasklist (void)
{
	gint j = 0, k = 0, num = 0, p = 0;
	GList *temp_tasks;
	TasklistTask *task;
	gint extra_space;
	gint num_rows = 0, num_cols = 0;
	gint curx = 0, cury = 0, curwidth = 0, curheight = 0;
	
	temp_tasks = get_visible_tasks ();
	num = g_list_length (temp_tasks);
	
	if (num == 0) {
		gtk_widget_draw (area, NULL);
		return;
	}

	switch (applet_widget_get_panel_orient (APPLET_WIDGET (applet))) {
	case ORIENT_UP:
	case ORIENT_DOWN:
		while (p < num) {
			if (num < Config.rows)
				num_rows = num;
			
			j++;
			if (num_cols < j)
				num_cols = j;
			if (num_rows < k + 1)
				num_rows = k + 1;
			
			if (j >= ((num + Config.rows - 1) / Config.rows)) {
				j = 0;
				k++;
			}
			p++;
		}

		curheight = (ROW_HEIGHT * Config.rows - 4) / num_rows;
		curwidth = (Config.width - 4) / num_cols;

		curx = 2;
		cury = 2;

		extra_space = Config.width - 4 - (curwidth * num_cols);
		/* FIXME: Do something with extra_space */

		while (temp_tasks) {
			task = (TasklistTask *) temp_tasks->data;
			
			task->x = curx;
			task->y = cury;
			task->width = curwidth;
			task->height = curheight;
			
			curx += curwidth;
			if (curx >= Config.width ||
			    curx + curwidth > Config.width) {
				cury += curheight;
				curx = 2;
			}
			
			if (temp_tasks->next)
				temp_tasks = temp_tasks->next;
			else {
				g_list_free (temp_tasks);
				break;
			}
		}
		break;

	case ORIENT_LEFT:
	case ORIENT_RIGHT:
		
		curheight = ROW_HEIGHT;
		curwidth = Config.horz_width - 4;
		
		num_cols = 1;
		num_rows = num;
		
		curx = 2;
		cury = 2;

		while (temp_tasks) {
			task = (TasklistTask *) temp_tasks->data;
			
			task->x = curx;
			task->y = cury;
			task->width = curwidth;
			task->height = curheight;
			
			curx += curwidth;
			if (curx >= Config.horz_width - 4) {
				cury += curheight;
				curx = 2;
			}
			
			if (temp_tasks->next)
				temp_tasks = temp_tasks->next;
			else {
				g_list_free (temp_tasks);
				break;
			}
		}
		break;
	}

	
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
	layout_tasklist ();

	return TRUE;
}

/* This routine gets called when tasks are created/destroyed etc */
gboolean
task_notifier (gpointer func_data, GwmhTask *gwmh_task,
	       GwmhTaskNotifyType ntype,
	       GwmhTaskInfoMask imask)
{
	TasklistTask *task;

	switch (ntype)
	{
	case GWMH_NOTIFY_INFO_CHANGED:
		if (imask & GWMH_TASK_INFO_FOCUSED)
			draw_task (find_gwmh_task (gwmh_task));
		if (imask & GWMH_TASK_INFO_MISC)
			draw_task (find_gwmh_task (gwmh_task));
		if (imask & GWMH_TASK_INFO_DESKTOP)
			/* Redraw entire tasklist */
			layout_tasklist ();
		break;
	case GWMH_NOTIFY_NEW:
		task = g_malloc0 (sizeof (TasklistTask));
		task->gwmh_task = gwmh_task;
		gwmh_task_get_mini_icon (task->gwmh_task, 
					 &task->pixmap, &task->mask);

		tasks = g_list_append (tasks, task);
	        layout_tasklist ();
		break;
	case GWMH_NOTIFY_DESTROY:
		tasks = g_list_remove (tasks, find_gwmh_task (gwmh_task));
		layout_tasklist ();
		break;
	default:
		g_print ("Unknown ntype: %d\n", ntype);
	}

	return TRUE;
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
		if (GWMH_TASK_ICONIFIED (task->gwmh_task))
			gwmh_task_show (task->gwmh_task);
		gwmh_task_show (task->gwmh_task);
	}

	if (event->button == 3) {
		gtk_signal_emit_stop_by_name (GTK_OBJECT (widget),
					      "button_press_event");
		menu_popup (task, event->button, event->time);
		return TRUE;
	}

	return FALSE;
}

/* This routine gets called when the tasklist is exposed */
gboolean
cb_expose_event (GtkWidget *widget, GdkEventExpose *event)
{
	GList *temp_tasks;
	TasklistTask *task;

	temp_tasks = get_visible_tasks ();

	gtk_paint_box (area->style, area->window,
		       GTK_STATE_NORMAL, GTK_SHADOW_IN,
		       NULL, area, "button",
		       0, 0,
		       area->allocation.width,
		       area->allocation.height);


	while (temp_tasks) {
		task = (TasklistTask *)temp_tasks->data;
		draw_task (task);
		temp_tasks = temp_tasks->next;
	}

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
	GtkWidget *dialog;

	const char *authors[] = {
		"Anders Carlsson (anders.carlsson@tordata.se)",
		NULL
	};
	
	dialog = gnome_about_new ("Gnome Tasklist",
				  VERSION,
				  "Copyright (C) 1999 Anders Carlsson",
				  authors,
				  "A tasklist for the GNOME desktop environment",
				  NULL);
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
		gtk_signal_emit_stop_by_name (GTK_OBJECT (widget), "event");
		return TRUE;
	}
	return FALSE;
}

/* Changes size of the applet */
void
change_size (void)
{
	switch (applet_widget_get_panel_orient (APPLET_WIDGET (applet))) {
	case ORIENT_UP:
	case ORIENT_DOWN:
		GTK_HANDLE_BOX (handle)->handle_position = GTK_POS_LEFT;
		gtk_widget_set_usize (handle, 
				      DRAG_HANDLE_SIZE + Config.width,
				      Config.rows * ROW_HEIGHT);
		gtk_drawing_area_size (GTK_DRAWING_AREA (area), 
				       Config.width,
				       Config.rows * ROW_HEIGHT);
		layout_tasklist ();
		break;
	case ORIENT_LEFT:
	case ORIENT_RIGHT:
		GTK_HANDLE_BOX (handle)->handle_position = GTK_POS_TOP;
		gtk_widget_set_usize (handle, 
				      Config.horz_width,
				      DRAG_HANDLE_SIZE + Config.height);
		gtk_drawing_area_size (GTK_DRAWING_AREA (area), 
				       Config.horz_width,
				       Config.height);
		layout_tasklist ();
	}
}

static gboolean
cb_change_orient (GtkWidget *widget, GNOME_Panel_OrientType orient)
{
	/* Change size accordingly */
	change_size ();

	return FALSE;
}
 
/* Create the applet */
void
create_applet (void)
{
	GtkWidget *hbox;

	applet = applet_widget_new ("tasklist_applet");
	
	hbox = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (hbox);
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

	gtk_signal_connect (GTK_OBJECT (applet), "change-orient",
			    GTK_SIGNAL_FUNC (cb_change_orient), NULL);
	applet_widget_register_stock_callback (APPLET_WIDGET (applet),
					       "about",
					       GNOME_STOCK_MENU_ABOUT,
					       _("About..."),
					       (AppletCallbackFunc) cb_about,
					       NULL);
	applet_widget_register_stock_callback (APPLET_WIDGET (applet),
					       "properties",
					       GNOME_STOCK_MENU_PROP,
					       _("Properties..."),
					       (AppletCallbackFunc) cb_properties,
					       NULL);
	change_size ();
	gtk_widget_show (applet);
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
	gwmh_init ();
	gwmh_task_notifier_add (task_notifier, NULL);
	gwmh_desk_notifier_add (desk_notifier, NULL);
	
	create_applet ();

	read_config ();

	/* FIXME: Move this elsewhere */
	unknown_icon = gdk_pixmap_create_from_xpm_d (area->window, &unknown_mask,
						     NULL, unknown_xpm);

	applet_widget_gtk_main ();

	return 0;
}

#include "gwmh.c"
#include "gstc.c"




