/*###################################################################*/
/*##                                                               ##*/
/*###################################################################*/

#include "slashapp.h"
#include "http_get.h"
#include "slashsplash.xpm"

static void flush_newline_chars(gchar *text, gint max);
static int get_current_headlines(gpointer data);
static int startup_delay_cb(gpointer data);
static void about_cb (AppletWidget *widget, gpointer data);
static void destroy_applet(GtkWidget *widget, gpointer data);
static gint applet_save_session(GtkWidget *widget, gchar *privcfgpath,
					gchar *globcfgpath, gpointer data);
static AppData *create_new_app(GtkWidget *applet);
static void applet_start_new_applet(const gchar *param, gpointer data);

static void launch_url(AppData *ad, gchar *url)
{
	gchar *command;
	char *argv[8];
	int status;

	if (ad->new_browser_window)
		command = g_strconcat("openURL(", url, ",new-window)", NULL);
	else
		command = g_strconcat("openURL(", url, ")", NULL);

	printf("running command: %s\n", command);

	argv[0] = "netscape";
	argv[1] = "-remote";
	argv[2] = command;
	argv[3] = NULL;

	/* based on the web control applet */
        if(fork() == 0)
		{
		/* child  */
		execvp (argv[0], argv);
		}
	else
		{
                wait(&status);
                if(WEXITSTATUS(status) != 0)
			{
			/* command didn't work */
			argv[0] = "netscape";
			argv[1] = url;
			argv[2] = NULL;
			if (gnome_execute_async (NULL, 2, argv) != 0)
				{
				printf("failed to start browser\n");
				}
			}
		}

	g_free(command);
}

static void click_headline_cb(AppData *ad, gpointer data)
{
	gchar *url = data;
	if (url)
		{
		printf ("clicked on %s\n", url);
		/* now launch the url */
		launch_url(ad, url);
		}
}

static int filesize(char *s)
{
   struct stat st;
   
   if ((!s)||(!*s)) return 0;
   if (stat(s,&st)<0) return 0;
   return (int)st.st_size;
}

static gchar *check_for_dir(char *d)
{
	if (!g_file_exists(d))
		{
		g_print(_("creating user directory: %s\n"), d);
		if (mkdir( d, 0755 ) < 0)
			{
			g_print(_("unable to create user directory: %s\n"), d);
			g_free(d);
			d = NULL;
			}
		}
	return d;
}

static void delete_if_empty(char *file)
{
	if (filesize(file) < 1) unlink(file);

	if (g_file_exists(file))
		{
		FILE *f = NULL;
		gchar buf[256];
		f = fopen (file, "r");
		if (!f) return;
		if (fgets(buf, sizeof(buf), f) == NULL)
			{
			fclose(f);
			return;
			}
		buf[255] = '\0';
		if (strstr(buf, "<HTML>") != NULL || strstr(buf, "<html>") != NULL)
			{
			fclose(f);
			unlink(file);
			return;
			}
		fclose(f);
		}
}

static GtkWidget *get_topic_image(gchar *topic, AppData *ad)
{
	GtkWidget *icon = NULL;
	gchar *gif_file;
	gchar *jpg_file;
	gchar *icon_file;
	gchar *gif_filename;
	gchar *jpg_filename;

	printf("trying to load icon: %s\n", topic);

	/* darn, must try both file types */
	gif_file = g_strconcat(topic, ".gif", NULL);
	jpg_file = g_strconcat(topic, ".jpg", NULL);

	gif_filename = g_strconcat(ad->slashapp_dir, "/", gif_file, NULL);
	jpg_filename = g_strconcat(ad->slashapp_dir, "/", jpg_file, NULL);


	if (!g_file_exists(gif_filename) && !g_file_exists(jpg_filename))
		{
		/* attempt download of images */
		FILE *f;
		gchar *gif_image;
		gchar *jpg_image;

		gif_image = g_strconcat("/images/topics/topic", gif_file, NULL);
		jpg_image = g_strconcat("/images/topics/topic", jpg_file, NULL);

		
		f = fopen(gif_filename, "w");
		if (f)
			{
			http_get_to_file("slashdot.wolfe.net", 80, gif_image, f);
			fclose(f);
			delete_if_empty(gif_filename);
			}
		if (!g_file_exists(gif_filename))
			{
			f = fopen(jpg_filename, "w");
			if (f)
				{
				http_get_to_file("slashdot.wolfe.net", 80, jpg_image, f);
				fclose(f);
				delete_if_empty(jpg_filename);
				}
			}
		g_free(gif_image);
		g_free(jpg_image);
		}

	if (g_file_exists(gif_filename))
		icon_file = gif_filename;
	else if (g_file_exists(jpg_filename))
		icon_file = jpg_filename;
	else
		icon_file = NULL;

	if (icon_file)
		{
		icon = gnome_pixmap_new_from_file_at_size(icon_file, 20, 24);
		}
	
	g_free(gif_file);
	g_free(jpg_file);
	g_free(gif_filename);
	g_free(jpg_filename);
	return icon;
}

