/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
#include "corba-glue.h"
#include "caplet-manager.h"
#include <orb/orbit.h>
#include <gnome.h>

/* prototypes */
static void orb_add_connection(GIOPConnection *cnx);
static void orb_remove_connection(GIOPConnection *cnx);
void control_center_corba_gtk_init(gint *argc, char **argv);
static void orb_handle_connection(GIOPConnection *cnx, gint source, GdkInputCondition cond);


/* Variables */
CORBA_ORB orb = NULL;
CORBA_Environment ev;
GNOME_control_center control_center;
gchar *ior;
PortableServer_ServantBase__epv base_epv = {
        NULL,
        NULL,
        NULL
};
POA_GNOME_control_center__epv control_center_epv = 
{  
        NULL, 
        NULL,
        NULL,
};
POA_GNOME_control_center__vepv poa_control_center_vepv = { &base_epv, &control_center_epv };
POA_GNOME_control_center poa_control_center_servant = { NULL, &poa_control_center_vepv };

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
void
control_center_corba_gtk_main_quit(void)
{
       CORBA_ORB_shutdown(orb, CORBA_FALSE, &ev);
       gtk_main_quit();
}

void
control_center_corba_gtk_init(gint *argc, char **argv)
{
        PortableServer_ObjectId objid = {0, sizeof("control_center_interface"), "control_center_interface"};
        PortableServer_POA poa;

        IIOPAddConnectionHandler = orb_add_connection;
        IIOPRemoveConnectionHandler = orb_remove_connection;
        CORBA_exception_init(&ev);
        orb = CORBA_ORB_init(argc, argv, "orbit-local-orb", &ev);

        POA_GNOME_control_center__init(&poa_control_center_servant, &ev);
        poa = orb->root_poa;
        PortableServer_POAManager_activate(PortableServer_POA__get_the_POAManager(poa, &ev), &ev);

        PortableServer_POA_activate_object_with_id(poa, 
                                                   &objid, &poa_control_center_servant, &ev);
        control_center = PortableServer_POA_servant_to_reference((PortableServer_POA)orb->root_poa, 
                                                      &poa_control_center_servant, &ev);

        if (!control_center) {
                g_error ("We cannot get objref\n");
                exit (1);
        }
        ior = CORBA_ORB_object_to_string(orb, control_center, &ev);
       
        CORBA_Object_release(control_center, &ev);
        ORBit_custom_run_setup(orb, &ev);
}

void
control_center_corba_gtk_main (gint *argc, char **argv)
{
        if(!orb)
                control_center_corba_gtk_init( argc, argv);
        gtk_main();
}

static void orb_handle_connection(GIOPConnection *cnx, gint source, GdkInputCondition cond)
{
        switch(cond) {
        case GDK_INPUT_EXCEPTION:
                giop_main_handle_connection_exception(cnx);
                break;
        default:
                giop_main_handle_connection(cnx);
        }
}
