/* save-session.c - Small program to talk to session manager.

   Copyright (C) 1998 Tom Tromey

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.  */

#include <config.h>

#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>

#include "libgnome/libgnome.h"
#include "libgnomeui/gnome-client.h"

/* Parsing function.  */
static error_t parse_an_arg (int key, char *arg, struct argp_state *state);

/* Arguments we understand.  */
static struct argp_option options[] =
{
  { "kill", -1, NULL, 0, N_("Kill session"), 1 },
  { NULL, 0, NULL, 0, NULL, 0 }
};

/* Our argument parser.  */
static struct argp parser =
{
  options,
  parse_an_arg,
  NULL,
  NULL,
  NULL,
  NULL,
  PACKAGE
};

/* True if killing.  */
static int zap = 0;

static error_t
parse_an_arg (int key, char *arg, struct argp_state *state)
{
  if (key != -1)
    return ARGP_ERR_UNKNOWN;
  zap = 1;
  return 0;
}

int
main (int argc, char *argv[])
{
  int zap = 0, i;
  GnomeClient *client;

  argp_program_version = VERSION;

  /* Initialize the i18n stuff */
  bindtextdomain (PACKAGE, GNOMELOCALEDIR);
  textdomain (PACKAGE);

  client = gnome_client_new_default ();

  gnome_init ("session", &parser, argc, argv, 0, NULL);

  gnome_client_set_restart_style (client, GNOME_RESTART_NEVER);
  /* We could expose more of the arguments to the user if we wanted
     to.  Some of them aren't particularly useful.  Interestingly,
     there is no way to request a shutdown without a save.  */
  gnome_client_request_save (client, GNOME_SAVE_BOTH, zap,
			     GNOME_INTERACT_ANY, 0, 1);
  gnome_client_disconnect (client);

  return 0;
}
