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
#include "capplet-widget.h"

static gboolean changing = FALSE;

static void
checkbutton_cb(GtkWidget *button, GtkWidget *capplet)
{
	if(changing)
		return;
	capplet_widget_state_changed(CAPPLET_WIDGET(capplet), TRUE);
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

static GtkWidget *enable_box;
static GtkWidget *fortune_box;

static void
loadup_vals(void)
{
	hints_enabled = gnome_config_get_bool("/Gnome/Login/RunHints=true");
	display_fortune = gnome_config_get_bool("/Gnome/Login/HintsAreFortune=false");
}

static void
try(GtkWidget *capplet, gpointer data)
{
	gnome_config_set_bool("/Gnome/Login/RunHints",
			      GTK_TOGGLE_BUTTON(enable_box)->active);
	gnome_config_set_bool("/Gnome/Login/HintsAreFortune",
			      GTK_TOGGLE_BUTTON(fortune_box)->active);
	gnome_config_sync();
}

static void
revert(GtkWidget *capplet, gpointer data)
{
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(enable_box),
				    hints_enabled);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(fortune_box),
				    display_fortune);
	try(capplet,data);
}

static void
setup_the_ui(GtkWidget *the_capplet)
{
	GtkWidget *vbox;

	gtk_container_set_border_width(GTK_CONTAINER(the_capplet), GNOME_PAD);
	
	vbox = gtk_vbox_new(FALSE,GNOME_PAD_SMALL);
	gtk_container_add(GTK_CONTAINER(the_capplet), vbox);
	
	/*add the enable box*/
	enable_box = gtk_check_button_new_with_label(_("Enable login hints"));
	gtk_box_pack_start(GTK_BOX(vbox),enable_box,FALSE,FALSE,0);
	gtk_signal_connect(GTK_OBJECT(enable_box),"toggled",
			   GTK_SIGNAL_FUNC(checkbutton_cb),
			   the_capplet);

	/*add the fortune box*/
	fortune_box = gtk_check_button_new_with_label(_("Display fortunes instead of hints"));
	gtk_box_pack_start(GTK_BOX(vbox),fortune_box,FALSE,FALSE,0);
	gtk_signal_connect(GTK_OBJECT(fortune_box),"toggled",
			   GTK_SIGNAL_FUNC(checkbutton_cb),
			   the_capplet);

	/*setup defaults*/
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(enable_box),
				    hints_enabled);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(fortune_box),
				    display_fortune);

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
	
	loadup_vals();

	the_capplet = capplet_widget_new();

	setup_the_ui(the_capplet);

	changing = FALSE;
	capplet_gtk_main();

	return 0;
}
