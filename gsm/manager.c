/* manager.c - Session manager back end.
   Written by Tom Tromey <tromey@cygnus.com>.  */

#include "manager.h"



/* This is true if we are saving the session.  */
static int saving;

/* This is true if the current save is in response to a shutdown
   request.  */
static int shutting_down;

/* List of all zombie clients.  A zombie client is one that was
   running in the previous session but has not yet been restored to
   life.  */
static GSList *zombie_list;

/* List of all live clients in the default state.  */
static GSList *live_list;

/* List of all clients waiting for the interaction token.  The head of
   the list actually has the token.  */
static GSList *interact_list;

/* List of all clients to which a `save yourself' message has been
   sent.  */
static GSList *save_yourself_list;

/* List of all clients which have requested a Phase 2 save.  */
static GSList *save_yourself_p2_list;

/* List of all clients which have been saved.  */
static GSList *save_finished_list;



#define APPEND(List,Elt) ((List) = (g_slist_append ((List), (Elt))))
#define REMOVE(List,Elt) ((List) = (g_slist_remove ((List), (Elt))))
#define CONCAT(L1,L2) ((L1) = (g_slist_concat ((L1), (L2))))



typedef void message_func (SmsConn connection);

/* Helper for send_message.  */
static void
do_send_message (gpointer data, gpointer user_data)
{
  Client *client = (Client *) data;
  message_func *message = (message_func *) user_data;
  (*message) (client->connection);
}

/* Send a message to every client on LIST.  */
static void
send_message (GSList *list, message_func *message)
{
  g_slist_foreach (list, do_send_message, message);
}



static Client *
find_client_by_id (GSList *list, char *id)
{
  for (; list; list = list->next)
    {
      Client *client = (Client *) list->data;
      if (! strcmp (client->id, id))
	return client;
    }
  return NULL;
}



static void
free_a_prop (gpointer data, gpointer user_data)
{
  SmProp *sp = (SmProp *) data;
  SmFreeProperty (sp);
}

static void
free_client (Client *client)
{
  if (client->id)
    free (client->id);

  g_slist_foreach (client->properties, free_a_prop, NULL);
  g_slist_free (client->properties);

  free (client);
}



static Status
register_client (SmsConn connection, SmPointer data, char *previous_id)
{
  Client *client = (Client *) data;

  if (previous_id)
    {
      /* Client from existing session.  */
      Client *old_client = find_client_by_id (zombie_list, previous_id);
      if (! old_client)
	{
	  /* Not found.  Let them try again.  */
	  free (previous_id);
	  return 0;
	}
      client->id = previous_id;
      REMOVE (zombie_list, old_client);
      free_client (old_client);
    }
  else
    {
      /* New client.  */
      client->id = SmsGenerateClientID (connection);
    }

  APPEND (live_list, client);

  return SmsRegisterClientReply (connection, client->id);
}

static void
interact_request (SmsConn connection, SmPointer data, int dialog_type)
{
  Client *client = (Client *) data;

  /* FIXME: what to do when this is sent by a client not on the
     save-yourself list?  */

  REMOVE (save_yourself_list, client);
  APPEND (interact_list, client);

  if (interact_list->data == client)
    SmsInteract (connection);
}

static void
interact_done (SmsConn connection, SmPointer data, Bool cancel)
{
  Client *client = (Client *) data;
  int start_next = 0;

  if (interact_list && interact_list->data == client)
    start_next = 1;

  REMOVE (interact_list, client);
  APPEND (save_yourself_list, client);

  if (cancel)
    {
      /* Cancel whatever we're doing.  */
      saving = 0;

      send_message (interact_list, SmsShutdownCancelled);
      CONCAT (live_list, interact_list);
      interact_list = NULL;

      send_message (save_yourself_list, SmsShutdownCancelled);
      CONCAT (live_list, save_yourself_list);
      save_yourself_list = NULL;

      send_message (save_yourself_p2_list, SmsShutdownCancelled);
      CONCAT (live_list, save_yourself_p2_list);
      save_yourself_p2_list = NULL;

      send_message (save_finished_list, SmsShutdownCancelled);
      CONCAT (live_list, save_finished_list);
      save_finished_list = NULL;
    }
  else if (interact_list && start_next)
    {
      client = (Client *) interact_list->data;
      SmsInteract (client->connection);
    }
}

static void
save_yourself_request (SmsConn connection, SmPointer data, int save_type,
		       Bool shutdown, int interact_style, Bool fast,
		       Bool global)
{
  if (saving)
    return;

  if (! global)
    {
      /* FIXME ... */
    }
  else
    {
      /* Global save.  Use same function the rest of gsm uses.  */
      save_session (save_type, shutdown, interact_style, fast);
    }
}

static void
save_yourself_p2_request (SmsConn connection, SmPointer data)
{
  Client *client = (Client *) data;
  REMOVE (save_yourself_list, client);
  APPEND (save_yourself_p2_list, client);
}

static void
save_yourself_done (SmsConn connection, SmPointer data, Bool success)
{
  Client *client = (Client *) data;
  GSList *found;

  found = g_slist_find (save_yourself_list, client);

  REMOVE (save_yourself_list, client);
  REMOVE (save_yourself_p2_list, client);
  APPEND (save_finished_list, client);

  if (! live_list && ! interact_list && ! save_yourself_list)
    {
      if (! save_yourself_p2_list)
	{
	  /* All clients have responded to the save.  Now shut down or
	     continue as appropriate.  */
	  saving = 0;
	  send_message (save_finished_list,
			shutting_down ? SmsDie : SmsSaveComplete);
	}
      else if (! found)
	{
	  /* Just saw the last ordinary client finish saving.  So tell
	     the Phase 2 clients that they're ok to go.  */
	  send_message (save_yourself_p2_list, SmsSaveYourselfPhase2);
	}
    }
}

