/* save.c - Code to save session.

   Copyright (C) 1998 Tom Tromey

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.  */

#include <config.h>
#include <glib.h>
#include <string.h>
#include <gtk/gtk.h>

#include "libgnome/libgnome.h"
#include "libgnomeui/gnome-client.h"
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
  { SmEnvironment, 1, 0 }
};

#define NUM_PROPERTIES (sizeof (properties) / sizeof (propsave))



/* Write a single client to the current session.  Return 1 on success,
   0 on failure.  */
static int
write_one_client (int number, const Client *client)
{
  /* We over-allocate; it doesn't matter.  */
  int i, vec_count, string_count, argcs[NUM_PROPERTIES], failure;
  char **argvs[NUM_PROPERTIES];
  char *strings[NUM_PROPERTIES];
  const char *argv_names[NUM_PROPERTIES];
  const char *string_names[NUM_PROPERTIES];
  int style;

  /* Do nothing with RestartNever clients.  */
  if (find_card8_property (client, SmRestartStyleHint, &style)
      && style == SmRestartNever)
    return 0;

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
write_session (const GSList *list1, const GSList *list2, int shutdown)
{
  char prefix[1024];
  int i, step;
  const GSList *list;

  /* This is somewhat losing.  But we really do want to make sure any
     existing session with this same name has been cleaned up before
     we write the new info.  */
  delete_session (session_name);

  i = 0;
  step = 0;
  list = list1;
  while (step < 2)
    {
      for (; list; list = list->next)
	{
	  Client *client = (Client *) list->data;
	  sprintf (prefix, "session/%s/%d,",
		   session_name ? session_name : DEFAULT_SESSION,
		   i);
	  gnome_config_push_prefix (prefix);
	  if (write_one_client (i, client))
	    ++i;
	  gnome_config_pop_prefix ();
	}
      list = list2;
      ++step;
    }

  sprintf (prefix, "session/%s/num_clients",
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
run_commands (const char *name, int number, const char *command)
{
  int i, result = 0;

  /* Run each command.  */
  for (i = 0; i < number; ++i)
    {
      int argc, envc;
      gboolean def, envd;
      char **argv, *dir, prefix[1024], **envv, **envp;

      sprintf (prefix, "session/%s/%d,%s=", name, i, SmCurrentDirectory);
      dir = gnome_config_get_string (prefix);
      sprintf (prefix, "session/%s/%d,%s=", name, i, command);
      gnome_config_get_vector_with_default (prefix, &argc, &argv, &def);

      sprintf (prefix, "session/%s/%d,%s=", name, i, SmEnvironment);
      gnome_config_get_vector_with_default (prefix, &envc, &envv, &envd);
      if (envd)
	envp = NULL;
      else
	{
	  char **newenv = (char **) malloc ((envc / 2 + 2) * sizeof (char *));
	  int i;

	  for (i = 0; i < envc / 2; ++i)
	    newenv[i] = g_copy_strings (envv[2 * i], "=", envv[2 * i + 1],
					NULL);
	  newenv[i] = NULL;
	  free_vector (envc, envv);
	  envc = i;
	  envv = newenv;
	  envp = newenv;
	}

      if (! def)
	{
	  gnome_execute_async_with_env (dir, argc, argv, envc, envp);
	  result = 1;
	}

      free_vector (argc, argv);
      free_vector (envc, envv);
      if (dir)
	free (dir);
    }

  return result;
}



/* Run the default session.  The default should probably be in a
   config file somewhere, instead of hard-coded here.  */
static int
run_default_session (void)
{
  char *argv[5];

  argv[0] = "panel";
  argv[1] = NULL;
  gnome_execute_async (NULL, 1, argv);

  argv[0] = "gnome-help-browser";
  gnome_execute_async (NULL, 1, argv);

  argv[0] = "gmc";
  gnome_execute_async (NULL, 1, argv);

#ifdef WINDOW_MANAGER
  argv[0] = WINDOW_MANAGER;
#else
  /* icewm is the default because it actually implements session
     management.  */
  argv[0] = "icewm";
#endif
  gnome_execute_async (NULL, 1, argv);

  argv[0] = "smproxy";
  gnome_execute_async (NULL, 1, argv);

  return 1;
}

/* Load a new session.  This does not shut down the current session.
   Returns 1 if anything happened, 0 otherwise.  */
int
read_session (const char *name)
{
  int i, num_clients;
  char prefix[1024];

  if (! session_name)
    set_session_name (name);
  if (! name)
    name = DEFAULT_SESSION;

  sprintf (prefix, "session/%s/num_clients=0", name);
  num_clients = gnome_config_get_int (prefix);

  /* No clients means either this is the first time gnome-session has
     been run, or the user exited everything the last time.  Either
     way, we start the default session to make sure something
     happens.  */
  if (! num_clients)
    {
      if (! strcmp (name, DEFAULT_SESSION))
	return run_default_session ();
      return 0;
    }

  /* We must register each saved client as a `zombie' client.  Then
     when the client restarts it will get its new client id
     correctly.  */
  for (i = 0; i < num_clients; ++i)
    {
      char *id;

      sprintf (prefix, "session/%s/%d,id", name, i);
      id = gnome_config_get_string (prefix);
      if (id)
	add_zombie (id);
    }

  /* Run each restart command.  */
  return run_commands (name, num_clients, SmRestartCommand);
}

/* Delete a session.  */
void
delete_session (const char *name)
{
  int number;
  gboolean def;
  char prefix[1024];

  if (! name)
    name = DEFAULT_SESSION;

  sprintf (prefix, "session/%s/num_clients=-1", name);
  number = gnome_config_get_int_with_default (prefix, &def);
  if (def)
    {
      /* No client info to get, so just bail.  */
      return;
    }

  run_commands (name, number, SmDiscardCommand);

  sprintf (prefix, "session/%s", name);
  gnome_config_clean_section (prefix);
  gnome_config_sync ();
}
