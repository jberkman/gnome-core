/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *   app-ui.c: Application for configuring look and feel
 *      
 *   Copyright (C) 1998 Free Software Foundation 
 *   Author: Havoc Pennington <hp@pobox.com>
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
#include "gnome-desktop.h"

extern void ui_register (GnomePropertyConfigurator *c);

char *
application_title (void)
{
  return _("Gnome Look and Feel Properties");
}

char *
application_property (void)
{
  return "GNOME_UI_PROPERTY";
}

void
application_help (void)
{
  g_warning("Help not implemented\n");
}

void
application_register (GnomePropertyConfigurator *pconf)
{
  ui_register (pconf);
}

int
main (int argc, char *argv[])
{
  return (property_main ("ui_properties", argc, argv));
}

