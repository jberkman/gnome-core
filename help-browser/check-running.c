#include <gnome.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>

static void (*runcmd_callback)(char *str);

static GString *tmpg;

static void
do_cb(void)
{
  char *tmp;

  g_string_sprintf(tmpg, "%s/instance/runcmd", gnome_app_id);
  tmp = gnome_config_get_string(tmpg->str);

  g_message("Doing callback %s\n", tmp);

  if(tmp)
    runcmd_callback(tmp);
  
  g_free(tmp);
}

gboolean send_command_to_running(char *string, void (*callback)(char *))
{
  int pid;
  gboolean pidMissing;
  struct sigaction act;

  tmpg = g_string_new(NULL);

  gnome_config_sync();

  g_string_sprintf(tmpg, "%s/instance/pid=0", gnome_app_id);
  pid = gnome_config_get_int_with_default(tmpg->str, &pidMissing);

  if (string) {
      g_string_sprintf(tmpg, "%s/instance/runcmd", gnome_app_id);
      gnome_config_set_string(tmpg->str, string);
      /*g_message("Set string %s to %s", tmpg->str, string);*/
      gnome_config_sync();
  }

  /*g_message("pidMissing = %d, pid = %d", pidMissing, pid);*/

  if (!pidMissing && pid && !kill(pid, SIGUSR2)) {
      /* We are done.  Someone else is handling this document now. */
      return TRUE;
  }

  /* Set up to handle incoming signals */
  
  g_string_sprintf(tmpg, "%s/instance/pid", gnome_app_id);
  gnome_config_set_int(tmpg->str, getpid());
  gnome_config_sync();
  /*g_message("Set string %s to %d", tmpg->str, getpid());*/
  
  runcmd_callback = callback;
  act.sa_handler = do_cb;
  act.sa_flags = SA_NODEFER;
  sigaction(SIGUSR2, &act, NULL);

  return FALSE;
}
