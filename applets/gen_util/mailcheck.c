/* GNOME panel mail check module.
 * (C) 1997, 1998, 1999 The Free Software Foundation
 *
 * Authors: Miguel de Icaza
 *          Jacob Berkman
 *          Jaka Mocnik
 *          Lennart Poettering
 *
 */

#include <config.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <config.h>
#include <gnome.h>
#include <applet-widget.h>
#include <gdk_imlib.h>

#include "popcheck.h"
#include "mailcheck.h"

GtkWidget *applet = NULL;

typedef enum {
	MAILBOX_LOCAL,
	MAILBOX_LOCALDIR,
	MAILBOX_POP3,
	MAILBOX_IMAP
} MailboxType;


typedef struct _MailCheck MailCheck;
struct _MailCheck {
	char *mail_file;

	/* Does the user have any mail at all? */
	int anymail;

	/* New mail has arrived? */
	int newmail;

	/* Does the user have unread mail? */
	int unreadmail;

	guint update_freq;

	/* execute a command when the applet is clicked (launch email prog) */
        char *clicked_cmd;
	gboolean clicked_enabled;

	/* execute a command when new mail arrives (play a sound etc.)
	   FIXME: actually executes the command when mc->newmail 
	   goes from 0 -> 1 (so not every time you get new mail) */
	char *newmail_cmd;
	gboolean newmail_enabled;

	/* execute a command before checking email (fetchmail etc.) */
	char *pre_check_cmd;
	gboolean pre_check_enabled;	

	/* This is the event box for catching events */
	GtkWidget *ebox;

	/* This holds either the drawing area or the label */
	GtkWidget *bin;

	/* The widget that holds the label with the mail information */
	GtkWidget *label;

	/* Points to whatever we have inside the bin */
	GtkWidget *containee;

	/* The drawing area */
	GtkWidget *da;
	GdkPixmap *email_pixmap;
	GdkBitmap *email_mask;

	/* handle for the timeout */
	int mail_timeout;

	/* how do we report the mail status */
	enum {
		REPORT_MAIL_USE_TEXT,
		REPORT_MAIL_USE_BITMAP,
		REPORT_MAIL_USE_ANIMATION
	} report_mail_mode;

	/* current frame on the animation */
	int nframe;

	/* number of frames on the pixmap */
	int frames;

	/* handle for the animation timeout handler */
	int animation_tag;

	/* for the selection routine */
	char *selected_pixmap_name;

	/* The property window */
	GtkWidget *property_window;
	GtkWidget *min_spin, *sec_spin;
	GtkWidget *pre_check_cmd_entry, *pre_check_cmd_check;
	GtkWidget *newmail_cmd_entry, *newmail_cmd_check;
	GtkWidget *clicked_cmd_entry, *clicked_cmd_check;

	gboolean anim_changed;

	char *mailcheck_text_only;

	char *animation_file;
        
	GtkWidget *mailfile_entry, *mailfile_label, *mailfile_fentry;
	GtkWidget *remote_server_entry, *remote_username_entry, *remote_password_entry;
	GtkWidget *remote_server_label, *remote_username_label, *remote_password_label;
	GtkWidget *remote_option_menu;
	GtkWidget *play_sound_check;
        
	char *remote_server, *remote_username, *remote_password;
	MailboxType mailbox_type; /* local = 0; maildir = 1; pop3 = 2; imap = 3 */
        MailboxType mailbox_type_temp;

	gboolean play_sound;
	
	int size;
};

#define WANT_BITMAPS(x) (x == REPORT_MAIL_USE_ANIMATION || x == REPORT_MAIL_USE_BITMAP)

static void close_callback (GtkWidget *widget, void *data);

static char *
mail_animation_filename (MailCheck *mc)
{
	if (!mc->animation_file){
		mc->animation_file =
			gnome_unconditional_pixmap_file ("mailcheck/email.png");
		if (g_file_exists (mc->animation_file))
			return g_strdup (mc->animation_file);
		g_free (mc->animation_file);
		mc->animation_file = NULL;
		return NULL;
	} else if (*mc->animation_file){
		if (g_file_exists (mc->animation_file))
			return g_strdup (mc->animation_file);
		g_free (mc->animation_file);
		mc->animation_file = NULL;
		return NULL;
	} else
		/* we are using text only, since the filename was "" */
		return NULL;
}

static int
calc_dir_contents (char *dir) {
       DIR* dr;
       struct dirent *de;
       int size=0;

       dr = opendir(dir);
       if (dr == NULL)
               return 0;
       while((de = readdir(dr))) {
               if (strlen(de->d_name) < 1 || de->d_name[0] == '.')
                       continue;
               size ++;
       }
       closedir(dr);
       return size;
}

/*
 * Get file modification time, based upon the code
 * of Byron C. Darrah for coolmail and reused on fvwm95
 */
