/* manager.h - Definitions for session manager.

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

#ifndef MANAGER_H
#define MANAGER_H

#include <X11/SM/SMlib.h>
#include <time.h>

#include "glib.h"

/* Rule used to match client: gets around need to specify proper client ids
 * when starting from sysadmin files or via the GsmAddClient protocol.
 * This is a big burden off end users and only creates the possibility of
 * confusion between PURGED SM aware clients started by GsmAddClient. */
typedef enum {
  MATCH_NONE,
  MATCH_ID,
  MATCH_FAKE_ID,
  MATCH_DONE,
  MATCH_PROP
} MatchRule;

/* Additional details for clients that speak our command protocol */
typedef struct _CommandData CommandData;

/* Each client is represented by one of these.  Note that the client's
   state is not kept explicitly.  A client is on only one of several
   linked lists at a given time; the state is implicit in the list.  */
typedef struct
{
  /* Client's session id.  */
  char *id;

  /* Handle for the GsmCommand protocol */
  gchar* handle;

  /* Client's connection.  */
  SmsConn connection;

  /* List of all properties for this client.  Each element of the list
     is an `SmProp *'.  */
  GSList *properties;

  /* Used to detect clients which are dying quickly */
  guint  attempts;
  time_t connect_time;

  /* Used to determine order in which clients are started */
  guint priority;

  /* Used to avoid registering clients with ids from default.session */
  MatchRule match_rule;

  /* Used to decouple SmsGetPropertiesProc and SmsReturnProperties
   * for the purpose of extending the protocol: */
  GSList* get_prop_replies;
  guint get_prop_requests;

  /* Additional details for clients that speak our command protocol */
  CommandData *command_data;
} Client;

typedef struct {
  /* Handle for the GsmCommand protocol */
  gchar*  handle;

  /* session name for user presentation */
  gchar*  name;

  /* The Client* members of the session */
  GSList *client_list;
} Session;


/* Milliseconds to wait for clients to register before assuming that
 * they have finished any initialisation needed by other clients. */
extern guint purge_delay;

/* Milliseconds to wait for clients to die before cutting our throat. */
extern guint suicide_delay;

/*
 * manager.c
 */

/* Start an individual client. */
void start_client (Client* client);

/* Start a list of clients in priority order. */
void start_clients (GSList* client_list);

/* Remove a client from the session (completely).
 * Returns 1 on success, 0 when the client was not found in the current 
 * session and -1 when a save is in progress. */
gint remove_client (Client* client);

/* Free the memory used by a client. */
void free_client (Client* client);

/* Run the Discard, Resign or Shutdown command on a client.
 * Returns the pid or -1 if unsuccessful. */
gint run_command (const Client* client, const gchar* command);

/* Call this to initiate a session save, and perhaps a shutdown.
   Save requests are queued internally. */
void save_session (int save_type, gboolean shutdown, int interact_style,
		   gboolean fast);

/* Returns true if shutdown in progress, false otherwise.  Note it is
   possible for this function to return true and then later return
   false -- the shutdown might be cancelled.  */
int shutdown_in_progress_p (void);

/* This is called via ICE when a new client first connects.  */
Status new_client (SmsConn connection, SmPointer data, unsigned long *maskp,
		   SmsCallbacks *callbacks, char **reasons);

/* Find a client from a list */
Client* find_client_by_id (const GSList *list, const char *id);

/* This is used to send properties to clients that use the _GSM_Command
 * protocol extension. */
void send_properties (Client* client, GSList* prop_list);

/*
 * save.c
 */

/* Attempts to set the session name (the requested name may be locked).
 * Returns the name that has been assigned to the session. */
gchar* set_session_name (const gchar *name);

/* Returns name of last session run (with a default) */
gchar* get_last_session (void);

/* Releases the lock on the session name when shutting down the session */
void unlock_session (void);

/* Write current session to the config file. */
void write_session (void);

/* Load a session from our configuration by name. */
Session* read_session (const char *name);

/* Starts the clients in a session and frees the session. */
void start_session (Session* session);

/* Frees the memory used by a session. */
void free_session (Session* session);

/* Delete a session from the config file and discard any stale
 * session info saved by clients that were in the session. */
void delete_session (const char *name);

/*
 * ice.c
 */

/* Initialize the ICE part of the session manager. */
void initialize_ice (void);

/* Set a clean up callback for when the ice_conn is closed. */
void ice_set_clean_up_handler (IceConn ice_conn, 
			       GDestroyNotify, gpointer data);

/* Clean up ICE when exiting.  */
void clean_ice (void);

/*
 * prop.c
 */

/* Call this to find the named property for a client.  Returns NULL if
   not found.  */
SmProp *find_property_by_name (const Client *client, const char *name);

/* Find property NAME attached to CLIENT.  If not found, or type is
   not CARD8, then return FALSE.  Otherwise set *RESULT to the value
   and return TRUE.  */
gboolean find_card8_property (const Client *client, const char *name,
			      int *result);

/* Find property NAME attached to CLIENT.  If not found, or type is
   not ARRAY8, then return FALSE.  Otherwise set *RESULT to the value
   and return TRUE.  *RESULT is malloc()d and must be freed by the
   caller.  */
gboolean find_string_property (const Client *client, const char *name,
			       char **result);

/* Find property NAME attached to CLIENT.  If not found, or type is
   not LISTofARRAY8, then return FALSE.  Otherwise set *ARGCP to the
   number of vector elements, *ARGVP to the elements themselves, and
   return TRUE.  Each element of *ARGVP is malloc()d, as is *ARGVP
   itself.  You can use `free_vector' to free the result.  */
gboolean find_vector_property (const Client *client, const char *name,
			       int *argcp, char ***argvp);

/* Free the return result from find_vector_property.  */
void free_vector (int argc, char **argv);

/*
 * command.c
 */

/* Log change in the state of a client with the client event selectors. */
void client_event (const gchar* handle, const gchar* event);

/* Log change in the properties of a client with the client event selectors. */
void client_property (const gchar* handle, int nprops, SmProp** props);

/* Log reasons for an event with the client event selectors. */
void client_reasons (const gchar* handle, gint count, gchar** reasons);

/* Process a _GSM_Command protocol message. */
void command (Client* client, int nprops, SmProp** props);

/* TRUE when the _GSM_Command protocol is enabled for this client. */
gboolean command_active (Client* client);

/* Clean up the _GSM_Command protocol for a client. */
void command_clean_up (Client* client);

/* Create an object handle for use in the _GSM_Command protocol. */
gchar* command_handle_new (gpointer object);

/* Move an object handle from one object to another. */ 
gchar* command_handle_reassign (gchar* handle, gpointer new_object);

/* Free an object handle */
void command_handle_free (gchar* handle);

/* Convenience macros */
#define APPEND(List,Elt) ((List) = (g_slist_append ((List), (Elt))))
#define REMOVE(List,Elt) ((List) = (g_slist_remove ((List), (Elt))))
#define CONCAT(L1,L2) ((L1) = (g_slist_concat ((L1), (L2))))

#endif /* MANAGER_H */
