/*###################################################################*/
/*##                         clock & mail applet 0.1.0 alpha       ##*/
/*###################################################################*/

#include "clockmail.h"

#include "backgrnd.xpm"
#include "digmed.xpm"
#include "mailpics.xpm"

static GtkWidget *applet;
static GtkWidget *frame;
static GtkWidget *display_area;
static GtkTooltips *tooltips;
static GdkPixmap *display;
static GdkPixmap *display_back;
static GdkPixmap *digmed;
static GdkPixmap *mailpics;

static char *mail_file;
static int update_timeout_id;
static int blink_timeout_id;
static int anymail, newmail, unreadmail, mailcleared;
static int blinking = FALSE;

/* display blink handlers */
static int blink_count = 0;


static void about_cb (AppletWidget *widget, gpointer data)
{
	GtkWidget *about;
	gchar *authors[2];
	gchar version[32];

	sprintf(version,_("%d.%d.%d"),CLOCKMAIL_APPLET_VERSION_MAJ,
		CLOCKMAIL_APPLET_VERSION_MIN, CLOCKMAIL_APPLET_VERSION_REV);

	authors[0] = _("John Ellis (gqview@geocities.com)");
	authors[1] = NULL;

        about = gnome_about_new ( _("Clock and Mail Notify Applet"), version,
			_("(C) 1998"),
			authors,
			_("Released under the GNU general public license.\n"
			"Basic digital clock with date in a tooltip. "
			"Optional 12/24 time display. Mail blinking can be "
			"for any unread mail, or only briefly when new mail arrives."),
			NULL);
	gtk_widget_show (about);
}

/*
 * Get file modification time, based upon the code
 * of Byron C. Darrah for coolmail and reused on fvwm95
 * I updated this by adding the old/new time check so the newmail notifier works
 * the way I wanted to work (eg. newmail is true only when mail has arrived since last check)
 */
static void check_mail_file_status ()
{
	static off_t oldsize = 0;
	static time_t oldtime = 0;
	struct stat s;
	off_t newsize;
	time_t newtime;
	int status;
	int checktime;

	status = stat (mail_file, &s);
	if (status < 0){
		oldsize = 0;
		anymail = newmail = unreadmail = 0;
		return;
	}
	
	newsize = s.st_size;
	newtime = s.st_mtime;
	anymail = newsize > 0;
	unreadmail = (s.st_mtime >= s.st_atime && newsize > 0);

	checktime = (ALWAYS_BLINK || newtime > oldtime);
	if (newsize >= oldsize && unreadmail && checktime){
		newmail = 1;
		mailcleared = 0;
	} else
		newmail = 0;
	oldtime = newtime;
	oldsize = newsize;
}

static void set_tooltip(gchar *t)
{
	static gchar text[64];
	gchar newtext[64];

	/* does this work for i18n? */
	sprintf(newtext,_("%s"),t);

	if (strcmp(newtext,text) != 0)
		{
		strcpy (text,newtext);
		gtk_tooltips_set_tip (tooltips, applet, text, NULL);
		}
}

static void redraw_display()
{
	gdk_window_set_back_pixmap(display_area->window,display,FALSE);
	gdk_window_clear(display_area->window);
}

static void update_mail_display(int n)
{
	static old_n;

	if (n != old_n)
		{
		switch (n)
		{
		case 0:
			gdk_draw_pixmap (display,display_area->style->fg_gc[GTK_WIDGET_STATE(display_area)],
				mailpics, 0, 0, 2, 21, 42, 18);
			break;
		case 1:
			gdk_draw_pixmap (display,display_area->style->fg_gc[GTK_WIDGET_STATE(display_area)],
				mailpics, 0, 18, 2, 21, 42, 18);
			break;
		case 2:
			gdk_draw_pixmap (display,display_area->style->fg_gc[GTK_WIDGET_STATE(display_area)],
				mailpics, 0, 36, 2, 21, 42, 18);
			break;
		default:
			break;
		}
		old_n = n;
		}
}


static void draw_big_digit(int n,int x,int y)
{
	if (n == -1) gdk_draw_pixmap     (display,display_area->style->fg_gc[GTK_WIDGET_STATE(display_area)],
			digmed, 90, 0, x, y, 9, 16);
	if (n >= 0 && n <= 9)
		gdk_draw_pixmap     (display,display_area->style->fg_gc[GTK_WIDGET_STATE(display_area)],
			digmed, n*9, 0, x, y, 9, 16);
}

static void update_time_count(int h, int m)
{
	int hl,hr;
	int ml,mr;

	if (!MILIT_TIME)
		{
		if (h > 12) h -= 12;
		if (h == 0) h = 12;
		}

	hl = h / 10;
	hr = h - (hl * 10);
	if (hl == 0) hl = -1;
	ml = m / 10;
	mr = m - (ml * 10);
	draw_big_digit(hl,2,3);
	draw_big_digit(hr,12,3);
	draw_big_digit(ml,25,3);
	draw_big_digit(mr,35,3);
}