static void
check_mail_file_status (MailCheck *mc)
{
	static off_t oldsize = 0;
	struct stat s;
	off_t newsize;
	int status, old_unreadmail;
	
	if ((mc->mailbox_type == MAILBOX_POP3) || 
	    (mc->mailbox_type == MAILBOX_IMAP)) {
		int v;
		
		if (mc->mailbox_type == MAILBOX_POP3)
			v = pop3_check(mc->remote_server, mc->remote_username, mc->remote_password);
		else
			v = imap_check(mc->remote_server, mc->remote_username, mc->remote_password);
		
		if (v == -1) {
#if 0
			/* don't notify about an error: think of people with
			 * dial-up connections; keep the current mail status
			 */
			GtkWidget *box = NULL;
			box = gnome_message_box_new (_("Remote-client-error occured. Remote-polling deactivated. Maybe you used a wrong server/username/password?"),
						     GNOME_MESSAGE_BOX_ERROR, GNOME_STOCK_BUTTON_CLOSE, NULL);
			gtk_window_set_modal (GTK_WINDOW(box),TRUE);
			gtk_widget_show (box);
			
			mc->mailbox_type = MAILBOX_LOCAL;
			mc->anymail = mc->newmail = 0;
#endif
		} else {
		  old_unreadmail = mc->unreadmail;
		  mc->unreadmail = (signed int) (((unsigned int) v) >> 16);
		  if(mc->unreadmail > 0 && old_unreadmail != mc->unreadmail)
		    mc->newmail = 1;
		  else
		    mc->newmail = 0;
		  mc->anymail = (signed int) (((unsigned int) v) & 0x0000FFFFL) ? 1 : 0;
		} 
	} else if(mc->mailbox_type == MAILBOX_LOCAL) {
	    status = stat (mc->mail_file, &s);
	    if (status < 0) {
	      oldsize = 0;
	      mc->anymail = mc->newmail = mc->unreadmail = 0;
	      return;
	    }
		
	    newsize = s.st_size;
	    mc->anymail = newsize > 0;
	    mc->unreadmail = (s.st_mtime >= s.st_atime && newsize > 0);
		
	    if (newsize != oldsize && mc->unreadmail)
	      mc->newmail = 1;
	    else
	      mc->newmail = 0;
		
	    oldsize = newsize;
	}
	else { /* MAILBOX_LOCALDIR */
	  int newmail, oldmail;
	  char tmp[1024];
	  snprintf(tmp, 1024, "%s/new", mc->mail_file);
	  newmail = calc_dir_contents(tmp);
	  snprintf(tmp, 1024, "%s/cur", mc->mail_file);
	  oldmail = calc_dir_contents(tmp);
	  mc->newmail = newmail > oldsize;
	  mc->unreadmail = newmail;
	  oldsize = newmail;
	  mc->anymail = newmail || oldmail;
	}	    
}

static void
mailcheck_load_animation (MailCheck *mc, char *fname)
{
	int width, height;
	GdkImlibImage *im;

	im = gdk_imlib_load_image (fname);

	height = mc->size;
	width = im->rgb_width*((double)height/im->rgb_height);

	gdk_imlib_render (im, width, height);

	mc->email_pixmap = gdk_imlib_copy_image (im);
	mc->email_mask = gdk_imlib_copy_mask (im);

	gdk_imlib_destroy_image (im);
	
	/* yeah, they have to be square, in case you were wondering :-) */
	mc->frames = width / height;
	if (mc->frames == 3)
		mc->report_mail_mode = REPORT_MAIL_USE_BITMAP;
	mc->nframe = 0;
}

static int
next_frame (gpointer data)
{
	MailCheck *mc = data;

	mc->nframe = (mc->nframe + 1) % mc->frames;
	if (mc->nframe == 0)
		mc->nframe = 1;
	gtk_widget_draw (mc->da, NULL);
	return TRUE;
}

static int
mail_check_timeout (gpointer data)
{
	static const char *supinfo[] = {"mailcheck", "new-mail", NULL};
	MailCheck *mc = data;

	if (mc->pre_check_enabled &&
	    mc->pre_check_cmd && 
	    (strlen(mc->pre_check_cmd) > 0)){
		/*
		 * if we have to execute a command before checking for mail, we
		 * remove the mail-check timeout and re-add it after the command
		 * returns, just in case the execution takes too long.
		 */
		
		gtk_timeout_remove (mc->mail_timeout);
		if (system(mc->pre_check_cmd) == 127)
			g_warning("Couldn't execute command");
		mc->mail_timeout = gtk_timeout_add(mc->update_freq, mail_check_timeout, mc);
	}

	check_mail_file_status (mc);

	if(mc->newmail) {
    if(mc->play_sound)
      gnome_triggers_vdo("", "program", supinfo);

		if (mc->newmail_enabled && 
		    mc->newmail_cmd && 
		    (strlen(mc->newmail_cmd) > 0))
			gnome_execute_shell(NULL, mc->newmail_cmd);
	}

	switch (mc->report_mail_mode){
	case REPORT_MAIL_USE_ANIMATION:
		if (mc->anymail){
			if (mc->unreadmail){
				if (mc->animation_tag == -1){
					mc->animation_tag = gtk_timeout_add (150, next_frame, mc);
					mc->nframe = 1;
				}
			} else {
				if (mc->animation_tag != -1){
					gtk_timeout_remove (mc->animation_tag);
					mc->animation_tag = -1;
				}
				mc->nframe = 1;
			}
		} else {
			if (mc->animation_tag != -1){
				gtk_timeout_remove (mc->animation_tag);
				mc->animation_tag = -1;
			}
			mc->nframe = 0;
		}
		gtk_widget_draw (mc->da, NULL);
		break;
		
	case REPORT_MAIL_USE_BITMAP:
		if (mc->anymail){
			if (mc->newmail)
				mc->nframe = 2;
			else
				mc->nframe = 1;
		} else
			mc->nframe = 0;
		gtk_widget_draw (mc->da, NULL);
		break;

	case REPORT_MAIL_USE_TEXT: {
		char *text;

		if (mc->anymail){
			if (mc->newmail)
				text = _("You have new mail.");
			else
				text = _("You have mail.");
		} else
			text = _("No mail.");
		gtk_label_set_text (GTK_LABEL (mc->label), text);
		break;
	}
	}
	return 1;
}

/*
 * this gets called when we have to redraw the nice icon
 */
static gint
icon_expose (GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
	MailCheck *mc = data;
	int h = mc->size;
	gdk_draw_pixmap (mc->da->window, mc->da->style->black_gc,
			 mc->email_pixmap, mc->nframe * h,
			 0, 0, 0, h, h);
	return TRUE;
}

