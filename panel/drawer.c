/*
 * GNOME panel drawer module.
 * (C) 1997 The Free Software Foundation
 *
 * Authors: Miguel de Icaza
 *          Federico Mena
 *          George Lebl
 */

#include <config.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include "gnome.h"
#include "panel-widget.h"
#include "panel.h"
#include "panel_config_global.h"
#include "drawer.h"


extern GlobalConfig global_config;

/*taken from panel-widget*/
static void
move_window(GtkWidget *widget, int x, int y)
{
	gdk_window_set_hints(widget->window, x, y, 0, 0, 0, 0, GDK_HINT_POS);
	gdk_window_move(widget->window, x, y);
	gtk_widget_draw(widget, NULL); /* FIXME: this should draw only the newly exposed area! */
}

void
reposition_drawer(Drawer *drawer)
{
	gint x=0,y=0;
	gint wx, wy;

	gdk_window_get_origin (drawer->button->window, &wx, &wy);

	switch(drawer->orient) {
		case DRAWER_UP:
			x = wx;
			y = wy - drawer->drawer->allocation.height;
			break;
		case DRAWER_DOWN:
			x = wx;
			y = wy + drawer->button->allocation.height;
			break;
		case DRAWER_LEFT:
			x = wx - drawer->drawer->allocation.width;
			y = wy;
			break;
		case DRAWER_RIGHT:
			x = wx + drawer->button->allocation.width;
			y = wy;
			break;
	}

	move_window(drawer->drawer,x,y);
}

static gint
drawer_click(GtkWidget *widget, gpointer data)
{
	Drawer *drawer = data;

	reposition_drawer(drawer);

	if(PANEL_WIDGET(drawer->drawer)->state == PANEL_SHOWN)
		panel_widget_close_drawer(PANEL_WIDGET(drawer->drawer));
	else
		panel_widget_open_drawer(PANEL_WIDGET(drawer->drawer));
	return TRUE;
}

static void
destroy_drawer(GtkWidget *widget, gpointer data)
{
	Drawer *drawer = data;
	g_free(drawer);
}

Drawer *
create_drawer_applet(GtkWidget * drawer_panel, DrawerOrient orient)
{
	GtkWidget *pixmap;
	Drawer *drawer;
	
	char *pixmap_name=NULL;

	drawer = g_new(Drawer,1);

	/*FIXME: drawers should have their own pixmaps I guess*/
	switch(orient) {
		case DRAWER_DOWN:
			pixmap_name = gnome_unconditional_pixmap_file ("gnome-menu-down.xpm");
			break;
		case DRAWER_UP:
			pixmap_name = gnome_unconditional_pixmap_file ("gnome-menu-up.xpm");
			break;
		case DRAWER_RIGHT:
			pixmap_name = gnome_unconditional_pixmap_file ("gnome-menu-right.xpm");
			break;
		case DRAWER_LEFT:
			pixmap_name = gnome_unconditional_pixmap_file ("gnome-menu-left.xpm");
			break;
	}
		
	drawer->orient = orient;

	/* main button */
	drawer->button = gtk_button_new ();
	
	/*make the pixmap*/
	pixmap = gnome_pixmap_new_from_file (pixmap_name);
	gtk_widget_show(pixmap);
	/*FIXME:this is not right, but it's how we can get the buttons to
	  be 48x48 (given the icons are 48x48)*/
	gtk_widget_set_usize (drawer->button,48,48);
	/*gtk_widget_set_usize (drawer->button, pixmap->requisition.width,
			      pixmap->requisition.height);*/

	/* put pixmap in button */
	gtk_container_add (GTK_CONTAINER(drawer->button), pixmap);
	gtk_widget_show (drawer->button);

	drawer->drawer = drawer_panel;

	gtk_signal_connect (GTK_OBJECT (drawer->button), "clicked",
			    GTK_SIGNAL_FUNC (drawer_click), drawer);
	gtk_signal_connect (GTK_OBJECT (drawer->button), "destroy",
			    GTK_SIGNAL_FUNC (destroy_drawer), drawer);

	gtk_object_set_user_data(GTK_OBJECT(drawer->button),drawer);

	if(PANEL_WIDGET(drawer_panel)->state == PANEL_SHOWN)
		gtk_widget_show(drawer_panel);
	else
		gtk_widget_hide(drawer_panel);

	gtk_object_set_data(GTK_OBJECT(drawer_panel),DRAWER_PANEL,drawer);

	g_free (pixmap_name);
	return drawer;
}

Drawer *
create_empty_drawer_applet(DrawerOrient orient)
{
	switch(orient) {
	case DRAWER_UP:
		return create_drawer_applet(panel_widget_new(0,
						PANEL_VERTICAL,
						PANEL_DRAWER,
						PANEL_EXPLICIT_HIDE,
						PANEL_SHOWN,
						0, 0, 
						DROP_ZONE_LEFT),
					    orient);
	case DRAWER_DOWN:
		return create_drawer_applet(panel_widget_new(0,
						PANEL_VERTICAL,
						PANEL_DRAWER,
						PANEL_EXPLICIT_HIDE,
						PANEL_SHOWN,
						0, 0, 
						DROP_ZONE_RIGHT),
					    orient);
	case DRAWER_LEFT:
		return create_drawer_applet(panel_widget_new(0,
						PANEL_HORIZONTAL,
						PANEL_DRAWER,
						PANEL_EXPLICIT_HIDE,
						PANEL_SHOWN,
						0, 0, 
						DROP_ZONE_LEFT),
					    orient);
	case DRAWER_RIGHT:
		return create_drawer_applet(panel_widget_new(0,
						PANEL_HORIZONTAL,
						PANEL_DRAWER,
						PANEL_EXPLICIT_HIDE,
						PANEL_SHOWN,
						0, 0, 
						DROP_ZONE_RIGHT),
					    orient);
	}
	return NULL;
}

void
set_drawer_applet_orient(Drawer *drawer, DrawerOrient orient)
{
	GtkWidget *pixmap;
	char *pixmap_name=NULL;

	g_return_if_fail(drawer!=NULL);

	drawer->orient = orient;

	switch (drawer->orient) {
		case DRAWER_DOWN:
			pixmap_name = gnome_unconditional_pixmap_file(
				"gnome-menu-down.xpm");
			panel_widget_change_orient(PANEL_WIDGET(drawer->drawer),
						   PANEL_VERTICAL);
			break;
		case DRAWER_UP:
			pixmap_name = gnome_unconditional_pixmap_file(
				"gnome-menu-up.xpm");
			panel_widget_change_orient(PANEL_WIDGET(drawer->drawer),
						   PANEL_VERTICAL);
			break;
		case DRAWER_RIGHT:
			pixmap_name = gnome_unconditional_pixmap_file(
				"gnome-menu-right.xpm");
			panel_widget_change_orient(PANEL_WIDGET(drawer->drawer),
						   PANEL_HORIZONTAL);
			break;
		case DRAWER_LEFT:
			pixmap_name = gnome_unconditional_pixmap_file(
				"gnome-menu-left.xpm");
			panel_widget_change_orient(PANEL_WIDGET(drawer->drawer),
						   PANEL_HORIZONTAL);
			break;
	}
		
	pixmap=GTK_BUTTON(drawer->button)->child;
	gtk_container_remove(GTK_CONTAINER(drawer->button),pixmap);

	/*make the pixmap*/
	pixmap = gnome_pixmap_new_from_file (pixmap_name);

	gtk_container_add (GTK_CONTAINER(drawer->button), pixmap);
	gtk_widget_show (pixmap);
	
	g_free(pixmap_name);
}
