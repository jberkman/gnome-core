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
extern GtkWidget *monitor;
extern gshort priority;
extern gshort waitmins;
extern gshort dpmsmins;
extern gboolean dpms;
extern gboolean password;
extern gchar *screensaver;
extern screensaver_data *sd;
extern GList *sdlist;
static pid_t pid = 0;
static pid_t pid_big = 0;

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
list_click_callback (GtkWidget *list, GdkEventButton *event, void *data)
{
        if (data == sd)
                /* FIXME.  if I click on "no screensaver" right away, it's wrong */
                return;
        if (data == NULL) 
                /* FIXME: No screensaver value for now.  Need to generate it and random. */
                return;

        sd = (screensaver_data *) data;
        if (sd->args == NULL)
                init_screensaver_data (sd);
        if ((sd->setup_data == NULL) || (sd->dialog != NULL))
                gtk_widget_set_sensitive (setup_button, FALSE);
        else
                gtk_widget_set_sensitive (setup_button, TRUE);
        launch_miniview (sd);

        capplet_widget_state_changed(CAPPLET_WIDGET (capplet), TRUE);
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
nice_callback (GtkWidget *nice, void *data)
{
        priority = GTK_ADJUSTMENT(nice)->value;
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
        gchar *command;
        if (sd->dialog)
                store_screensaver_data (sd);
        gnome_config_sync ();
        system ("xscreensaver-command -exit");
        
}
void
try_callback ()
{
        g_print ("try clicked\n");
}
void
revert_callback ()
{
        g_print ("revert clicked\n");
}
void
sig_child(int sig)
{
        int pid;
        int status;
        while ( (pid = wait3(&status, WNOHANG, (struct rusage *) 0)) > 0)
                ;
}