static gint
exec_clicked_cmd (GtkWidget *widget, GdkEvent *evt, gpointer data)
{
         MailCheck *mc = data;
	 if (mc->clicked_enabled && 
	     mc->clicked_cmd && 
	     (strlen(mc->clicked_cmd) > 0) )
	        gnome_execute_shell(NULL, mc->clicked_cmd);
	 return TRUE;
}

static void
mailcheck_destroy (GtkWidget *widget, gpointer data)
{
	MailCheck *mc = data;
	mc->bin = NULL;

	if (mc->property_window)
		close_callback (NULL, mc);

	g_free (mc->pre_check_cmd);
	g_free (mc->newmail_cmd);
	g_free (mc->clicked_cmd);

	g_free(mc->remote_server);
	g_free(mc->remote_username);
	g_free(mc->remote_password);
                
	gtk_timeout_remove (mc->mail_timeout);
}

static GtkWidget *
create_mail_widgets (MailCheck *mc)
{
	char *fname = mail_animation_filename (mc);

	mc->ebox = gtk_event_box_new();
	gtk_widget_show (mc->ebox);
	
	/*
	 * This is so that the properties dialog is destroyed if the
	 * applet is removed from the panel while the dialog is
	 * active.
	 */
	gtk_signal_connect (GTK_OBJECT (mc->ebox), "destroy",
			    (GtkSignalFunc) mailcheck_destroy,
			    mc);

	mc->bin = gtk_hbox_new (0, 0);
	gtk_container_add(GTK_CONTAINER(mc->ebox), mc->bin);

	gtk_widget_show (mc->bin);
	
	mc->mail_timeout = gtk_timeout_add (mc->update_freq, mail_check_timeout, mc);

	/* The drawing area */
	gtk_widget_push_visual (gdk_imlib_get_visual ());
	gtk_widget_push_colormap (gdk_imlib_get_colormap ());
	mc->da = gtk_drawing_area_new ();
	gtk_widget_pop_colormap ();
	gtk_widget_pop_visual ();
	gtk_widget_ref (mc->da);
	gtk_drawing_area_size (GTK_DRAWING_AREA(mc->da),
			       mc->size, mc->size);
	gtk_signal_connect (GTK_OBJECT(mc->da), "expose_event", (GtkSignalFunc)icon_expose, mc);
	gtk_widget_set_events(GTK_WIDGET(mc->da),GDK_EXPOSURE_MASK);
	gtk_widget_show (mc->da);

	/* The label */
	mc->label = gtk_label_new ("");
	gtk_widget_show (mc->label);
	gtk_widget_ref (mc->label);
	
	if (fname && WANT_BITMAPS (mc->report_mail_mode)) {
		mailcheck_load_animation (mc,fname);
		mc->containee = mc->da;
	} else {
		mc->report_mail_mode = REPORT_MAIL_USE_TEXT;
		mc->containee = mc->label;
	}
	free (fname);
	gtk_container_add (GTK_CONTAINER (mc->bin), mc->containee);
	return mc->ebox;
}

static void
set_selection (GtkWidget *widget, gpointer data)
{
	MailCheck *mc = gtk_object_get_user_data(GTK_OBJECT(widget));
	mc->selected_pixmap_name = data;
	mc->anim_changed = TRUE;

	gnome_property_box_changed (GNOME_PROPERTY_BOX (mc->property_window));
}

static void
property_box_changed(GtkWidget *widget, gpointer data)
{
	MailCheck *mc = data;
	
	gnome_property_box_changed (GNOME_PROPERTY_BOX (mc->property_window));
}

static void
free_str (GtkWidget *widget, void *data)
{
	g_free (data);
}

static void
mailcheck_new_entry (MailCheck *mc, GtkWidget *menu, GtkWidget *item, char *s)
{
	gtk_menu_append (GTK_MENU (menu), item);

	gtk_object_set_user_data(GTK_OBJECT(item),mc);

	gtk_signal_connect (GTK_OBJECT (item), "activate",
			    GTK_SIGNAL_FUNC(set_selection), s);
	if (s != mc->mailcheck_text_only)
		gtk_signal_connect (GTK_OBJECT (item), "destroy",
				    GTK_SIGNAL_FUNC(free_str), s);
}

static GtkWidget *
mailcheck_get_animation_menu (MailCheck *mc)
{
	GtkWidget *omenu, *menu, *item;
	struct    dirent *e;
	char      *dname = gnome_unconditional_pixmap_file ("mailcheck");
	DIR       *dir;
	char      *basename = NULL;
	int       i = 0, select_item = 0;

	mc->selected_pixmap_name = mc->mailcheck_text_only;
	omenu = gtk_option_menu_new ();
	menu = gtk_menu_new ();

	item = gtk_menu_item_new_with_label (mc->mailcheck_text_only);
	gtk_widget_show (item);
	mailcheck_new_entry (mc, menu, item, mc->mailcheck_text_only);

	if (mc->animation_file){
		basename = strrchr (mc->animation_file, '/');
		if (basename)
			basename++;
	} else
		mc->animation_file = NULL;

	i = 1;
	dir = opendir (dname);
	if (dir){
		while ((e = readdir (dir)) != NULL){
			char *s;
			
			if (! (strstr (e->d_name, ".xpm") ||
			       strstr (e->d_name, ".png") ||
			       strstr (e->d_name, ".gif") ||
			       strstr (e->d_name, ".jpg")))
				continue;

			s = g_strdup (e->d_name);
			if (!mc->selected_pixmap_name)
				mc->selected_pixmap_name = s;
			if (basename && strcmp (basename, e->d_name) == 0)
				select_item = i;
			item = gtk_menu_item_new_with_label (s);
			i++;
			gtk_widget_show (item);
			
			mailcheck_new_entry (mc,menu, item, s);
		}
		closedir (dir);
	}
	gtk_option_menu_set_menu (GTK_OPTION_MENU (omenu), menu);
	gtk_option_menu_set_history (GTK_OPTION_MENU (omenu), select_item);
	gtk_widget_show (omenu);
	return omenu;
}

