/*
 * GNOME panel mail check module.
 * (C) 1997 The Free Software Foundation
 *
 * Author: Miguel de Icaza
 *
 */

#include <config.h>
#ifdef HAVE_LIBINTL
#    include <libintl.h>
#endif
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <gnome.h>
#include "applet-lib.h"
#include "panel.h"
#include <gdk_imlib.h>

#define WIDGET_HEIGHT 48

GtkWidget *plug = NULL;

int applet_id = -1;/*this is our id we use to comunicate with the panel */

static char *mail_file;

/* If set, the user has launched the mail viewer */
static int mailcleared;

/* Does the user have any mail at all? */
static int anymail;

/* New mail has arrived? */
static int newmail;

/* Does the user have unread mail? */
static int unreadmail;

/* This holds either the drawing area or the label */
static GtkWidget *bin;

/* The widget that holds the label with the mail information */
static GtkWidget *label;

/* Points to whatever we have inside the bin */
static GtkWidget *containee;

/* The drawing area */
static GtkWidget *da;
static GdkPixmap *email_pixmap;
static GdkBitmap *email_mask;

/* handle for the timeout */
static int mail_timeout;

/* how do we report the mail status */
enum {
	REPORT_MAIL_USE_TEXT,
	REPORT_MAIL_USE_BITMAP,
	REPORT_MAIL_USE_ANIMATION
} report_mail_mode;

#define WANT_BITMAPS(x) (x == REPORT_MAIL_USE_ANIMATION || x == REPORT_MAIL_USE_BITMAP)

/* current frame on the animation */
static int nframe;

/* number of frames on the pixmap */
static int frames;

/* handle for the animation timeout handler */
static int animation_tag = -1;

/* for the selection routine */
static char *selected_pixmap_name;

/* The property window */
static GtkWidget *property_window;

static char *mailcheck_text_only;

static char *animation_file = NULL;


static void close_callback (GtkWidget *widget, void *data);


static char *
mail_animation_filename ()
{
	if(!animation_file) {
		animation_file =
			gnome_unconditional_pixmap_file("mailcheck/email.xpm");
		if (g_file_exists (animation_file))
			return g_strdup(animation_file);
		g_free (animation_file);
		animation_file = NULL;
		return NULL;
	} else if (*animation_file){
		if (g_file_exists (animation_file))
			return g_strdup(animation_file);
		g_free (animation_file);
		animation_file = NULL;
		return NULL;
	} else
		/*we are using text only, since the filename was ""!*/
		return NULL;
}

/*
 * Get file modification time, based upon the code
 * of Byron C. Darrah for coolmail and reused on fvwm95
 */
static void
check_mail_file_status ()
{
	static off_t oldsize = 0;
	struct stat s;
	off_t newsize;
	int status;

	status = stat (mail_file, &s);
	if (status < 0){
		oldsize = 0;
		anymail = newmail = unreadmail = 0;
		return;
	}
	
	newsize = s.st_size;
	anymail = newsize > 0;
	unreadmail = (s.st_mtime >= s.st_atime && newsize > 0);

	if (newsize >= oldsize && unreadmail){
		newmail = 1;
		mailcleared = 0;
	} else
		newmail = 0;
	oldsize = newsize;
}

static void
mailcheck_load_animation (char *fname)
{
	int width, height;
	GdkImlibImage *im;

	im = gdk_imlib_load_image (fname);

	width = im->rgb_width;
	height = im->rgb_height;

	gdk_imlib_render (im, width, height);

	email_pixmap = gdk_imlib_copy_image (im);
	email_mask = gdk_imlib_copy_mask (im);

	gdk_imlib_destroy_image (im);
	
	/* yeah, they have to be square, in case you were wondering :-) */
	frames = width / WIDGET_HEIGHT;
	if (frames == 3)
		report_mail_mode = REPORT_MAIL_USE_BITMAP;
	nframe = 0;
}

static int
next_frame (void *data)
{
	nframe = (nframe + 1) % frames;
	if (nframe == 0)
		nframe = 1;
	gtk_widget_draw (da, NULL);
	return TRUE;
}

static int
mail_check_timeout (void *data)
{
	check_mail_file_status ();

	switch (report_mail_mode){
	case REPORT_MAIL_USE_ANIMATION:
		if (anymail){
			if (newmail){
				if (animation_tag == -1){
					animation_tag = gtk_timeout_add (150, next_frame, NULL);
					nframe = 1;
				}
			} else {
				if (animation_tag != -1){
					gtk_timeout_remove (animation_tag);
					animation_tag = -1;
				}
				nframe = 1;
			}
		} else {
			if (animation_tag != -1){
				gtk_timeout_remove (animation_tag);
				animation_tag = -1;
			}
			nframe = 0;
		}
		gtk_widget_draw (da, NULL);
		break;
		
	case REPORT_MAIL_USE_BITMAP:
		if (anymail){
			if (newmail)
				nframe = 2;
			else
				nframe = 1;
		} else
			nframe = 0;
		gtk_widget_draw (da, NULL);
		break;

	case REPORT_MAIL_USE_TEXT: {
		char *text;

		if (anymail){
			if (newmail)
				text = _("You have new mail.");
			else
				text = _("You have mail.");
		} else
			text = _("No mail.");
		gtk_label_set (GTK_LABEL (label), text);
		break;
	}
	}
	return 1;
}

