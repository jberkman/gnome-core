#include <config.h>
#include <gdk/gdk.h>
#include <gnome.h>
#include <applet-widget.h>
#include <stdio.h>
#include <math.h>
#include "asclock.h"

void about_dialog(AppletWidget *applet, gpointer data)
{
  GtkWidget *about;
  const gchar *authors[] = {
       "Beat Christen (spiff@longstreet.ch)",
       "Patrick Rogan (rogan@lycos.com)",
                NULL
                };

  about = gnome_about_new (_("ASClock"), 
			ASCLOCK_VERSION,
                        _("(C) 1998 the Free Software Foundation"),
                        authors,
                        _("Who said NeXT is dead?"),
                        NULL);

        gtk_widget_show (about);

        return;
}

static GtkWidget * properties_timezone_render(asclock *my_asclock, GtkWidget *parent, float lat, float lon)
{
  GdkPixmap *pmap;
  GdkBitmap * mask;
  char *fname;
  float yloc;
  char cmd[1024];

  fname = tempnam(NULL, "asclock_globe");

  snprintf(cmd, 1024, 
          "xearth -ppm -night 15 -size '320 320' -mag 0.95 -pos 'fixed 0 %.4f' -nostars > %s",
          lon, fname);

  system(cmd);

  gdk_imlib_load_file_to_pixmap(fname, &pmap, &mask);

  yloc = 160.0 - (sin((lat/180.0)*3.1415926))*160*0.95;
  
  if(strlen(my_asclock->timezone)>0)
   {
     gdk_draw_rectangle(pmap, my_asclock->black_gc, 1, 158, fabs(yloc)-2, 4, 4);
     gdk_draw_rectangle(pmap, my_asclock->white_gc, 1, 159, fabs(yloc)-1, 2, 2);

     if(strncmp(my_asclock->timezone, my_asclock->selected_timezone, MAX_PATH_LEN)!=0)
       gnome_property_box_changed( GNOME_PROPERTY_BOX(my_asclock->pwin));
  }

  if(my_asclock->pic) 
  {
    gtk_pixmap_set(GTK_PIXMAP(my_asclock->pic), pmap, mask);
    gtk_widget_draw_default(my_asclock->pic);
  }
  else
    my_asclock->pic = gtk_pixmap_new(pmap, mask);
  unlink(fname);
  free(fname);
  return NULL;
}

static void theme_selected(GtkWidget *list, gint row, gint column, GdkEventButton *event, gpointer data)
{
  gchar *line;
  GtkStyle *style;
  asclock *my_asclock = (asclock *) data;

  gtk_clist_get_text(GTK_CLIST(list), row, 0, &line);

  strncpy(my_asclock->selected_theme_filename, line, MAX_PATH_LEN);

  if(strncmp(my_asclock->selected_theme_filename, my_asclock->theme_filename, MAX_PATH_LEN)!=0)
    gnome_property_box_changed( GNOME_PROPERTY_BOX(my_asclock->pwin));

}

static void location_selected(GtkWidget *list, gint row, gint column, GdkEventButton *event, gpointer data)
{
  location *my;
  gchar *line;
  asclock *my_asclock = (asclock *)data;

  gtk_clist_get_text(GTK_CLIST(list), row, 0, &line); 

  my = (location *) gtk_clist_get_row_data(GTK_CLIST(list), row);

  gtk_clist_get_text(GTK_CLIST(list), row, 1, &line);

  strncpy(my_asclock->selected_timezone, line, MAX_PATH_LEN);

  properties_timezone_render(my_asclock, list, my->lat, my->lon);

}

static asclock *static_my_asclock;
static void dialog_clicked_cb(GnomeDialog * dialog, gint button_number, 
		       gpointer data)
{
  char cmd[1024];

  switch (button_number) {
  case 0: /* OK button */
    /* replace the /etc/localtime link with this timezone */
    snprintf(cmd, 1024, "rm -f /etc/localtime; ln -s ../usr/share/zoneinfo/%s /etc/localtime", 
	     static_my_asclock->timezone);
    
    system(cmd);
    break;
  case 1: /* Cancel Button */
    break;
  default:
    g_assert_not_reached();
  };
  gnome_dialog_close(dialog);
}
         
