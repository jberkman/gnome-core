/*   gnome-hint-properties: crapplet for gnome-hint
 *
 *   Copyright (C) 1999 Free Software Foundation
 *   Author: George Lebl <jirka@5z.com>
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
#include <capplet-widget.h>
#include <libgnomeui/gnome-window-icon.h>

static gboolean changing = FALSE;

static void
changed_cb(GtkWidget *button, GtkWidget *capplet)
{
	if(changing)
		return;
	capplet_widget_state_changed(CAPPLET_WIDGET(capplet), TRUE);
}

static void
cb_enable_cb(GtkWidget *button, GtkWidget *enable)
{
	gtk_widget_set_sensitive(enable,GTK_TOGGLE_BUTTON(button)->active);
}

/*FIXME: we need some help for this I guess*/
#if 0
static void
help(GtkWidget *capplet)
{
  gchar *tmp, *helpfile;

  helpfile = "blahblah.html#BLAHBLAH";

  tmp = gnome_help_file_find_file ("users-guide", helpfile);
  if (tmp) {
    gnome_help_goto(0, tmp);
    g_free(tmp);
  } else {
    GtkWidget *mbox;

    mbox = gnome_message_box_new(_("No help is available/installed for these settings. Please make sure you\nhave the GNOME User's Guide installed on your system."),
				 GNOME_MESSAGE_BOX_ERROR,
				 _("Close"), NULL);

    gtk_widget_show(mbox);
  }
}
#endif

static gboolean hints_enabled = TRUE;
static gboolean display_fortune = FALSE;
static gboolean display_motd = FALSE;
static char *motdfile = NULL;

static GtkWidget *enable_box;
static GtkWidget *fortune_box;
static GtkWidget *motd_box;
static GtkWidget *motd_entry;

static void
loadup_vals(void)
{
	hints_enabled = gnome_config_get_bool("/Gnome/Login/RunHints=true");
	display_fortune = gnome_config_get_bool("/Gnome/Login/HintsAreFortune=false");
	display_motd = gnome_config_get_bool("/Gnome/Login/HintsAreMotd=false");
	g_free(motdfile);
	motdfile = gnome_config_get_string("/Gnome/Login/MotdFile=/etc/motd");
}

static void
try(GtkWidget *capplet, gpointer data)
{
	gnome_config_set_bool("/Gnome/Login/RunHints",
			      GTK_TOGGLE_BUTTON(enable_box)->active);
	gnome_config_set_bool("/Gnome/Login/HintsAreFortune",
			      GTK_TOGGLE_BUTTON(fortune_box)->active);
	gnome_config_set_bool("/Gnome/Login/HintsAreMotd",
			      GTK_TOGGLE_BUTTON(motd_box)->active);
	gnome_config_set_string("/Gnome/Login/MotdFile",
				gtk_entry_get_text(GTK_ENTRY(motd_entry)));
	gnome_config_sync();
}

static void
revert(GtkWidget *capplet, gpointer data)
{
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(enable_box),
				    hints_enabled);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(fortune_box),
				    display_fortune);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(motd_box),
				    display_motd);
	gtk_entry_set_text(GTK_ENTRY(motd_entry),motdfile);
	try(capplet,data);
}

