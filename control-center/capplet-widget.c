/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
#include <gnome.h>
#include <orb/orbit.h>
#include "capplet-widget.h"
#include "capplet-widget-libs.h"
#include "control-center.h"

/* variables */
static GtkPlugClass *parent_class;
static GList *capplet_list = NULL;
gchar* cc_ior = NULL;
static gint id = -1;
guint32 xid = 0;
extern GNOME_control_center control_center;
extern CORBA_Environment ev;

enum {
	TRY_SIGNAL,
	REVERT_SIGNAL,
	OK_SIGNAL,
        HELP_SIGNAL,
        LAST_SIGNAL
};
static int capplet_widget_signals[LAST_SIGNAL] = {0,0,0,0};

/* prototypes */ 
static void capplet_widget_class_init	(CappletWidgetClass *klass);
static void capplet_widget_init		(CappletWidget      *applet_widget);
GtkWidget *get_widget_by_id(gint id);


/* administrative calls */
guint
capplet_widget_get_type (void)
{
        static guint capplet_widget_type = 0;
        
        if (!capplet_widget_type) {
                GtkTypeInfo capplet_widget_info = {
                        "CappletWidget",
                        sizeof (CappletWidget),
                        sizeof (CappletWidgetClass),
                        (GtkClassInitFunc) capplet_widget_class_init,
                        (GtkObjectInitFunc) capplet_widget_init,
                        (GtkArgSetFunc) NULL,
                        (GtkArgGetFunc) NULL,
                };
                
                capplet_widget_type = gtk_type_unique (gtk_plug_get_type (),
                                                              &capplet_widget_info);
        }
        
        return capplet_widget_type;
}
static void
capplet_widget_class_init (CappletWidgetClass *klass)
{
        GtkObjectClass *object_class;
	object_class = (GtkObjectClass*) klass;
	parent_class = gtk_type_class (gtk_plug_get_type ());

	capplet_widget_signals[TRY_SIGNAL] =
		gtk_signal_new("try",
			       GTK_RUN_LAST,
			       object_class->type,
			       GTK_SIGNAL_OFFSET(CappletWidgetClass,
			       			 try),
                               gtk_signal_default_marshaller,
                               GTK_TYPE_NONE, 0);
	capplet_widget_signals[REVERT_SIGNAL] =
		gtk_signal_new("revert",
			       GTK_RUN_LAST,
			       object_class->type,
			       GTK_SIGNAL_OFFSET(CappletWidgetClass,
			       			 revert),
                               gtk_signal_default_marshaller,
                               GTK_TYPE_NONE, 0);
	capplet_widget_signals[OK_SIGNAL] =
		gtk_signal_new("ok",
			       GTK_RUN_LAST,
			       object_class->type,
			       GTK_SIGNAL_OFFSET(CappletWidgetClass,
			       			 ok),
                               gtk_signal_default_marshaller,
                               GTK_TYPE_NONE, 0);
	capplet_widget_signals[HELP_SIGNAL] =
		gtk_signal_new("help",
			       GTK_RUN_LAST,
			       object_class->type,
			       GTK_SIGNAL_OFFSET(CappletWidgetClass,
			       			 help),
                               gtk_signal_default_marshaller,
                               GTK_TYPE_NONE, 0);

        klass->try = NULL;
        klass->revert = NULL;
        klass->ok = NULL;
        klass->help = NULL;
}
static void
capplet_widget_init (CappletWidget *widget)
{
        widget->control_center_id = id;
        capplet_list = g_list_prepend (capplet_list, widget);
        widget->changed = FALSE;
}
GtkWidget *
capplet_widget_new ()
{
        CappletWidget * retval;
        retval = CAPPLET_WIDGET (gtk_type_new (capplet_widget_get_type()));
        gtk_plug_construct (GTK_PLUG (retval), xid);
        
        
        return GTK_WIDGET (retval);
}

/* non widget calls */
void
capplet_gtk_main (void)
{
        capplet_corba_gtk_main();
}
error_t
parse_an_arg (int key, char *arg, struct argp_state *state)
{
        switch (key) {
        case 'd':
                id = atoi (arg);
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
        { "id",    'd', N_("ID"),       0, N_("id of the caplet -- assigned by the control-center"),      1 },
        { "ior",    'i', N_("IOR"),      0, N_("ior of the control-center"), 1 },
        { "xid",    'x', N_("XID"),      0, N_("X id of the socket it's plugged into"), 1 },
        { NULL,     0,  NULL,         0, NULL,                       0 }
};
static struct argp parser = {
        options, parse_an_arg, NULL,  NULL,  NULL, NULL, NULL
};

error_t
gnome_capplet_init (char *app_id, struct argp *app_parser,
                     int argc, char **argv, unsigned int flags,
                     int *arg_index)
{
        /* FIXME: er, we need to actually parse app_parser... (: */
        error_t retval;
        retval = gnome_init(app_id,&parser,argc,argv,flags,arg_index);

        if ((xid == 0) || (cc_ior == NULL) || (id == -1)) {
                g_warning ("Insufficient arguments passed to the arg parser.\n");
                exit (1);
        }
        capplet_widget_corba_init(&argc, argv, cc_ior, id);

        return retval;
}
void
_capplet_widget_server_try()
{
        g_print (" in _capplet_widget_server_try\n");
        //        gtk_signal_emit();
}
void
_capplet_widget_server_revert()
{
        
}
void
_capplet_widget_server_ok()
{
        
}
void
_capplet_widget_server_help()
{
        
}
void
capplet_widget_state_changed(CappletWidget *cap, gboolean undoable)
{
        if (cap->changed == FALSE) {
                GNOME_control_center_state_changed(control_center, cap->control_center_id, undoable, &ev);
                cap->changed = TRUE;
        }
}
GtkWidget *
get_widget_by_id(gint id)
{
        return NULL;
}