/* FIXME: Display REASONS to user, per spec.  */
static void
close_connection (SmsConn connection, SmPointer data, int count,
		  char **reasons)
{
  Client *client = (Client *) data;
  int interact_next = 0;

  /* Just try every list.  */
  REMOVE (zombie_list, client);
  REMOVE (live_list, client);

  if (interact_list && interact_list->data == client)
    interact_next = 1;
  REMOVE (interact_list, client);

  if (interact_list && interact_next)
    {
      client = (Client *) interact_list->data;
      SmsInteract (client->connection);
    }

  SmFreeReasons (count, reasons);
}



SmProp *
find_property_by_name (Client *client, char *name)
{
  GSList *list;

  for (list = client->properties; list; list = list->next)
    {
      SmProp *prop = (SmProp *) list->data;
      if (! strcmp (prop->name, name))
	return prop;
    }

  return NULL;
}

/* It doesn't matter that this is inefficient.  */
static void
set_properties (SmsConn connection, SmPointer data, int nprops, SmProp **props)
{
  Client *client = (Client *) data;
  int i;

  for (i = 0; i < nprops; ++i)
    {
      SmProp *prop;

      prop = find_property_by_name (client, props[i]->name);
      if (prop)
	{
	  REMOVE (client->properties, prop);
	  SmFreeProperty (prop);
	}

      APPEND (client->properties, props[i]);
    }

  free (props);
}

static void
delete_properties (SmsConn connection, SmPointer data, int nprops,
		   char **prop_names)
{
  Client *client = (Client *) data;
  int i;

  for (i = 0; i < nprops; ++i)
    {
      SmProp *prop;

      prop = find_property_by_name (client, prop_names[i]);
      if (prop)
	{
	  REMOVE (client->properties, prop);
	  SmFreeProperty (prop);
	}
    }

  /* FIXME: free prop_names?? */
}

static void
get_properties (SmsConn connection, SmPointer data)
{
  Client *client = (Client *) data;
  SmProp **props;
  GSList *list;
  int i, len;

  len = g_slist_length (client->properties);
  props = g_new (SmProp *, len);

  i = 0;
  for (list = client->properties; list; list = list->next)
    props[i] = list->data;

  SmsReturnProperties (connection, len, props);
  free (props);
}



/* This is run when a new client connects.  We register all our
   callbacks.  */
Status
new_client (SmsConn connection, SmPointer data, unsigned long *maskp,
	    SmsCallbacks *callbacks, char **reasons)
{
  Client *client;

  client = g_new (Client, 1);
  client->id = NULL;
  client->connection = connection;
  client->properties = NULL;

  *maskp = 0;

  *maskp |= SmsRegisterClientProcMask;
  callbacks->register_client.callback = register_client;
  callbacks->register_client.manager_data = (SmPointer) client;

  *maskp |= SmsInteractRequestProcMask;
  callbacks->interact_request.callback = interact_request;
  callbacks->interact_request.manager_data = (SmPointer) client;

  *maskp |= SmsInteractDoneProcMask;
  callbacks->interact_done.callback = interact_done;
  callbacks->interact_done.manager_data = (SmPointer) client;

  *maskp |= SmsSaveYourselfRequestProcMask;
  callbacks->save_yourself_request.callback = save_yourself_request;
  callbacks->save_yourself_request.manager_data = (SmPointer) client;

  *maskp |= SmsSaveYourselfP2RequestProcMask;
  callbacks->save_yourself_phase2_request.callback = save_yourself_p2_request;
  callbacks->save_yourself_phase2_request.manager_data = (SmPointer) client;

  *maskp |= SmsSaveYourselfDoneProcMask;
  callbacks->save_yourself_done.callback = save_yourself_done;
  callbacks->save_yourself_done.manager_data = (SmPointer) client;

  *maskp |= SmsCloseConnectionProcMask;
  callbacks->close_connection.callback = close_connection;
  callbacks->close_connection.manager_data = (SmPointer) client;

  *maskp |= SmsSetPropertiesProcMask;
  callbacks->set_properties.callback = set_properties;
  callbacks->set_properties.manager_data = (SmPointer) client;

  *maskp |= SmsDeletePropertiesProcMask;
  callbacks->delete_properties.callback = delete_properties;
  callbacks->delete_properties.manager_data = (SmPointer) client;

  *maskp |= SmsGetPropertiesProcMask;
  callbacks->get_properties.callback = get_properties;
  callbacks->get_properties.manager_data = (SmPointer) client;

  return 1;
}



/* Some data for save_helper.  */
static int x_save_type;
static int x_interact_style;
static int x_fast;

/* Helper for save_session.  Sends save yourself message to all
   clients.  */
static void
save_helper (gpointer data, gpointer dummy)
{
  Client *client = (Client *) data;
  SmsSaveYourself (client->connection, x_save_type, shutting_down,
		   x_interact_style, x_fast);
}

/* This function is exported to the rest of gsm.  It sets up and
   performs a session save, possibly followed by a shutdown.  */
void
save_session (int save_type, Bool shutdown, int interact_style, Bool fast)
{
  if (saving)
    return;

  saving = 1;
  shutting_down = shutdown;

  /* Sigh.  */
  x_save_type = save_type;
  x_interact_style = interact_style;
  x_fast = fast;

  g_slist_foreach (live_list, save_helper, NULL);

  g_assert (! save_yourself_list);
  save_yourself_list = live_list;
  live_list = NULL;
}



int
shutdown_in_progress_p (void)
{
  return saving && shutting_down;
}
