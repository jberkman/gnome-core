/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
#include "capplet-widget-libs.h"
#include "control-center.h"
#include <libgnorba/gnorba.h>
#include <orb/orbit.h>

static CORBA_ORB orb;
static CORBA_Environment ev;
static GNOME_capplet capplet = NULL;
static GNOME_control_center control_center;
static gchar* cc_ior = NULL;
static gint id = -1;
static guint32 xid = 0;
static gint capid = -1;
static GList *id_list = NULL;
/* structs */
typedef struct _keyval keyval;
struct _keyval
{
        gint capid;
        guint32 xid;
        gint id;
};

/* prototypes... */
static void orb_add_connection(GIOPConnection *cnx);
static void orb_remove_connection(GIOPConnection *cnx);
static void orb_handle_connection(GIOPConnection *cnx, gint source, GdkInputCondition cond);
static void server_try (PortableServer_Servant servant, CORBA_long id, CORBA_Environment * ev);
static void server_revert (PortableServer_Servant servant, CORBA_long id, CORBA_Environment * ev);
static void server_ok (PortableServer_Servant servant, CORBA_long id, CORBA_Environment * ev);
static void server_cancel (PortableServer_Servant servant, CORBA_long id, CORBA_Environment * ev);
static void server_help (PortableServer_Servant servant, CORBA_long id, CORBA_Environment * ev);
static void server_new_multi_capplet(GNOME_capplet _obj, CORBA_long id, CORBA_long newid, CORBA_unsigned_long newxid, CORBA_long newcapid, CORBA_Environment * ev);
extern void _capplet_widget_server_try(gint id);
extern void _capplet_widget_server_revert(gint id);
extern void _capplet_widget_server_ok(gint id);
extern void _capplet_widget_server_cancel(gint id);
extern void _capplet_widget_server_help(gint id);
extern void _capplet_widget_server_new_multi_capplet(gint id, gint capid);

/* parser stuff...*/
static error_t
parse_an_arg (int key, char *arg, struct argp_state *state)
{
        switch (key) {
        case 'd':
                id = atoi (arg);
                break;
        case 'c':
                capid = atoi (arg);
                break;
        case 'i':
                cc_ior = strdup (arg);
                break;
        case 'x':
                xid = strtol(arg, NULL, 10);
                break;
        default:
                return ARGP_ERR_UNKNOWN;
        }
        return 0;
}
static struct argp_option options[] = {
        { "id",      'd', N_("ID"),       0, N_("id of the capplet -- assigned by the control-center"), 1},
        { "cap-id",  'c', N_("CAPID"),    0, N_("multi-capplet id."), 1},
        { "ior",     'i', N_("IOR"),      0, N_("ior of the control-center"), 1},
        { "xid",     'x', N_("XID"),      0, N_("X id of the socket it's plugged into"), 1},
        { NULL,       0,  NULL,           0, NULL, 0 }
};
static struct argp parser = {
        options, parse_an_arg, NULL,  NULL,  NULL, NULL, NULL
};

/* ORB stuff... */
PortableServer_ServantBase__epv base_epv = {
        NULL,
        NULL,
        NULL
};
POA_GNOME_capplet__epv capplet_epv = 
{  
        NULL,
        (gpointer)&server_try, 
        (gpointer)&server_revert,
        (gpointer)&server_ok,
        (gpointer)&server_cancel,
        (gpointer)&server_help,
        (gpointer)&server_new_multi_capplet
};
POA_GNOME_capplet__vepv poa_capplet_vepv = { &base_epv, &capplet_epv };
POA_GNOME_capplet poa_capplet_servant = { NULL, &poa_capplet_vepv };

static void
orb_add_connection(GIOPConnection *cnx)
{
         cnx->user_data = (gpointer)gtk_input_add_full(GIOP_CONNECTION_GET_FD(cnx),
                                                       GDK_INPUT_READ|GDK_INPUT_EXCEPTION,
                                                       (GdkInputFunction)orb_handle_connection,
                                                       NULL, cnx, NULL);
}

static void
orb_remove_connection(GIOPConnection *cnx)
{
        gtk_input_remove((guint)cnx->user_data);
        cnx->user_data = (gpointer)-1;
}
static void
orb_handle_connection(GIOPConnection *cnx, gint source, GdkInputCondition cond)
{
        switch(cond) {
        case GDK_INPUT_EXCEPTION:
                giop_main_handle_connection_exception(cnx);
                break;
        default:
                giop_main_handle_connection(cnx);
        }
}

