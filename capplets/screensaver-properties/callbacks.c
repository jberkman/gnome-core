/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* Copyright (C) 1998 Redhat Software Inc. 
 * Authors: Jonathan Blandford <jrb@redhat.com>
 */
#include "callbacks.h"
#include "capplet-widget.h"
#include "gnome.h"
#include "screensaver-dialog.h"
#include <X11/Xlib.h>
#include <gdk/gdkx.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#ifndef errno
extern int errno;
#endif

extern GtkWidget *capplet;
extern GtkWidget *setup_button;
extern GtkWidget *setup_label;
extern GtkWidget *monitor;
extern gint ss_priority;
extern gshort waitmins;
extern gshort dpmsmins;
extern gboolean dpms;
extern gboolean password;
extern gchar *screensaver;
extern screensaver_data *sd;
extern GList *sdlist;
static pid_t pid = 0;
static pid_t pid_big = 0;


/* Loading info... */
void
screensaver_load ()
{
        ss_priority = gnome_config_get_int ("/Screensaver/Default/nice=10");
        waitmins = gnome_config_get_int ("/Screensaver/Default/waitmins=20");
        if (waitmins > 9999)
                waitmins = 9999;
        else if (waitmins < 0)
                waitmins = 0;
        dpmsmins = gnome_config_get_int ("/Screensaver/Default/dpmsmins=20");
        if (dpmsmins > 9999)
                dpmsmins = 9999;
        else if (dpmsmins < 1)
                dpmsmins = 1;
        dpms = gnome_config_get_bool ("/Screensaver/Default/dpms=true");
        password = gnome_config_get_bool ("/Screensaver/Default/password=true");
        screensaver = gnome_config_get_string ("/Screensaver/Default/mode=NONE");
        sd = NULL;
}

GtkWidget *
get_and_set_min_entry ()
{
        gchar tempmin[5];
        static GtkWidget *retval = NULL;

        if (!retval) {
                retval = gtk_entry_new ();
                gtk_signal_connect (GTK_OBJECT (retval), "insert_text", GTK_SIGNAL_FUNC (insert_text_callback), &waitmins);
                gtk_signal_connect_after (GTK_OBJECT (retval), "insert_text", GTK_SIGNAL_FUNC (insert_text_callback2), &waitmins);
                gtk_signal_connect_after (GTK_OBJECT (retval), "delete_text", GTK_SIGNAL_FUNC (delete_text_callback), &waitmins);
                gtk_entry_set_max_length (GTK_ENTRY (retval), 4);
                gtk_widget_set_usize (retval, 50, -2);
        }
        snprintf (tempmin, 5, "%d",waitmins);
        gtk_entry_set_text (GTK_ENTRY (retval), tempmin);
        return retval;
}
GtkWidget *
get_and_set_pword ()
{
        static GtkWidget *retval = NULL;

        if (!retval) {
                retval = gtk_check_button_new_with_label (_("Requires Password"));
                gtk_signal_connect (GTK_OBJECT (retval), "toggled", (GtkSignalFunc) password_callback, NULL);
        }
        gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (retval), password);
        return retval;
}

GtkAdjustment *
get_and_set_nice ()
{
        static GtkAdjustment *retval = NULL;

        if (!retval) {
                retval = GTK_ADJUSTMENT (gtk_adjustment_new ((gfloat)ss_priority,0.0, 19.0, 1.0, 1.0, 0.0));
                gtk_signal_connect (GTK_OBJECT (retval), "value_changed", (GtkSignalFunc) nice_callback, NULL);
        } else {
                retval->value = (gfloat) ss_priority;
                gtk_adjustment_value_changed (retval);
        }
        return retval;                
}

GtkWidget *
get_and_set_dpmsmin ()
{
        static GtkWidget *retval = NULL;
        gchar tempmin[5];

        if (!retval) {
                retval = gtk_entry_new ();
                gtk_signal_connect (GTK_OBJECT (retval), "insert_text", GTK_SIGNAL_FUNC (insert_text_callback), &dpmsmins);
                gtk_signal_connect_after (GTK_OBJECT (retval), "insert_text", GTK_SIGNAL_FUNC (insert_text_callback2), &dpmsmins);
                gtk_signal_connect_after (GTK_OBJECT (retval), "delete_text", GTK_SIGNAL_FUNC (delete_text_callback), &dpmsmins);
                gtk_entry_set_max_length (GTK_ENTRY (retval), 4);
                gtk_widget_set_usize (retval, 50, -2);
        }
        snprintf (tempmin, 5, "%d",dpmsmins);
        gtk_entry_set_text (GTK_ENTRY (retval), tempmin);

        return retval;
}

