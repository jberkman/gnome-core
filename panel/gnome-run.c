/*
 *   grun: Popup a command dialog. Original version by Elliot Lee, 
 *    bloatware edition by Havoc Pennington. Both versions written in 10
 *    minutes or less. :-)
 *   Copyright (C) 1998 Havoc Pennington <hp@pobox.com>
 *
 * This program is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License as 
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

#include <config.h>
#include <gnome.h>
#include <errno.h>

#include "gnome-run.h"

#include "panel_config_global.h"
#include "panel-util.h"

extern GlobalConfig global_config;

static void 
string_callback (GtkWidget *w, int button_num, gpointer data)
{
	GtkEntry *entry;
	GtkToggleButton *terminal;
	char **argv, **temp_argv;
	int argc, temp_argc;
	char *s;
	GSList *tofree = NULL;

	if (button_num != 0)
		return;

	entry = GTK_ENTRY (gtk_object_get_data(GTK_OBJECT(w), "entry"));
	terminal = GTK_TOGGLE_BUTTON (gtk_object_get_data(GTK_OBJECT(w),
							  "terminal"));

	s = gtk_entry_get_text(entry);

	if (!s || !*s)
		return;

	/* we use a popt function as it does exactly what we want to do and
	   gnome already uses popt */
	if(poptParseArgvString(s, &temp_argc, &temp_argv) != 0) {
		panel_error_dialog(_("Failed to execute command:\n"
				     "%s"), s);
		return;
	}


	if(terminal->active) {
		char **term_argv;
		int term_argc;
		gnome_config_get_vector ("/Gnome/Applications/Terminal",
					 &term_argc, &term_argv);
		if (term_argv) {
			int i;
			argv = g_new(char *, term_argc + temp_argc + 1);
			tofree = g_slist_prepend(tofree, argv);
			argc = term_argc + temp_argc;
			for(i = 0; i < term_argc; i++) {
				argv[i] = term_argv[i];
				tofree = g_slist_prepend(tofree, argv[i]);
			}
			for(i = term_argc; i < term_argc+temp_argc; i++)
				argv[i] = temp_argv[i-term_argc];
			argv[i] = NULL;
			g_free(argv);
		} else {
			char *check;
			int i;
			check = gnome_is_program_in_path("gnome-terminal");
			argv = g_new(char *, 2 + temp_argc + 1);
			tofree = g_slist_prepend(tofree, argv);
			argc = 2 + temp_argc;
			if(!check) {
				argv[0] = "xterm";
				argv[1] = "-e";
			} else {
				argv[0] = check;
				tofree = g_slist_prepend(tofree, check);
				argv[1] = "-x";
			}
			for(i = 2; i < 2+temp_argc; i++)
				argv[i] = temp_argv[i-2];
			argv[i] = NULL;
		}
	} else {
		argv = temp_argv;
		argc = temp_argc;
	}

	if (gnome_execute_async (NULL, argc, argv) >= 0) {
		g_slist_foreach(tofree, (GFunc)g_free, NULL);
		g_slist_free(tofree);
		return;
	}
	g_slist_foreach(tofree, (GFunc)g_free, NULL);
	g_slist_free(tofree);
	
	panel_error_dialog(_("Failed to execute command:\n"
			     "%s\n"
			     "%s"),
			   s, g_unix_error_string(errno));
}

static void
browse_ok(GtkWidget *widget, GtkFileSelection *fsel)
{
	char *fname;
	GtkWidget *entry;

	g_return_if_fail(GTK_IS_FILE_SELECTION(fsel));

	entry = gtk_object_get_user_data(GTK_OBJECT(fsel));

	fname = gtk_file_selection_get_filename(fsel);
	if(fname) {
		char *s = gtk_entry_get_text(GTK_ENTRY(entry));
		if(!s || !*s)
			gtk_entry_set_text(GTK_ENTRY(entry), fname);
		else {
			s = g_strconcat(s, " ", fname, NULL);
			gtk_entry_set_text(GTK_ENTRY(entry), s);
			g_free(s);
		}
	}
	gtk_widget_destroy(GTK_WIDGET(fsel));
}