static void
server_try (PortableServer_Servant servant, CORBA_long id, CORBA_Environment * ev)
{
        _capplet_widget_server_try (id);
}
static void
server_revert (PortableServer_Servant servant, CORBA_long id, CORBA_Environment * ev)
{
        _capplet_widget_server_revert (id);
}
static void
server_ok (PortableServer_Servant servant, CORBA_long id, CORBA_Environment * ev)
{
        _capplet_widget_server_ok (id);
}
static void
server_cancel (PortableServer_Servant servant, CORBA_long id, CORBA_Environment * ev)
{
        _capplet_widget_server_cancel (id);
}
static void
server_help (PortableServer_Servant servant, CORBA_long id, CORBA_Environment * ev)
{
        _capplet_widget_server_help (id);
}
static void
server_new_multi_capplet(GNOME_capplet _obj, CORBA_long id, CORBA_long newid, CORBA_unsigned_long newxid, CORBA_long newcapid, CORBA_Environment * ev)
{
        GList *temp;
        keyval *nkv;
        g_print ("in new multi_capplet\n");
        for (temp = id_list; temp; temp = temp->next)
                if (((keyval *)temp->data)->capid == capid) {
                        ((keyval *)temp->data)->xid = xid;
                        ((keyval *)temp->data)->id = newid;
                        _capplet_widget_server_new_multi_capplet (id, capid);
                        return;
                };
        nkv = g_malloc (sizeof (nkv));
        nkv->capid = capid;
        nkv->id = newid;
        nkv->xid = newcapid;
        id_list = g_list_prepend (id_list, nkv);
        _capplet_widget_server_new_multi_capplet (id, capid);
}

/* public methods... */
void
capplet_corba_gtk_main (void)
{
        if (!orb) {
                g_warning ("Corba must be initialized before gtk_main can be called\n");
                exit (1);
        }
        gtk_main();
}
void
capplet_corba_gtk_main_quit (void)
{
        CORBA_Object_release (control_center, &ev);
        CORBA_ORB_shutdown(orb, CORBA_FALSE, &ev);
        gtk_main_quit();
}
void
capplet_corba_state_changed (gint id, gboolean undoable)
{
        GNOME_control_center_state_changed(control_center, id, undoable, &ev);
}
guint32
get_xid (gint cid)
{
        GList *temp;
        if ((cid == -1) || (cid == capid))
                return xid;

        for (temp = id_list; temp; temp = temp->next)
                if (((keyval *)temp->data)->capid == cid)
                        return ((keyval *)temp->data)->xid;
        g_warning ("received an unknown cid: %d\n", cid);
        return 0;
}
gint get_ccid (gint cid)
{
        GList *temp;
        if ((cid == -1) || (cid == capid))
                return id;

        for (temp = id_list; temp; temp = temp->next)
                if (((keyval *)temp->data)->capid == cid)
                        return ((keyval *)temp->data)->id;
        g_warning ("received an unknown cid: %d\n", cid);
        return id;
}
gint
capplet_widget_corba_init(char *app_id,
                               struct argp *app_parser,
                               int *argc, char **argv,
                               unsigned int flags,
                               int *arg_index)
{
        PortableServer_ObjectId objid = {0, sizeof("capplet_interface"), "capplet_interface"};
        PortableServer_POA poa;

        IIOPAddConnectionHandler = orb_add_connection;
        IIOPRemoveConnectionHandler = orb_remove_connection;
        CORBA_exception_init(&ev);
        gnome_parse_register_arguments (&parser);
        
        orb = gnome_CORBA_init (app_id, app_parser, argc, argv, flags, arg_index, &ev);
        /* sanity check */
        if ((xid == 0) || (cc_ior == NULL) || (id == -1)) {
                g_warning ("Insufficient arguments passed to the arg parser.\n");
                exit (1);
        }
        if (!orb) {
                g_warning ("unable to connect to the server.\nexitting...\n");
                exit (1);
        }

        /* setup CORBA fully */
        POA_GNOME_capplet__init(&poa_capplet_servant, &ev);
        poa = orb->root_poa;
        PortableServer_POAManager_activate(PortableServer_POA__get_the_POAManager(poa, &ev), &ev);

        PortableServer_POA_activate_object_with_id(poa, 
                                                   &objid, &poa_capplet_servant, &ev);
        capplet = PortableServer_POA_servant_to_reference((PortableServer_POA)orb->root_poa, 
                                                      &poa_capplet_servant, &ev);
        if (!capplet) {
                g_warning ("We cannot connect to a control_center\nexitting...\n");
                exit (1);
        }
        ORBit_custom_run_setup(orb, &ev);

        /* now we get the control center. */
        control_center = CORBA_ORB_string_to_object(orb, cc_ior, &ev);
        if (! control_center) {
                g_warning ("Unable reach the control-center.\nExiting...");
                exit (1);
        }
        GNOME_control_center_register_capplet(control_center, id, capplet, &ev);

        /* this will be -1 if we are not a multi-capplet.  
         * Otherwise, it'll be the multi-capplets id.
         */
        return capid;
}