static void
close_callback (GtkWidget *widget, gpointer data)
{
	MailCheck *mc = data;
	gtk_widget_destroy (mc->property_window);
	mc->property_window = NULL;
}

static void
load_new_pixmap (MailCheck *mc)
{
	gtk_widget_hide (mc->containee);
	gtk_container_remove (GTK_CONTAINER (mc->bin), mc->containee);
	
	if (mc->selected_pixmap_name == mc->mailcheck_text_only) {
		mc->report_mail_mode = REPORT_MAIL_USE_TEXT;
		mc->containee = mc->label;
		if(mc->animation_file) g_free(mc->animation_file);
		mc->animation_file = NULL;
	} else {
		char *fname = g_strconcat ("mailcheck/", mc->selected_pixmap_name, NULL);
		char *full;
		
		full = gnome_unconditional_pixmap_file (fname);
		free (fname);
		
		mailcheck_load_animation (mc,full);
		mc->containee = mc->da;
		if(mc->animation_file) g_free(mc->animation_file);
		mc->animation_file = full;
	}
	mail_check_timeout (mc);
	gtk_widget_set_uposition (GTK_WIDGET (mc->containee), 0, 0);
	gtk_container_add (GTK_CONTAINER (mc->bin), mc->containee);
	gtk_widget_show (mc->containee);
}

static void
apply_properties_callback (GtkWidget *widget, gint page, gpointer data)
{
	char *text;
	MailCheck *mc = (MailCheck *)data;
	
	if(page!=-1) return;
	
	mc->update_freq = 1000 * (guint)(gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (mc->sec_spin)) + 
					 60 * gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (mc->min_spin)));
	/* do this here since we can no longer make the seconds
	 * minimum 1
	 */
	if (mc->update_freq == 0)
		mc->update_freq = 1000;

	gtk_timeout_remove (mc->mail_timeout);
	mc->mail_timeout = gtk_timeout_add (mc->update_freq, mail_check_timeout, mc);
	
	if (mc->clicked_cmd) {
		g_free(mc->pre_check_cmd);
		mc->pre_check_cmd = NULL;
	}

	text = gtk_entry_get_text (GTK_ENTRY(mc->pre_check_cmd_entry));
	
	if (strlen (text) > 0)
		mc->pre_check_cmd = g_strdup (text);
	mc->pre_check_enabled = GTK_TOGGLE_BUTTON(mc->pre_check_cmd_check)->active;
	
	if (mc->clicked_cmd) {
		g_free(mc->clicked_cmd);
		mc->clicked_cmd = NULL;
        }

        text = gtk_entry_get_text (GTK_ENTRY(mc->clicked_cmd_entry));

        if (strlen(text) > 0)
                mc->clicked_cmd = g_strdup(text);
	mc->clicked_enabled = GTK_TOGGLE_BUTTON(mc->clicked_cmd_check)->active;

	if (mc->newmail_cmd) {
		g_free(mc->newmail_cmd);
		mc->newmail_cmd = NULL;
	}

	text = gtk_entry_get_text (GTK_ENTRY(mc->newmail_cmd_entry));

	if (strlen(text) > 0)
		mc->newmail_cmd = g_strdup(text);
	mc->newmail_enabled = GTK_TOGGLE_BUTTON(mc->newmail_cmd_check)->active;

	if (mc->anim_changed)
		load_new_pixmap (mc);
	
	mc->anim_changed = FALSE;
        
	if (mc->mail_file) {
		g_free(mc->mail_file);
		mc->mail_file = NULL;
	}

	text = gtk_entry_get_text (GTK_ENTRY (mc->mailfile_entry));

	if (strlen(text) > 0)
		mc->mail_file = g_strdup(text);

	if (mc->remote_server) {
		g_free(mc->remote_server);
		mc->remote_server = NULL;
	}

        text = gtk_entry_get_text (GTK_ENTRY(mc->remote_server_entry));
	
	if (strlen(text) > 0)
		mc->remote_server = g_strdup(text);

	if (mc->remote_username) {
		g_free(mc->remote_username);
		mc->remote_username = NULL;
	}

        text = gtk_entry_get_text (GTK_ENTRY(mc->remote_username_entry));

	if (strlen(text) > 0)
		mc->remote_username = g_strdup(text);

	if (mc->remote_password) {
		g_free(mc->remote_password);
		mc->remote_password = NULL;
	}

        text = gtk_entry_get_text (GTK_ENTRY(mc->remote_password_entry));

	if (strlen(text) > 0)
		mc->remote_password = g_strdup(text);
        
	mc->mailbox_type = mc->mailbox_type_temp;

	mc->play_sound = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(mc->play_sound_check));
}

static void
make_remote_widgets_sensitive(MailCheck *mc)
{
	gboolean b = mc->mailbox_type_temp != MAILBOX_LOCAL &&
	             mc->mailbox_type_temp != MAILBOX_LOCALDIR;
	
	gtk_widget_set_sensitive (mc->mailfile_fentry, !b);
	gtk_widget_set_sensitive (mc->mailfile_label, !b);
	
	gtk_widget_set_sensitive (mc->remote_server_entry, b);
	gtk_widget_set_sensitive (mc->remote_password_entry, b);
	gtk_widget_set_sensitive (mc->remote_username_entry, b);
	gtk_widget_set_sensitive (mc->remote_server_label, b);
	gtk_widget_set_sensitive (mc->remote_password_label, b);
	gtk_widget_set_sensitive (mc->remote_username_label, b);
}