static void
browse(GtkWidget *w, GtkWidget *entry)
{
	GtkFileSelection *fsel;

	fsel = GTK_FILE_SELECTION(gtk_file_selection_new(_("Browse...")));
	gtk_object_set_user_data(GTK_OBJECT(fsel), entry);

	gtk_signal_connect (GTK_OBJECT (fsel->ok_button), "clicked",
			    GTK_SIGNAL_FUNC (browse_ok), fsel);
	gtk_signal_connect_object
		(GTK_OBJECT (fsel->cancel_button), "clicked",
		 GTK_SIGNAL_FUNC (gtk_widget_destroy), 
		 GTK_OBJECT(fsel));
	gtk_signal_connect_object_while_alive(GTK_OBJECT(entry), "destroy",
					      GTK_SIGNAL_FUNC(gtk_widget_destroy),
					      GTK_OBJECT(fsel));

	gtk_window_position (GTK_WINDOW (fsel), GTK_WIN_POS_MOUSE);
	/* we must do show_now so that we can raise the window in the next
	 * call after set_dialog_layer */
	gtk_widget_show_now (GTK_WIDGET (fsel));
	panel_set_dialog_layer (fsel);
	gdk_window_raise (GTK_WIDGET (fsel)->window);
}

void
show_run_dialog ()
{
	static GtkWidget *dialog = NULL;
	GtkWidget *entry;
	GtkWidget *gentry;
	GtkWidget *hbox;
	GtkWidget *w;

	if(dialog) {
		gtk_widget_show_now(dialog);
		gdk_window_raise(dialog->window);
		return;
	}

	dialog = gnome_dialog_new(_("Run Program"), NULL);
	gtk_signal_connect(GTK_OBJECT(dialog), "destroy",
			   GTK_SIGNAL_FUNC(gtk_widget_destroyed),
			   &dialog);
	gtk_window_position(GTK_WINDOW(dialog), GTK_WIN_POS_MOUSE);
	gnome_dialog_append_button_with_pixmap (GNOME_DIALOG (dialog),
						_("Run"),
						GNOME_STOCK_PIXMAP_EXEC);
	gnome_dialog_append_button (GNOME_DIALOG (dialog),
				    GNOME_STOCK_BUTTON_CANCEL);

	gnome_dialog_set_default (GNOME_DIALOG (dialog), 0);
	gnome_dialog_set_close (GNOME_DIALOG (dialog), TRUE);

	hbox = gtk_hbox_new(0, FALSE);
	
	gentry = gnome_entry_new ("gnome-run");
	gtk_box_pack_start (GTK_BOX (hbox), gentry, TRUE, TRUE, 0);
	gtk_widget_set_usize (GTK_WIDGET (gentry),
			      gdk_screen_width () / 4, -2);

	entry = gnome_entry_gtk_entry (GNOME_ENTRY (gentry));

	gtk_window_set_focus (GTK_WINDOW (dialog), entry);
	gtk_combo_set_use_arrows_always (GTK_COMBO (gentry), TRUE);
	gtk_signal_connect (GTK_OBJECT (dialog), "clicked", 
			    GTK_SIGNAL_FUNC (string_callback), NULL);
	gtk_object_set_data (GTK_OBJECT (dialog), "entry", entry);

	gnome_dialog_editable_enters (GNOME_DIALOG (dialog),
				      GTK_EDITABLE (entry));

	w = gtk_button_new_with_label(_("Browse..."));
	gtk_signal_connect(GTK_OBJECT(w), "clicked",
			   GTK_SIGNAL_FUNC (browse), entry);
	gtk_box_pack_start (GTK_BOX (hbox), w, FALSE, FALSE,
			    GNOME_PAD_SMALL);

	gtk_box_pack_start (GTK_BOX (GNOME_DIALOG (dialog)->vbox), hbox,
			    FALSE, FALSE, GNOME_PAD_SMALL);

	w = gtk_check_button_new_with_label(_("Run in terminal"));
	gtk_object_set_data (GTK_OBJECT (dialog), "terminal", w);
	gtk_box_pack_start (GTK_BOX (GNOME_DIALOG (dialog)->vbox), w,
			    FALSE, FALSE, GNOME_PAD_SMALL);

	gtk_widget_show_all (dialog);
	panel_set_dialog_layer (dialog);
}