static void
setup_the_ui(GtkWidget *the_capplet)
{
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *w,*e,*l;

	gtk_container_set_border_width(GTK_CONTAINER(the_capplet), GNOME_PAD);
	
	vbox = gtk_vbox_new(FALSE,GNOME_PAD_SMALL);
	gtk_container_add(GTK_CONTAINER(the_capplet), vbox);
	
	/*add the enable box*/
	enable_box = gtk_check_button_new_with_label(_("Enable login hints"));
	gtk_box_pack_start(GTK_BOX(vbox),enable_box,FALSE,FALSE,0);
	gtk_signal_connect(GTK_OBJECT(enable_box),"toggled",
			   GTK_SIGNAL_FUNC(changed_cb),
			   the_capplet);

	/*add the hint box*/
	w = gtk_radio_button_new_with_label(NULL,_("Display normal hints"));
	gtk_box_pack_start(GTK_BOX(vbox),w,FALSE,FALSE,0);
	gtk_signal_connect(GTK_OBJECT(w),"toggled",
			   GTK_SIGNAL_FUNC(changed_cb),
			   the_capplet);

	/*add the fortune box*/
	fortune_box = gtk_radio_button_new_with_label(
		gtk_radio_button_group(GTK_RADIO_BUTTON(w)),
		_("Display fortunes instead of hints"));
	gtk_box_pack_start(GTK_BOX(vbox),fortune_box,FALSE,FALSE,0);
	gtk_signal_connect(GTK_OBJECT(fortune_box),"toggled",
			   GTK_SIGNAL_FUNC(changed_cb),
			   the_capplet);

	/*add the motd box*/
	motd_box = gtk_radio_button_new_with_label(
		gtk_radio_button_group(GTK_RADIO_BUTTON(w)),
		_("Display message of the day instead of hints"));
	gtk_box_pack_start(GTK_BOX(vbox),motd_box,FALSE,FALSE,0);
	gtk_signal_connect(GTK_OBJECT(motd_box),"toggled",
			   GTK_SIGNAL_FUNC(changed_cb),
			   the_capplet);

	/*the motd entry*/
	hbox = gtk_hbox_new(FALSE,GNOME_PAD_SMALL);
	gtk_box_pack_start(GTK_BOX(vbox),hbox,FALSE,FALSE,0);
	l = gtk_label_new(_("Message of the day file to use: ")),
	gtk_box_pack_start(GTK_BOX(hbox), l, FALSE,FALSE,0);
	e = gnome_file_entry_new("motd",_("Browse"));
	gtk_box_pack_start(GTK_BOX(hbox),e,TRUE,TRUE,0);
	motd_entry = gnome_file_entry_gtk_entry(GNOME_FILE_ENTRY(e));
	gtk_signal_connect(GTK_OBJECT(motd_entry),"changed",
			   GTK_SIGNAL_FUNC(changed_cb),
			   the_capplet);

	/*here we set up the enable disable thing*/
	gtk_signal_connect(GTK_OBJECT(enable_box),"toggled",
			   GTK_SIGNAL_FUNC(cb_enable_cb),
			   w);
	gtk_signal_connect(GTK_OBJECT(enable_box),"toggled",
			   GTK_SIGNAL_FUNC(cb_enable_cb),
			   fortune_box);
	gtk_signal_connect(GTK_OBJECT(enable_box),"toggled",
			   GTK_SIGNAL_FUNC(cb_enable_cb),
			   motd_box);
	/*!!!we need to use different widgets for enable box and motd_box*/
	gtk_signal_connect(GTK_OBJECT(enable_box),"toggled",
			   GTK_SIGNAL_FUNC(cb_enable_cb),
			   hbox);
	gtk_signal_connect(GTK_OBJECT(motd_box),"toggled",
			   GTK_SIGNAL_FUNC(cb_enable_cb),
			   e);
	gtk_signal_connect(GTK_OBJECT(motd_box),"toggled",
			   GTK_SIGNAL_FUNC(cb_enable_cb),
			   l);
	



	/*setup defaults*/
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(enable_box),
				    hints_enabled);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(fortune_box),
				    display_fortune);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(motd_box),
				    display_motd);
	gtk_entry_set_text(GTK_ENTRY(motd_entry),motdfile);

	/* Finished */
	gtk_widget_show_all(the_capplet);
	gtk_signal_connect(GTK_OBJECT(the_capplet), "try",
			   GTK_SIGNAL_FUNC(try), NULL);
	gtk_signal_connect(GTK_OBJECT(the_capplet), "revert",
			   GTK_SIGNAL_FUNC(revert), NULL);
	gtk_signal_connect(GTK_OBJECT(the_capplet), "ok",
			   GTK_SIGNAL_FUNC(try), NULL);
	gtk_signal_connect(GTK_OBJECT(the_capplet), "cancel",
			   GTK_SIGNAL_FUNC(revert), NULL);
	/*gtk_signal_connect(GTK_OBJECT(the_capplet), "help",
			   GTK_SIGNAL_FUNC(help), NULL);*/
}

int
main (int argc, char **argv)
{
	GtkWidget *the_capplet;

	bindtextdomain(PACKAGE, GNOMELOCALEDIR);
	textdomain(PACKAGE);

	changing = TRUE;

	if(gnome_capplet_init("gnome-hint-properties", VERSION, argc,
			      argv, NULL, 0, NULL) < 0)
		return 1;
	gnome_window_icon_set_default_from_file (GNOME_ICONDIR"/gnome-hint.png");
	loadup_vals();

	the_capplet = capplet_widget_new();

	setup_the_ui(the_capplet);

	changing = FALSE;
	capplet_gtk_main();

	return 0;
}
