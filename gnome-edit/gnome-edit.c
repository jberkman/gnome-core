/* GnomeEdit
 * Copyright (C) 1999 Chris Lahey <clahey@umich.edu>
 *
 * Author: Chris Lahey <clahey@umich.edu>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc.,  59 Temple Place - Suite 330, Cambridge, MA 02139, USA.
 */

#include <glib.h>
#include <libgnome/gnome-defs.h>
#include <libgnome/gnome-config.h>
#include <libgnome/libgnome.h>
#include <string.h>
#include <unistd.h>

gint
main( gint argc, gchar *argv[])
{
  gchar *executable, *type;
  gchar **new_argv;
  gint i, j;
  gboolean recognizes_lineno;
  gboolean needs_term;

  gnomelib_init( "gnome-edit", "0.1.0" );
  
  gnome_config_push_prefix( "/editor/Editor/" );
  type = gnome_config_get_string( "EDITOR_TYPE=executable" );
  executable = gnome_config_get_string( "EDITOR=emacs" );
  recognizes_lineno = gnome_config_get_bool( "ACCEPTS_LINE_NO=TRUE" );
  needs_term = gnome_config_get_bool( "NEEDS_TERM=FALSE" );
  gnome_config_pop_prefix();
  gnome_config_sync();

  new_argv = g_new( char *, argc + 1 );
  for ( i = 0, j = 0; i < argc; i++ )
    {
      if ( ! strncmp( argv[i], "--", 2 ) )
	{
	  if ( strlen( argv[i] ) == 2 )
	    {
	      new_argv[j] = argv[i];
	      i++;
	      j++;
	      break;
	    }
	  if ( !strncmp( argv[i], "--line-number", strlen( "--line-number" ) ) )
	    {
	      if ( strlen( argv[i] ) != strlen( "--line-number" ) )
		{
		  if ( recognizes_lineno )
		    {
		      new_argv[j] = g_strdup_printf( "+%s", argv[i]+strlen( "--line-number" ) );
		      j++;
		    }
		}
	      else
		{
		  i++;
		  if ( i < argc )
		    {
		      if ( recognizes_lineno )
			{
			  new_argv[j] = g_strdup_printf( "+%s", argv[i] );
			  j++;
			}
		    }
		}
	    }
	}
      else
	{
	  new_argv[j] = argv[i];
	  j++;
	}
    }
  for ( ; i < argc; i++, j++ )
    {
      new_argv[j] = argv[i];
    }
  new_argv[j] = NULL;

  if ( ! strcmp ( type, "executable" ) )
    {
      *argv = executable;
      if ( execvp( executable, new_argv ) )
	g_error( "Error during execution of chosen editor.  The editor you have chosen is probably either not available, or is not on your current path." );
    }
  else
    {
      g_error( "Alternate editor types are not supported by gnome-edit yet.  Please choose a standard executable editor in the gnome-edit capplet in the gnome control center." );
    }
  return 0;
}
