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
  gnome_config_sync();
  tmp = gnome_config_get_string(tmpg->str);

  g_print("Doing callback %s\n", tmp);

  if(tmp)
    runcmd_callback(tmp);
  
  g_free(tmp);
}

gboolean send_command_to_running(char *string, void (*callback)(char *))
{
  int tmpi;
  gboolean tmpd;
  gboolean retval = FALSE;
  struct sigaction act;

  tmpg = g_string_new(NULL);

  gnome_config_sync();

  g_string_sprintf(tmpg, "%s/instance/pid=0", gnome_app_id);
  tmpi = gnome_config_get_int_with_default(tmpg->str, &tmpd);

  if(string)
    {
      g_string_sprintf(tmpg, "%s/instance/runcmd", gnome_app_id);
      gnome_config_set_string(tmpg->str, string);
    }

  gnome_config_sync();

  g_print("tmpd = %d, tmpi = %d\n", tmpd, tmpi);

  if(!tmpd && tmpi && !kill(tmpi, SIGUSR2))
    {
      retval = TRUE;
    }
  else
    {
      g_string_sprintf(tmpg, "%s/instance/pid", gnome_app_id);
      gnome_config_set_int(tmpg->str, getpid());
      g_print("Set string %s to %d\n", tmpg->str, getpid());
      runcmd_callback = callback;
      act.sa_handler = do_cb;
      act.sa_flags = SA_NODEFER;
      sigaction(SIGUSR2, &act, NULL);
    }

  gnome_config_sync();

  g_print("return %d\n", retval);

  return retval;
}
