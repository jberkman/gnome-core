/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
#include <gnome.h>
#include <orb/orbit.h>
#include "caplet-widget.h"
#include "control-center.h"

/* static variables */

static GtkPlugClass *parent_class;
CORBA_ORB orb;
CORBA_Environment ev;
GNOME_caplet caplet;
gchar* ior;

/* prototypes */ 
static void caplet_widget_class_init	(CapletWidgetClass *klass);
static void caplet_widget_init		(CapletWidget      *applet_widget);

/* administrative calls */
guint
caplet_widget_get_type (void)
{
        static guint caplet_widget_type = 0;
        
        if (!caplet_widget_type) {
                GtkTypeInfo caplet_widget_info = {
                        "CapletWidget",
                        sizeof (CapletWidget),
                        sizeof (CapletWidgetClass),
                        (GtkClassInitFunc) caplet_widget_class_init,
                        (GtkObjectInitFunc) caplet_widget_init,
                        (GtkArgSetFunc) NULL,
                        (GtkArgGetFunc) NULL,
                };
                
                caplet_widget_type = gtk_type_unique (gtk_plug_get_type (),
                                                              &caplet_widget_info);
        }
        
        return caplet_widget_type;
}
static void
caplet_widget_class_init (CapletWidgetClass *klass)
{
        GtkObjectClass *object_class;
	object_class = (GtkObjectClass*) klass;
	parent_class = gtk_type_class (gtk_plug_get_type ());
}
static void
caplet_widget_init (CapletWidget *applet_widget)
{
        
}
GtkWidget *
caplet_widget_new ()
{
        CapletWidget * retval;
        /*        retval CAPLET_WIDGET (gtk_type_new 
                  (caplet_widget_get_type()));*/
        return GTK_WIDGET (retval);
}

/* non widget calls */
void
caplet_gtk_main (void)
{
        gtk_main();
}
static error_t
parse_an_arg (int key, char *arg, struct argp_state *state)
{
        if (key != ARGP_KEY_ARG)
                return ARGP_ERR_UNKNOWN;

        ior = strdup (arg);
        return 0;
}
static struct argp parser = {
        NULL, parse_an_arg, N_("[IOR]"),  NULL,  NULL, NULL, NULL
};
error_t
caplet_init (char *app_id, struct argp *app_parser,
                     int argc, char **argv, unsigned int flags,
                     int *arg_index)
{
        error_t retval;
        gushort id;
        guint32 xid;
        GtkWidget *plug;

        retval = gnome_init(app_id,&parser,argc,argv,flags,arg_index);
  
        CORBA_exception_init(&ev);
        orb = CORBA_ORB_init(&argc, argv, "orbit-local-orb", &ev);

        caplet = CORBA_ORB_string_to_object(orb, ior, &ev);
        if (! caplet) {
                g_warning ("Unable reach gnomecc\n");
                return 1;
        }
        GNOME_control_center_request_id(caplet,NULL, &xid, &id, &ev);
        plug = gtk_plug_new (xid);
        gtk_widget_show (plug);
        return retval;
}