static void 
set_mailbox_selection (GtkWidget *widget, gpointer data)
{
	MailCheck *mc = gtk_object_get_user_data(GTK_OBJECT(widget));
	mc->mailbox_type_temp = GPOINTER_TO_INT(data);
        
        make_remote_widgets_sensitive(mc);
	gnome_property_box_changed (GNOME_PROPERTY_BOX (mc->property_window));
}

static GtkWidget *
mailbox_properties_page(MailCheck *mc)
{
	GtkWidget *vbox, *hbox, *l, *l2, *item;

	vbox = gtk_vbox_new (FALSE, GNOME_PAD_SMALL);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
	gtk_widget_show (vbox);

	hbox = gtk_hbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
	gtk_widget_show (hbox);        

	l = gtk_label_new(_("Mailbox resides on:"));
	gtk_widget_show(l);
	gtk_box_pack_start (GTK_BOX (hbox), l, FALSE, FALSE, 0);

	mc->remote_option_menu = l = gtk_option_menu_new();
        
	l2 = gtk_menu_new();
	item = gtk_menu_item_new_with_label(_("Local mailspool")); 
	gtk_widget_show(item);
	gtk_object_set_user_data(GTK_OBJECT(item), mc);
	gtk_signal_connect (GTK_OBJECT(item), "activate", 
			    GTK_SIGNAL_FUNC(set_mailbox_selection), 
			    GINT_TO_POINTER(MAILBOX_LOCAL));
	gtk_menu_append(GTK_MENU(l2), item);

	item = gtk_menu_item_new_with_label(_("Local maildir")); 
	gtk_widget_show(item);
	gtk_object_set_user_data(GTK_OBJECT(item), mc);
	gtk_signal_connect (GTK_OBJECT(item), "activate", 
			    GTK_SIGNAL_FUNC(set_mailbox_selection), 
			    GINT_TO_POINTER(MAILBOX_LOCALDIR));
	gtk_menu_append(GTK_MENU(l2), item);

	item = gtk_menu_item_new_with_label(_("Remote POP3-server")); 
	gtk_widget_show(item);
	gtk_object_set_user_data(GTK_OBJECT(item), mc);
	gtk_signal_connect (GTK_OBJECT(item), "activate", 
			    GTK_SIGNAL_FUNC(set_mailbox_selection), 
			    GINT_TO_POINTER(MAILBOX_POP3));
        
	gtk_menu_append(GTK_MENU(l2), item);
	item = gtk_menu_item_new_with_label(_("Remote IMAP-server")); 
	gtk_widget_show(item);
	gtk_object_set_user_data(GTK_OBJECT(item), mc);
	gtk_signal_connect (GTK_OBJECT(item), "activate", 
			    GTK_SIGNAL_FUNC(set_mailbox_selection), 
			    GINT_TO_POINTER(MAILBOX_IMAP));
	gtk_menu_append(GTK_MENU(l2), item);
	
	gtk_widget_show(l2);
  
	gtk_option_menu_set_menu(GTK_OPTION_MENU(l), l2);
	gtk_option_menu_set_history(GTK_OPTION_MENU(l), mc->mailbox_type_temp = mc->mailbox_type);
	gtk_widget_show(l);
  
	gtk_box_pack_start (GTK_BOX (hbox), l, FALSE, FALSE, 0);
  
	hbox = gtk_hbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
	gtk_widget_show (hbox);

	mc->mailfile_label = l = gtk_label_new(_("Mail spool file:"));
	gtk_widget_show(l);
	gtk_box_pack_start (GTK_BOX (hbox), l, FALSE, FALSE, 0);

	mc->mailfile_fentry = l = gnome_file_entry_new ("spool file", _("Browse"));
	gtk_widget_show(l);
	gtk_box_pack_start (GTK_BOX (hbox), l, TRUE, TRUE, 0);

	mc->mailfile_entry = l = gnome_file_entry_gtk_entry(GNOME_FILE_ENTRY (l));
	gtk_entry_set_text(GTK_ENTRY(l), mc->mail_file);
	gtk_signal_connect(GTK_OBJECT(l), "changed",
			   GTK_SIGNAL_FUNC(property_box_changed), mc);

	hbox = gtk_hbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
	gtk_widget_show (hbox);  
  
	mc->remote_server_label = l = gtk_label_new(_("Mail server:"));
	gtk_widget_show(l);
	gtk_box_pack_start (GTK_BOX (hbox), l, FALSE, FALSE, 0);
  
	mc->remote_server_entry = l = gtk_entry_new();
	if (mc->remote_server)
		gtk_entry_set_text(GTK_ENTRY(l), mc->remote_server);
  	gtk_widget_show(l);
	gtk_box_pack_start (GTK_BOX (hbox), l, TRUE, TRUE, 0);      
	
	gtk_signal_connect(GTK_OBJECT(l), "changed",
			   GTK_SIGNAL_FUNC(property_box_changed), mc);
	
	hbox = gtk_hbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
	gtk_widget_show (hbox);  
  
	mc->remote_username_label = l = gtk_label_new(_("Username:"));
	gtk_widget_show(l);
	gtk_box_pack_start (GTK_BOX (hbox), l, FALSE, FALSE, 0);
	
	mc->remote_username_entry = l = gtk_entry_new();
	if (mc->remote_username)
		gtk_entry_set_text(GTK_ENTRY(l), mc->remote_username);
  
	gtk_widget_show(l);
	gtk_box_pack_start (GTK_BOX (hbox), l, FALSE, FALSE, 0);      
  
	gtk_signal_connect(GTK_OBJECT(l), "changed",
			   GTK_SIGNAL_FUNC(property_box_changed), mc);

	mc->remote_password_label = l = gtk_label_new(_("Password:"));
	gtk_widget_show(l);
	gtk_box_pack_start (GTK_BOX (hbox), l, FALSE, FALSE, 0);
	
	mc->remote_password_entry = l = gtk_entry_new();
	if (mc->remote_password)
		gtk_entry_set_text(GTK_ENTRY(l), mc->remote_password);
	gtk_entry_set_visibility(GTK_ENTRY (l), FALSE);
	gtk_widget_show(l);
	gtk_box_pack_start (GTK_BOX (hbox), l, FALSE, FALSE, 0);      
	
	gtk_signal_connect(GTK_OBJECT(l), "changed",
                     GTK_SIGNAL_FUNC(property_box_changed), mc);
  
	make_remote_widgets_sensitive(mc);
	
	return vbox;
}

