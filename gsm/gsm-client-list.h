/* gsm-client-list.h - a scrolled window listing the gnome-session clients.

   Copyright 1999 Free Software Foundation, Inc.

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
   02111-1307, USA. 

   Authors: Felix Bellaby */

#ifndef GSM_CLIENT_LIST_H
#define GSM_CLIENT_LIST_H

#include <gtk/gtk.h>

#define GSM_IS_CLIENT_LIST(obj)      GTK_CHECK_TYPE (obj, gsm_client_list_get_type ())
#define GSM_CLIENT_LIST(obj)         GTK_CHECK_CAST (obj, gsm_client_list_get_type (), GsmClientList)
#define GSM_CLIENT_LIST_CLASS(klass) GTK_CHECK_CLASS_CAST (klass, gsm_client_list_get_type (), GsmClientListClass)

typedef struct _GsmClientList GsmClientList;
typedef struct _GsmClientListClass GsmClientListClass;

struct _GsmClientList {
  GtkCList   clist;

  GtkTooltips* tooltips;
  GtkObject* selector;
  GtkWidget* order_col;
  GtkWidget* style_col;
  GtkWidget* state_col;
  gboolean   committed;
  gboolean   dirty;
  GSList*    changes;
};

struct _GsmClientListClass {
  GtkCListClass parent_class;

  void (* dirty)       (GsmClientList *client_list); /* user did something */
  void (* initialized) (GsmClientList *client_list); /* list ready to show */
};

guint gsm_client_list_get_type  (void);

/* Creates a client list.
 * NB: The current implementation only allows one list per gnome executable.
 * If anyone ever wants more than one than it could be changed... */
GtkWidget* gsm_client_list_new (void);

/* Removes the currently selected clients from the list. */
void gsm_client_list_remove_selection (GsmClientList* client_list);

/* Adds a client to the list. 
 * Returns FALSE when the command line contains unbalanced quotes. */
gboolean gsm_client_list_add_program (GsmClientList* client_list,
				      gchar* command);

/* Commits any changes made in the list over to gnome-session. */
void gsm_client_list_commit_changes (GsmClientList* client_list);

/* Reverts any changes made in the list. */
void gsm_client_list_revert_changes (GsmClientList* client_list);

/* Discards information about changes. */
void gsm_client_list_discard_changes (GsmClientList* client_list);

#endif /* GSM_CLIENT_LIST_H */
