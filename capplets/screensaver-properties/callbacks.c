/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* Copyright (C) 1998 Redhat Software Inc. 
 * Authors: Jonathan Blandford <jrb@redhat.com>
 */
#include "callbacks.h"
#include "capplet-widget.h"
#include "gnome.h"
#include "screensaver-dialog.h"
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
static pid_t pid = 0;

void
launch_miniview (screensaver_data *sd)
{
        gchar test[1000];
        int p[2];

        if (pid)
                kill (pid, SIGTERM);

        if (pipe (p) == -1)
                return;
        pid = fork ();
        if (pid == (pid_t) -1)
                return;
        if (pid == 0) {
                close (p[0]);
                
                snprintf (test, 1000,"0x%x", GDK_WINDOW_XWINDOW (monitor->window));
                execlp ("/usr/local/bin/deco", "deco", "-window-id", test, NULL);
                /* This call should never return */
        }
        close (p[1]);

        return;
}
void
setup_callback (GtkWidget *widget, gpointer data)
{
        make_dialog (sd);
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
        if (sd->setup_data == NULL)
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
void
dialog_callback (GtkWidget *dialog, gint button, void *data)
{
        switch (button) {
        case 0:

                break;
        case 1:
                gnome_dialog_close (GNOME_DIALOG (dialog));
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
sig_child(int sig)
{
        int pid;
        int status;
        while ( (pid = wait3(&status, WNOHANG, (struct rusage *) 0)) > 0)
                ;
}
