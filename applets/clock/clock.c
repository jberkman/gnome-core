/*
 * GNOME time/date display module.
 * (C) 1997 The Free Software Foundation
 *
 * Authors: Miguel de Icaza
 *          Federico Mena
 *
 * Feel free to implement new look and feels :-)
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
#include "panel.h"
#include "mico-parse.h"

#define CLOCK_DATA "clock_data"

typedef void (*ClockUpdateFunc) (GtkWidget * clock, time_t current_time);

GtkWidget *plug = NULL;

int applet_id = (-1); /*this is our id we use to comunicate with the panel */


typedef struct {
	int timeout;
	ClockUpdateFunc update_func;
} ClockData;

typedef struct {
	GtkWidget *date;
	GtkWidget *time;
} ComputerClock;


static void
free_data(GtkWidget * widget, gpointer data)
{
	g_free(data);
}

static int
clock_timeout_callback(gpointer data)
{
	time_t current_time;
	GtkWidget *clock;
	ClockData *cd;

	time(&current_time);

	clock = data;
	cd = gtk_object_get_data(GTK_OBJECT(clock), CLOCK_DATA);

	(*cd->update_func) (clock, current_time);

	return 1;
}

static void
computer_clock_update_func(GtkWidget * clock, time_t current_time)
{
	ComputerClock *cc;
	char *strtime;
	char date[20], hour[20];

	cc = gtk_object_get_user_data(GTK_OBJECT(clock));

	strtime = ctime(&current_time);

	strncpy(date, strtime, 10);
	date[10] = '\0';
	gtk_label_set(GTK_LABEL(cc->date), date);

	strtime += 11;
	strncpy(hour, strtime, 5);
	hour[5] = '\0';
	gtk_label_set(GTK_LABEL(cc->time), hour);
}

static void
create_computer_clock_widget(GtkWidget ** clock, ClockUpdateFunc * update_func)
{
	GtkWidget *frame;
	GtkWidget *align;
	GtkWidget *vbox;
	ComputerClock *cc;

	frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
	gtk_widget_show(frame);

	align = gtk_alignment_new(0.5, 0.5, 0.0, 0.0);
	gtk_container_border_width(GTK_CONTAINER(align), 4);
	gtk_container_add(GTK_CONTAINER(frame), align);
	gtk_widget_show(align);

	vbox = gtk_vbox_new(FALSE, FALSE);
	gtk_container_add(GTK_CONTAINER(align), vbox);
	gtk_widget_show(vbox);

	cc = g_new(ComputerClock, 1);
	cc->date = gtk_label_new("");
	cc->time = gtk_label_new("");

	gtk_box_pack_start_defaults(GTK_BOX(vbox), cc->date);
	gtk_box_pack_start_defaults(GTK_BOX(vbox), cc->time);
	gtk_widget_show(cc->date);
	gtk_widget_show(cc->time);

	gtk_object_set_user_data(GTK_OBJECT(frame), cc);
	gtk_signal_connect(GTK_OBJECT(frame), "destroy",
			   (GtkSignalFunc) free_data,
			   cc);

	*clock = frame;
	*update_func = computer_clock_update_func;
}

static void
destroy_clock(GtkWidget * widget, void *data)
{
	ClockData *cd;

	cd = gtk_object_get_data(GTK_OBJECT(widget), CLOCK_DATA);

	gtk_timeout_remove(cd->timeout);

	g_free(cd);
}

static GtkWidget *
create_clock_widget(void)
{
	ClockData *cd;
	GtkWidget *clock;
	time_t current_time;

	cd = g_new(ClockData, 1);

	/*FIXME: different clock types here */
	create_computer_clock_widget(&clock, &cd->update_func);

	/* Install timeout handler */

	cd->timeout = gtk_timeout_add(3000, clock_timeout_callback, clock);

	gtk_object_set_data(GTK_OBJECT(clock), CLOCK_DATA, cd);

	gtk_signal_connect(GTK_OBJECT(clock), "destroy",
			   (GtkSignalFunc) destroy_clock,
			   NULL);
	/* Call the clock's update function so that it paints its first state */

	time(&current_time);

	(*cd->update_func) (clock, current_time);

	return clock;
}

/*these are commands sent over corba: */
void
change_orient(int id, int orient)
{
	PanelOrientType o = (PanelOrientType) orient;
}

void
session_save(int id, const char *cfgpath, const char *globcfgpath)
{
	/*save the session here */
}

static gint
quit_clock(gpointer data)
{
	exit(0);
}

void
shutdown_applet(int id)
{
	/*kill our plug using destroy to avoid warnings we need to
	   kill the plug but we also need to return from this call */
	if (plug)
		gtk_widget_destroy(plug);
	gtk_idle_add(quit_clock, NULL);
}


/*
void
test_callback(int id, gpointer data)
{
	puts("TEST");
}
*/


int
main(int argc, char **argv)
{
	GtkWidget *clock;
	char *result;
	char *cfgpath;
	char *globcfgpath;

	char *myinvoc;
	guint32 winid;

	myinvoc = get_which_output(argv[0]);
	if(!myinvoc)
		return 1;

	panel_corba_register_arguments();
	gnome_init("clock_applet", NULL, argc, argv, 0, NULL);

	if (!gnome_panel_applet_init_corba())
		g_error("Could not comunicate with the panel\n");

	result = gnome_panel_applet_request_id(myinvoc, &applet_id,
					       &cfgpath, &globcfgpath,
					       &winid);

	g_free(myinvoc);
	if (result)
		g_error("Could not talk to the Panel: %s\n", result);
	/*use cfg path for loading up data! */

	g_free(globcfgpath);
	g_free(cfgpath);

	plug = gtk_plug_new(winid);

	clock = create_clock_widget();
	gtk_widget_show(clock);
	gtk_container_add(GTK_CONTAINER(plug), clock);
	gtk_widget_show(plug);


	result = gnome_panel_applet_register(plug, applet_id);
	if (result)
		g_error("Could not talk to the Panel: %s\n", result);

/*
	gnome_panel_applet_register_callback(applet_id,
					     "test",
					     "TEST CALLBACK",
					     test_callback,
					     NULL);
*/

	applet_corba_gtk_main("IDL:GNOME/Applet:1.0");

	return 0;
}
