#include "tasklist_applet.h"

/* The configuration */
TasklistConfig Config;

void read_config (void)
{
  /* This should be fixed */
  Config.width = 450;
  Config.rows = 2;
  Config.show_pixmaps = TRUE;
  Config.confirm_before_kill = TRUE;
}