GtkWidget *
get_and_set_dpmscheck (GtkWidget *box)
{
        static GtkWidget *retval = NULL;
        static GtkWidget *localbox;

        if (!retval) {
                localbox = box;
                retval = gtk_check_button_new_with_label (_("Use power management."));
                gtk_signal_connect (GTK_OBJECT (retval), "toggled", (GtkSignalFunc) dpms_callback, localbox);
        }
        gtk_signal_handler_block_by_func (GTK_OBJECT (retval), (GtkSignalFunc) dpms_callback, localbox);
        gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (retval), dpms);
        gtk_signal_handler_unblock_by_func (GTK_OBJECT (retval), (GtkSignalFunc) dpms_callback, localbox);
        gtk_widget_set_sensitive (localbox, dpms);
        return retval;
}


void
launch_miniview (screensaver_data *sd)
{
        gchar xid[11];
        gchar *temp;
        gchar **argv;
        int p[2];

        if (pid) {
                kill (pid, SIGTERM);
                pid = 0;
        }

        if (sd->demo == NULL)
                /* FIXME: um, we _should_ do something.  Maybe steal the icon field.  */
                /* Maybe come up with a default demo.  dunno... */
                return;
        

        /* we fork and launch a spankin' new screensaver demo!!! */
        if (pipe (p) == -1)
                return;

        pid = fork ();
        if (pid == (pid_t) -1) {
                pid = 0;
                return;
        }
        if (pid == 0) {
                close (p[0]);
                snprintf (xid, 11,"0x%x", GDK_WINDOW_XWINDOW (monitor->window));
                temp = g_copy_strings (sd->demo, " ", sd->windowid, " ", xid, NULL);
                argv = gnome_string_split (temp, " ", -1);
                g_free (temp);
                temp = gnome_is_program_in_path (argv[0]);
                execvp (temp, argv);
                /* This call should never return */
        }
        close (p[1]);

        return;
}
void
setup_callback (GtkWidget *widget, gpointer data)
{
        if (sd->dialog == NULL) {
                gtk_widget_set_sensitive (setup_button, FALSE);
                sd->dialog = make_dialog (sd);
        }
}
void
list_click_callback (GtkWidget *widget, GdkEventButton *event, void *data)
{
        static gint visited = FALSE;
        gchar *label;

        if (data == sd)
                return;

        if (!visited) {
                gtk_signal_disconnect_by_func (GTK_OBJECT (widget), list_click_callback, data);
                visited = TRUE;

        } else
                capplet_widget_state_changed(CAPPLET_WIDGET (capplet), TRUE);
        if (data == NULL){
                screensaver = NULL;
                sd = NULL;
                gtk_label_set (GTK_LABEL (setup_label), _("Settings..."));                
                gtk_widget_set_sensitive (setup_button, FALSE);
                if (pid) {
                        kill (pid, SIGTERM);
                        pid = 0;
                }
                gdk_draw_rectangle (monitor->window,monitor->style->black_gc, TRUE, 0, 0, monitor->allocation.width, monitor->allocation.height);
                return;
        }
        sd = (screensaver_data *) data;
        if (sd->args == NULL)
                init_screensaver_data (sd);
        label = g_copy_strings (sd->name, _(" Settings"), NULL);
        gtk_label_set (GTK_LABEL (setup_label),label);
        g_free (label);
        if ((sd->setup_data == NULL) || (sd->dialog != NULL))
                gtk_widget_set_sensitive (setup_button, FALSE);
        else
                gtk_widget_set_sensitive (setup_button, TRUE);
        
        launch_miniview (sd);
        screensaver = sd->name;
}
void
insert_text_callback (GtkEditable    *editable, const gchar    *text,
                           gint length, gint *position,
                           void *data)
{
        gint i;

        for (i = 0; i < length; i++)
                if (!isdigit(text[i])) {
                        gtk_signal_emit_stop_by_name (GTK_OBJECT (editable), "insert_text");
                        return;
                }
}
void
insert_text_callback2 (GtkEditable    *editable, const gchar    *text,
                            gint            length,
                            gint           *position,
                            void *data)
{
        *((gshort *) data) = atoi (gtk_entry_get_text (GTK_ENTRY (editable)));
        capplet_widget_state_changed(CAPPLET_WIDGET (capplet), TRUE);
}
void
delete_text_callback (GtkEditable    *editable,
                            gint            length,
                            gint           *position,
                            void *data)
{
        *((gshort *) data) = atoi (gtk_entry_get_text (GTK_ENTRY (editable)));
        capplet_widget_state_changed(CAPPLET_WIDGET (capplet), TRUE);
}
void
dpms_callback (GtkWidget *check, void *data)
{
        dpms = !dpms;
        if (dpms)
                gtk_widget_set_sensitive (GTK_WIDGET (data), TRUE);
        else
                gtk_widget_set_sensitive (GTK_WIDGET (data), FALSE);

        capplet_widget_state_changed(CAPPLET_WIDGET (capplet), TRUE);
}
void
password_callback (GtkWidget *pword, void *data)
{
        password = !password;
        capplet_widget_state_changed(CAPPLET_WIDGET (capplet), TRUE);
}
void
nice_callback (GtkObject *adj, void *data)
{
        ss_priority = (gint) GTK_ADJUSTMENT(adj)->value;
        capplet_widget_state_changed(CAPPLET_WIDGET (capplet), TRUE);
}
static void
ssaver_callback (GtkWidget *dialog, GdkEventKey *event, GdkWindow *sswin)
{
        
        if (pid_big){
                kill (pid_big, SIGTERM);
                pid_big = 0;
        }
        /*gtk_signal_emit_stop_by_name (GTK_OBJECT (dialog),"key_press_event");
          gtk_signal_emit_stop_by_name (GTK_OBJECT (dialog),"button_press_event");*/
        gdk_window_hide (sswin);
}
void
dialog_destroy_callback (GtkWidget *dialog, screensaver_data *newsd)
{
        newsd->dialog = NULL;
        newsd->dialog = NULL;
        if (sd == newsd)
                gtk_widget_set_sensitive (setup_button, TRUE);
}
void
dialog_callback (GtkWidget *dialog, gint button, screensaver_data *newsd)
{
        gchar *temp;
        static GdkWindow *sswin = NULL;
        GdkWindowAttr attributes;
        gchar xid[11];
        gchar **argv;


        store_screensaver_data (newsd);
        switch (button) {
        case 0:
                if (sswin == NULL) {
                        attributes.title = NULL;
                        attributes.x = 0;
                        attributes.y = 0;
                        attributes.width = gdk_screen_width ();
                        attributes.height = gdk_screen_height ();
                        attributes.wclass = GDK_INPUT_OUTPUT;
                        attributes.window_type = GDK_WINDOW_TOPLEVEL;
                        attributes.override_redirect = TRUE;
                        sswin = gdk_window_new (NULL, &attributes, GDK_WA_X|GDK_WA_Y|GDK_WA_NOREDIR);
                }

                gtk_signal_connect (GTK_OBJECT (dialog), "key_press_event", (GtkSignalFunc) ssaver_callback, sswin);
                gtk_signal_connect (GTK_OBJECT (dialog), "button_press_event", (GtkSignalFunc) ssaver_callback, sswin);
                XSelectInput (GDK_WINDOW_XDISPLAY (sswin), GDK_WINDOW_XWINDOW (sswin), ButtonPressMask | KeyPressMask);
                gdk_window_show (sswin);
                gdk_window_set_user_data (sswin, dialog);
                

                pid_big = fork ();
                if (pid_big == (pid_t) -1)
                        return;
                if (pid_big == 0) {
                        snprintf (xid, 11,"0x%x", GDK_WINDOW_XWINDOW (sswin));
                        temp = g_copy_strings (newsd->args, " ", newsd->windowid, " ", xid, NULL);
                        
                        argv = gnome_string_split (temp, " ", -1);
                        g_free (temp);
                        temp = gnome_is_program_in_path (argv[0]);
                        execvp (temp, argv);
                        /* This call should never return */
                }
                break;
        case 1:
                gnome_dialog_close (GNOME_DIALOG (dialog));
                newsd->dialog = NULL;
                if (sd == newsd)
                        gtk_widget_set_sensitive (setup_button, TRUE);
                break;
        }
}
void
destroy_callback (GtkWidget *widget, void *data)
{
        if (pid)
                kill (pid, SIGTERM);
}
void
ok_callback ()
{
        /* we want to commit our changes to disk. */
        GString *command;
        gchar temp[10];


        /* save the stuff... */
        gnome_config_set_int ("/Screensaver/Default/nice", ss_priority);
        gnome_config_set_int ("/Screensaver/Default/waitmins", waitmins);
        gnome_config_set_int ("/Screensaver/Default/dpmsmins", dpmsmins);
        gnome_config_set_bool ("/Screensaver/Default/dpms", dpms);
        gnome_config_set_bool ("/Screensaver/Default/password", password);
        if (screensaver)
                gnome_config_set_string ("/Screensaver/Default/mode", screensaver);
        else
                gnome_config_set_string ("/Screensaver/Default/mode", "NONE");

        gnome_config_sync ();
        system ("xscreensaver-command -exit");
        
        if (sd && waitmins) {
                if (sd->dialog)
                        store_screensaver_data (sd);
                command = g_string_new ("xscreensaver -no-splash -timeout ");
                snprintf (temp, 5, "%d", waitmins );
                g_string_append (command, temp);
                g_string_append (command, " -nice ");
                snprintf (temp, 5, "%d",20 - ss_priority);
                g_string_append (command, temp);
                if (!password)
                        g_string_append (command, " -lock-mode");
                g_string_append (command," -xrm \"*programs:\t");
                g_string_append (command, sd->args);
                if (sd->root) {
                        g_string_append (command, " ");
                        g_string_append (command, sd->root);
                }
                g_string_append (command, "\"&");
                
                system (command->str);
                gnome_config_set_string ("/Screensaver/Default/command", command->str);
                g_string_free (command, TRUE);
        }
        if (dpms) {
                /* does anyone know what standby is? */
                /* does it matter? laptops? */
                snprintf (temp, 10, "%d",(gint) (dpmsmins + waitmins) * 60);
                command = g_string_new ("xset dpms ");
                g_string_append (command, temp); /* suspend */
                g_string_append_c (command, ' ');
                g_string_append (command, temp); /* standby */
                g_string_append_c (command, ' ');
                g_string_append (command, temp); /* off */

                system (command->str);
                g_string_free (command, TRUE);
        }
        else
                system ("xset -dpms");

}
void
try_callback ()
{
        GString *command;
        gchar temp[10];
        system ("xscreensaver-command -exit");
        
        if (sd && waitmins) {
                if (sd->dialog)
                        store_screensaver_data (sd);
                command = g_string_new ("xscreensaver -no-splash -timeout ");
                snprintf (temp, 5, "%d", waitmins );
                g_string_append (command, temp);
                g_string_append (command, " -nice ");
                snprintf (temp, 5, "%d",20 - ss_priority);
                g_string_append (command, temp);
                if (!password)
                        g_string_append (command, " -lock-mode");
                g_string_append (command," -xrm \"*programs:\t");
                g_string_append (command, sd->args);
                if (sd->root) {
                        g_string_append (command, " ");
                        g_string_append (command, sd->root);
                }
                g_string_append (command, "\"&");
                
                system (command->str);
                g_string_free (command, TRUE);
        }
        if (dpms) {
                /* does anyone know what standby is? */
                /* does it matter? laptops? */
                snprintf (temp, 10, "%d",(gint) (dpmsmins + waitmins) * 60);
                command = g_string_new ("xset dpms ");
                g_string_append (command, temp); /* suspend */
                g_string_append_c (command, ' ');
                g_string_append (command, temp); /* standby */
                g_string_append_c (command, ' ');
                g_string_append (command, temp); /* off */

                system (command->str);
                g_string_free (command, TRUE);
        }
        else
                system ("xset -dpms");
}
void
revert_callback ()
{
        gchar *temp, *temp2;
        
        if (sd && sd->dialog) {
                gnome_dialog_close (GNOME_DIALOG (sd->dialog));
                sd->dialog = NULL;
                gtk_widget_set_sensitive (setup_button, TRUE);
        }
        gnome_config_drop_all ();
        screensaver_load ();

        temp = gnome_config_get_string ("/Screensaver/Default/command=xscreensaver -no-splash");
        temp2 = g_copy_strings (temp, " &", NULL);
        system (temp2);
        g_free (temp);
        g_free (temp2);
        /* set the devices to the correct one. */
        get_and_set_min_entry ();
        get_and_set_pword ();
        get_and_set_nice ();
        get_and_set_dpmsmin();
        get_and_set_dpmscheck(NULL);
        
}
void
sig_child(int sig)
{
        int pid;
        int status;
        while ( (pid = wait3(&status, WNOHANG, (struct rusage *) 0)) > 0)
                ;
}