static void make_lowercase(gchar *text)
{
	gchar *p = text;
	while(p[0] != '\0')
		{
		if (isupper(p[0])) p[0] = tolower(p[0]);
		p++;
		}
}

static void flush_newline_chars(gchar *text, gint max)
{
	gchar *p = text;
	gint c = 0;
	while (p[0] != '\0' && c <= max)
		{
		if (p[0] == '\n')
			p[0] = '\0';
		else
			{
			p++;
			c++;
			if (c >= max) p[0] = '\0';
			}
		}
}

static int get_current_headlines(gpointer data)
{
	AppData *ad = data;
	GtkWidget *icon;
	gchar buf[256];
	gchar headline[128];
	gchar url[128];
	gchar entrydate[64];
	gchar author[32];
	gchar department[128];
	gchar topic[32];
	gchar comments[12];
	FILE *slash_file = NULL;
	gchar *filename = g_strconcat(ad->slashapp_dir, "/slashnews", NULL);
	gint h = FALSE;

	if ((slash_file = fopen(filename, "w")) == NULL)
		{
		fprintf(stderr, "Failed to open file \"%s\": %s\n",
				filename, strerror(errno));
		g_free(filename);
		return TRUE;
		}
	http_get_to_file("slashdot.org", 80, "/ultramode.txt", slash_file);
	fclose(slash_file);

	/* refresh the headlines in the display */
	if ((slash_file = fopen(filename, "r")) == NULL)
		{
		fprintf(stderr, "Failed to open file \"%s\": %s\n",
				filename, strerror(errno));
		g_free(filename);
		return TRUE;
		}

	/* clear the current headlines from display list */
	remove_all_lines(ad);

	/* add a generic header image */
	icon = gnome_pixmap_new_from_xpm_d(slashsplash_xpm);
	add_info_line_with_pixmap(ad, "", icon, 0, FALSE, 1, 40);

	while (fgets(buf, sizeof(buf), slash_file) != NULL)
		{
		if (strcmp(buf, "%%\n") == 0) 
			{
			if (fgets(buf, sizeof(buf), slash_file) != NULL)
				{
				InfoData *id;
				gchar *text;
				gchar *edate;
				h = TRUE;
				strncpy(headline, buf, 80);
				flush_newline_chars(headline, 80);
				g_print("%d long: %s\n", strlen(headline), headline);
				fgets(buf, sizeof(buf), slash_file);
				strncpy(url, buf, 120);
				flush_newline_chars(url, 120);
				fgets(buf, sizeof(buf), slash_file);
				strncpy(entrydate, buf, 64);
				flush_newline_chars(entrydate, 23);
				if (strlen(entrydate) > 11)
					edate = entrydate + 11;
				else
					edate = entrydate;
				fgets(buf, sizeof(buf), slash_file);
				strncpy(author, buf, 10);
				flush_newline_chars(author, 8);
				fgets(buf, sizeof(buf), slash_file);
				strncpy(department, buf, 80);
				flush_newline_chars(department, 80);
				fgets(buf, sizeof(buf), slash_file);
				strncpy(topic, buf, 20);
				flush_newline_chars(topic, 16);
				make_lowercase(topic);
				fgets(buf, sizeof(buf), slash_file);
				strncpy(comments, buf, 8);
				flush_newline_chars(comments, 7);

				icon = NULL;
				if (ad->show_images) icon = get_topic_image(topic, ad);

				if (ad->show_department)
					text = g_strconcat(headline, "\nDpt: ", department , NULL);
				else
					text = g_strdup(headline);

				if (ad->show_info)
					{
					gchar *temp = g_strconcat(text, "\n[ ", edate, " by " , author, " ] (", comments,")", NULL);
					g_free(text);
					text = temp;
					}

				/* add the headline */
				if (icon)
					id = add_info_line_with_pixmap(ad, text, icon, 0, FALSE, FALSE, 30);
				else
					id = add_info_line(ad, text, NULL, 0, FALSE, FALSE, 30);
				set_info_click_signal(id, click_headline_cb, g_strdup(url), g_free);

				/* a space separater, could include a graphic divider too */
				add_info_line(ad, "", NULL, 0, FALSE, 0, 0);
				g_free(text);
				}
			}
		}

	fclose(slash_file);

	if (!h)	add_info_line(ad, "  \n  \nNo articles found", NULL, 0, FALSE, 1, 30);

	g_free(filename);
	return TRUE;
}

