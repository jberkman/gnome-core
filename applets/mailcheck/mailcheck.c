/*
 * GNOME panel mail check module.
 * (C) 1997 The Free Software Foundation
 *
 * Author: Miguel de Icaza
 *
 */

#include <stdio.h>
#ifdef HAVE_LIBINTL
#include <libintl.h>
#define _(String) gettext(String)
#else
#define _(String) (String)
#endif
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include "gnome.h"
#include "../panel_cmds.h"
#include "../applet_cmds.h"
#include "../panel.h"

#define WIDGET_HEIGHT 48

#define APPLET_ID "Mail check"

static PanelCmdFunc panel_cmd_func;

static char *mail_file;

gpointer applet_cmd_func(AppletCommand *cmd);

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

/* The panel main window */
static GtkWidget *panel_window;

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

static char *config_animation_file = "/panel/Mail Check/animation_file";

static char *
mail_animation_filename ()
{
	char *fname;

	fname = gnome_config_get_string (config_animation_file);
	if (fname && *fname){
		if (g_file_exists (fname))
			return fname;
		else
			free (fname);
	} else if(fname && !*fname) {
		free (fname);
		/*we are using text only, since the filename was ""!*/
		return NULL;
	}
	fname = gnome_unconditional_pixmap_file ("mailcheck/email.xpm");
	if (g_file_exists (fname))
		return fname;
	free (fname);
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

	gnome_create_pixmap_gtk (panel_window, &email_pixmap, &email_mask, da, fname);
	gdk_window_get_size (email_pixmap, &width, &height);
	
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

static GtkWidget *
create_mail_widgets ()
{
	char *fname = mail_animation_filename ();

	bin = gtk_hbox_new (0, 0);
	gtk_widget_show (bin);
	check_mail_file_status ();
	
	mail_timeout = gtk_timeout_add (10000, mail_check_timeout, 0);

	/* The drawing area */
	da = gtk_drawing_area_new ();
	gtk_drawing_area_size (GTK_DRAWING_AREA(da), 48, 48);
	gtk_signal_connect (GTK_OBJECT(da), "expose_event", (GtkSignalFunc)icon_expose, 0);
	gtk_widget_set_events(GTK_WIDGET(da),GDK_EXPOSURE_MASK);
	gtk_widget_show (da);

	/* The label */
	label = gtk_label_new ("");
	gtk_widget_show (label);
	
	if (fname && WANT_BITMAPS (report_mail_mode)){
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

static void
create_instance (Panel *panel, char *params, int pos)
{
	PanelCommand cmd;
	GtkWidget *mailcheck;

	/* Only allow one instance of this module */
	if (mail_file)
		return;
	
	mail_file = getenv ("MAIL");
	if (!mail_file)
		return;

	/* default: use animations */
	report_mail_mode = REPORT_MAIL_USE_ANIMATION;

	panel_window = panel->window;
	mailcheck = create_mail_widgets ();
	cmd.cmd = PANEL_CMD_REGISTER_TOY;
	cmd.params.register_toy.applet = mailcheck;
	cmd.params.register_toy.id     = APPLET_ID;
	cmd.params.register_toy.pos    = pos;
	cmd.params.register_toy.flags  = APPLET_HAS_PROPERTIES;

	(*panel_cmd_func) (&cmd);
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
	PanelCommand cmd;

	gtk_container_remove (GTK_CONTAINER (bin), containee);
	gtk_widget_hide (containee);
	
	if (selected_pixmap_name == mailcheck_text_only){
		report_mail_mode = REPORT_MAIL_USE_TEXT;
		containee = label;
		gnome_config_set_string (config_animation_file, "");
		mail_check_timeout (0);
	} else {
		char *fname = g_copy_strings ("mailcheck/", selected_pixmap_name, NULL);
		char *full;
		
		full = gnome_unconditional_pixmap_file (fname);
		free (fname);
		
		mailcheck_load_animation (full);
		containee = da;
		gnome_config_set_string (config_animation_file, full);
		free (full);
	}
	gtk_widget_set_uposition (GTK_WIDGET (containee), 0, 0);
	gtk_container_add (GTK_CONTAINER (bin), containee);
	gtk_widget_show (containee);

	/* save new setting */
	gnome_config_sync ();
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
	
	vbox = gtk_vbox_new (0, 5);
	gtk_container_add (GTK_CONTAINER (f), vbox);
	gtk_widget_show (vbox);

	hbox = gtk_hbox_new (0, 5);
	gtk_box_pack_start_defaults (GTK_BOX (vbox), hbox);
	gtk_widget_show (hbox);

	l = gtk_label_new (_("Select animation"));
	gtk_widget_show (l);
	gtk_box_pack_start_defaults (GTK_BOX (hbox), l);
	gtk_box_pack_start_defaults (GTK_BOX (hbox), mailcheck_get_animation_menu ());

	return f;
}

void
mailcheck_properties (void)
{
	GtkWidget *f;
	GtkDialog *d;
	
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

gpointer
applet_cmd_func(AppletCommand *cmd)
{
	g_assert(cmd != NULL);

	switch (cmd->cmd) {
		case APPLET_CMD_QUERY:
			return APPLET_ID;

		case APPLET_CMD_INIT_MODULE:
			mailcheck_text_only = _("No bitmap, use only text");
			panel_cmd_func = cmd->params.init_module.cmd_func;
			break;

		case APPLET_CMD_DESTROY_MODULE:
			mail_destroy ();
			break;

		case APPLET_CMD_GET_DEFAULT_PARAMS:
			return g_strdup("");

		case APPLET_CMD_CREATE_INSTANCE:
			create_instance(cmd->panel,
					cmd->params.create_instance.params,
					cmd->params.create_instance.pos);
			break;

		case APPLET_CMD_GET_INSTANCE_PARAMS:
			return g_strdup("");

		case APPLET_CMD_ORIENTATION_CHANGE_NOTIFY:
			break;

		case APPLET_CMD_PROPERTIES:
			mailcheck_properties ();
			break;

		default:
			fprintf(stderr,
				APPLET_ID " applet_cmd_func: Oops, unknown command type %d\n",
				(int) cmd->cmd);
			break;
	}

	return NULL;
}
