#include <config.h>
#include <gnome.h>

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
void draw_task (TasklistTask *task);
TasklistTask *find_gwmh_task (GwmhTask *gwmh_task);
gboolean desk_notifier (gpointer func_data, GwmhDesk *desk, GwmhDeskInfoMask change_mask);
gboolean task_notifier (gpointer func_data, GwmhTask *gwmh_task, GwmhTaskNotifyType ntype, GwmhTaskInfoMask imask);
gboolean cb_button_press_event (GtkWidget *widget, GdkEventButton *event);
gboolean cb_expose_event (GtkWidget *widget, GdkEventExpose *event);
void create_applet (void);
TasklistTask *task_get_xy (gint x, gint y);
GList *get_visible_tasks (void);
GdkPixbuf *create_minimized_icon (GdkPixbuf *pixbuf);
gint get_horz_rows(void);

GNOME_Panel_OrientType tasklist_orient; /* Tasklist orient */
GtkWidget *handle; /* The handle box */
GtkWidget *applet; /* The applet */
GtkWidget *area; /* The drawing area used to display tasks */
GList *tasks; /* The list of tasks used */

TasklistIcon *unknown_icon; /* The unknown icon */

gint vert_height=0; /* Vertical height, used for resizing */
gint horz_width=0;  /* Horizontal width, used for resizing */

gint panel_size = 48;

extern TasklistConfig Config;

/* from gtkhandlebox.c */
#define DRAG_HANDLE_SIZE 10

/* get the horz_rows depending on the configuration settings */
gint
get_horz_rows(void)
{
	if(Config.follow_panel_size)
		return panel_size/ROW_HEIGHT;
	else
		return Config.horz_rows;
}

/* Create a minimized version of a mini icon.
   This code is stolen from gnome-pixmap2.
 */
GdkPixbuf *
create_minimized_icon (GdkPixbuf *pixbuf)
{
	GdkPixbuf *minimized;
	GdkColor color;
	gint w, h, x, y;
	gint32 red, green, blue;

	/* Get colors */
	color = area->style->bg[0];
	red = color.red / 255;
	blue = color.blue / 255;
	green = color.green / 255;

	w = gdk_pixbuf_get_width (pixbuf);
	h = gdk_pixbuf_get_height (pixbuf);

	minimized = gdk_pixbuf_new (gdk_pixbuf_get_format (pixbuf),
				    gdk_pixbuf_get_has_alpha (pixbuf),
				    gdk_pixbuf_get_bits_per_sample (pixbuf),
				    w, h);

	/* Fade each pixel */
	for (y = 0; y < h; y++) {
		for (x = 0; x < w; x++) {
			guchar *src_pixel;
			guchar *dest_pixel;
			
			src_pixel = pixbuf->art_pixbuf->pixels +
				y * pixbuf->art_pixbuf->rowstride +
				x * (pixbuf->art_pixbuf->has_alpha ? 4 : 3);

			dest_pixel = minimized->art_pixbuf->pixels +
				y * minimized->art_pixbuf->rowstride +
				x * (minimized->art_pixbuf->has_alpha ? 4 : 3);
			dest_pixel [0] = ((src_pixel [0] - red) >> 2) + red;
			dest_pixel [1] = ((src_pixel [1] - green) >> 2) + green;
			dest_pixel [2] = ((src_pixel [2] - blue) >> 2) + blue;
		}
	}

	return minimized;
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
draw_task (TasklistTask *task)
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
				 (Config.show_mini_icons ? 8 : 0) +
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

		gdk_gc_set_clip_mask (area->style->black_gc, icon->mask);
		gdk_gc_set_clip_origin (area->style->black_gc,
					task->x + 3 + (16 - pixbuf->art_pixbuf->width) / 2,
					task->y + (task->height - pixbuf->art_pixbuf->height) / 2);
		
		if (pixbuf->art_pixbuf->has_alpha) {
			gdk_draw_rgb_32_image (area->window,
					       area->style->black_gc,
					       task->x + 3 + (16 - pixbuf->art_pixbuf->width) / 2, 
					       task->y + (task->height - pixbuf->art_pixbuf->height) / 2,
					       pixbuf->art_pixbuf->width,
					       pixbuf->art_pixbuf->height,
					       GDK_RGB_DITHER_NORMAL,
					       pixbuf->art_pixbuf->pixels,
					       pixbuf->art_pixbuf->rowstride);
		}
		else {
			gdk_draw_rgb_image (area->window,
					    area->style->black_gc,
					    task->x + 3 + (16 - pixbuf->art_pixbuf->width) / 2, 
					    task->y + (task->height - pixbuf->art_pixbuf->height) / 2,
					    pixbuf->art_pixbuf->width,
					    pixbuf->art_pixbuf->height,
					    GDK_RGB_DITHER_NORMAL,
					    pixbuf->art_pixbuf->pixels,
					    pixbuf->art_pixbuf->rowstride);
		}

		gdk_gc_set_clip_mask (area->style->black_gc, NULL);

	}
}

