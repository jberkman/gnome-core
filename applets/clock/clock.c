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
#include "applet-widget.h"

typedef struct _ClockData ClockData;
typedef void (*ClockUpdateFunc) (ClockData *, time_t);
struct _ClockData {
	GtkWidget *applet;
	GtkWidget *clockw;
	int timeout;
	ClockUpdateFunc update_func;
	PanelOrientType orient;
};



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
	ClockData *cd = data;
	time_t current_time;

	time(&current_time);

	(*cd->update_func) (cd, current_time);

	return 1;
}

static void
computer_clock_update_func(ClockData *cd, time_t current_time)
{
	ComputerClock *cc;
	char *strtime;
	char date[20], hour[20];

	cc = gtk_object_get_user_data(GTK_OBJECT(cd->clockw));

	strtime = ctime(&current_time);

	if(cd->orient == ORIENT_LEFT || cd->orient == ORIENT_RIGHT)
		strtime[3] ='\n';
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
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
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
	ClockData *cd = data;
	gtk_timeout_remove(cd->timeout);
	g_free(cd);
}

static ClockData *
create_clock_widget(GtkWidget *applet)
{
	GtkWidget *clock;
	ClockData *cd;
	time_t current_time;

	cd = g_new(ClockData, 1);

	/*FIXME: different clock types here */
	create_computer_clock_widget(&clock, &cd->update_func);

	cd->clockw = clock;
	cd->applet = applet;

	/* Install timeout handler */

	cd->timeout = gtk_timeout_add(3000, clock_timeout_callback, cd);

	cd->orient = ORIENT_UP;

	gtk_signal_connect(GTK_OBJECT(clock), "destroy",
			   (GtkSignalFunc) destroy_clock,
			   cd);
	/* Call the clock's update function so that it paints its first state */

	time(&current_time);

	(*cd->update_func) (cd, current_time);

	return cd;
}

static gint
destroy_applet(GtkWidget *widget, gpointer data)
{
	/*only die if this was the last applet of this kind*/
	if(applet_widget_get_applet_count()==0)
		gtk_exit(0);
	return FALSE;
}

/*these are commands sent over corba: */

/*this is when the panel orientation changes*/
static void
applet_change_orient(GtkWidget *w, PanelOrientType o, gpointer data)
{
	ClockData *cd = data;
	time_t current_time;

	time(&current_time);
	cd->orient = o;
	(*cd->update_func) (cd, current_time);
}

/*when we get a command to start a new widget*/
static void
applet_start_new_applet(GtkWidget *w, gchar *param, gpointer data)
{
	ClockData *cd;
	GtkWidget *applet;
	gchar *argv0 = data;

	applet = applet_widget_new_multi(argv0);
	if (!applet)
		g_error("Can't create applet!\n");

	cd = create_clock_widget(applet);

	gtk_signal_connect(GTK_OBJECT(applet),"start_new_applet",
			   GTK_SIGNAL_FUNC(applet_start_new_applet),
			   argv0);

	gtk_signal_connect(GTK_OBJECT(applet),"change_orient",
			   GTK_SIGNAL_FUNC(applet_change_orient),
			   cd);

	gtk_signal_connect(GTK_OBJECT(applet),"destroy",
			   GTK_SIGNAL_FUNC(destroy_applet),
			   cd);

	gtk_widget_show(cd->clockw);
	applet_widget_add(APPLET_WIDGET(applet), cd->clockw);
	gtk_widget_show(applet);
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
	ClockData *cd;
	GtkWidget *applet;

	panel_corba_register_arguments();
	gnome_init("clock_applet", NULL, argc, argv, 0, NULL);

	applet = applet_widget_new_multi(argv[0]);
	if (!applet)
		g_error("Can't create applet!\n");

	cd = create_clock_widget(applet);

	/*this will also need to be bound on every new applet we make*/
	gtk_signal_connect(GTK_OBJECT(applet),"start_new_applet",
			   GTK_SIGNAL_FUNC(applet_start_new_applet),
			   argv[0]);

	/*we have to bind change_orient before we do applet_widget_add 
	  since we need to get an initial change_orient signal to set our
	  initial oriantation, and we get that during the _add call*/
	gtk_signal_connect(GTK_OBJECT(applet),"change_orient",
			   GTK_SIGNAL_FUNC(applet_change_orient),
			   cd);

	gtk_signal_connect(GTK_OBJECT(applet),"destroy",
			   GTK_SIGNAL_FUNC(destroy_applet),
			   cd);

	gtk_widget_show(cd->clockw);
	applet_widget_add(APPLET_WIDGET(applet), cd->clockw);
	gtk_widget_show(applet);

/*
	gnome_panel_applet_register_callback(applet_id,
					     "test",
					     "TEST CALLBACK",
					     test_callback,
					     NULL);
*/

	applet_widget_gtk_main();

	return 0;
}
