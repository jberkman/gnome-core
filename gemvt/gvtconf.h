/*  GemVt - GNU Emulator of a Virtual Terminal
 *  Copyright (C) 1997  Tim Janik
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifndef __GVTCONF_H__
#define __GVTCONF_H__

#include	<glib.h>
#include        <stdio.h>


#ifdef __cplusplus
extern "C" {
#pragma }
#endif /* __cplusplus */


/* --- typedefs --- */
typedef	struct	_GvtConfig	GvtConfig;


/* --- structures --- */
struct	_GvtConfig
{
  gboolean	have_status_bar;
  gboolean	prefix_dash;
  GList		*strings;
};


/* --- variables --- */
extern	gchar	*prg_name;


/* --- prototypes -- */
/* parse command line arguments and return the exit staus
 * of the program. if the returned value is less than -128
 * proceed with execution.
 */
gint    gvt_config_args		(GvtConfig      *config,
				 FILE           *f_error,
				 gint           argc,
				 gchar          *argv[]);
gchar*	g_downcase		(gchar		*string);
gchar*	g_upcase		(gchar		*string);
     





#ifdef __cplusplus
#pragma {
}
#endif /* __cplusplus */


#endif /* __GVTCONF_H__ */