/* Layout the tasklist */
void
layout_tasklist (void)
{
	gint j = 0, k = 0, num = 0, p = 0;
	GList *temp_tasks;
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
			
			change_size (FALSE);
			
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
			
			if (j >= ((num + get_horz_rows() - 1) / get_horz_rows())) {
				j = 0;
				k++;
			}
			p++;
		}
		
		if (Config.horz_fixed) {
			curheight = (ROW_HEIGHT * get_horz_rows() - 4) / num_rows;
			curwidth = (Config.horz_width - 4) / num_cols;

		} else {
			curheight = (ROW_HEIGHT * get_horz_rows() - 4) / num_rows;
			curwidth = Config.horz_taskwidth;

			/* If the total width is higher than allowed, 
			   we use the "fixed" way instead */
			if ((curwidth * num_cols) > Config.horz_width)
				curwidth = (Config.horz_width - 4) / num_cols;
		}


		curx = 2;
		cury = 2;


		while (temp_tasks) {
			task = (TasklistTask *) temp_tasks->data;
			
			task->x = curx;
			task->y = cury;
			task->width = curwidth;
			task->height = curheight;
			
			if (Config.horz_fixed) {
				curx += curwidth;
			
				if (curx >= Config.horz_width ||
				    curx + curwidth > Config.horz_width) {
					cury += curheight;
					curx = 2;
				}
			} else {

				curx += curwidth;

				if (curx >= num_cols * curwidth) {
					cury += curheight;
					curx = 2;
				}
			}
			
			if (temp_tasks->next)
				temp_tasks = temp_tasks->next;
			else {
				g_list_free (temp_tasks);
				break;
			}
		}
		if (Config.horz_fixed)
			horz_width = Config.horz_width;
		else
			horz_width = num_cols * curwidth + 4;

		change_size (FALSE);

		break;

	case ORIENT_LEFT:
	case ORIENT_RIGHT:

		if (num == 0) {
			if (Config.vert_fixed)
				vert_height = Config.vert_height;
			else
				vert_height = 4;
			
			change_size (FALSE);
			
			gtk_widget_draw (area, NULL);
			return;
		}

		curheight = ROW_HEIGHT;
		if(Config.follow_panel_size)
			curwidth = panel_size - 4;
		else
			curwidth = Config.vert_width - 4;
		
		num_cols = 1;
		num_rows = num;
		
		curx = 2;
		cury = 2;

		if (Config.vert_fixed)
			vert_height = Config.vert_height;
		else
			vert_height = curheight * num_rows + 4;
		
		change_size (FALSE);

		while (temp_tasks) {
			task = (TasklistTask *) temp_tasks->data;
			
			task->x = curx;
			task->y = cury;
			task->width = curwidth;
			task->height = curheight;
			
			curx += curwidth;

			if (curx >= (Config.follow_panel_size?
				     panel_size:
				     Config.vert_width) - 4) {
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
	if (Config.all_desks_minimized && 
	    Config.all_desks_normal)
		return TRUE;

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
	GdkPixmap *pixmap = NULL;
	GdkBitmap *mask = NULL;
	
	switch (ntype)
	{
	case GWMH_NOTIFY_INFO_CHANGED:
		if (imask & GWMH_TASK_INFO_GSTATE)
			layout_tasklist ();
		if (imask & GWMH_TASK_INFO_ICONIFIED)
			layout_tasklist ();
		if (imask & GWMH_TASK_INFO_FOCUSED)
			draw_task (find_gwmh_task (gwmh_task));
		if (imask & GWMH_TASK_INFO_MISC)
			draw_task (find_gwmh_task (gwmh_task));
		if (imask & GWMH_TASK_INFO_DESKTOP) {
			if (Config.all_desks_minimized && 
			    Config.all_desks_normal)
				break;

			/* Redraw entire tasklist */
			layout_tasklist ();
		}
		break;
	case GWMH_NOTIFY_NEW:
		task = g_malloc0 (sizeof (TasklistTask));
		task->gwmh_task = gwmh_task;
		task->icon = g_new (TasklistIcon, 1);
		gwmh_task_get_mini_icon (task->gwmh_task, 
					 &pixmap, &mask);
		if (pixmap) {
			gint width, height, x, y, depth;
			gdk_window_get_geometry (pixmap,
						 &x, &y,
						 &width, &height,
						 &depth);
			task->icon->mask = mask;
			task->icon->normal = gdk_pixbuf_get_from_drawable (NULL,
									   pixmap,
									   gtk_widget_get_colormap (area),
									   0, 0,
									   0, 0,
									   width, height);
									   
			task->icon->minimized = create_minimized_icon (task->icon->normal);
		}
		else {

			task->icon->mask = unknown_icon->mask;
			task->icon->normal = unknown_icon->normal;
			task->icon->minimized = unknown_icon->minimized;
		}
		tasks = g_list_append (tasks, task);
	        layout_tasklist ();
		break;
	case GWMH_NOTIFY_DESTROY:
		task = find_gwmh_task (gwmh_task);
		tasks = g_list_remove (tasks, task);
		g_free (task->icon);
		g_free (task);
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
		if (!Config.all_desks_minimized)
			gwmh_desk_set_current_area (task->gwmh_task->desktop,
						    task->gwmh_task->harea,
						    task->gwmh_task->varea);

		if (GWMH_TASK_ICONIFIED (task->gwmh_task) || !GWMH_TASK_FOCUSED (task->gwmh_task)) {
			gwmh_task_show (task->gwmh_task);
			/* Why is a focus needed here?
			   gwmh_task_show is supposed to give focus */
			gwmh_task_focus (task->gwmh_task);
		}
		else
		  gwmh_task_iconify (task->gwmh_task);
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
		"Anders Carlsson (andersca@gnu.org)",
		NULL
	};
	
	dialog = gnome_about_new ("Gnome Tasklist",
				  VERSION,
				  "Copyright (C) 1999 Anders Carlsson",
				  authors,
				  "A tasklist for the GNOME desktop environment.\nIcons by Tuomas Kuosmanen (tigert@gimp.org).",
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

/* Changes size of the applet */
void
change_size (gboolean layout)
{
	switch (applet_widget_get_panel_orient (APPLET_WIDGET (applet))) {
	case ORIENT_UP:
	case ORIENT_DOWN:
		GTK_HANDLE_BOX (handle)->handle_position = GTK_POS_LEFT;
		gtk_widget_set_usize (handle, 
				      DRAG_HANDLE_SIZE + horz_width,
				      get_horz_rows() * ROW_HEIGHT);
		gtk_drawing_area_size (GTK_DRAWING_AREA (area), 
				       horz_width,
				       get_horz_rows() * ROW_HEIGHT);
		break;
	case ORIENT_LEFT:
	case ORIENT_RIGHT:
		GTK_HANDLE_BOX (handle)->handle_position = GTK_POS_TOP;
		gtk_widget_set_usize (handle, 
				      (Config.follow_panel_size?
				       panel_size:
				       Config.vert_width),
				      DRAG_HANDLE_SIZE + vert_height);
		gtk_drawing_area_size (GTK_DRAWING_AREA (area), 
				       (Config.follow_panel_size?
					panel_size:
					Config.vert_width),
				       vert_height);
	}
	if (layout)
		layout_tasklist ();
}

/* Called when the panel's orient changes */
static void
cb_change_orient (GtkWidget *widget, GNOME_Panel_OrientType orient)
{
	
	tasklist_orient = orient;

	/* Change size accordingly */
	change_size (TRUE);
}

static void
cb_change_pixel_size (GtkWidget *widget, int size)
{
	panel_size = size;
	
	/* Change size accordingly */
	if(Config.follow_panel_size)
		change_size (TRUE);
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
	gtk_signal_connect (GTK_OBJECT (applet), "save-session",
			    GTK_SIGNAL_FUNC (write_config), NULL);
	gtk_signal_connect (GTK_OBJECT (applet), "change-pixel-size",
			    GTK_SIGNAL_FUNC (cb_change_pixel_size), NULL);

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
	change_size (TRUE);
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

	gdk_rgb_init ();

	gtk_widget_set_default_colormap (gdk_rgb_get_cmap ());
	gtk_widget_set_default_visual (gdk_rgb_get_visual ());

	gwmh_init ();
	gwmh_task_notifier_add (task_notifier, NULL);
	gwmh_desk_notifier_add (desk_notifier, NULL);
	
	create_applet ();

	read_config ();

	unknown_icon = g_new (TasklistIcon, 1);
	/* XXX: why do we need to cast unknown_xpm here,
	   I don't get it -George*/
	unknown_icon->normal = gdk_pixbuf_new_from_xpm_data ((const gchar **)unknown_xpm);
	unknown_icon->mask = gdk_pixmap_new (NULL, 16, 16, 1);

	gdk_pixbuf_render_threshold_alpha (unknown_icon->normal,
					   unknown_icon->mask,
					   0, 0, 0, 0, 16, 16, 128);

	unknown_icon->minimized = create_minimized_icon (unknown_icon->normal);

	applet_widget_gtk_main ();

	return 0;
}