static int property_apply_cb(AppletWidget *applet, gpointer data)
{

  if(strncmp(static_my_asclock->timezone, static_my_asclock->selected_timezone, MAX_PATH_LEN)!=0) {
    if(0==getuid())
      {
	GtkWidget * label;
	GtkWidget * dialog;
	
	label  = gtk_label_new(_("Since you are root, would you like to set the system's default timezone?"));
	
	dialog = gnome_dialog_new(_("My Title"), GNOME_STOCK_BUTTON_YES,
				  GNOME_STOCK_BUTTON_NO, 
				  NULL); 
	
	gtk_box_pack_start(GTK_BOX(GNOME_DIALOG(dialog)->vbox),
			   label, TRUE, TRUE, GNOME_PAD);
	
	gtk_signal_connect(GTK_OBJECT(dialog), "clicked",
			   GTK_SIGNAL_FUNC(dialog_clicked_cb),
			   NULL);
	gtk_widget_show(label);
	gtk_widget_show(dialog);
	gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
      }

    strncpy(static_my_asclock->timezone, static_my_asclock->selected_timezone, MAX_PATH_LEN);
#ifdef HAVE_SETENV
     setenv("TZ", static_my_asclock->timezone, TRUE);
#else
#ifdef HAVE_PUTENV
     {
       char line[MAX_PATH_LEN];
       snprintf(line, MAX_PATH_LEN, "TZ=%s", static_my_asclock->timezone);
       putenv(line);
     }
#else
#error neither setenv nor putenv defined
#endif
#endif
    tzset();

  }

  if(strncmp(static_my_asclock->selected_theme_filename, static_my_asclock->theme_filename, MAX_PATH_LEN)!=0)
  {
    if(!loadTheme(static_my_asclock->selected_theme_filename)) return FALSE;

    /* get all pixmaps and store size of the clock pixmap for further usage */
    load_pixmaps(static_my_asclock->window, static_my_asclock->style);
    postconfig();

    set_clock_pixmap();
  
    strncpy(static_my_asclock->theme_filename, static_my_asclock->selected_theme_filename, MAX_PATH_LEN);
  }

  if(static_my_asclock->showampm != static_my_asclock->selected_showampm)
  {
    static_my_asclock->showampm = static_my_asclock->selected_showampm;
    set_clock_pixmap();
  }
  static_my_asclock->itblinks = static_my_asclock->selected_itblinks;

  return FALSE;
}

static int showampm_selected_cb(GtkWidget *b, gpointer data)
{
  asclock *my_asclock = (asclock *) data;
  
  my_asclock->selected_showampm = GTK_TOGGLE_BUTTON (b)->active;

  gnome_property_box_changed( GNOME_PROPERTY_BOX(my_asclock->pwin));

  return FALSE;
}

static int itblinks_selected_cb(GtkWidget *b, gpointer data)
{
  asclock *my_asclock = (asclock *) data;
  
  my_asclock->selected_itblinks = GTK_TOGGLE_BUTTON (b)->active;

  gnome_property_box_changed( GNOME_PROPERTY_BOX(my_asclock->pwin));

  return FALSE;
}

static int property_close_cb(AppletWidget *applet, gpointer data)
{
  asclock *my_asclock = (asclock *)data;

  my_asclock->pwin = NULL;
  my_asclock->pic = NULL;

  return FALSE;
}

void properties_dialog(AppletWidget *applet, gpointer data)
{
  gchar *timezone_titles[2] = { N_("Continent/City") , NULL};
  gchar *themes_titles[2] = { N_("Clock Theme"), NULL};
  GtkWidget *label;
  GtkWidget *frame;
  GtkWidget *hbox;
  GtkWidget *opts;
  GtkWidget *list;
  GtkWidget *scroll_win;
  asclock *my_asclock = (asclock *)data;

/*
  GtkWidget *calendar;
  time_t now;
  struct timeval now_tv;
  struct timezone now_tz;
  struct tm *now_tm;
*/
  gchar filename[MAX_PATH_LEN];
  char **cpp;
  DIR *dfd;
  struct dirent *dp;

  static_my_asclock = my_asclock;
 
  if(my_asclock->pwin) 
  {
    gdk_window_raise(my_asclock->pwin->window);
    return;
  }

  my_asclock->pwin = gnome_property_box_new();
  
  gtk_window_set_title(GTK_WINDOW(&GNOME_PROPERTY_BOX(my_asclock->pwin)->dialog.window),
                _("ASClock Settings"));

  frame = gtk_vbox_new(5, TRUE);
  hbox =  gtk_hbox_new(TRUE, 5);
  opts = gtk_vbox_new(5, TRUE);

#ifdef ENABLE_NLS
  themes_titles[0]=_(themes_titles[0]);
#endif
  list = gtk_clist_new_with_titles(1, themes_titles);
  gtk_clist_set_column_width(GTK_CLIST(list), 0, 200);

  gtk_signal_connect( GTK_OBJECT(list), "select_row", GTK_SIGNAL_FUNC(theme_selected), my_asclock);
  for (cpp= themes_directories; *cpp; cpp++)
    {

      if((dfd = opendir(*cpp)) != NULL)
        while((dp = readdir(dfd)) != NULL)
          if ( dp->d_name[0]!='.' ) {
	    gchar *elems[2] = { filename, NULL };
	    strcpy(filename, *cpp);
	    strcat(filename, dp->d_name);
            gtk_clist_append(GTK_CLIST(list), elems );
      }
      closedir(dfd);
    }

  /* show ampm toggle button */
  label = gtk_check_button_new_with_label (_("Display time in 12 hour format (AM/PM)"));
  gtk_box_pack_start(GTK_BOX(opts), label, FALSE, FALSE, 0);
 
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(label), my_asclock->showampm);
  gtk_signal_connect (GTK_OBJECT(label), "clicked", (GtkSignalFunc) showampm_selected_cb, my_asclock);
  gtk_widget_show(label);

  /* show ampm toggle button */
  label = gtk_check_button_new_with_label (_("Blinking elements in clock"));
  gtk_box_pack_start(GTK_BOX(opts), label, FALSE, FALSE, 0);
 
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(label), my_asclock->itblinks);
  gtk_signal_connect (GTK_OBJECT(label), "clicked", (GtkSignalFunc) itblinks_selected_cb, my_asclock);
  gtk_widget_show(label);