static void
mail_destroy (void)
{
	gtk_timeout_remove (mail_timeout);
}

/*
 * this gets called when we have to redraw the nice icon
 */
static gint
icon_expose (GtkWidget *widget, GdkEventExpose *event, gpointer closure)
{
	gdk_draw_pixmap (da->window, da->style->black_gc, email_pixmap,
			 nframe * WIDGET_HEIGHT, 0, 0, 0, WIDGET_HEIGHT, WIDGET_HEIGHT);
	return TRUE;
}

static void
mailcheck_destroy (GtkWidget *widget, gpointer data)
{
	bin = NULL;

	if (property_window)
		close_callback (NULL, NULL);
}

static GtkWidget *
create_mail_widgets ()
{
	char *fname = mail_animation_filename ();

	bin = gtk_hbox_new (0, 0);

	/* This is so that the properties dialog is destroyed if the
	 * applet is removed from the panel while the dialog is
	 * active.
	 */
	gtk_signal_connect (GTK_OBJECT (bin), "destroy",
			    (GtkSignalFunc) mailcheck_destroy,
			    NULL);

	gtk_widget_show (bin);
	check_mail_file_status ();
	
	mail_timeout = gtk_timeout_add (10000, mail_check_timeout, 0);

	/* The drawing area */
	gtk_widget_push_visual (gdk_imlib_get_visual ());
	gtk_widget_push_colormap (gdk_imlib_get_colormap ());
	da = gtk_drawing_area_new ();
	gtk_widget_pop_colormap ();
	gtk_widget_pop_visual ();
	gtk_widget_ref (da);
	gtk_drawing_area_size (GTK_DRAWING_AREA(da), 48, 48);
	gtk_signal_connect (GTK_OBJECT(da), "expose_event", (GtkSignalFunc)icon_expose, 0);
	gtk_widget_set_events(GTK_WIDGET(da),GDK_EXPOSURE_MASK);
	gtk_widget_show (da);

	/* The label */
	label = gtk_label_new ("");
	gtk_widget_show (label);
	gtk_widget_ref (label);
	
	if (fname && WANT_BITMAPS (report_mail_mode)) {
		mailcheck_load_animation (fname);
		containee = da;
	} else {
		report_mail_mode = REPORT_MAIL_USE_TEXT;
		containee = label;
	}
	free (fname);
	gtk_container_add (GTK_CONTAINER (bin), containee);
	mail_check_timeout (0);
	return bin;
}

void
set_selection (GtkWidget *widget, void *data)
{
	selected_pixmap_name = data;
}

static void
free_str (GtkWidget *widget, void *data)
{
	g_free (data);
}

static void
mailcheck_new_entry (GtkWidget *menu, GtkWidget *item, char *s)
{
	gtk_menu_append (GTK_MENU (menu), item);

	gtk_signal_connect (GTK_OBJECT (item), "activate", (GtkSignalFunc) set_selection, s);
	if (s != mailcheck_text_only)
		gtk_signal_connect (GTK_OBJECT (item), "destroy", (GtkSignalFunc) free_str, s);
}

GtkWidget *
mailcheck_get_animation_menu (void)
{
	GtkWidget *omenu, *menu, *item;
	struct    dirent *e;
	char      *dname = gnome_unconditional_pixmap_file ("mailcheck");
	DIR       *dir;

	selected_pixmap_name = mailcheck_text_only;
	omenu = gtk_option_menu_new ();
	menu = gtk_menu_new ();

	item = gtk_menu_item_new_with_label (mailcheck_text_only);
	gtk_widget_show (item);
	mailcheck_new_entry (menu, item, mailcheck_text_only);

	dir = opendir (dname);
	if (dir){
		while ((e = readdir (dir)) != NULL){
			char *s;
			
			if (strstr (e->d_name, ".xpm") == NULL)
				continue;
			
			if (!selected_pixmap_name)
				selected_pixmap_name = s;
			s = g_strdup (e->d_name);
			item = gtk_menu_item_new_with_label (s);
			gtk_widget_show (item);
			
			mailcheck_new_entry (menu, item, s);
		}
		closedir (dir);
	}
	gtk_option_menu_set_menu (GTK_OPTION_MENU (omenu), menu);
	gtk_widget_show (omenu);
	return omenu;
}

void
close_callback (GtkWidget *widget, void *data)
{
	gtk_widget_destroy (property_window);
	property_window = NULL;
}

