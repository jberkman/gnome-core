#include <config.h>
#include <stdio.h>

#include <gnome.h>
#include "gnome-desktop.h"

/* Options used by this program.  */
static struct argp_option arguments[] =
{
  { "init", -1, NULL, 0,
    N_("Set parameters from saved state and exit"), 1 },
  { NULL, 0, NULL, 0, NULL, 0 }
};

/* Forward decl of our parsing function.  */
static error_t parse_func (int key, char *arg, struct argp_state *state);

/* The parser used by this program.  */
struct argp parser =
{
  arguments,
  parse_func,
  NULL,
  NULL,
  NULL,
  NULL
};

static error_t
parse_func (int key, char *arg, struct argp_state *state)
{
  if (key == ARGP_KEY_ARG)
    {
      /* This program has no command-line options.  */
      argp_usage (state);
    }
  else if (key != -1)
    return ARGP_ERR_UNKNOWN;

  init = 1;
  return 0;
}