static int startup_delay_cb(gpointer data)
{
	AppData *ad = data;
	get_current_headlines(ad);
	ad->startup_timeout_id = 0;
	return FALSE;	/* return false to stop this timeout callback, needed only once */
}

static void refresh_cb(AppletWidget *widget, gpointer data)
{
	AppData *ad = data;
	if (ad->startup_timeout_id > 0) return;
	ad->startup_timeout_id = gtk_timeout_add(5000, startup_delay_cb, ad);
}


static void about_cb (AppletWidget *widget, gpointer data)
{
	GtkWidget *about;
	const gchar *authors[8];
	gchar version[32];

	sprintf(version,_("%d.%d.%d"),APPLET_VERSION_MAJ,
		APPLET_VERSION_MIN, APPLET_VERSION_REV);

	authors[0] = _("Justin Maurer <mike911@clark.net>");
	authors[1] = _("Craig Small <csmall@small.dropbear.co.uk>");
	authors[2] = _("John Ellis <johne@bellatlantic.net> - Display engine");
	authors[3] = NULL;

        about = gnome_about_new ( _("Slash Applet"), version,
			_("(C) 1998"),
			authors,
			_("Released under the GNU general public license.\n"
			"Display scroller for slashapp. "),
			NULL);
	gtk_widget_show (about);
}


static void destroy_applet(GtkWidget *widget, gpointer data)
{
	AppData *ad = data;

	gtk_timeout_remove(ad->display_timeout_id);
	gtk_timeout_remove(ad->headline_timeout_id);
	if (ad->startup_timeout_id > 0) gtk_timeout_remove(ad->startup_timeout_id);

	free_all_info_lines(ad->text);
	gtk_widget_destroy(ad->display_w);
	gtk_widget_destroy(ad->disp_buf_w);
	gtk_widget_destroy(ad->background_w);
	g_free(ad);
}

static gint applet_save_session(GtkWidget *widget, gchar *privcfgpath,
					gchar *globcfgpath, gpointer data)
{
	AppData *ad = data;
	property_save(privcfgpath, ad);
        return FALSE;
}

static AppData *create_new_app(GtkWidget *applet)
{
	AppData *ad;
	ad = g_new0(AppData, 1);

	ad->applet = applet;
	ad->slashapp_dir = check_for_dir(gnome_util_home_file("slashapp"));
	if (!ad->slashapp_dir) exit;

	init_app_display(ad);
        gtk_signal_connect(GTK_OBJECT(ad->applet), "destroy",
                GTK_SIGNAL_FUNC(destroy_applet), ad);

	property_load(APPLET_WIDGET(applet)->privcfgpath, ad);

	add_info_line(ad, "Slashdot.org Applet\n", NULL, 0, TRUE, 1, 0);
	add_info_line(ad, "Loading headlines........", NULL, 0, FALSE, 1, 20);

/* applet signals */
        gtk_signal_connect(GTK_OBJECT(applet),"save_session",
                                GTK_SIGNAL_FUNC(applet_save_session),
                                ad);
	applet_widget_register_stock_callback(APPLET_WIDGET(applet),
                                              "properties",
                                              GNOME_STOCK_MENU_PROP,
                                              _("Properties..."),
                                              property_show,
                                              ad);
	applet_widget_register_stock_callback(APPLET_WIDGET(applet),
                                              "about",
                                              GNOME_STOCK_MENU_ABOUT,
                                              _("About..."),
                                              about_cb, NULL);
	applet_widget_register_stock_callback(APPLET_WIDGET(applet),
                                              "refresh",
                                              GNOME_STOCK_MENU_REFRESH,
                                              _("Refresh articles"),
                                              refresh_cb, ad);

	ad->headline_timeout_id = gtk_timeout_add(1800000, get_current_headlines, ad);

        gtk_widget_show(ad->applet);

	/* this is so the app is displayed first before calling the download command */
	ad->startup_timeout_id = gtk_timeout_add(5000, startup_delay_cb, ad);

	return ad;
}

static void applet_start_new_applet(const gchar *param, gpointer data)
{
	GtkWidget *applet;

	applet = applet_widget_new_with_param(param);
		if (!applet)
			g_error("Can't create applet!\n");

	create_new_app(applet);
}

int main (int argc, char *argv[])
{
	GtkWidget *applet;

	/* Initialize the i18n stuff */
	bindtextdomain (PACKAGE, GNOMELOCALEDIR);
	textdomain (PACKAGE);

	applet_widget_init("scroll_applet", NULL, argc, argv, 0, NULL,
			argv[0], TRUE, TRUE, applet_start_new_applet, NULL);

	applet = applet_widget_new();
	if (!applet)
		g_error("Can't create applet!\n");

	create_new_app(applet);

	applet_widget_gtk_main();
	return 0;
}