void
load_new_pixmap_callback (GtkWidget *widget, void *data)
{
	gtk_widget_hide (containee);
	gtk_container_remove (GTK_CONTAINER (bin), containee);
	
	if (selected_pixmap_name == mailcheck_text_only) {
		report_mail_mode = REPORT_MAIL_USE_TEXT;
		containee = label;
		if(animation_file) g_free(animation_file);
		animation_file = NULL;
	} else {
		char *fname = g_copy_strings ("mailcheck/", selected_pixmap_name, NULL);
		char *full;
		
		full = gnome_unconditional_pixmap_file (fname);
		free (fname);
		
		mailcheck_load_animation (full);
		containee = da;
		if(animation_file) g_free(animation_file);
		animation_file = full;
	}
	mail_check_timeout (0);
	gtk_widget_set_uposition (GTK_WIDGET (containee), 0, 0);
	gtk_container_add (GTK_CONTAINER (bin), containee);
	gtk_widget_show (containee);
}

static GnomeActionAreaItem sel_actions [] = {
	{ NULL, load_new_pixmap_callback },
	{ NULL, close_callback },
};

GtkWidget *
mailcheck_notification_frame (void)
{
	GtkWidget *f, *vbox, *hbox, *l;

	f = gtk_frame_new (_("Notification"));
	gtk_widget_show (f);
	gtk_container_border_width (GTK_CONTAINER (f), 5);

	/* If a lot of label/optionmenu pairs are added to the dialog,
	 * you should use a GtkTable to keep stuff nicely aligned.
	 */
	
	vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_border_width (GTK_CONTAINER (vbox), 6);
	gtk_container_add (GTK_CONTAINER (f), vbox);
	gtk_widget_show (vbox);

	hbox = gtk_hbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
	gtk_widget_show (hbox);

	l = gtk_label_new (_("Select animation"));
	gtk_misc_set_alignment (GTK_MISC (l), 0.0, 0.5);
	gtk_widget_show (l);
	gtk_box_pack_start (GTK_BOX (hbox), l, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), mailcheck_get_animation_menu (), FALSE, FALSE, 0);

	return f;
}

void
mailcheck_properties (void)
{
	GtkWidget *f;
	GtkDialog *d;

	if (property_window != NULL)
		return; /* Only one instance of the properties dialog! */
	
	property_window = gtk_dialog_new ();
	gtk_window_set_title (GTK_WINDOW (property_window), _("Mail check properties"));
	d = GTK_DIALOG (property_window);
	f = mailcheck_notification_frame ();
	
	sel_actions [0].label = _("Apply");
	sel_actions [1].label = _("Close");
		
	gnome_build_action_area (d, sel_actions, 2, 0);

	gtk_box_pack_start_defaults (GTK_BOX (d->vbox), f);
	gtk_widget_show (property_window);
}


/*these are commands sent over corba:*/
void
change_orient(int id, int orient)
{
	PanelOrientType o = (PanelOrientType)orient;
}

int
session_save(int id, const char *cfgpath, const char *globcfgpath)
{
	char *query;

	query = g_copy_strings(cfgpath,"animation_file",NULL);
	gnome_config_set_string(query,animation_file?animation_file:"");
	g_free(query);

	gnome_config_sync();
	gnome_config_drop_all();

	return TRUE;
}

static gint
destroy_plug(GtkWidget *widget, gpointer data)
{
	gtk_exit(0);
	return FALSE;
}


static void
properties_corba_callback(int id, gpointer data)
{
	mailcheck_properties();
}


int
main(int argc, char **argv)
{
	GtkWidget *mailcheck;
	char *result;
	char *cfgpath;
	char *globcfgpath;

	char *myinvoc;
	guint32 winid;

	myinvoc = get_full_path(argv[0]);
	if(!myinvoc)
		return 1;

	panel_corba_register_arguments ();
	gnome_init("cdplayer_applet", NULL, argc, argv, 0, NULL);

	/*initial state*/
	report_mail_mode = REPORT_MAIL_USE_ANIMATION;

	mail_file = getenv ("MAIL");
	if (!mail_file)
		return 1;

	if (!gnome_panel_applet_init_corba ())
		g_error ("Could not comunicate with the panel\n");

	result = gnome_panel_applet_request_id(myinvoc,&applet_id,
					       &cfgpath,&globcfgpath,
					       &winid);

	g_free(myinvoc);
	if (result)
		g_error ("Could not talk to the Panel: %s\n", result);

	if(cfgpath && *cfgpath) {
		char *query = g_copy_strings(cfgpath,"animation_file=",NULL);
		animation_file = gnome_config_get_string(query);
		g_free(query);
	} else 
		animation_file = NULL;
		

	g_free(globcfgpath);
	g_free(cfgpath);

	plug = gtk_plug_new (winid);

	mailcheck_text_only = _("Text only");
	mailcheck = create_mail_widgets ();
	gtk_widget_show(mailcheck);
	gtk_container_add (GTK_CONTAINER (plug), mailcheck);
	gtk_widget_show (plug);
	gtk_signal_connect(GTK_OBJECT(plug),"destroy",
			   GTK_SIGNAL_FUNC(destroy_plug),
			   NULL);


	result = gnome_panel_applet_register(plug,applet_id);
	if (result)
		g_error ("Could not talk to the Panel: %s\n", result);

	gnome_panel_applet_register_callback(applet_id,
					     "properties",
					     _("Properties"),
					     properties_corba_callback,
					     NULL);

	applet_corba_gtk_main ("IDL:GNOME/Applet:1.0");

	return 0;
}

