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

/* The widget that holds the label with the mail information */
static GtkWidget *label;

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

static char *
mail_animation_filename ()
{
	char *fname;

	fname = gnome_config_get_string ("/panel/Mail Check/animation_file");
	if (fname){
		if (g_file_exists (fname))
			return fname;
		else
			free (fname);
	}
	fname = gnome_unconditional_pixmap_file ("email.xpm");
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
create_mail_widgets (GtkWidget *window)
{
	char *fname = mail_animation_filename ();

	check_mail_file_status ();
	
	mail_timeout = gtk_timeout_add (10000, mail_check_timeout, 0);
	
	if (fname && WANT_BITMAPS (report_mail_mode)){
		int width, height;

		da = gtk_drawing_area_new ();
		gtk_drawing_area_size (GTK_DRAWING_AREA(da), 48, 48);
		gnome_create_pixmap_gtk (window, &email_pixmap, &email_mask, da, fname);
		gdk_window_get_size (email_pixmap, &width, &height);

		/* yeah, they have to be square, in case you were wondering :-) */
		frames = width / WIDGET_HEIGHT;
		if (frames == 3)
			report_mail_mode = REPORT_MAIL_USE_BITMAP;
		
		free (fname);
		gtk_widget_show (da);
		gtk_signal_connect (GTK_OBJECT(da), "expose_event", (GtkSignalFunc)icon_expose, 0);
		gtk_widget_set_events(GTK_WIDGET(da),GDK_EXPOSURE_MASK);
		return da;
	} else {
		report_mail_mode = REPORT_MAIL_USE_TEXT;
		label = gtk_label_new ("");
		gtk_widget_show (label);
		return label;
	}
}

static void
create_instance (Panel *panel, char *params, int xpos, int ypos)
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
	
	mailcheck = create_mail_widgets (panel->window);
	cmd.cmd = PANEL_CMD_REGISTER_TOY;
	cmd.params.register_toy.applet = mailcheck;
	cmd.params.register_toy.id     = APPLET_ID;
	cmd.params.register_toy.xpos   = xpos;
	cmd.params.register_toy.ypos   = ypos;
	cmd.params.register_toy.flags  = 0;

	(*panel_cmd_func) (&cmd);
}

gpointer
applet_cmd_func(AppletCommand *cmd)
{
	g_assert(cmd != NULL);

	switch (cmd->cmd) {
		case APPLET_CMD_QUERY:
			return APPLET_ID;

		case APPLET_CMD_INIT_MODULE:
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
					cmd->params.create_instance.xpos,
					cmd->params.create_instance.ypos);
			break;

		case APPLET_CMD_GET_INSTANCE_PARAMS:
			return g_strdup("");

		case APPLET_CMD_ORIENTATION_CHANGE_NOTIFY:
			break;

		case APPLET_CMD_PROPERTIES:
			fprintf(stderr, "Mail check properties\n"); /* FIXME */
			break;

		default:
			fprintf(stderr,
				APPLET_ID " applet_cmd_func: Oops, unknown command type %d\n",
				(int) cmd->cmd);
			break;
	}

	return NULL;
}
