/*  GemVt - GNU Emulator of a Virtual Terminal
 *  Copyright (C) 1997	Tim Janik
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
#include	"config.h"
#include	"gvtconf.h"
#include	<getopt.h>
#include	<stdlib.h>
#include	<string.h>
#include        <ctype.h>



/* --- defines --- */
#define	EXIT_PROCESS	(-129)


/* --- variables --- */
gchar	*prg_name = NULL;


/* --- command line arguments --- */
enum {
  ARG_HELP		= 'h',
  ARG_EXEC		= 'e',
  ARG_STATUS_BAR	= 'x',
  ARG_IGNORE	= 256,
  ARG_VERSION,
};
static gchar		short_options[] = {
  ARG_HELP,
  ARG_EXEC, ':',
  ARG_STATUS_BAR, ':', ':',
  0
};
static struct option	long_options[] = {
  { "help",		no_argument,		NULL,	ARG_HELP	},
  { "execute",		required_argument,	NULL,	ARG_EXEC	},
  { "status-bar",	optional_argument,	NULL,	ARG_STATUS_BAR	},
  { "version",		no_argument,		NULL,	ARG_VERSION	},
  /* Gdk options */
  { "display",            required_argument,	NULL,	ARG_IGNORE	},
  { "sync",               no_argument,          NULL,   ARG_IGNORE	},
  { "show-events",        no_argument,          NULL,   ARG_IGNORE	},
  { "no-show-events",     no_argument,          NULL,   ARG_IGNORE	},
  { "no-xshm",            no_argument,          NULL,   ARG_IGNORE	},
  { "debug-level",        required_argument,    NULL,   ARG_IGNORE	},
  { "name",		  required_argument,    NULL,   ARG_IGNORE	},
  { "class",		  required_argument,    NULL,   ARG_IGNORE	},
  { "gxid_host",	  required_argument,    NULL,   ARG_IGNORE	},
  { "gxid_port",	  required_argument,    NULL,   ARG_IGNORE	},
  { "xim-preedit",	  required_argument,    NULL,   ARG_IGNORE	},
  { "xim-status",	  required_argument,    NULL,   ARG_IGNORE	},
  { NULL, 0, NULL, 0 }
};

/* --- functions --- */
gint
gvt_config_args	(GvtConfig	*config,
		 FILE		*f_error,
		 gint		argc,
		 gchar		*argv[])
{
  register gint ch;
  register gint exit_status;

  g_assert (config);
  g_assert (f_error);
  g_assert (argc > 0);
  g_assert (argv);

  /* initialize getopt()
   */
  optarg = NULL;
  optind = 0;
  optopt = 0;

  /* don't let getopt() print error messages
   */
  opterr = 0;

  ch = ~EOF;
  exit_status = EXIT_PROCESS;
  while (ch != EOF && exit_status == EXIT_PROCESS)
  {
    static int longindex;

    longindex = -1;
    ch = getopt_long (argc, argv, short_options, long_options, &longindex);
    switch (ch)
    {
      register gchar	*string;
      register guint	i;

    case  ARG_HELP:
      string = g_downcase (g_strdup (prg_name));
      fprintf (f_error,
	       "usage: %s [ Options ] [ -e <program> [program-args] ]\n"
	       "Options:\n"
	       "  -e, --execute\t\t"	"execute <program>, should be last arg!\n"
	       "  -x, --status-bar\t"	"have status bar? [{0|1}]\n"
	       "  -h, --help\t\t"	"print usage and exit successfully\n"
	       "  --version\t\t"	"print version and exit successfully\n",
	       string);
      g_free (string);
      exit_status = 0;
      break;

    case  EOF:
      break;

    case  ARG_IGNORE:
      break;

    case  ARG_STATUS_BAR:
      if (optarg)
	config->have_status_bar = strtol (optarg, NULL, 10) != 0;
      else
	config->have_status_bar = TRUE;
      break;

    case  ARG_VERSION:
      fprintf (f_error, "%s %s\n", PRGNAME_LONG, VERSION);
      exit_status = 0;
      break;

    case '?':
    default:
      if (optopt != '?' || ch != '?')
      {
	if (ch != '?')
	  fprintf (f_error, "%s: invalid option `%c'\n", prg_name, ch);
	else if (optopt != ':' && strchr (short_options, optopt))
	  fprintf (f_error, "%s: option `%c' misses argument\n", prg_name, optopt);
	else
	  fprintf (f_error, "%s: invalid option `%c'\n", prg_name, optopt);
      }
      else
      {
	g_assert (strlen (argv [optind - 1]) > 1);

	i = 0;
	while (long_options[i].name &&
	       strcmp (long_options[i].name, &argv[optind - 1][2]) != 0)
	  i++;
	if (long_options[i].name)
	  fprintf (f_error, "%s: option `%s' misses argument\n", prg_name, long_options[i].name);
	else
	  fprintf (f_error, "%s: invalid option `%s'\n", prg_name, argv[optind - 1]);
      }
      exit_status = -1;
      break;
    }
  }

  if (exit_status == EXIT_PROCESS)
    while (optind < argc)
    {
      config->strings = g_list_append (config->strings, argv[optind++]);
    }
  
  return exit_status;
}

gchar*
g_downcase (gchar  *string)
{
  register gchar *s;

  g_return_val_if_fail (string, NULL);

  s = string;

  while (*s)
  {
    *s = tolower (*s);
    s++;
  }

  return string;
}

gchar*
g_upcase (gchar  *string)
{
  register gchar *s;

  g_return_val_if_fail (string, NULL);

  s = string;

  while (*s)
  {
    *s = toupper (*s);
    s++;
  }

  return string;
}
