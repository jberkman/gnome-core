#include <config.h>
#include <stdio.h>

#include <gnome.h>
#include "gnome-desktop.h"

const struct poptOption parser[] = {
  {"init", '\0', POPT_ARG_NONE, &init, 0, N_("Set parameters from saved state and exit"), NULL}, 
  {NULL, '\0', 0, NULL, 0}
};
