/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
#include <gnome.h>
#include <orb/orbit.h>
#include "control-center-widget.h"
#include "control-center.h"

/* static variables */

static GtkPlugClass *parent_class;
CORBA_ORB orb;
CORBA_Environment ev;
GNOME_control_panel control_center;
gchar* ior;

/* prototypes */ 
static void control_center_widget_class_init	(ControlCenterWidgetClass *klass);
static void control_center_widget_init		(ControlCenterWidget      *applet_widget);

/* administrative calls */
guint
control_center_widget_get_type (void)
{
        static guint control_center_widget_type = 0;
        
        if (!control_center_widget_type) {
                GtkTypeInfo control_center_widget_info = {
                        "ControlCenterWidget",
                        sizeof (ControlCenterWidget),
                        sizeof (ControlCenterWidgetClass),
                        (GtkClassInitFunc) control_center_widget_class_init,
                        (GtkObjectInitFunc) control_center_widget_init,
                        (GtkArgSetFunc) NULL,
                        (GtkArgGetFunc) NULL,
                };
                
                control_center_widget_type = gtk_type_unique (gtk_plug_get_type (),
                                                              &control_center_widget_info);
        }
        
        return control_center_widget_type;
}
static void
control_center_widget_class_init (ControlCenterWidgetClass *klass)
{
        GtkObjectClass *object_class;
	object_class = (GtkObjectClass*) klass;
	parent_class = gtk_type_class (gtk_plug_get_type ());
}
static void
control_center_widget_init (ControlCenterWidget *applet_widget)
{
}
GtkWidget *
control_center_widget_new ()
{
        ControlCenterWidget * retval;
        /*        retval CONTROL_CENTER_WIDGET (gtk_type_new 
                  (control_center_widget_get_type()));*/
        return GTK_WIDGET (retval);
}

/* non widget calls */
void
control_center_gtk_main (void)
{
        gtk_main();
}
static error_t
parse_an_arg (int key, char *arg, struct argp_state *state)
{
        if (key != ARGP_KEY_ARG)
                return ARGP_ERR_UNKNOWN;

        ior = strdup (arg);
        g_print ("we think the IOR is %s\n\n", ior);
        return 0;
}
static struct argp parser = {
        NULL, parse_an_arg, N_("[IOR]"),  NULL,  NULL, NULL, NULL
};
error_t
control_center_init (char *app_id, struct argp *app_parser,
                     int argc, char **argv, unsigned int flags,
                     int *arg_index)
{
        error_t retval;

        retval = gnome_init(app_id,&parser,argc,argv,flags,arg_index);
  
        CORBA_exception_init(&ev);
        orb = CORBA_ORB_init(&argc, argv, "orbit-local-orb", &ev);

        control_center = CORBA_ORB_string_to_object(orb, ior, &ev);
        if (! control_center) {
                g_error ("Unable reach gnomecc\n");
                return 1;
        }
        g_print ("and for the money card...\n");
        GNOME_control_panel_cpo_request_id(control_center, "Dr. Watson, come here, i need you", &ev);
        g_print ("did we succeed?");

        return retval;
}