static gint blink_callback()
{
	static int blink_lit;

	if (blink_count == 0) blink_lit = FALSE;

	blink_count++;

	if (blink_lit)
		{
		blink_lit = FALSE;
		update_mail_display(1);
		}
	else
		{
		blink_lit = TRUE;
		update_mail_display(0);
		}
	redraw_display();

	if (blink_count >= BLINK_TIMES || !anymail)
		{
		if (ALWAYS_BLINK && anymail)
			{
			if (blink_count >= BLINK_TIMES) blink_count = 1;
			}
		else
			{
			blink_count = 0;
			if (anymail)
				update_mail_display(2);
			else
				update_mail_display(0);
			redraw_display();
			blinking = FALSE;
			return FALSE;
			}
		}

	return TRUE;
}

static gint update_display()
{
	time_t current_time;
	struct tm *time_data;
	char *strtime;
	char date[20];

	time(&current_time);
	time_data = localtime(&current_time);
	strtime = ctime(&current_time);

	/* update time */
	update_time_count(time_data->tm_hour,time_data->tm_min);

	/* now check mail */
	check_mail_file_status ();

	if (!blinking)
		{
		if (anymail)
			if (newmail)
				{
				update_mail_display(1);
				blinking = TRUE;
				blink_timeout_id = gtk_timeout_add( BLINK_DELAY , (GtkFunction)blink_callback, NULL);
				}
			else
				{
				update_mail_display(2);
				}
		else
			{
			update_mail_display(0);
			}
		redraw_display();
		}

	/* chack date */
	strncpy(date, strtime, 10);
	date[10] = '\0';

	set_tooltip(date);

	return TRUE;
}

static void create_pixmaps()
{
	GdkBitmap *mask;
	GtkStyle *style;
	style = gtk_widget_get_style(applet);

	display_back = gdk_pixmap_create_from_xpm_d(display_area->window, &mask,
		&style->bg[GTK_STATE_NORMAL], (gchar **)backgrnd_xpm);
	digmed = gdk_pixmap_create_from_xpm_d(display_area->window, &mask,
		&style->bg[GTK_STATE_NORMAL], (gchar **)digmed_xpm);
	mailpics = gdk_pixmap_create_from_xpm_d(display_area->window, &mask,
		&style->bg[GTK_STATE_NORMAL], (gchar **)mailpics_xpm);

}

static void applet_change_orient(GtkWidget *w, PanelOrientType o, gpointer data)
{
	/* orient change (nothing is done in this applet */
}

static gint applet_session_save(GtkWidget *widget, char *cfgpath, char *globcfgpath)
{
        property_save(cfgpath);
        return FALSE;
}

int main (int argc, char *argv[])
{
	/* Initialize the i18n stuff */
	bindtextdomain (PACKAGE, GNOMELOCALEDIR);
	textdomain (PACKAGE);

	applet_widget_init_defaults("clockmail_applet", NULL, argc, argv, 0,
			NULL,argv[0]);
        
        

	/* get mail filename */
	mail_file = getenv ("MAIL");
	if (!mail_file)
		return 1;

	applet = applet_widget_new();
	if (!applet)
		g_error("Can't create applet!\n");

	property_load(APPLET_WIDGET(applet)->cfgpath);

	gtk_signal_connect(GTK_OBJECT(applet),"change_orient",
			   GTK_SIGNAL_FUNC(applet_change_orient),
			   NULL);

	/* create a tooltip widget to display song title */
	tooltips = gtk_tooltips_new();
	set_tooltip(_("date"));

	/* frame for all widgets */
	frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
	gtk_widget_show(frame);

	display_area = gtk_drawing_area_new();
	gtk_drawing_area_size(GTK_DRAWING_AREA(display_area),46,42);
	gtk_container_add(GTK_CONTAINER(frame),display_area);
	gtk_widget_show(display_area);

	applet_widget_add(APPLET_WIDGET(applet), frame);

	gtk_widget_realize(display_area);

	display = gdk_pixmap_new(display_area->window,46,42,-1);

	create_pixmaps();

	gtk_widget_show(frame);

	gtk_widget_show(applet);

	gtk_signal_connect(GTK_OBJECT(applet),"session_save",
		GTK_SIGNAL_FUNC(applet_session_save), NULL);

	applet_widget_register_callback(APPLET_WIDGET(applet),
					     "properties",
					     _("Properties..."),
					     property_show,
					     NULL);

	applet_widget_register_callback(APPLET_WIDGET(applet), "about", _("About..."), about_cb, NULL);

	gdk_draw_pixmap     (display,display_area->style->fg_gc[GTK_WIDGET_STATE(display_area)],
		display_back, 0, 0, 0, 0, 46, 42);

	update_display();

	update_timeout_id = gtk_timeout_add(10000, (GtkFunction)update_display, NULL);

	applet_widget_gtk_main();
	return 0;
}
