/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
#include "capplet-widget-libs.h"
#include "control-center.h"
#include <orb/orbit.h>

CORBA_ORB orb;
CORBA_Environment ev;
GNOME_capplet capplet = NULL;
GNOME_control_center control_center;

/* prototypes... (: */
static void orb_add_connection(GIOPConnection *cnx);
static void orb_remove_connection(GIOPConnection *cnx);
static void orb_handle_connection(GIOPConnection *cnx, gint source, GdkInputCondition cond);
static void server_try (PortableServer_Servant servant, CORBA_Environment * ev);
static void server_revert (PortableServer_Servant servant, CORBA_Environment * ev);
static void server_ok (PortableServer_Servant servant, CORBA_Environment * ev);
static void server_help (PortableServer_Servant servant, CORBA_Environment * ev);
void _capplet_widget_server_try();
void _capplet_widget_server_revert();
void _capplet_widget_server_ok();
void _capplet_widget_server_help();


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
        (gpointer)&server_help,
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
server_try (PortableServer_Servant servant, CORBA_Environment * ev)
{
        _capplet_widget_server_try();
}
static void
server_revert (PortableServer_Servant servant, CORBA_Environment * ev)
{
        _capplet_widget_server_revert();
}
static void
server_ok (PortableServer_Servant servant, CORBA_Environment * ev)
{
        _capplet_widget_server_ok();
}
static void
server_help (PortableServer_Servant servant, CORBA_Environment * ev)
{
        _capplet_widget_server_help();
}

void
capplet_widget_corba_init(gint *argc, char **argv, gchar *cc_ior, gint id)
{
        PortableServer_ObjectId objid = {0, sizeof("capplet_interface"), "capplet_interface"};
        PortableServer_POA poa;

        IIOPAddConnectionHandler = orb_add_connection;
        IIOPRemoveConnectionHandler = orb_remove_connection;
        CORBA_exception_init(&ev);
        orb = CORBA_ORB_init(argc, argv, "orbit-local-orb", &ev);

        POA_GNOME_capplet__init(&poa_capplet_servant, &ev);
        poa = orb->root_poa;
        PortableServer_POAManager_activate(PortableServer_POA__get_the_POAManager(poa, &ev), &ev);

        PortableServer_POA_activate_object_with_id(poa, 
                                                   &objid, &poa_capplet_servant, &ev);
        capplet = PortableServer_POA_servant_to_reference((PortableServer_POA)orb->root_poa, 
                                                      &poa_capplet_servant, &ev);

        if (!capplet) {
                g_warning ("We cannot get objref\n");
                exit (1);
        }
       
        CORBA_Object_release(capplet, &ev);
        ORBit_custom_run_setup(orb, &ev);

        /* now we get the control center. */
        control_center = CORBA_ORB_string_to_object(orb, cc_ior, &ev);
        
        if (! control_center) {
                g_warning ("Unable reach the control-center.\nExiting...");
                exit (1);
        }
        GNOME_control_center_register_capplet(control_center, id,control_center,&ev);
}
void
capplet_corba_gtk_main (void)
{
        if (!orb) {
                g_warning ("Corba must be initialized before gtk_main can be called\n");
                exit (1);
        }
        
        gtk_main();
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
void
capplet_corba_gtk_main_quit (void)
{
        CORBA_ORB_shutdown(orb, CORBA_FALSE, &ev);
        gtk_main_quit();

}
