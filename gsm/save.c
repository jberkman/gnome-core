/* save.c - Code to save session.
   Written by Tom Tromey <tromey@cygnus.com>.  */

#include <glib.h>
#include <string.h>

#include "libgnome/libgnome.h"
#include "libgnomeui/gnome-session.h"
#include "manager.h"



/* Default session name.  */
#define DEFAULT_SESSION "Default"

/* Name of current session.  A NULL value means the default.  */
static char *session_name = NULL;

/* This is used to hold a table of all properties we care to save.  */
typedef struct
{
  const char *name;		/* Name of property.  */
  gboolean is_vector;		/* TRUE if vector.  */
  gboolean required;		/* TRUE if required (by us, not by the
				   spec).  */
} propsave;

static propsave properties[] =
{
  { SmCurrentDirectory, 0, 0 },
  { SmDiscardCommand, 1, 0 },
  { SmRestartCommand, 1, 1 },
  { GNOME_SM_INIT_COMMAND, 1, 0 }
};

#define NUM_PROPERTIES (sizeof (properties) / sizeof (propsave))



/* Write a single client to the current session.  Return 1 on success,
   0 on failure.  */
static int
write_one_client (int number, const Client *client)
{
  /* We over-allocate; it doesn't matter.  */
  int i, j, vec_count, string_count, argcs[NUM_PROPERTIES], failure;
  char **argvs[NUM_PROPERTIES];
  char *strings[NUM_PROPERTIES];
  const char *argv_names[NUM_PROPERTIES];
  const char *string_names[NUM_PROPERTIES];

  /* Read each property we care to save.  */
  failure = 0;
  vec_count = string_count = 0;
  for (i = 0; i < NUM_PROPERTIES; ++i)
    {
      if (properties[i].is_vector)
	{
	  if (! find_vector_property (client, properties[i].name,
				      &argcs[vec_count], &argvs[vec_count]))
	    {
	      if (properties[i].required)
		{
		  failure = 1;
		  break;
		}
	    }
	  else
	    argv_names[vec_count++] = properties[i].name;
	}
      else
	{
	  if (! find_string_property (client, properties[i].name,
				      &strings[string_count]))
	    {
	      if (properties[i].required)
		{
		  failure = 1;
		  break;
		}
	    }
	  else
	    string_names[string_count++] = properties[i].name;
	}
    }

  /* Write each property we found.  */
  if (! failure)
    {
      gnome_config_set_string ("id", client->id);

      for (i = 0; i < vec_count; ++i)
	gnome_config_set_vector (argv_names[i], argcs[i],
				 (const char * const *) argvs[i]);

      for (i = 0; i < string_count; ++i)
	gnome_config_set_string (string_names[i], strings[i]);
    }

  /* Clean up.  */
  for (i = 0; i < vec_count; ++i)
    free_vector (argcs[i], argvs[i]);
  for (i = 0; i < string_count; ++i)
    free (strings[i]);

  return ! failure;
}

/* Actually write the session data.  */
void
write_session (const GSList *list, int shutdown)
{
  char prefix[1024];
  int i;

  /* This is somewhat losing.  But we really do want to make sure any
     existing session with this same name has been cleaned up before
     we write the new info.  */
  delete_session (session_name);

  i = 0;
  for (; list; list = list->next)
    {
      Client *client = (Client *) list->data;
      sprintf (prefix, "gnome/%s/%d,",
	       session_name ? session_name : DEFAULT_SESSION,
	       i);
      gnome_config_push_prefix (prefix);
      if (write_one_client (i, client))
	++i;
      gnome_config_pop_prefix ();
    }

  sprintf (prefix, "gnome/%s/num_clients",
	   session_name ? session_name : DEFAULT_SESSION);
  gnome_config_set_int (prefix, i);

  gnome_config_sync ();
}

/* Set current session name.  */
void
set_session_name (const char *name)
{
  if (session_name)
    free (session_name);
  if (name)
    session_name = strdup (name);
  else
    session_name = NULL;
}



/* Run a set of commands from a session.  Return 1 if any were run.  */
static int
run_commands (const char *name, int number, const char *command,
	      execute_func *executor)
{
  int i, result = 0;

  /* Run each delete command.  */
  for (i = 0; i < number; ++i)
    {
      int argc, def, j;
      char **argv, prefix[1024];

      sprintf (prefix, "gnome/%s/%d,%s=", name, i, command);
      gnome_config_get_vector_with_default (prefix, &argc, &argv, &def);

      if (! def)
	{
	  (*executor) (argc, argv);
	  result = 1;
	}

      free_vector (argc, argv);
    }

  return result;
}



/* Load a new session.  This does not shut down the current session.
   Returns 1 if anything happened, 0 otherwise.  */
int
read_session (const char *name)
{
  int i, def, num_clients;
  char prefix[1024];

  if (! session_name)
    set_session_name (name);
  if (! name)
    name = DEFAULT_SESSION;

  sprintf (prefix, "gnome/%s/num_clients=-1", name);
  num_clients = gnome_config_get_int_with_default (prefix, &def);

  /* If default, then no client info exists.  */
  if (def)
    return 0;

  /* We must register each saved client as a `zombie' client.  Then
     when the client restarts it will get its new client id
     correctly.  */
  for (i = 0; i < num_clients; ++i)
    {
      char *id;

      sprintf (prefix, "gnome/%s/%d,id", name, i);
      id = gnome_config_get_string (prefix);
      if (id)
	add_zombie (id);
    }

  /* Run each initialization command.  */
  run_commands (name, num_clients, GNOME_SM_INIT_COMMAND, execute_once);

  /* Run each restart command.  */
  return run_commands (name, num_clients, SmRestartCommand, execute_async);
}

/* Delete a session.  */
void
delete_session (const char *name)
{
  int i, def, number;
  char prefix[1024];

  if (! name)
    name = DEFAULT_SESSION;

  sprintf (prefix, "gnome/%s/num_clients=-1", name);
  number = gnome_config_get_int_with_default (prefix, &def);
  if (def)
    {
      /* No client info to get, so just bail.  */
      return;
    }

  run_commands (name, number, SmDiscardCommand, execute_async);

  sprintf (prefix, "gnome/%s", name);
  gnome_config_clean_section (prefix);
  gnome_config_sync ();
}