static GtkWidget *
mailcheck_properties_page (MailCheck *mc)
{
	GtkWidget *vbox, *hbox, *l, *table, *frame;
	GtkObject *freq_a;
	
	vbox = gtk_vbox_new (FALSE, GNOME_PAD_SMALL);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
	gtk_widget_show (vbox);

	frame = gtk_frame_new (_("Execute"));
	gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
	gtk_widget_show (frame);

	table = gtk_table_new (3, 2, FALSE);
	gtk_table_set_col_spacings (GTK_TABLE (table), GNOME_PAD/2);
	gtk_table_set_row_spacings (GTK_TABLE (table), GNOME_PAD/2);
	gtk_container_set_border_width (GTK_CONTAINER (table), GNOME_PAD/2);
	gtk_widget_show(table);
	gtk_container_add (GTK_CONTAINER (frame), table);

	l = gtk_check_button_new_with_label(_("Before each update:"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(l), mc->pre_check_enabled);
	gtk_signal_connect(GTK_OBJECT(l), "toggled",
			   GTK_SIGNAL_FUNC(property_box_changed), mc);
	gtk_widget_show(l);
	mc->pre_check_cmd_check = l;
	
	gtk_table_attach (GTK_TABLE (table), mc->pre_check_cmd_check, 
			  0, 1, 0, 1, GTK_FILL, 0, 0, 0);
				   
	
	mc->pre_check_cmd_entry = gtk_entry_new();
	if(mc->pre_check_cmd)
		gtk_entry_set_text(GTK_ENTRY(mc->pre_check_cmd_entry), 
				   mc->pre_check_cmd);
	gtk_signal_connect(GTK_OBJECT(mc->pre_check_cmd_entry), "changed",
			   GTK_SIGNAL_FUNC(property_box_changed), mc);
	gtk_widget_show(mc->pre_check_cmd_entry);
	gtk_table_attach_defaults (GTK_TABLE (table), mc->pre_check_cmd_entry,
				   1, 2, 0, 1);

	l = gtk_check_button_new_with_label (_("When new mail arrives:"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(l), mc->newmail_enabled);
	gtk_signal_connect(GTK_OBJECT(l), "toggled",
			   GTK_SIGNAL_FUNC(property_box_changed), mc);
	gtk_widget_show(l);
	gtk_table_attach (GTK_TABLE (table), l, 0, 1, 1, 2, GTK_FILL, 0, 0, 0);
	mc->newmail_cmd_check = l;

	mc->newmail_cmd_entry = gtk_entry_new();
	if (mc->newmail_cmd) {
		gtk_entry_set_text(GTK_ENTRY(mc->newmail_cmd_entry),
				   mc->newmail_cmd);
	}
	gtk_signal_connect(GTK_OBJECT (mc->newmail_cmd_entry), "changed",
			   GTK_SIGNAL_FUNC(property_box_changed), mc);
	gtk_widget_show(mc->newmail_cmd_entry);
	gtk_table_attach_defaults (GTK_TABLE (table), mc->newmail_cmd_entry,
				    1, 2, 1, 2);

        l = gtk_check_button_new_with_label (_("When clicked:"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(l), mc->clicked_enabled);
	gtk_signal_connect(GTK_OBJECT(l), "toggled",
			   GTK_SIGNAL_FUNC(property_box_changed), mc);
        gtk_widget_show(l);
	gtk_table_attach (GTK_TABLE (table), l, 0, 1, 2, 3, GTK_FILL, 0, 0, 0);
	mc->clicked_cmd_check = l;

        mc->clicked_cmd_entry = gtk_entry_new();
        if(mc->clicked_cmd) {
		gtk_entry_set_text(GTK_ENTRY(mc->clicked_cmd_entry), 
				   mc->clicked_cmd);
        }
        gtk_signal_connect(GTK_OBJECT(mc->clicked_cmd_entry), "changed",
                           GTK_SIGNAL_FUNC(property_box_changed), mc);
        gtk_widget_show(mc->clicked_cmd_entry);
	gtk_table_attach_defaults (GTK_TABLE (table), mc->clicked_cmd_entry,
				   1, 2, 2, 3);

        hbox = gtk_hbox_new (FALSE, 6);
        gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
        gtk_widget_show (hbox); 
        
        l = gtk_label_new (_("Check for mail every"));
	gtk_widget_show(l);
	gtk_box_pack_start (GTK_BOX (hbox), l, FALSE, FALSE, 0);

	freq_a = gtk_adjustment_new((float)((mc->update_freq/1000)/60), 0, 1440, 1, 5, 5);
	mc->min_spin = gtk_spin_button_new( GTK_ADJUSTMENT (freq_a), 1, 0);
	gtk_signal_connect(GTK_OBJECT(freq_a), "value_changed",
			   GTK_SIGNAL_FUNC(property_box_changed), mc);
	gtk_signal_connect(GTK_OBJECT(mc->min_spin), "changed",
			   GTK_SIGNAL_FUNC(property_box_changed), mc);
	gtk_box_pack_start (GTK_BOX (hbox), mc->min_spin,  FALSE, FALSE, 0);
	gtk_widget_show(mc->min_spin);
	
	l = gtk_label_new (_("minutes"));
	gtk_widget_show(l);
	gtk_box_pack_start (GTK_BOX (hbox), l, FALSE, FALSE, 0);
	
	freq_a = gtk_adjustment_new((float)((mc->update_freq/1000)%60), 0, 59, 1, 5, 5);
	mc->sec_spin  = gtk_spin_button_new (GTK_ADJUSTMENT (freq_a), 1, 0);
	gtk_signal_connect(GTK_OBJECT(freq_a), "value_changed",
			   GTK_SIGNAL_FUNC(property_box_changed), mc);
	gtk_signal_connect(GTK_OBJECT(mc->sec_spin), "changed",
			   GTK_SIGNAL_FUNC(property_box_changed), mc);
	gtk_box_pack_start (GTK_BOX (hbox), mc->sec_spin,  FALSE, FALSE, 0);
	gtk_widget_show(mc->sec_spin);
	
	l = gtk_label_new (_("seconds"));
	gtk_widget_show(l);
	gtk_box_pack_start (GTK_BOX (hbox), l, FALSE, FALSE, 0);
	
	mc->play_sound_check = gtk_check_button_new_with_label(_("Play a sound when new mail arrives"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(mc->play_sound_check), mc->play_sound);
	gtk_signal_connect(GTK_OBJECT(mc->play_sound_check), "toggled",
			   GTK_SIGNAL_FUNC(property_box_changed), mc);
	gtk_widget_show(mc->play_sound_check);
	gtk_box_pack_start(GTK_BOX (vbox), mc->play_sound_check, FALSE, FALSE, 0);
	
	hbox = gtk_hbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
	gtk_widget_show (hbox);  
	
	l = gtk_label_new (_("Select animation"));
	gtk_misc_set_alignment (GTK_MISC (l), 0.0, 0.5);
	gtk_widget_show (l);
	gtk_box_pack_start (GTK_BOX (hbox), l, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), mailcheck_get_animation_menu (mc), FALSE, FALSE, 0);

	return vbox;
}

static void
mailcheck_properties (AppletWidget *applet, gpointer data)
{
        static GnomeHelpMenuEntry help_entry = {
		NULL, "properties-mailcheck"
	};
	GtkWidget *p;

	MailCheck *mc = data;

	help_entry.name = gnome_app_id;

	if (mc->property_window != NULL) {
		gdk_window_raise(mc->property_window->window);
		return; /* Only one instance of the properties dialog! */
	}
	
	mc->property_window = gnome_property_box_new ();
	gtk_window_set_title (GTK_WINDOW (mc->property_window),
			      _("Mail check properties"));

	p = mailcheck_properties_page (mc);
	gnome_property_box_append_page (GNOME_PROPERTY_BOX(mc->property_window),
					p, gtk_label_new (_("Mail check")));
	p = mailbox_properties_page (mc);
	gnome_property_box_append_page (GNOME_PROPERTY_BOX(mc->property_window),
					p, gtk_label_new (_("Mailbox")));

	gtk_signal_connect (GTK_OBJECT (mc->property_window), "apply",
			    GTK_SIGNAL_FUNC(apply_properties_callback), mc);
	gtk_signal_connect (GTK_OBJECT (mc->property_window), "destroy",
			    GTK_SIGNAL_FUNC(close_callback), mc);
	gtk_signal_connect (GTK_OBJECT (mc->property_window), "help",
			    GTK_SIGNAL_FUNC(gnome_help_pbox_display),
			    &help_entry);

	gtk_widget_show (mc->property_window);
}

static gint
applet_save_session(GtkWidget *w,
		    const char *privcfgpath,
		    const char *globcfgpath,
		    gpointer data)
{
	MailCheck *mc = data;

	gnome_config_push_prefix(privcfgpath);
	gnome_config_set_string("mail/animation_file",
                          mc->animation_file?mc->animation_file:"");
	gnome_config_set_int("mail/update_frequency", mc->update_freq);
	gnome_config_set_string("mail/exec_command",
				mc->pre_check_cmd?mc->pre_check_cmd:"");
	gnome_config_set_bool("mail/exec_enabled",mc->pre_check_enabled);
	gnome_config_set_string("mail/newmail_command",
				mc->newmail_cmd?mc->newmail_cmd:"");
	gnome_config_set_bool("mail/newmail_enabled",mc->newmail_enabled);
	gnome_config_set_string("mail/clicked_command",
				mc->clicked_cmd?mc->clicked_cmd:"");
	gnome_config_set_bool("mail/clicked_enabled",mc->clicked_enabled);
	gnome_config_set_string("mail/mail_file",
				mc->mail_file?mc->mail_file:"");
        gnome_config_private_set_string("mail/remote_server", 
				mc->remote_server?mc->remote_server:"");
        gnome_config_private_set_string("mail/remote_username", 
				mc->remote_username?mc->remote_username:"");
        gnome_config_private_set_string("mail/remote_password", 
				mc->remote_password?mc->remote_password:"");
        gnome_config_set_int("mail/mailbox_type", (int) mc->mailbox_type);
	gnome_config_set_bool("mail/play_sound", mc->play_sound);

	gnome_config_pop_prefix();

	gnome_config_sync();
	gnome_config_drop_all();

	return FALSE;
}

static void
mailcheck_about(AppletWidget *a_widget, gpointer a_data)
{
	static GtkWidget *about = NULL;
	static const gchar     *authors [] =
	{
		"Miguel de Icaza <miguel@kernel.org>",
		"Jacob Berkman <jberkman@andrew.cmu.edu>",
		"Jaka Mocnik <jaka.mocnik@kiss.uni-lj.si>",
		"Lennart Poettering <poettering@gmx.net>",
		NULL
	};

	if (about != NULL)
	{
		gdk_window_show(about->window);
		gdk_window_raise(about->window);
		return;
	}
	
	about = gnome_about_new ( _("Mail check Applet"), "1.0",
				  _("(c) 1998 the Free Software Foundation"),
				  authors,
				  _("Mail check notifies you when new mail is on your mailbox"),
				  NULL);
	gtk_signal_connect( GTK_OBJECT(about), "destroy",
			    GTK_SIGNAL_FUNC(gtk_widget_destroyed), &about );
	gtk_widget_show(about);
}

/*this is when the panel size changes */
static void
applet_change_pixel_size(GtkWidget * w, int size, gpointer data)
{
	MailCheck *mc = data;
	
	mc->size = size;

	if(mc->report_mail_mode != REPORT_MAIL_USE_TEXT) {
		char *fname = mail_animation_filename (mc);
		int size = mc->size;

		gtk_drawing_area_size (GTK_DRAWING_AREA(mc->da),size,size);
		gtk_widget_set_usize (GTK_WIDGET(mc->da), size, size);
	
		if (fname)
			mailcheck_load_animation (mc,fname);
	}
}

GtkWidget *
make_mailcheck_applet(const gchar *goad_id)
{
	GtkWidget *mailcheck;
	MailCheck *mc;
	char *emailfile;
	char *query;

	mc = g_new0(MailCheck,1);
	mc->animation_tag = -1;
	mc->animation_file = NULL;
	mc->property_window = NULL;
	mc->anim_changed = FALSE;
	mc->anymail = mc->unreadmail = mc->newmail = FALSE;

	/*initial state*/
	mc->report_mail_mode = REPORT_MAIL_USE_ANIMATION;

	applet = applet_widget_new(goad_id);
	if (!applet)
		g_error(_("Can't create applet!\n"));

	gnome_config_push_prefix(APPLET_WIDGET(applet)->privcfgpath);

	mc->mail_file = gnome_config_get_string("mail/mail_file");

	if (!mc->mail_file) {
		mc->mail_file = getenv ("MAIL");
		if (!mc->mail_file){
			char *user;
			
			if ((user = getenv("USER")) != NULL){
				mc->mail_file = g_malloc(strlen(user) + 20);
				sprintf(mc->mail_file, "/var/spool/mail/%s", user);
			} else {
				return NULL;
			}
		}
		/* little hack to get a dup'ed string */
		mc->mail_file = g_strdup(mc->mail_file);
	}

	emailfile = gnome_unconditional_pixmap_file("mailcheck/email.png");
	query = g_strconcat("mail/animation_file=",emailfile,NULL);
	g_free(emailfile);
	mc->animation_file = gnome_config_get_string(query);
	g_free(query);

	mc->update_freq = gnome_config_get_int("mail/update_frequency=120000");
		
	mc->pre_check_cmd = gnome_config_get_string("mail/exec_command");
	mc->pre_check_enabled = gnome_config_get_bool("mail/exec_enabled=0");

	mc->newmail_cmd = gnome_config_get_string("mail/newmail_command");
	mc->newmail_enabled = gnome_config_get_bool("mail/newmail_enabled=0");

        mc->clicked_cmd = gnome_config_get_string("mail/clicked_command");
	mc->clicked_enabled = gnome_config_get_bool("mail/clicked_enabled=0");

	mc->remote_server = gnome_config_private_get_string("mail/remote_server=mail");
	
	query = g_strconcat("mail/remote_username=", getenv("USER"), NULL);
	mc->remote_username = gnome_config_private_get_string(query);
	g_free(query);

	mc->remote_password = gnome_config_private_get_string("mail/remote_password");
	mc->mailbox_type = gnome_config_get_int("mail/mailbox_type=0");

	mc->play_sound = gnome_config_get_bool("mail/play_sound=false");

	gnome_config_pop_prefix();

	mc->mailcheck_text_only = _("Text only");
	
	mc->size = PIXEL_SIZE_STANDARD;

	gtk_signal_connect(GTK_OBJECT(applet), "change_pixel_size",
			   GTK_SIGNAL_FUNC(applet_change_pixel_size),
			   mc);

	mailcheck = create_mail_widgets (mc);
	gtk_widget_show(mailcheck);
	applet_widget_add (APPLET_WIDGET (applet), mailcheck);

        gtk_widget_set_events(GTK_WIDGET(mc->ebox), 
                              gtk_widget_get_events(GTK_WIDGET(mc->ebox)) |
                              GDK_BUTTON_PRESS_MASK);

        gtk_signal_connect(GTK_OBJECT(mc->ebox), "button_press_event",
                           GTK_SIGNAL_FUNC(exec_clicked_cmd), mc);

	gtk_signal_connect(GTK_OBJECT(applet),"save_session",
			   GTK_SIGNAL_FUNC(applet_save_session),
			   mc);

	applet_widget_register_stock_callback(APPLET_WIDGET(applet),
					      "properties",
					      GNOME_STOCK_MENU_PROP,
					      _("Properties..."),
					      mailcheck_properties,
					      mc);

	applet_widget_register_stock_callback(APPLET_WIDGET(applet),
					      "about",
					      GNOME_STOCK_MENU_ABOUT,
					      _("About..."),
					      mailcheck_about,
					      NULL);
	gtk_widget_show (applet);
	return applet;
}
