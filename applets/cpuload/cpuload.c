/* GNOME cpuload panel applet
 * (C) 1997 The Free Software Foundation
 *
 * Author: Tim P. Gerla
 * With code from wmload.c, v0.9.2, apparently by Ryan Land, rland@bc1.com.
 *
 */

#include <stdio.h>
#ifdef HAVE_LIBINTL
#    include <libintl.h>
#endif
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <time.h>
#include <gnome.h>
#include <gdk/gdkx.h>
#include "applet-lib.h"
#include "applet-widget.h"

#include "linux-proc.h"
#include "properties.h"

void start_timer( void );

GtkWidget *cpuload;
GdkPixmap *pixmap;
GtkWidget *disp;
GdkGC *gc;
GdkColor ucolor, scolor;
cpuload_properties props;
int height, width, timer_index=-1;

guchar udata[128];
guchar oudata[128];
guchar sdata[128];
guchar osdata[128];

int draw(void)
{
	int usr=0, sys=0, nice=0, free=0, i;

	GetLoad(height, &usr, &nice, &sys, &free );

	for( i=0; i < width-1; i++ )
	{
		udata[i+1] = oudata[i];
		sdata[i+1] = osdata[i];
	}
	udata[0] = usr;
	sdata[0] = sys;
	
	/* Erase Rectangle */
	gdk_draw_rectangle( pixmap,
		disp->style->black_gc,
		TRUE, 0,0,
		disp->allocation.width,
		disp->allocation.height );
	
	gdk_gc_set_foreground( gc, &ucolor );
	for( i=0; i < width; i++ )
	{
		if( udata[i] )
			gdk_draw_line( pixmap,
			       gc,
			       i,height,
			       i,(height-udata[i]) );
	}
	
	gdk_gc_set_foreground( gc, &scolor );
	for( i=0; i < width; i++ )
	{
		if( sdata[i] )
			gdk_draw_line( pixmap,
			       gc,
			       i,(height-udata[i]),
			       i,(height-udata[i])-sdata[i] );
	}
	gdk_draw_pixmap(disp->window,
		disp->style->fg_gc[GTK_WIDGET_STATE(disp)],
	        pixmap,
	        0, 0,
	        0, 0,
	        disp->allocation.width,
	        disp->allocation.height);

	for( i=0; i < width; i++ )
	{
		oudata[i] = udata[i];
		osdata[i] = sdata[i];
	}
	return TRUE;
}

static gint cpuload_configure(GtkWidget *widget, GdkEventConfigure *event)
{
        pixmap = gdk_pixmap_new( widget->window,
                                 widget->allocation.width,
                                 widget->allocation.height,
                                 gtk_widget_get_visual(disp)->depth );
        gdk_draw_rectangle( pixmap,
                            widget->style->black_gc,
                            TRUE, 0,0,
                            widget->allocation.width,
                            widget->allocation.height );
        gdk_draw_pixmap(widget->window,
                disp->style->fg_gc[GTK_WIDGET_STATE(widget)],
                pixmap,
                0, 0,
                0, 0,
                disp->allocation.width,
                disp->allocation.height);
	return TRUE;
} 

static gint cpuload_expose(GtkWidget *widget, GdkEventExpose *event)
{
        gdk_draw_pixmap(widget->window,
                widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
                pixmap,
                event->area.x, event->area.y,
                event->area.x, event->area.y,
                event->area.width, event->area.height);
        return FALSE;
}

GtkWidget *cpuload_new( void )
{
	GtkWidget *frame, *box;

	box = gtk_vbox_new(FALSE, FALSE);
	gtk_widget_show(box);

	frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type( GTK_FRAME(frame), props.look?GTK_SHADOW_OUT:GTK_SHADOW_IN );

	disp = gtk_drawing_area_new();
	gtk_signal_connect( GTK_OBJECT(disp), "expose_event",
                (GtkSignalFunc)cpuload_expose, NULL);
        gtk_signal_connect( GTK_OBJECT(disp),"configure_event",
                (GtkSignalFunc)cpuload_configure, NULL);
        gtk_widget_set_events( disp, GDK_EXPOSURE_MASK );

	gtk_box_pack_start_defaults( GTK_BOX(box), disp );
	gtk_container_add( GTK_CONTAINER(frame), box );

	gtk_widget_set_usize(disp, width, height);

	start_timer();
        
        gtk_widget_show_all(frame);
	return frame;
}

void start_timer( void )
{
	if( timer_index != -1 )
		gtk_timeout_remove(timer_index);

	timer_index = gtk_timeout_add(props.speed, (GtkFunction)draw, NULL);
}

void setup_colors(void)
{
	GdkColormap *colormap;

	colormap = gtk_widget_get_colormap(disp);
                
        gdk_color_parse(props.ucolor, &ucolor);
        gdk_color_alloc(colormap, &ucolor);

        gdk_color_parse(props.scolor, &scolor);
        gdk_color_alloc(colormap, &scolor);
}
	        
void create_gc(void)
{
        gc = gdk_gc_new( disp->window );
        gdk_gc_copy( gc, disp->style->white_gc );
}

static gint destroy_applet(GtkWidget *widget, gpointer data)
{
        gtk_exit(0);
        return FALSE;
}

int main(int argc, char **argv)
{
	GtkWidget *applet;

        panel_corba_register_arguments();

        gnome_init("cpuload_applet", NULL, argc, argv, 0, NULL);
        load_properties(&props);

	height = props.height;
	width = props.width;
        
	applet = applet_widget_new(argv[0]);
	if (!applet)
		g_error("Can't create applet!\n");

        cpuload = cpuload_new();
        applet_widget_add( APPLET_WIDGET(applet), cpuload );
        gtk_widget_show(applet);
	
	create_gc();
	setup_colors();
        gtk_signal_connect(GTK_OBJECT(applet),"destroy",
                           GTK_SIGNAL_FUNC(destroy_applet),
                           NULL);

       	applet_widget_register_callback(APPLET_WIDGET(applet),
					"properties",
                                        _("Properties..."),
                                        properties,
                                        NULL);

	applet_widget_gtk_main();
        return 0;
}
