/*
 * GNOME panel swallow module.
 * (C) 1997 The Free Software Foundation
 *
 * Author: George Lebl
 */

#include <config.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <gnome.h>
#include <gdk/gdkx.h>
#include <X11/Xlib.h>
#include "panel-widget.h"
#include "panel.h"
#include "panel_config_global.h"
#include "swallow.h"
#include "mico-glue.h"


static gulong
get_window_id(Window win, char *title)
{
	Window root_return;
	Window parent_return;
	Window *children;
	unsigned int nchildren;
	int i;
	char *tit;
	gulong wid=-1;

	XQueryTree(GDK_DISPLAY(),
		   win,
		   &root_return,
		   &parent_return,
		   &children,
		   &nchildren);

	for(i=0;i<nchildren;i++) {
		XFetchName(GDK_DISPLAY(),
			   children[i],
			   &tit);
		if(tit) {
			puts(tit);
			if(strcmp(tit,title)==0) {
				XFree(tit);
				wid = children[i];
				break;
			}
			XFree(tit);
		}
	}
	for(i=0;wid==-1 && i<nchildren;i++)
		wid = get_window_id(children[i],title);
	if(children)
		XFree(children);
	return wid;
}

Swallow *
create_swallow_applet(char *arguments, SwallowOrient orient)
{
	Swallow *swallow;
	GtkWidget *w;

	swallow = g_new(Swallow,1);

	swallow->table = gtk_table_new(2,2,FALSE);
	gtk_widget_show(swallow->table);

	swallow->handle_n = gtk_vbox_new(FALSE,0);
	gtk_table_attach(GTK_TABLE(swallow->table),swallow->handle_n,
			 1,2,0,1,
			 GTK_FILL|GTK_EXPAND|GTK_SHRINK,
			 GTK_FILL|GTK_EXPAND|GTK_SHRINK,
			 0,0);

	w = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(w),GTK_SHADOW_OUT);
	gtk_box_pack_start(GTK_BOX(swallow->handle_n),w,TRUE,TRUE,0);
	gtk_widget_show(w);
	w = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(w),GTK_SHADOW_OUT);
	gtk_box_pack_start(GTK_BOX(swallow->handle_n),w,TRUE,TRUE,0);
	gtk_widget_show(w);
	w = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(w),GTK_SHADOW_OUT);
	gtk_box_pack_start(GTK_BOX(swallow->handle_n),w,TRUE,TRUE,0);
	gtk_widget_show(w);

	swallow->handle_w = gtk_hbox_new(FALSE,0);
	gtk_table_attach(GTK_TABLE(swallow->table),swallow->handle_w,
			 0,1,1,2,
			 GTK_FILL|GTK_EXPAND|GTK_SHRINK,
			 GTK_FILL|GTK_EXPAND|GTK_SHRINK,
			 0,0);

	w = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(w),GTK_SHADOW_OUT);
	gtk_box_pack_start(GTK_BOX(swallow->handle_w),w,TRUE,TRUE,0);
	gtk_widget_show(w);
	w = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(w),GTK_SHADOW_OUT);
	gtk_box_pack_start(GTK_BOX(swallow->handle_w),w,TRUE,TRUE,0);
	gtk_widget_show(w);
	w = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(w),GTK_SHADOW_OUT);
	gtk_box_pack_start(GTK_BOX(swallow->handle_w),w,TRUE,TRUE,0);
	gtk_widget_show(w);
	
	swallow->socket=gtk_socket_new();

	gtk_table_attach(GTK_TABLE(swallow->table),swallow->socket,
			 1,2,1,2,
			 GTK_FILL|GTK_EXPAND|GTK_SHRINK,
			 GTK_FILL|GTK_EXPAND|GTK_SHRINK,
			 0,0);

	gtk_widget_show(swallow->socket);

	/*FIXME: add the right window or wait for it or something here*/
	//gtk_widget_set_usize(swallow->socket,48,48);
	{
		long wid;
		char buf[256];
		puts("window name to get:");
		scanf("%s",buf);
		wid = get_window_id(GDK_ROOT_WINDOW(),buf);
		if(wid==-1)
			puts("DANG!");
		else
			gtk_socket_steal(GTK_SOCKET(swallow->socket),wid);
	}


	gtk_object_set_user_data(GTK_OBJECT(swallow->socket),swallow);

	set_swallow_applet_orient(swallow, orient);

	return swallow;
}

void
set_swallow_applet_orient(Swallow *swallow, SwallowOrient orient)
{
	if(orient==SWALLOW_VERTICAL) {
		gtk_widget_show(swallow->handle_n);
		gtk_widget_hide(swallow->handle_w);
	} else {
		gtk_widget_hide(swallow->handle_n);
		gtk_widget_show(swallow->handle_w);
	}
}
