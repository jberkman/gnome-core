/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
#include <gnome.h>
#include <orb/orbit.h>
#include "capplet-widget.h"
#include "capplet-widget-libs.h"
#include "control-center.h"

/* variables */
static GtkPlugClass *parent_class;
static GList *capplet_list = NULL;

enum {
	TRY_SIGNAL,
	REVERT_SIGNAL,
	OK_SIGNAL,
        CANCEL_SIGNAL,
        NEW_MULTI_CAPPLET,
        LAST_SIGNAL
};
static int capplet_widget_signals[LAST_SIGNAL] = {0,0,0,0};

/* prototypes */ 
static void capplet_widget_class_init	(CappletWidgetClass *klass);
static void capplet_widget_init		(CappletWidget      *applet_widget);
static GtkWidget *get_widget_by_id(gint id);
void _capplet_widget_server_try(gint id);
void _capplet_widget_server_revert(gint id);
void _capplet_widget_server_ok(gint id);
void _capplet_widget_server_cancel(gint id);
void _capplet_widget_server_new_multi_capplet(gint id, gint capid);


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
capplet_close_callback (GtkWidget *capplet)
{
        GList *temp;
        for (temp = capplet_list; temp; temp=temp->next)
                if (CAPPLET_WIDGET (temp->data) == CAPPLET_WIDGET (capplet)) {
                        capplet_list = g_list_remove_link (capplet_list, temp);
                        if (capplet_list == NULL) {
                                capplet_corba_gtk_main_quit ();
                        }
                }
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
                               gtk_marshal_NONE__NONE,
                               GTK_TYPE_NONE, 0);
	capplet_widget_signals[REVERT_SIGNAL] =
		gtk_signal_new("revert",
			       GTK_RUN_LAST,
			       object_class->type,
			       GTK_SIGNAL_OFFSET(CappletWidgetClass,
			       			 revert),
                               gtk_marshal_NONE__NONE,
                               GTK_TYPE_NONE, 0);
  	capplet_widget_signals[OK_SIGNAL] =
		gtk_signal_new("ok",
			       GTK_RUN_LAST,
			       object_class->type,
			       GTK_SIGNAL_OFFSET(CappletWidgetClass,
			       			 ok),
                               gtk_marshal_NONE__NONE,
                               GTK_TYPE_NONE, 0);
  	capplet_widget_signals[CANCEL_SIGNAL] =
		gtk_signal_new("cancel",
			       GTK_RUN_LAST,
			       object_class->type,
			       GTK_SIGNAL_OFFSET(CappletWidgetClass,
			       			 cancel),
                               gtk_marshal_NONE__NONE,
                               GTK_TYPE_NONE, 0);
	capplet_widget_signals[NEW_MULTI_CAPPLET] =
		gtk_signal_new("new_multi_capplet",
			       GTK_RUN_LAST,
			       object_class->type,
			       GTK_SIGNAL_OFFSET(CappletWidgetClass,
			       			 new_multi_capplet),
                               gtk_marshal_NONE__POINTER,
                               GTK_TYPE_NONE, 1, GTK_TYPE_POINTER);

        gtk_object_class_add_signals (object_class, capplet_widget_signals, LAST_SIGNAL);
        klass->try = NULL;
        klass->revert = NULL;
        klass->ok = NULL;
        klass->new_multi_capplet = NULL;
}
static void
capplet_widget_init (CappletWidget *widget)
{
        capplet_list = g_list_prepend (capplet_list, widget);
        gtk_signal_connect (GTK_OBJECT (widget), "destroy",
                            (GtkSignalFunc) capplet_close_callback, NULL);
        widget->changed = FALSE;
}
GtkWidget *
capplet_widget_new ()
{
        CappletWidget * retval;

        retval = CAPPLET_WIDGET (gtk_type_new (capplet_widget_get_type()));
        /* we should set this: */
        retval->capid = get_capid ();

        retval->control_center_id = get_ccid (retval->capid);
        gtk_plug_construct (GTK_PLUG (retval), get_xid (retval->capid));

        return GTK_WIDGET (retval);
}
GtkWidget *
capplet_widget_multi_new (gint capid)
{
        CappletWidget * retval;

        retval = CAPPLET_WIDGET (gtk_type_new (capplet_widget_get_type()));
        retval->capid = capid;
        retval->control_center_id = get_ccid (retval->capid);
        gtk_plug_construct (GTK_PLUG (retval), get_xid (retval->capid));
        
        return GTK_WIDGET (retval);
}
void
capplet_widget_state_changed(CappletWidget *cap, gboolean undoable)
{
        if (cap->changed == FALSE) {
                capplet_corba_state_changed (cap->control_center_id, undoable);
                cap->changed = TRUE;
        }
}
/* non widget calls */
void
capplet_gtk_main (void)
{
        capplet_corba_gtk_main();
}
gint
gnome_capplet_init (const char *app_id, const char *app_version,
                    int argc, char **argv, struct poptOption *options,
                    unsigned int flags, poptContext *return_ctx)
{

        return capplet_widget_corba_init (app_id, app_version, &argc,
                                          argv, options, flags, return_ctx);

}

/* internal calls */
void
_capplet_widget_server_try(gint id)
{
        GtkWidget *capplet = get_widget_by_id (id);
        CAPPLET_WIDGET (capplet)->changed = FALSE;
        gtk_signal_emit_by_name(GTK_OBJECT (capplet) ,"try");
}
void
_capplet_widget_server_revert(gint id)
{
        GtkWidget *capplet = get_widget_by_id (id);
        CAPPLET_WIDGET (capplet)->changed = FALSE;
        gtk_signal_emit_by_name(GTK_OBJECT (capplet) ,"revert");
}
void
_capplet_widget_server_ok(gint id)
{
        GtkWidget *capplet = get_widget_by_id (id);
        gtk_signal_emit_by_name(GTK_OBJECT (capplet) ,"ok");
}
void
_capplet_widget_server_cancel(gint id)
{
        GtkWidget *capplet = get_widget_by_id (id);
        gtk_signal_emit_by_name(GTK_OBJECT (capplet) ,"cancel");
}
void
_capplet_widget_server_new_multi_capplet(gint id, gint capid)
{
        GtkWidget *capplet = get_widget_by_id (id);
        gtk_signal_emit_by_name(GTK_OBJECT (capplet) ,"new_multi_capplet", capplet_widget_multi_new (capid));
}
static GtkWidget *
get_widget_by_id(gint id)
{
        GList *temp;
        for (temp = capplet_list; temp; temp=temp->next) 
                if (CAPPLET_WIDGET (temp->data)->control_center_id == id)
                        return GTK_WIDGET (temp->data);
        return NULL;
}
