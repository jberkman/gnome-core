/* GNOME Help Browser */
/* Copyright (C) 1998 Red Hat Software, Inc    */
/* Written by Michael Fulbright and Marc Ewing */

/*
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <gnome.h>
#include "window.h"

int
main(int argc, char *argv[])
{
        /* XXX - About box should be handled here, not in newWindow() */
        /* Global application history should be here as well          */
    
	gnome_init("gnome_help_browser", &argc, &argv);
	
	newWindow((argc > 1) ? argv[1] : NULL);
	
	gtk_main();
	
	return 0;
}