/*
  calendar = gtk_calendar_new();
  gettimeofday(&now_tv, &now_tz);
  now = now_tv.tv_sec;
  now_tm = localtime(&now);

  gtk_calendar_select_month(GTK_CALENDAR(calendar), now_tm->tm_mon, 1900+now_tm->tm_year);
  gtk_calendar_select_day(GTK_CALENDAR(calendar), now_tm->tm_mday);
  gtk_calendar_clear_marks(GTK_CALENDAR(calendar));
  gtk_calendar_mark_day(GTK_CALENDAR(calendar), now_tm->tm_mday);
*/ 
  gtk_box_pack_start(GTK_BOX(hbox), opts, TRUE, TRUE, 5);

  gtk_box_pack_start(GTK_BOX(frame), hbox, TRUE, TRUE, 5);
/*
  gtk_container_add( GTK_CONTAINER( hbox ), calendar);
*/
  scroll_win = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll_win),
                                           GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_container_add( GTK_CONTAINER( scroll_win ), list);
  gtk_widget_show(list);
  gtk_container_add(GTK_CONTAINER( hbox ), scroll_win);
  gtk_widget_show (scroll_win);

/*
  gtk_widget_show(calendar);
*/
  gtk_widget_show(hbox);
  gtk_widget_show(frame);

  label = gtk_label_new(_("General"));
  gnome_property_box_append_page( GNOME_PROPERTY_BOX(my_asclock->pwin),frame ,label);

  frame =  gtk_vbox_new(5, TRUE);

  label = gtk_label_new(_("Timezone"));

  properties_timezone_render(my_asclock, frame, 0, 0);

  hbox = gtk_hbox_new(FALSE, 5);
  gtk_box_pack_start(GTK_BOX(hbox), my_asclock->pic, FALSE, FALSE, 5);

  gtk_widget_show(my_asclock->pic);


#ifdef ENABLE_NLS
  timezone_titles[0]=_(timezone_titles[0]);
#endif
  list = gtk_clist_new_with_titles(1, timezone_titles);

  gtk_clist_set_column_width(GTK_CLIST(list), 0, 200);
  gtk_widget_set_usize(list, 260, 320);
/*
  gtk_clist_set_policy(GTK_CLIST(list), GTK_POLICY_ALWAYS, GTK_POLICY_AUTOMATIC); 
*/
  scroll_win = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll_win),
                                           GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  gtk_widget_show(list);
  gtk_container_add( GTK_CONTAINER( scroll_win ), list);

  gtk_container_add(GTK_CONTAINER( hbox ), scroll_win);
  gtk_widget_show (scroll_win);
 
  gtk_box_pack_start( GTK_BOX(frame), hbox, FALSE, FALSE, 5);

  gtk_widget_show(hbox);
  
  gtk_widget_show(frame);
  gnome_property_box_append_page( GNOME_PROPERTY_BOX(my_asclock->pwin),frame ,label);
  
  gtk_signal_connect( GTK_OBJECT(list), "select_row", GTK_SIGNAL_FUNC(location_selected), my_asclock);
  gtk_signal_connect( GTK_OBJECT(my_asclock->pwin), "delete_event", GTK_SIGNAL_FUNC(property_close_cb), my_asclock);
  gtk_signal_connect( GTK_OBJECT(my_asclock->pwin), "close", GTK_SIGNAL_FUNC(property_close_cb), my_asclock);

  gtk_signal_connect( GTK_OBJECT(my_asclock->pwin),"apply", GTK_SIGNAL_FUNC(property_apply_cb), my_asclock );

/*
  gtk_signal_connect( GTK_OBJECT(my_asclock->pwin),"destroy", GTK_SIGNAL_FUNC(property_destroy_cb), my_asclock );
*/
  enum_timezones(my_asclock, list);

  gtk_widget_show_all(my_asclock->pwin);
} 







