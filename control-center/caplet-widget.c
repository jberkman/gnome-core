/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
#include <gnome.h>
#include <orb/orbit.h>
#include "caplet-widget.h"
#include "caplet-widget-libs.h"
#include "control-center.h"

/* static variables */

static GtkPlugClass *parent_class;
CORBA_ORB orb;
CORBA_Environment ev;
GNOME_caplet caplet;
gchar* ior = NULL;
guint32 xid = 0;
gint id = -1;

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
        retval = CAPLET_WIDGET (gtk_type_new (caplet_widget_get_type()));
        gtk_plug_construct (GTK_PLUG (retval), xid);
        return GTK_WIDGET (retval);
}

/* non widget calls */
void
caplet_gtk_main (void)
{
        control_center_corba_gtk_main("Something should go here???");
}
static error_t
parse_an_arg (int key, char *arg, struct argp_state *state)
{
        switch (key) {
        case 'd':
                id = atoi (arg);
                break;
        case 'i':
                ior = strdup (arg);
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
        { "id",    'd', N_("ID"),       0, N_("id of the caplet -- assigned by the control-center"),      1 },
        { "ior",    'i', N_("IOR"),      0, N_("ior of the control-center"), 1 },
        { "xid",    'x', N_("XID"),      0, N_("X id of the socket it's plugged into"), 1 },
        { NULL,     0,  NULL,         0, NULL,                       0 }
};
static struct argp parser = {
        options, parse_an_arg, NULL,  NULL,  NULL, NULL, NULL
};

error_t
gnome_caplet_init (char *app_id, struct argp *app_parser,
                     int argc, char **argv, unsigned int flags,
                     int *arg_index)
{
        /* FIXME: er, we need to actually parse app_parser... (: */

        error_t retval;
        retval = gnome_init(app_id,&parser,argc,argv,flags,arg_index);

        if ((xid == 0) || (id == -1) || (ior == NULL)) {
                g_warning ("Insufficient arguments passed to the arg parser.\n");
                exit (1);
        }
        CORBA_exception_init(&ev);
        orb = CORBA_ORB_init(&argc, argv, "orbit-local-orb", &ev);
        caplet = CORBA_ORB_string_to_object(orb, ior, &ev);
        if (! caplet) {
                g_warning ("Unable reach the control-center.\nExiting...");
                exit (1);
        }
        return retval;
}
