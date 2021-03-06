/* Gnome panel: panel widget
 * (C) 1997,1998,1999,2000 the Free Software Foundation
 * (C) 2000 Eazel, Inc.
 *
 * Authors:  George Lebl
 */
#include <config.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include <libart_lgpl/art_misc.h>
#include <libart_lgpl/art_affine.h>
#include <libart_lgpl/art_filterlevel.h>

#include "applet.h"
#include "panel-widget.h"
#include "button-widget.h"
#include "panel.h"
#include "panel-util.h"
#include "panel-marshal.h"
#include "rgb-stuff.h"
#include "panel-typebuiltins.h"

#define MOVE_INCREMENT 2

typedef enum {
	PANEL_SWITCH_MOVE = 0,
	PANEL_FREE_MOVE,
	PANEL_PUSH_MOVE
} PanelMovementType;

GSList *panels = NULL; /*other panels we might want to move the applet to*/

/*define for some debug output*/
#undef PANEL_WIDGET_DEBUG

/*there  can universally be only one applet being dragged since we assume
we only have one mouse :) */
gboolean panel_applet_in_drag = FALSE;

/* Commie mode! */
extern gboolean commie_mode;

static void panel_widget_class_init     (PanelWidgetClass *klass);
static void panel_widget_instance_init  (PanelWidget      *panel_widget);
static int  panel_try_to_set_pixmap     (PanelWidget      *panel,
					 const char       *pixmap);
static void panel_resize_pixmap         (PanelWidget      *panel);
static void panel_try_to_set_back_color (PanelWidget      *panel,
					 GdkColor         *color);
static void panel_widget_size_request   (GtkWidget        *widget,
					 GtkRequisition   *requisition);
static void panel_widget_size_allocate  (GtkWidget        *widget,
					 GtkAllocation    *allocation);
static void panel_widget_cadd           (GtkContainer     *container,
					 GtkWidget        *widget);
static void panel_widget_cremove        (GtkContainer     *container,
					 GtkWidget        *widget);
static int  panel_widget_expose         (GtkWidget        *widget,
					 GdkEventExpose   *event);
static void panel_widget_destroy        (GtkObject        *obj);
static void panel_widget_style_set      (GtkWidget        *widget,
					 GtkStyle         *previous_style);
static void panel_widget_realize        (GtkWidget        *widget);

static gboolean panel_widget_focus_in_event (GtkWidget        *widget,
					     GdkEventFocus    *event);
static gboolean panel_widget_focus_out_event(GtkWidget        *widget,
					     GdkEventFocus    *event);

static void panel_widget_push_move_applet   (PanelWidget      *panel,
                                             GtkDirectionType  dir);
static void panel_widget_switch_move_applet (PanelWidget      *panel,
                                             GtkDirectionType  dir);
static void panel_widget_free_move_applet   (PanelWidget      *panel,
                                             GtkDirectionType  dir);
static void panel_widget_tab_move           (PanelWidget      *panel,
                                             gboolean          next);
static void panel_widget_end_move           (PanelWidget      *panel);


typedef void (*BackSignal) (GtkObject * object,
			    PanelBackType type,
			    char *pixmap,
			    GdkColor *color,
			    gpointer data);

/************************
 debugging
 ************************/

#ifdef PANEL_WIDGET_DEBUG
static void
debug_dump_panel_list(PanelWidget *panel)
{
	GList *list;
	puts("\nDUMP START\n");
	for(list = panel->applet_list;list!=NULL;list=g_list_next(list)) {
		AppletData *ad = list->data;
		printf("pos: %d cells: %d\n",ad->pos,ad->cells);
	}
	puts("\nDUMP END\n");
}

static void
debug_panel_widget_dump_applet_data (AppletData *data) 
{
	printf ("===== Applet Dumped =====\n");
	printf ("\tPosition : %d\n", data->pos);
	printf ("\tCells : %d\n", data->cells);
	printf ("\tDirty : %d\n", data->dirty);
	printf ("\tDrag Off: %d\n", data->drag_off);
}
#endif

/************************
 convenience functions
 ************************/
static int
applet_data_compare (AppletData *ad1, AppletData *ad2)
{
	return ad1->pos - ad2->pos;
}

/************************
 widget core
 ************************/

static GtkFixedClass *panel_widget_parent_class = NULL;

GType
panel_widget_get_type (void)
{
	static GType object_type= 0;

	if (object_type == 0) {
		static const GTypeInfo object_info = {
                    sizeof (PanelWidgetClass),
                    (GBaseInitFunc)         NULL,
                    (GBaseFinalizeFunc)     NULL,
                    (GClassInitFunc)        panel_widget_class_init,
                    NULL,                   /* class_finalize */
                    NULL,                   /* class_data */
                    sizeof (PanelWidget),
                    0,                      /* n_preallocs */
                    (GInstanceInitFunc)     panel_widget_instance_init 
		};

		object_type = g_type_register_static (GTK_TYPE_FIXED , "PanelWidget", &object_info, 0);
		panel_widget_parent_class = g_type_class_ref (GTK_TYPE_FIXED);
	}

	return object_type;
}

enum {
	ORIENT_CHANGE_SIGNAL,
	SIZE_CHANGE_SIGNAL,
	APPLET_MOVE_SIGNAL,
	APPLET_ADDED_SIGNAL,
	APPLET_REMOVED_SIGNAL,
	BACK_CHANGE_SIGNAL,
	APPLET_ABOUT_TO_DIE_SIGNAL,
	PUSH_MOVE_SIGNAL,
	SWITCH_MOVE_SIGNAL,
	FREE_MOVE_SIGNAL,
	TAB_MOVE_SIGNAL,
	END_MOVE_SIGNAL,
	LAST_SIGNAL
};

static guint panel_widget_signals[LAST_SIGNAL] = {0};

static void
add_tab_bindings (GtkBindingSet    *binding_set,
   	          GdkModifierType   modifiers,
		  gboolean          next)
{
	gtk_binding_entry_add_signal (binding_set, GDK_Tab, modifiers,
				      "tab_move", 1,
				      G_TYPE_BOOLEAN, next);
  	gtk_binding_entry_add_signal (binding_set, GDK_KP_Tab, modifiers,
				      "tab_move", 1,
				      G_TYPE_BOOLEAN, next);
  	gtk_binding_entry_add_signal (binding_set, GDK_ISO_Left_Tab, modifiers,
				      "tab_move", 1,
				      G_TYPE_BOOLEAN, next);
}

static void
add_move_bindings (GtkBindingSet    *binding_set,
		   GdkModifierType   modifiers,
		   const gchar      *name)
{
	gtk_binding_entry_add_signal (binding_set, GDK_Up, modifiers,
                                      name, 1,
                                      GTK_TYPE_DIRECTION_TYPE, GTK_DIR_UP);
	gtk_binding_entry_add_signal (binding_set, GDK_Down, modifiers,
                                      name, 1,
                                      GTK_TYPE_DIRECTION_TYPE, GTK_DIR_DOWN);
	gtk_binding_entry_add_signal (binding_set, GDK_Left, modifiers,
                                      name, 1,
                                      GTK_TYPE_DIRECTION_TYPE, GTK_DIR_LEFT);
	gtk_binding_entry_add_signal (binding_set, GDK_Right, modifiers,
                                      name, 1,
                                      GTK_TYPE_DIRECTION_TYPE, GTK_DIR_RIGHT);
}

static void
add_all_move_bindings (PanelWidget *panel)
{
	GtkWidgetClass *class;
	GtkBindingSet *binding_set;

	class = GTK_WIDGET_GET_CLASS (panel);

	binding_set = gtk_binding_set_by_class (class);

	add_move_bindings (binding_set, GDK_SHIFT_MASK, "push_move");
	add_move_bindings (binding_set, GDK_CONTROL_MASK, "switch_move");
	add_move_bindings (binding_set, GDK_MOD1_MASK, "free_move");

	if (!GTK_WIDGET_HAS_FOCUS (panel)) {
		add_tab_bindings (binding_set, 0, TRUE);
		add_tab_bindings (binding_set, GDK_SHIFT_MASK, FALSE);
	}
	gtk_binding_entry_add_signal (binding_set,
                                      GDK_Escape, 0,
                                      "end_move", 0);
}

static void
remove_tab_bindings (GtkBindingSet    *binding_set,
		     GdkModifierType   modifiers,
		     gboolean          next)
{
	gtk_binding_entry_clear (binding_set, GDK_Tab, modifiers);
  	gtk_binding_entry_clear (binding_set, GDK_KP_Tab, modifiers);
  	gtk_binding_entry_clear (binding_set, GDK_ISO_Left_Tab, modifiers);
}

static void
remove_move_bindings (GtkBindingSet    *binding_set,
		      GdkModifierType   modifiers)
{
	gtk_binding_entry_clear (binding_set, GDK_Up, modifiers);
	gtk_binding_entry_clear (binding_set, GDK_Down, modifiers);
	gtk_binding_entry_clear (binding_set, GDK_Left, modifiers);
	gtk_binding_entry_clear (binding_set, GDK_Right, modifiers);
}

static void
remove_all_move_bindings (PanelWidget *panel)
{
	GtkWidgetClass *class;
	GtkBindingSet *binding_set;

	class = GTK_WIDGET_GET_CLASS (panel);

	binding_set = gtk_binding_set_by_class (class);

	remove_move_bindings (binding_set, GDK_SHIFT_MASK);
	remove_move_bindings (binding_set, GDK_CONTROL_MASK);
	remove_move_bindings (binding_set, GDK_MOD1_MASK);
	if (!GTK_WIDGET_HAS_FOCUS (panel)) {
		remove_tab_bindings (binding_set, 0, TRUE);
		remove_tab_bindings (binding_set, GDK_SHIFT_MASK, FALSE);
	}
	gtk_binding_entry_clear (binding_set, GDK_Escape, 0);
}

static void
panel_widget_class_init (PanelWidgetClass *class)
{
	GtkObjectClass *object_class = (GtkObjectClass*) class;
	GtkWidgetClass *widget_class = (GtkWidgetClass*) class;
	GtkContainerClass *container_class = (GtkContainerClass*) class;

	GtkBindingSet *binding_set;

	panel_widget_signals[ORIENT_CHANGE_SIGNAL] =
                g_signal_new ("orient_change",
                              G_TYPE_FROM_CLASS (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (PanelWidgetClass, orient_change),
                              NULL,
                              NULL, 
                              panel_marshal_VOID__ENUM,
                              G_TYPE_NONE,
                              1,
                              GTK_TYPE_ORIENTATION); 

	panel_widget_signals[SIZE_CHANGE_SIGNAL] =
                g_signal_new ("size_change",
                              G_TYPE_FROM_CLASS (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (PanelWidgetClass, size_change),
                              NULL,
                              NULL, 
                              panel_marshal_VOID__INT,
                              G_TYPE_NONE,
                              1,
                              G_TYPE_INT); 

	panel_widget_signals[APPLET_MOVE_SIGNAL] =
                g_signal_new ("applet_move",
                              G_TYPE_FROM_CLASS (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (PanelWidgetClass, applet_move),
                              NULL,
                              NULL, 
                              panel_marshal_VOID__POINTER,
                              G_TYPE_NONE,
                              1,
                              G_TYPE_POINTER); 

	panel_widget_signals[APPLET_ADDED_SIGNAL] =
                g_signal_new ("applet_added",
                              G_TYPE_FROM_CLASS (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (PanelWidgetClass, applet_added),
                              NULL,
                              NULL, 
                              panel_marshal_VOID__POINTER,
                              G_TYPE_NONE,
                              1,
                              G_TYPE_POINTER); 

	panel_widget_signals[APPLET_REMOVED_SIGNAL] =
                g_signal_new ("applet_removed",
                              G_TYPE_FROM_CLASS (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (PanelWidgetClass, applet_removed),
                              NULL,
                              NULL, 
                              panel_marshal_VOID__POINTER,
                              G_TYPE_NONE,
                              1,
                              G_TYPE_POINTER); 

	panel_widget_signals[BACK_CHANGE_SIGNAL] =
                g_signal_new ("back_change",
                              G_TYPE_FROM_CLASS (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (PanelWidgetClass, back_change),
                              NULL,
                              NULL, 
                              panel_marshal_VOID__ENUM_POINTER_POINTER,
                              G_TYPE_NONE,
                              3,
			      PANEL_TYPE_PANEL_BACK_TYPE,
			      G_TYPE_POINTER,
			      G_TYPE_POINTER);

	panel_widget_signals[APPLET_ABOUT_TO_DIE_SIGNAL] =
                g_signal_new ("applet_about_to_die",
                              G_TYPE_FROM_CLASS (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (PanelWidgetClass, applet_about_to_die),
                              NULL,
                              NULL, 
                              panel_marshal_VOID__POINTER,
                              G_TYPE_NONE,
                              1,
                              G_TYPE_POINTER); 

	panel_widget_signals[PUSH_MOVE_SIGNAL] =
		g_signal_new ("push_move",
                              G_TYPE_FROM_CLASS (class),
                              G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                              G_STRUCT_OFFSET (PanelWidgetClass, push_move),
                              NULL,
                              NULL,
                              panel_marshal_VOID__ENUM,
                              G_TYPE_NONE,
                              1,
                              GTK_TYPE_DIRECTION_TYPE);
	
	panel_widget_signals[SWITCH_MOVE_SIGNAL] =
                g_signal_new ("switch_move",
                              G_TYPE_FROM_CLASS (class),
                              G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                              G_STRUCT_OFFSET (PanelWidgetClass, switch_move),
                              NULL,
                              NULL,
                              panel_marshal_VOID__ENUM,
                              G_TYPE_NONE,
                              1,
                              GTK_TYPE_DIRECTION_TYPE);

	panel_widget_signals[FREE_MOVE_SIGNAL] =
                g_signal_new ("free_move",
                              G_TYPE_FROM_CLASS (class),
                              G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                              G_STRUCT_OFFSET (PanelWidgetClass, free_move),
                              NULL,
                              NULL,
                              panel_marshal_VOID__ENUM,
                              G_TYPE_NONE,
                              1,
                              GTK_TYPE_DIRECTION_TYPE);

	panel_widget_signals[TAB_MOVE_SIGNAL] =
                g_signal_new ("tab_move",
                              G_TYPE_FROM_CLASS (class),
                              G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			      G_STRUCT_OFFSET (PanelWidgetClass, tab_move),
                              NULL,
                              NULL,
                              panel_marshal_VOID__BOOLEAN,
                              G_TYPE_NONE,
                              1,
                              G_TYPE_BOOLEAN);

	panel_widget_signals[END_MOVE_SIGNAL] =
		g_signal_new ("end_move",
                              G_TYPE_FROM_CLASS (class),
                              G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                              G_STRUCT_OFFSET (PanelWidgetClass, end_move),
                              NULL,
                              NULL,
                              panel_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);

	class->orient_change = NULL;
	class->size_change = NULL;
	class->applet_move = NULL;
	class->applet_added = NULL;
	class->applet_removed = NULL;
	class->back_change = NULL;
	class->applet_about_to_die = NULL;
	class->push_move = panel_widget_push_move_applet;
	class->switch_move = panel_widget_switch_move_applet;
	class->free_move = panel_widget_free_move_applet;
	class->tab_move = panel_widget_tab_move;
	class->end_move = panel_widget_end_move;

	object_class->destroy = panel_widget_destroy;
	
	widget_class->size_request = panel_widget_size_request;
	widget_class->size_allocate = panel_widget_size_allocate;
	widget_class->realize = panel_widget_realize;
	widget_class->expose_event = panel_widget_expose;
	widget_class->style_set = panel_widget_style_set;
	widget_class->focus_in_event = panel_widget_focus_in_event;
	widget_class->focus_out_event = panel_widget_focus_out_event;

	container_class->add = panel_widget_cadd;
	container_class->remove = panel_widget_cremove;

	binding_set = gtk_binding_set_by_class (class);
	add_tab_bindings (binding_set, 0, TRUE);
	add_tab_bindings (binding_set, GDK_SHIFT_MASK, FALSE);
}

static void
remove_panel_from_forbidden(PanelWidget *panel, PanelWidget *r)
{
	GSList *list;
	GtkWidget *parent_panel;
	
	g_return_if_fail(PANEL_IS_WIDGET(panel));
	g_return_if_fail(PANEL_IS_WIDGET(r));

	if(!panel->master_widget)
		return;

	list = g_object_get_data (G_OBJECT(panel->master_widget),
				  PANEL_APPLET_FORBIDDEN_PANELS);
	if(list) {
		list = g_slist_remove(list,r);
		g_object_set_data (G_OBJECT(panel->master_widget),
				   PANEL_APPLET_FORBIDDEN_PANELS,
				   list);
	}
	parent_panel = panel->master_widget->parent;
	if (parent_panel)
		remove_panel_from_forbidden(PANEL_WIDGET(parent_panel), r);
}

static void
add_panel_to_forbidden(PanelWidget *panel, PanelWidget *r)
{
	GSList *list;
	GtkWidget *parent_panel;

	g_return_if_fail(PANEL_IS_WIDGET(panel));
	g_return_if_fail(PANEL_IS_WIDGET(r));

	if(!panel->master_widget)
		return;

	list = g_object_get_data (G_OBJECT(panel->master_widget),
				  PANEL_APPLET_FORBIDDEN_PANELS);
	if(g_slist_find(list,r)==NULL) {
		list = g_slist_prepend(list,r);

		g_object_set_data (G_OBJECT(panel->master_widget),
				   PANEL_APPLET_FORBIDDEN_PANELS,
				   list);
	}
	parent_panel = panel->master_widget->parent;
	if (parent_panel)
		add_panel_to_forbidden(PANEL_WIDGET(parent_panel), r);
}

static void
run_up_forbidden(PanelWidget *panel,
		 void (*runfunc)(PanelWidget *,PanelWidget *))
{
	GList *list;

	g_return_if_fail(PANEL_IS_WIDGET(panel));

	for(list = panel->applet_list;list!=NULL;list = g_list_next(list)) {
		AppletData *ad = list->data;
		PanelWidget *p =
			g_object_get_data (G_OBJECT(ad->applet),
					   PANEL_APPLET_ASSOC_PANEL_KEY);
		if(p)
			run_up_forbidden(p,runfunc);
	}
	(*runfunc)(panel,panel);
}


static void
panel_widget_cadd(GtkContainer *container, GtkWidget *widget)
{
	PanelWidget *p;

	g_return_if_fail (PANEL_IS_WIDGET (container));
	g_return_if_fail (GTK_IS_WIDGET (widget));

	panel_widget_add(PANEL_WIDGET(container),widget,0);
	p = g_object_get_data (G_OBJECT(widget),
			       PANEL_APPLET_ASSOC_PANEL_KEY);
	if(p)
		run_up_forbidden(p,add_panel_to_forbidden);
}

static void
panel_widget_cremove (GtkContainer *container, GtkWidget *widget)
{
	AppletData *ad;
	PanelWidget *p;
	PanelWidget *panel;

	g_return_if_fail (PANEL_IS_WIDGET (container));
	g_return_if_fail (GTK_IS_WIDGET (widget));
	
	panel = PANEL_WIDGET (container);
	
	ad = g_object_get_data (G_OBJECT (widget), PANEL_APPLET_DATA);
	p = g_object_get_data (G_OBJECT (widget),
				 PANEL_APPLET_ASSOC_PANEL_KEY);

	if (ad != NULL &&
	    ad->no_die == 0) {
		g_signal_emit (G_OBJECT (container),
			       panel_widget_signals[APPLET_ABOUT_TO_DIE_SIGNAL],
			       0, widget);
	}


	if (p != NULL)
		run_up_forbidden (p, remove_panel_from_forbidden);

	if(panel->currently_dragged_applet == ad)
		panel_widget_applet_drag_end(panel);

	gtk_widget_ref(widget);
	if (GTK_CONTAINER_CLASS (panel_widget_parent_class)->remove)
		(* GTK_CONTAINER_CLASS (panel_widget_parent_class)->remove) (container,
								widget);
	if (ad != NULL) {
		PanelWidget *panel = PANEL_WIDGET (container);

		panel->applet_list = g_list_remove (panel->applet_list, ad);
	}

	g_signal_emit (G_OBJECT (container),
		       panel_widget_signals[APPLET_REMOVED_SIGNAL],
		       0, widget);
	gtk_widget_unref(widget);
}


/*get the list item of the data on the position pos*/
static GList *
get_applet_list_pos(PanelWidget *panel, int pos)
{
	GList *list;

	g_return_val_if_fail(PANEL_IS_WIDGET(panel),NULL);
	
	for(list=panel->applet_list;list!=NULL;list=g_list_next(list)) {
		AppletData *ad = list->data;
		if(ad->pos <= pos) {
		       if(ad->pos+ad->cells > pos)
			       return list;
		} else
			return NULL;
	}
	return NULL;
}

/*tells us if an applet is "stuck" on the right side*/
int
panel_widget_is_applet_stuck(PanelWidget *panel, GtkWidget *applet)
{
	AppletData *ad;
	GList *list;
	int i;

	g_return_val_if_fail(PANEL_IS_WIDGET(panel),FALSE);
	g_return_val_if_fail(GTK_IS_WIDGET(applet),FALSE);

	ad = g_object_get_data (G_OBJECT(applet), PANEL_APPLET_DATA);

	g_return_val_if_fail(ad!=NULL,FALSE);

	list = g_list_find(panel->applet_list,ad);

	g_return_val_if_fail(list!=NULL,FALSE);
	
	do {
		i=ad->pos+ad->cells;
		if(i==panel->size)
			return TRUE;
		list = g_list_next(list);
		if(!list)
			break;
		ad = list->data;
	} while(ad->pos==i);
	return FALSE;
}

static int
allocate_dirty_child(gpointer data)
{
	AppletData *ad = data;
	AppletData *nad;
	PanelWidget *panel;
	GtkAllocation challoc;
	GtkRequisition chreq;
	
	g_return_val_if_fail(ad != NULL,FALSE);
	g_return_val_if_fail(ad->applet != NULL,FALSE);
	g_return_val_if_fail(PANEL_IS_WIDGET(ad->applet->parent),FALSE);

	panel = PANEL_WIDGET(ad->applet->parent);
	
	if(!ad->dirty)
		return FALSE;
	
	gtk_widget_get_child_requisition(ad->applet,&chreq);

	challoc.width = chreq.width;
	challoc.height = chreq.height;
	if(panel->orient == GTK_ORIENTATION_HORIZONTAL) {
		if (ad->expand_minor) {
			challoc.height = GTK_WIDGET (panel)->allocation.height;
		}
		ad->cells = chreq.width; 
		challoc.x = ad->pos;
		challoc.y = (GTK_WIDGET(panel)->allocation.height -
			     challoc.height) / 2;
	} else {
		if (ad->expand_minor) {
			challoc.width = GTK_WIDGET (panel)->allocation.width;
		}
		ad->cells = chreq.height;
		challoc.x = (GTK_WIDGET(panel)->allocation.width -
			     challoc.width) / 2;
		challoc.y = ad->pos;
	}

	ad->min_cells = ad->cells;

	if (!panel->packed && ad->expand_major) {
		GList *l = g_list_find (panel->applet_list, ad);
		if (l) {
			if (l->next) {
				nad = l->next->data;
				ad->cells = nad->pos - ad->pos - 1;
			} else {
				ad->cells = panel->size - ad->pos - 1;
			}
			
			if (panel->orient == GTK_ORIENTATION_HORIZONTAL) {
				challoc.width = ad->cells;
			} else {
				challoc.height = ad->cells;
			}
				

		}
	}

	gtk_widget_size_allocate(ad->applet,&challoc);

	g_signal_emit (G_OBJECT(panel),
		       panel_widget_signals[APPLET_MOVE_SIGNAL],
		       0, ad->applet);

	return FALSE;
}

static void
panel_widget_queue_applet_for_resize(AppletData *ad)
{
	ad->dirty = TRUE;
	g_idle_add(allocate_dirty_child,ad);
}


static void
panel_widget_switch_applet_right(PanelWidget *panel, GList *list)
{
	AppletData *ad;
	AppletData *nad = NULL;

	g_return_if_fail(PANEL_IS_WIDGET(panel));
	g_return_if_fail(list!=NULL);
	
	ad = list->data;

	if(list->next)
		nad = list->next->data;
	if(!nad || nad->pos > ad->pos+ad->min_cells) {
		ad->pos++;
		panel_widget_queue_applet_for_resize(ad);
		return;
	}

	nad->pos = ad->pos;
	ad->pos = nad->pos+nad->min_cells;
	panel->applet_list = panel_g_list_swap_next(panel->applet_list,list);

	panel_widget_queue_applet_for_resize(ad);
	panel_widget_queue_applet_for_resize(nad);
}

static void
panel_widget_switch_applet_left(PanelWidget *panel, GList *list)
{
	AppletData *ad;
	AppletData *pad = NULL;

	g_return_if_fail(PANEL_IS_WIDGET(panel));
	g_return_if_fail(list!=NULL);
	
	ad = list->data;


	if(list->prev)
		pad = list->prev->data;


	if(!pad || pad->pos+pad->min_cells < ad->pos) {
		ad->pos--;
		panel_widget_queue_applet_for_resize(ad);
		if (pad && pad->pos+pad->cells >= ad->pos)
			panel_widget_queue_applet_for_resize(pad);
		return;
	}

	ad->pos = pad->pos;
	pad->pos = ad->pos+ad->min_cells;
	panel->applet_list = panel_g_list_swap_prev(panel->applet_list,list);

	panel_widget_queue_applet_for_resize(ad);
	panel_widget_queue_applet_for_resize(pad);
}

static int
panel_widget_get_right_switch_pos(PanelWidget *panel, GList *list)
{
	AppletData *ad;
	AppletData *nad = NULL;

	g_return_val_if_fail(PANEL_IS_WIDGET(panel),-1);
	g_return_val_if_fail(list!=NULL,-1);
	
	ad = list->data;


	if(list->next)
		nad = list->next->data;

	if(!nad || nad->pos > ad->pos+ad->cells) {
		return ad->pos+1;
	}

	return nad->pos+nad->min_cells-ad->cells;
}

static int
panel_widget_get_left_switch_pos(PanelWidget *panel, GList *list)
{
	AppletData *ad;
	AppletData *pad = NULL;

	g_return_val_if_fail(PANEL_IS_WIDGET(panel),-1);
	g_return_val_if_fail(list!=NULL,-1);
	
	ad = list->data;

	if(list->prev)
		pad = list->prev->data;
	if(!pad || pad->pos+pad->min_cells < ad->pos)
		return ad->pos-1;
	return pad->pos;
}


static void
panel_widget_switch_move (PanelWidget *panel, AppletData *ad, int moveby)
{
	int finalpos;
	int pos;
	GList *list;
	AppletData *pad;

	g_return_if_fail (ad != NULL);
	g_return_if_fail (PANEL_IS_WIDGET (panel));

	if (moveby == 0)
		return;

	list = g_list_find (panel->applet_list, ad);
	g_return_if_fail (list != NULL);

	finalpos = ad->pos + moveby;

	if (ad->pos < finalpos) {
		while (ad->pos < finalpos) {
			pos = panel_widget_get_right_switch_pos (panel, list);
			if (pos + ad->min_cells > panel->size ||
			    abs (pos - finalpos) > abs (ad->pos - finalpos))
				break;
			panel_widget_switch_applet_right (panel, list);
		}
		if (list->prev) {
			pad = list->prev->data;
			if (pad->expand_major)
				panel_widget_queue_applet_for_resize(pad);
		}
	} else {
		while (ad->pos > finalpos) {
			pos = panel_widget_get_left_switch_pos (panel, list);
			if (pos < 0 || abs (pos - finalpos) > abs (ad->pos - finalpos))
				break;
			panel_widget_switch_applet_left (panel, list);
		}
	}
}

static int
push_applet_right(PanelWidget *panel, GList *list)
{
	AppletData *ad = NULL;
	AppletData *nad = NULL;

	g_return_val_if_fail(PANEL_IS_WIDGET(panel),FALSE);
	
	if (list) {
		ad = list->data;
	
		if(ad->pos + ad->min_cells >= panel->size)
			return FALSE;

		if(list->next)
			nad = list->next->data;
	}

	if(!nad || nad->pos > ad->pos+ad->min_cells) {
		ad->pos++;
		panel_widget_queue_applet_for_resize(ad);
		return TRUE;
	}

	g_return_val_if_fail(list!=NULL,FALSE);
	
	if(!push_applet_right(panel,list->next)) {
		return FALSE;
	}
	ad->pos++;
	panel_widget_queue_applet_for_resize(ad);
	return TRUE;
}

static int
push_applet_left(PanelWidget *panel, GList *list)
{
	AppletData *ad = NULL;
	AppletData *pad = NULL;

	g_return_val_if_fail(PANEL_IS_WIDGET(panel),FALSE);

	if (list) {
		ad = list->data;

		if(ad->pos <= 0)
			return FALSE;

		if(list->prev)
			pad = list->prev->data;
	}

	if(!list || !pad || pad->pos+pad->min_cells < ad->pos) {
		ad->pos--;
		panel_widget_queue_applet_for_resize(ad);
		if (pad && pad->pos+pad->cells >= ad->pos)
			panel_widget_queue_applet_for_resize(pad);
		return TRUE;
	}

	g_return_val_if_fail(list!=NULL,FALSE);
	
	if(!push_applet_left(panel,list->prev)) {
		return FALSE;
	}
	ad->pos--;
	panel_widget_queue_applet_for_resize(ad);
	return TRUE;
}

static void
panel_widget_push_move (PanelWidget *panel, AppletData *ad, int moveby)
{
	AppletData *pad;
	int finalpos;
	GList *list;

	g_return_if_fail (ad != NULL);
	g_return_if_fail (PANEL_IS_WIDGET (panel));

	if (moveby == 0)
		return;

	list = g_list_find (panel->applet_list, ad);
	g_return_if_fail (list != NULL);

	finalpos = ad->pos + moveby;

	if (ad->pos < finalpos) {
		while (ad->pos < finalpos) {
			if ( ! push_applet_right (panel, list))
				break;
		}
                if (list->prev) {
			pad = list->prev->data;
			if (pad->expand_major)
				panel_widget_queue_applet_for_resize(pad);
		}
	} else {
                while (ad->pos > finalpos) {
			if ( ! push_applet_left (panel, list))
				break;
		}
	}
}


/*this is a special function and may fail if called improperly, it works
only under special circumstance when we know there is nothing from
old_size to panel->size*/
static void
panel_widget_right_stick(PanelWidget *panel,int old_size)
{
	int i,pos;
	GList *list,*prev;
	AppletData *ad;

	g_return_if_fail(PANEL_IS_WIDGET(panel));
	g_return_if_fail(old_size>=0);
	
	if(old_size>=panel->size ||
	   panel->packed)
	   	return;
	
	list = get_applet_list_pos(panel,old_size-1);

	if(!list)
		return;
	
	pos = panel->size-1;

	ad = list->data;
	do { 
		i = ad->pos;
		ad->pos = pos--;
		ad->cells = 1;
		prev = list;
		list = g_list_previous(list);
		if(!list)
			break;
		ad = list->data;
	} while(ad->pos+ad->cells == i);

	for(list = prev;list!=NULL;list=g_list_next(list))
		g_signal_emit (G_OBJECT(panel),
			       panel_widget_signals[APPLET_MOVE_SIGNAL],
			       0, ((AppletData *)list->data)->applet);
}

static void
panel_widget_size_request(GtkWidget *widget, GtkRequisition *requisition)
{
	PanelWidget *panel;
	GList *list;

	g_return_if_fail(PANEL_IS_WIDGET(widget));
	g_return_if_fail(requisition!=NULL);

	panel = PANEL_WIDGET(widget);

	if(panel->orient == GTK_ORIENTATION_HORIZONTAL) {
		requisition->width = 0;
		requisition->height = panel->sz;
	} else {
		requisition->height = 0;
		requisition->width = panel->sz;
	}

	for(list = panel->applet_list; list!=NULL; list = g_list_next(list)) {
		AppletData *ad = list->data;
		GtkRequisition chreq;
		gtk_widget_size_request(ad->applet,&chreq);
		if(panel->orient == GTK_ORIENTATION_HORIZONTAL) {
			if(requisition->height < chreq.height)
				requisition->height = chreq.height;
			if(panel->packed)
				requisition->width += chreq.width; 
		} else {
			if(requisition->width < chreq.width)
				requisition->width = chreq.width;
			if(panel->packed)
				requisition->height += chreq.height; 
		}
	}

	if(!panel->packed) {
		if(panel->orient == GTK_ORIENTATION_HORIZONTAL) {
			requisition->width = panel->size;
		} else {
			requisition->height = panel->size;
		}
	} 
	
	requisition->width = CLAMP (requisition->width, 12, gdk_screen_width ());
	requisition->height = CLAMP (requisition->height, 12, gdk_screen_height ());
}

static void
queue_resize_on_all_applets(PanelWidget *panel)
{
	GList *li;
	for(li = panel->applet_list; li != NULL;
	    li = g_list_next(li)) {
		AppletData *ad = li->data;
		gtk_widget_queue_resize (ad->applet);
	}
}

static void
setup_background(PanelWidget *panel, GdkPixbuf **pb, int *scale_w, int *scale_h,
		 gboolean *rotate)
{
	GtkWidget *widget = GTK_WIDGET(panel);
	GdkPixmap *bg_pixmap;
	
	*pb = NULL;
	*scale_w = *scale_h = 0;
	*rotate = FALSE;

	bg_pixmap = widget->style->bg_pixmap[GTK_WIDGET_STATE(widget)];

	if(panel->back_type == PANEL_BACK_NONE) {
		if(bg_pixmap && !panel->backpix) {
			int w, h;
			gdk_drawable_get_size (bg_pixmap, &w, &h);
			panel->backpix = gdk_pixbuf_get_from_drawable
				(NULL, bg_pixmap, widget->style->colormap,
				 0, 0, 0, 0, w, h);
		} else if(!bg_pixmap && panel->backpix) {
			g_object_unref (G_OBJECT (panel->backpix));
			panel->backpix = NULL;
		}
		*pb = panel->backpix;
	} else if(panel->back_type == PANEL_BACK_PIXMAP) {
		if(!panel->backpixmap)
			panel_resize_pixmap(panel);

		*pb = panel->backpix;

		if(panel->fit_pixmap_bg ||
		   panel->stretch_pixmap_bg) {
			*scale_w = panel->scale_w;
			*scale_h = panel->scale_h;
			if(panel->orient == GTK_ORIENTATION_VERTICAL &&
			   panel->rotate_pixmap_bg)
				*rotate = TRUE;
		} else {
			if(panel->orient == GTK_ORIENTATION_VERTICAL &&
			   panel->rotate_pixmap_bg) {
				/* we need to set scales to rotate*/
				*scale_w = gdk_pixbuf_get_width((*pb));
				*scale_h = gdk_pixbuf_get_height((*pb));
				*rotate = TRUE;
			}
		}
	} 
}

static int
panel_widget_expose(GtkWidget *widget, GdkEventExpose *event)
{
	PanelWidget *panel;
	GdkGC *gc;
	GdkPixbuf *pb = NULL;
	int scale_w = 0,scale_h = 0;
	int rotate = FALSE;

	g_return_val_if_fail(PANEL_IS_WIDGET(widget),FALSE);
	g_return_val_if_fail(event!=NULL,FALSE);

	panel = PANEL_WIDGET(widget);

#ifdef PANEL_WIDGET_DEBUG
	puts("PANEL_WIDGET_EXPOSE");
#endif

	if(!GTK_WIDGET_DRAWABLE(widget) ||
	   widget->allocation.width <= 0 ||
	   widget->allocation.height <= 0)
		return FALSE;

	setup_background (panel, &pb, &scale_w, &scale_h, &rotate);
	
	gc = gdk_gc_new (widget->window);
	gdk_gc_copy (gc, widget->style->bg_gc[GTK_WIDGET_STATE(widget)]);

	if (panel->back_type == PANEL_BACK_NONE) {
		if (widget->style->bg_pixmap[GTK_WIDGET_STATE(widget)]) {
			gdk_gc_set_fill (gc, GDK_TILED);
			gdk_gc_set_tile (gc,
					 widget->style->bg_pixmap[GTK_WIDGET_STATE(widget)]);
		}
	} else if (panel->back_type == PANEL_BACK_PIXMAP) {
		if(panel->backpixmap) {
			gdk_gc_set_fill (gc, GDK_TILED);
			gdk_gc_set_tile (gc, panel->backpixmap);
		}
	} else /*COLOR*/ {
		gdk_gc_set_foreground (gc, &panel->back_color);
	}

	gdk_draw_rectangle (widget->window, gc, TRUE,
			    event->area.x, event->area.y, event->area.width, event->area.height);

	g_object_unref (G_OBJECT (gc));

	/* Chain up to get no-window widget exposes */
	return (* GTK_WIDGET_CLASS (panel_widget_parent_class)->expose_event) (widget, event);
}

static void
panel_widget_size_allocate(GtkWidget *widget, GtkAllocation *allocation)
{
	PanelWidget *panel;
	GList *list;
	GSList *send_move = NULL;
	GSList *li;
	int i;
	int old_size;
	int old_thick;
	GtkAllocation old_alloc;

	g_return_if_fail(PANEL_IS_WIDGET(widget));
	g_return_if_fail(allocation!=NULL);
	
	panel = PANEL_WIDGET(widget);

#ifdef PANEL_WIDGET_DEBUG
	puts("PANEL_WIDGET_SIZE_ALLOCATE");
	printf("allocation %d x %d\n",
	       allocation->width,
	       allocation->height);
#endif

	/* allow drawing if it was inhibited */
	panel->inhibit_draw = FALSE;
	
	old_size = panel->size;
	old_thick = panel->thick;
	
	old_alloc = widget->allocation;

	widget->allocation = *allocation;
	if (GTK_WIDGET_REALIZED (widget))
		gdk_window_move_resize (widget->window,
					allocation->x, 
					allocation->y,
					allocation->width, 
					allocation->height);

	if(panel->orient == GTK_ORIENTATION_HORIZONTAL)
		panel->size = allocation->width;
	else
		panel->size = allocation->height;
	if(old_size<panel->size)
		panel_widget_right_stick(panel,old_size);

	if(panel->packed) {
		i = 0;
		for(list = panel->applet_list;
		    list!=NULL;
		    list = g_list_next(list)) {
			AppletData *ad = list->data;
			GtkAllocation challoc;
			GtkRequisition chreq;
			gtk_widget_get_child_requisition(ad->applet,&chreq);
			
			if(i != ad->pos) {
				ad->pos = i;
				send_move = g_slist_prepend(send_move,ad);
			}
			challoc.width = chreq.width;
			challoc.height = chreq.height;
			if(panel->orient == GTK_ORIENTATION_HORIZONTAL) {
				if (ad->expand_minor) {
					challoc.height = allocation->height;
				}
				ad->cells = chreq.width;
				challoc.x = ad->pos;
				challoc.y = (allocation->height - challoc.height) / 2;
			} else {
				if (ad->expand_minor) {
					challoc.width = allocation->width;
				}
				ad->cells = chreq.height;
				challoc.x = (allocation->width - challoc.width) / 2;
				challoc.y = ad->pos;
			}
			ad->min_cells  = ad->cells;
			ad->dirty = FALSE;
			gtk_widget_size_allocate(ad->applet,&challoc);
			i += ad->cells;
		}
	} else { /*not packed*/
		i = panel->size;
		for(list = g_list_last(panel->applet_list);
		    list!=NULL;
		    list = g_list_previous(list)) {
			AppletData *ad = list->data;
			GtkRequisition chreq;
			gtk_widget_get_child_requisition(ad->applet,&chreq);

			if(panel->orient == GTK_ORIENTATION_HORIZONTAL)
				ad->cells = chreq.width;
			else
				ad->cells = chreq.height;

			ad->min_cells = ad->cells;
			
			if (ad->expand_major) {
				ad->cells = (i - ad->pos) - 1;
			}

			if(ad->pos+ad->min_cells > i) {
				ad->pos = i - ad->min_cells;
				send_move = g_slist_prepend(send_move,ad);
			}
			i = ad->pos;
		}
		if(i < 0) {
			i = 0;
			for(list = panel->applet_list;
			    list!=NULL;
			    list = g_list_next(list)) {
				AppletData *ad = list->data;
				
				if(ad->pos < i) {
					ad->pos = i;
					if(!g_slist_find(send_move,ad))
						send_move = g_slist_prepend(send_move,ad);
				}
				
				i = ad->pos + ad->cells;
			}
		}

		for(list = panel->applet_list;
		    list!=NULL;
		    list = g_list_next(list)) {
			AppletData *ad = list->data;
			GtkAllocation challoc;
			GtkRequisition chreq;
			gtk_widget_get_child_requisition(ad->applet,&chreq);
			challoc.width = chreq.width;
			challoc.height = chreq.height;
			if(panel->orient == GTK_ORIENTATION_HORIZONTAL) {
				challoc.width = ad->cells;
				if (ad->expand_minor) {
					challoc.height = allocation->height;
				}
				challoc.x = ad->pos;
				challoc.y = (allocation->height - challoc.height) / 2;
			} else {
				challoc.height = ad->cells;
				if (ad->expand_minor) {
					challoc.width = allocation->width;
				}
				challoc.x = (allocation->width - challoc.width) / 2;
				challoc.y = ad->pos;
			}
			ad->dirty = FALSE;
			
			gtk_widget_size_allocate(ad->applet,&challoc);
		}
	}
	if(panel->orient == GTK_ORIENTATION_HORIZONTAL)
		panel->thick = allocation->height;
	else
		panel->thick = allocation->width;

	if(panel->back_type == PANEL_BACK_PIXMAP &&
	   (panel->fit_pixmap_bg || panel->stretch_pixmap_bg) &&
	   (old_alloc.width != allocation->width ||
	    old_alloc.height != allocation->height)) {
		if(panel->backpixmap) {
			g_object_unref (G_OBJECT (panel->backpixmap));
			panel->backpixmap = NULL;
		}
	}

	
	for(li=send_move;li!=NULL;li=g_slist_next(li)) {
		AppletData *ad = li->data;
		g_signal_emit (G_OBJECT(panel),
			       panel_widget_signals[APPLET_MOVE_SIGNAL],
			       0, ad->applet);
	}
	g_slist_free(send_move);
}

gboolean
panel_widget_is_cursor(PanelWidget *panel, int overlap)
{
	int x,y;
	int w,h;
	GtkWidget *widget;

	g_return_val_if_fail(PANEL_IS_WIDGET(panel),FALSE);

	widget = panel->drop_widget;
	
	if(!widget ||
	   !GTK_IS_WIDGET(widget) ||
	   !GTK_WIDGET_VISIBLE(widget))
		return FALSE;

	gtk_widget_get_pointer(widget, &x, &y);
	w = widget->allocation.width;
	h = widget->allocation.height;

	if((x+overlap)>=0 &&
	   (x-overlap)<=w &&
	   (y+overlap)>=0 &&
	   (y-overlap)<=h)
		return TRUE;
	return FALSE;
}

void
panel_widget_set_back_pixmap (PanelWidget *panel, const char *file)
{
	g_return_if_fail(PANEL_IS_WIDGET(panel));
	g_return_if_fail(file!=NULL);

#ifdef PANEL_WIDGET_DEBUG
	puts("PANEL_WIDGET_SET_BACK_PIXMAP");
#endif

	if (panel_try_to_set_pixmap (panel, file)) {
		g_free (panel->back_pixmap);
		panel->back_pixmap = g_strdup (file);
		panel->back_type = PANEL_BACK_PIXMAP;
		gtk_widget_queue_draw (GTK_WIDGET (panel));

		g_signal_emit (G_OBJECT(panel),
			       panel_widget_signals[BACK_CHANGE_SIGNAL],
			       0, panel->back_type,
				panel->back_pixmap,
				&panel->back_color);
	}
	else
		gtk_widget_queue_draw (GTK_WIDGET (panel));
}

static void
panel_try_to_set_back_color(PanelWidget *panel, GdkColor *color)
{
	GdkColormap *cmap;

	g_return_if_fail(panel!=NULL);
	g_return_if_fail(PANEL_IS_WIDGET(panel));
	g_return_if_fail(color!=NULL);
	
	cmap = gtk_widget_get_colormap(GTK_WIDGET(panel));

	if(gdk_colormap_alloc_color(cmap,color,FALSE,TRUE)) {
		panel->back_color = *color;
	}
}

void
panel_widget_set_back_color(PanelWidget *panel, GdkColor *color)
{
	g_return_if_fail(PANEL_IS_WIDGET(panel));
	g_return_if_fail(color!=NULL);

	panel->back_type = PANEL_BACK_COLOR;
	panel_try_to_set_back_color(panel, color);
	gtk_widget_queue_draw (GTK_WIDGET (panel));

	g_signal_emit(GTK_OBJECT(panel),
		      panel_widget_signals[BACK_CHANGE_SIGNAL],
		      0,
		      panel->back_type,
		      panel->back_pixmap,
		      &panel->back_color);
}

static GdkPixmap *
get_pixmap_from_pixbuf(GtkWidget *w, GdkPixbuf *pb, int scale_w, int scale_h,
		       int rotate)
{
	GdkGC *gc;
	GdkPixmap *p;
	gdouble affine[6];
	guchar *rgb;
	
	affine[1] = affine[2] = affine[4] = affine[5] = 0;

	affine[0] = scale_w / (double)(gdk_pixbuf_get_width(pb));
	affine[3] = scale_h / (double)(gdk_pixbuf_get_height(pb));

	if(rotate) {
		int tmp;
		gdouble aff[6];

		art_affine_rotate(aff,270);
		art_affine_multiply(affine,affine,aff);
		art_affine_translate(aff,0,scale_w);
		art_affine_multiply(affine,affine,aff);
		
		tmp = scale_h;
		scale_h = scale_w;
		scale_w = tmp;
	}
	
	rgb = g_new0(guchar,scale_h*scale_w*3);
#ifdef PANEL_WIDGET_DEBUG
	printf("scale_w %d scale_h %d\n",scale_w,scale_h);
#endif
	transform_pixbuf(rgb,
		         0,0,scale_w,scale_h,scale_w*3,
		         pb,affine,ART_FILTER_NEAREST,NULL);
	p = gdk_pixmap_new(w->window, scale_w,scale_h,
			   gtk_widget_get_visual(GTK_WIDGET(w))->depth);
	gc = gdk_gc_new(p);
	gdk_draw_rgb_image(p,gc,0,0,
			   scale_w,scale_h,
			   GDK_RGB_DITHER_NORMAL,
			   rgb, scale_w*3);
	g_free(rgb);
	g_object_unref (G_OBJECT (gc));

	return p;
}


static void
panel_resize_pixmap(PanelWidget *panel)
{
	int w, h;
	int pw, ph;

	g_return_if_fail(PANEL_IS_WIDGET(panel));

	if(panel->backpixmap)
		g_object_unref (G_OBJECT (panel->backpixmap));
	panel->backpixmap = NULL;
	
	if(!panel->backpix) return;
	
	panel->scale_w = w = gdk_pixbuf_get_width(panel->backpix);
	panel->scale_h = h = gdk_pixbuf_get_height(panel->backpix);
	
	pw = GTK_WIDGET(panel)->allocation.width;
	ph = GTK_WIDGET(panel)->allocation.height;

	if(panel->fit_pixmap_bg) {
		switch (panel->orient) {
		case GTK_ORIENTATION_HORIZONTAL:
			panel->scale_w = w * ph / h;
			panel->scale_h = ph;
			break;

		case GTK_ORIENTATION_VERTICAL:
			if(panel->rotate_pixmap_bg) {
				panel->scale_w = w * pw / h;
				panel->scale_h = pw;
			} else {
				panel->scale_w = pw;
				panel->scale_h = h * pw / w;
			}
			break;

		default:
			g_assert_not_reached ();
		}
	} else if(panel->stretch_pixmap_bg) {
		if(panel->orient == GTK_ORIENTATION_VERTICAL &&
		   panel->rotate_pixmap_bg) {
			panel->scale_w = ph;
			panel->scale_h = pw;
		} else {
			panel->scale_w = pw;
			panel->scale_h = ph;
		}
	}
	
	panel->backpixmap = get_pixmap_from_pixbuf(
					GTK_WIDGET(panel),
					panel->backpix,
					panel->scale_w,
					panel->scale_h,
					panel->orient == GTK_ORIENTATION_VERTICAL &&
					panel->rotate_pixmap_bg);
}

static int
panel_try_to_set_pixmap (PanelWidget *panel, const char *pixmap)
{
	g_return_val_if_fail(PANEL_IS_WIDGET(panel),FALSE);

	if(panel->backpix)
		g_object_unref (G_OBJECT (panel->backpix));
	panel->backpix = NULL;
	if (panel->backpixmap != NULL)
		g_object_unref (G_OBJECT (panel->backpixmap));
	panel->backpixmap = NULL;

	if (string_empty (pixmap)) {
		return TRUE;
	}

	if ( ! g_file_test (pixmap, G_FILE_TEST_EXISTS))
		return FALSE;
	
	panel->backpix = gdk_pixbuf_new_from_file (pixmap, NULL);
	if (panel->backpix == NULL)
		return FALSE;

	panel->scale_w = gdk_pixbuf_get_width (panel->backpix);
	panel->scale_h = gdk_pixbuf_get_height (panel->backpix);

	return TRUE;
}

static void
panel_widget_realize(GtkWidget *w)
{
	PanelWidget *panel;

	g_return_if_fail(PANEL_IS_WIDGET(w));

	panel = PANEL_WIDGET(w);

	if (GTK_WIDGET_CLASS (panel_widget_parent_class)->realize)
		(* GTK_WIDGET_CLASS (panel_widget_parent_class)->realize) (w);
	
	if(panel->back_type == PANEL_BACK_PIXMAP) {
		if (!panel_try_to_set_pixmap (panel, panel->back_pixmap))
			panel->back_type = PANEL_BACK_NONE;
	} else if(panel->back_type == PANEL_BACK_COLOR) {
		panel_try_to_set_back_color(panel, &panel->back_color);
	}

#ifdef PANEL_WIDGET_DEBUG
	puts("PANEL_WIDGET_REALIZE");
#endif
}

static void
panel_widget_destroy (GtkObject *obj)
{
	PanelWidget *panel;
	GtkWidget *w = GTK_WIDGET (obj);

	g_return_if_fail(PANEL_IS_WIDGET(w));

	panel = PANEL_WIDGET(w);

	if(panel->backpix)
		g_object_unref (G_OBJECT (panel->backpix));
	panel->backpix = NULL;

	g_free (panel->back_pixmap);
	panel->back_pixmap = NULL;

	/*remove from panels list*/
	panels = g_slist_remove(panels,w);
	panel_session_remove_panel_from_config (panel);

	if (GTK_OBJECT_CLASS (panel_widget_parent_class)->destroy)
		GTK_OBJECT_CLASS (panel_widget_parent_class)->destroy (obj);
}

static void
panel_widget_style_set(GtkWidget *widget,
		       GtkStyle  *previous_style)
{
	PanelWidget *panel = PANEL_WIDGET (widget);
	if (panel->back_type == PANEL_BACK_NONE) {
		if(panel->backpix)
			g_object_unref (G_OBJECT (panel->backpix));
		panel->backpix = NULL;
	}

	if (GTK_WIDGET_CLASS (panel_widget_parent_class)->style_set)
		(* GTK_WIDGET_CLASS (panel_widget_parent_class)->style_set) (widget, previous_style);

}

static gchar * 
generate_unique_id (void)
{
	gint32 id;
	GTimeVal tv;
	static int incr = -1;
	gchar *retval = NULL;

	if (incr == -1)
		incr = (rand () >> 3) & 0xFF;

	id = (rand () >> 3) && 0xFFF;
	id += (incr << 12);

	g_get_current_time (&tv);
	id += (tv.tv_usec & 0x7FF) << 20;

	incr ++;

	retval = g_strdup_printf ("%u", id);	
#ifdef PANEL_WIDGET_DEBUG
	printf ("generating %s\n", retval);
#endif
	if (panel_widget_get_by_id (retval) != NULL) {
		g_free (retval);
		retval = generate_unique_id ();
	}

	return retval;
}

PanelWidget *
panel_widget_get_by_id (gchar *id)
{
	GSList *li;

	g_return_val_if_fail (id != NULL, NULL);

#ifdef PANEL_WIDGET_DEBUG
	printf ("Call panel_widget_get_by_id (%s) \n", id);
#endif
	for (li = panels; li != NULL; li = li->next) {
		PanelWidget *panel = li->data;

#ifdef PANEL_WIDGET_DEBUG
		printf ("Testing panel id %s\n", panel->unique_id);
#endif
		if (panel->unique_id != NULL) {
			if (strcmp (panel->unique_id, id) == 0) {

#ifdef PANEL_WIDGET_DEBUG
	printf ("We have found our panel!\n");
#endif
				return panel;
			}
		}
	}
	return NULL;
}

void
panel_widget_set_id (PanelWidget *panel, const char *id)
{
	g_return_if_fail (id != NULL);
	if (panel->unique_id != NULL)
		g_free (panel->unique_id);
	panel->unique_id = g_strdup (id);
#ifdef PANEL_WIDGET_DEBUG
	printf ("panel_widget_set_id : TO %s\n", panel->unique_id);
#endif
	return;
}

void
panel_widget_set_new_id (PanelWidget *panel) 
{
	if (panel->unique_id)
		g_free (panel->unique_id);

	panel->unique_id = generate_unique_id ();
}

static void
panel_widget_instance_init (PanelWidget *panel)
{
	g_return_if_fail(PANEL_IS_WIDGET(panel));

	panel->unique_id = NULL;

	/*this makes the popup "pop down" once the button is released*/
	gtk_widget_set_events(GTK_WIDGET(panel),
			      gtk_widget_get_events(GTK_WIDGET(panel)) |
			      GDK_BUTTON_RELEASE_MASK);
	
	panel->back_type =PANEL_BACK_NONE;
	panel->fit_pixmap_bg = FALSE;
	panel->stretch_pixmap_bg = FALSE;
	panel->back_pixmap = NULL;
	panel->back_color.red = 0;
	panel->back_color.green = 0;
	panel->back_color.blue = 0;
	panel->back_color.pixel = 1;
	panel->packed = FALSE;
	panel->orient = GTK_ORIENTATION_HORIZONTAL;
	panel->thick = PANEL_MINIMUM_WIDTH;
	panel->size = G_MAXINT;
	panel->applet_list = NULL;
	panel->master_widget = NULL;
	panel->drop_widget = GTK_WIDGET(panel);
	panel->backpix = NULL;
	panel->backpixmap = NULL;
	panel->inhibit_draw = FALSE;

	panels = g_slist_append(panels,panel);
}

GtkWidget *
panel_widget_new (gchar *panel_id,
		  gboolean packed,
		  GtkOrientation orient,
		  int sz,
		  PanelBackType back_type,
		  char *back_pixmap,
		  gboolean fit_pixmap_bg,
		  gboolean stretch_pixmap_bg,
		  gboolean rotate_pixmap_bg,
		  GdkColor *back_color)
{
	PanelWidget *panel;

	panel = g_object_new (PANEL_TYPE_WIDGET, NULL);
        GTK_WIDGET_UNSET_FLAGS (panel, GTK_NO_WINDOW);
        GTK_WIDGET_SET_FLAGS (panel, GTK_CAN_FOCUS);

	if (!panel_id)
		panel->unique_id = generate_unique_id ();

	else {
		g_free (panel->unique_id);
		panel->unique_id = g_strdup (panel_id);
	} 

	panel->back_type = back_type;

	panel->fit_pixmap_bg = fit_pixmap_bg;
	panel->stretch_pixmap_bg = stretch_pixmap_bg;
	panel->rotate_pixmap_bg = rotate_pixmap_bg;
	panel->back_pixmap = g_strdup (sure_string (back_pixmap));
	
	if(back_color)
		panel->back_color = *back_color;
	else {
		panel->back_color.red = 0;
		panel->back_color.green = 0;
		panel->back_color.blue = 0;
		panel->back_color.pixel = 1;
	}	

	panel->orient = orient;
	panel->sz = sz;

#ifdef PANEL_WIDGET_DEBUG
	printf("GOT SIZE OF %d\n",sz);
#endif

	panel->packed = packed;
	if(packed)
		panel->size = 0;
	else
		panel->size = G_MAXINT;
	
	return GTK_WIDGET(panel);
}

static guint moving_timeout = 0;
static gboolean been_moved = FALSE;
static gboolean repeat_if_outside = FALSE;

void
panel_widget_applet_drag_start_no_grab (PanelWidget *panel,
					GtkWidget *applet,
					int drag_off)
{
	AppletData *ad;

	g_return_if_fail (PANEL_IS_WIDGET (panel));
	g_return_if_fail (GTK_IS_WIDGET (panel));

	ad = g_object_get_data (G_OBJECT (applet), PANEL_APPLET_DATA);
	g_return_if_fail (ad != NULL);

	if (moving_timeout != 0) {
		gtk_timeout_remove (moving_timeout);
		moving_timeout = 0;
		been_moved = FALSE;
	}

#ifdef PANEL_WIDGET_DEBUG
	g_message("Starting drag on a %s at %p\n",
		  g_type_name(G_TYPE_FROM_INSTANCE (applet)), applet);
#endif
	panel->currently_dragged_applet = ad;
	if (drag_off == PW_DRAG_OFF_CURSOR)
		ad->drag_off = panel_widget_get_cursorloc (panel) - ad->pos;
	else if (drag_off == PW_DRAG_OFF_CENTER)
		ad->drag_off = ad->cells / 2;
	else
		ad->drag_off = drag_off;

	add_all_move_bindings (panel);

	panel_applet_in_drag = TRUE;
}


void
panel_widget_applet_drag_end_no_grab (PanelWidget *panel)
{
	g_return_if_fail (panel != NULL);
	g_return_if_fail (PANEL_IS_WIDGET (panel));

#ifdef PANEL_WIDGET_DEBUG
	g_message("Ending drag\n");
#endif
	panel->currently_dragged_applet = NULL;
	panel_applet_in_drag = FALSE;

	remove_all_move_bindings (panel);
	if (moving_timeout != 0) {
		gtk_timeout_remove (moving_timeout);
		moving_timeout = 0;
		been_moved = FALSE;
	}
}

void
panel_widget_applet_drag_start (PanelWidget *panel,
				GtkWidget *applet,
				int drag_off)
{
	g_return_if_fail (PANEL_IS_WIDGET (panel));
	g_return_if_fail (GTK_IS_WIDGET (applet));

#ifdef PANEL_WIDGET_DEBUG
	g_message("Starting drag [grabbed] on a %s at %p\n",
		  g_type_name(G_TYPE_FROM_INSTANCE(applet)), applet);
#endif
	panel_widget_applet_drag_start_no_grab (panel, applet, drag_off);

	gtk_grab_add (applet);
	if (applet->window != NULL) {
		GdkCursor *fleur_cursor = gdk_cursor_new (GDK_FLEUR);
		gdk_pointer_grab (applet->window,
				  FALSE,
				  APPLET_EVENT_MASK,
				  NULL,
				  fleur_cursor,
				  GDK_CURRENT_TIME);
		gdk_cursor_unref (fleur_cursor);
		gdk_flush ();
	}
}

void
panel_widget_applet_drag_end (PanelWidget *panel)
{
	g_return_if_fail (PANEL_IS_WIDGET (panel));

	if (panel->currently_dragged_applet == NULL)
		return;
	gdk_pointer_ungrab (GDK_CURRENT_TIME);
	gtk_grab_remove (panel->currently_dragged_applet->applet);
	panel_widget_applet_drag_end_no_grab (panel);
	gdk_flush ();
}

/*get pos of the cursor location*/
int
panel_widget_get_cursorloc (PanelWidget *panel)
{
	int x, y;

	g_return_val_if_fail (PANEL_IS_WIDGET (panel), -1);

	gtk_widget_get_pointer (GTK_WIDGET (panel), &x, &y);

	if (panel->orient == GTK_ORIENTATION_HORIZONTAL) {
		return x;
	} else {
		return y;
	}
}

#ifdef FIXME /* This isn't used anymore but looks useful */

/*get amount of free space around the applet (including the applet size),
  or return 0 on error or if the panel is packed*/
int
panel_widget_get_free_space(PanelWidget *panel, GtkWidget *applet)
{
	int right,left;
	GList *li;
	AppletData *ad;

	g_return_val_if_fail(PANEL_IS_WIDGET(panel), 0);
	g_return_val_if_fail(GTK_IS_WIDGET(applet), 0);
	
	/*this function doesn't make sense on packed panels*/
	if(panel->packed)
		return 0;
	
	right = panel->size;
	left = 0;
	
	for(li = panel->applet_list; li; li = g_list_next(li)) {
		ad = li->data;
		if(ad->applet == applet)
			break;
	}
	/*the applet is not on this panel*/
	if(!li)
		return 0;
	
	if(li->prev) {
		AppletData *pad = li->prev->data;
		left = pad->pos+pad->cells;
	}
	if(li->next) {
		AppletData *nad = li->next->data;
		right = nad->pos;
	}
	
	return right - left;
}
#endif

/*calculates the value to move the applet by*/
static int
panel_widget_get_moveby (PanelWidget *panel, int pos, int offset)
{
	g_return_val_if_fail (PANEL_IS_WIDGET (panel), -1);

	return panel_widget_get_cursorloc (panel) - offset - pos;
}

static GList *
walk_up_to (int pos, GList *list)
{
	AppletData *ad;

	g_return_val_if_fail (list != NULL, NULL);

	ad = list->data;

	if (ad->pos <= pos &&
	    ad->pos + ad->cells > pos)
		return list;
	while (list->next != NULL &&
	       ad->pos + ad->cells <= pos) {
		list = list->next;
		ad = list->data;
	}
	while (list->prev != NULL &&
	       ad->pos > pos) {
		list = list->prev;
		ad = list->data;
	}
	return list;
}

static GtkWidget *
is_in_applet (int pos, AppletData *ad)
{
	g_return_val_if_fail (ad != NULL, NULL);

	if (ad->pos <= pos &&
	    ad->pos + ad->min_cells > pos)
		return ad->applet;
	return NULL;
}

static int
panel_widget_get_free_spot (PanelWidget *panel, AppletData *ad, int place)
{
	int i, e;
	int start;
	int right = -1, left = -1;
	GList *list;

	g_return_val_if_fail (PANEL_IS_WIDGET (panel), -1);
	g_return_val_if_fail (ad != NULL, -1);

	if (ad->pos >= panel->size)
		return -1;

	if (panel->applet_list == NULL) {
		if (place + ad->min_cells > panel->size)
			return panel->size-ad->min_cells;
		else
			return place;
	}

	list = panel->applet_list;

	start = place - ad->drag_off;
	if (start < 0)
		start = 0;
	for (e = 0, i = start; i < panel->size; i++) {
		GtkWidget *applet;
		list = walk_up_to (i, list);
		applet = is_in_applet (i, list->data);
		if (applet == NULL ||
		    applet == ad->applet) {
			e++;
			if (e >= ad->min_cells) {
				right = i - e + 1;
				break;
			}
		} else {
			e = 0;
		}
	}

	start = place + ad->drag_off;
	if (start >= panel->size)
		start = panel->size - 1;
	for (e = 0, i = start; i >= 0; i--) {
		GtkWidget *applet;
		list = walk_up_to (i, list);
		applet = is_in_applet (i, list->data);
		if (applet == NULL ||
		    applet == ad->applet) {
			e++;
			if (e >= ad->min_cells) {
				left = i;
				break;
			}
		} else {
			e=0;
		}
	}

	start = place - ad->drag_off;

	if (left == -1) {
		if (right == -1)
			return -1;
		else
			return right;
	} else {
		if (right == -1)
			return left;
		else
			return abs (left - start) > abs (right - start) ?
				right : left;
	}
}

/*to call this function we MUST know that there is at least
  ad->cells free at pos otherwise we will mess up the panel*/
static void
panel_widget_nice_move (PanelWidget *panel, AppletData *ad, int pos)
{
	AppletData *pad1 = NULL, *pad2 = NULL;
	GList *l;
	
	g_return_if_fail (PANEL_IS_WIDGET (panel));
	g_return_if_fail (ad != NULL);

	if (pos < 0 ||
	    pos == ad->pos)
		return;

	l = g_list_find (panel->applet_list, ad);
	if (l && l->prev) {
		pad1 = l->prev->data;
	}

	ad->pos = pos;

	panel->applet_list =
		panel_g_list_resort_item (panel->applet_list, ad,
					  (GCompareFunc)applet_data_compare);

	l = g_list_find (panel->applet_list, ad);
	if (l && l->prev) {
		pad2 = l->prev->data;
	}

	panel_widget_queue_applet_for_resize (ad);

	if (pad1 && pad1->expand_major) {
		panel_widget_queue_applet_for_resize (pad1);
	}
	
	if (pad2 && pad2 != pad1 && pad2->expand_major) {
		panel_widget_queue_applet_for_resize (pad2);
	}
}

/* schedule to run the below function */
static void schedule_try_move (PanelWidget *panel, gboolean repeater);

/*find the cursor position and move the applet to that position*/
static void
panel_widget_applet_move_to_cursor (PanelWidget *panel)
{
	int moveby;
	int pos;
	int movement;
	GtkWidget *applet;
	GSList *forb;
	GdkModifierType mods;
	AppletData *ad;

	g_return_if_fail(PANEL_IS_WIDGET(panel));

	if (panel->currently_dragged_applet == NULL)
		return;

	ad = panel->currently_dragged_applet;

	pos = ad->pos;

	applet = ad->applet;
	g_assert(GTK_IS_WIDGET(applet));
	forb = g_object_get_data (G_OBJECT(applet),
				  PANEL_APPLET_FORBIDDEN_PANELS);

	if(!panel_widget_is_cursor(panel,10)) {
		GSList *list;

		for(list=panels;
		    list!=NULL;
		    list=g_slist_next(list)) {
			PanelWidget *new_panel =
				PANEL_WIDGET(list->data);

			if(panel != new_panel &&
			   panel_widget_is_cursor(new_panel,10) &&
			   (!g_slist_find(forb,new_panel))) {
				pos = panel_widget_get_moveby (new_panel, 0,
							       ad->drag_off);
				if (pos < 0)
					pos = 0;
				panel_widget_applet_drag_end (panel);
				/*disable reentrancy into this
				  function*/
				if (panel_widget_reparent (panel,
							   new_panel,
							   applet,
							   pos) == -1) {
					panel_widget_applet_drag_start
						(panel, applet, ad->drag_off);
					/*can't find a free pos
					  so cancel the reparent*/
					continue;
				}
				panel_widget_applet_drag_start (new_panel,
								applet,
								ad->drag_off);
				/* schedule a move, the thing might have
				 * gone outside the cursor, thus we need to
				 * schedule a move */
				schedule_try_move(new_panel, TRUE);
				return;
			}
		}
	}

	gdk_window_get_pointer(GTK_WIDGET(panel)->window,
			       NULL,NULL,&mods);

	movement = PANEL_SWITCH_MOVE;

	if (panel->packed) {
		movement = PANEL_SWITCH_MOVE;
	} else {
		if (mods & GDK_CONTROL_MASK)
			movement = PANEL_SWITCH_MOVE;
		else if (mods & GDK_SHIFT_MASK)
			movement = PANEL_PUSH_MOVE;
		else if (mods & GDK_MOD1_MASK)
			movement = PANEL_FREE_MOVE;
	}
	
	switch(movement) {
	case PANEL_SWITCH_MOVE:
		moveby = panel_widget_get_moveby (panel, pos, ad->drag_off);
		panel_widget_switch_move (panel, ad, moveby);
		break;
	case PANEL_FREE_MOVE:
		pos = panel_widget_get_free_spot (panel, ad,
					panel_widget_get_cursorloc (panel));
		panel_widget_nice_move (panel, ad, pos);
		break;
	case PANEL_PUSH_MOVE:
		moveby = panel_widget_get_moveby (panel, pos, ad->drag_off);
		panel_widget_push_move (panel, ad, moveby);
		break;
	}
}

static int
move_timeout_handler(gpointer data)
{
	PanelWidget *panel = data;
	g_return_val_if_fail(PANEL_IS_WIDGET(data),FALSE);

	if(been_moved &&
	   panel->currently_dragged_applet) {
		panel_widget_applet_move_to_cursor(panel);
		been_moved = FALSE;
		return TRUE;
	}
	been_moved = FALSE;

	if(panel->currently_dragged_applet && repeat_if_outside) {
		int x,y;
		int w,h;
		GtkWidget *widget;

		widget = panel->currently_dragged_applet->applet;

		gtk_widget_get_pointer(widget, &x, &y);
		w = widget->allocation.width;
		h = widget->allocation.height;

		/* if NOT inside return TRUE, this means we will be
		 * kept inside the timeout until we hit the damn widget
		 * or the drag ends */
		if(!(x>=0 && x<=w && y>=0 && y<=h))
			return TRUE;
	}

	moving_timeout = 0;
	
	return FALSE;
}

static void
schedule_try_move(PanelWidget *panel, gboolean repeater)
{
	if (!panel->currently_dragged_applet)
		return;
	repeat_if_outside = repeater;
	if(moving_timeout == 0) {
		been_moved = FALSE;
		panel_widget_applet_move_to_cursor(panel);
		moving_timeout =
			gtk_timeout_add (50, move_timeout_handler, panel);
	} else
		been_moved = TRUE;
}

static gboolean
panel_widget_applet_event(GtkWidget *widget, GdkEvent *event)
{
	PanelWidget *panel;
	GdkEventButton *bevent;

	g_return_val_if_fail(GTK_IS_WIDGET(widget),FALSE);
	g_return_val_if_fail(PANEL_IS_WIDGET(widget->parent),FALSE);
	g_return_val_if_fail(event!=NULL,FALSE);

	panel = PANEL_WIDGET(widget->parent);

	switch (event->type) {
		case GDK_BUTTON_PRESS:
			bevent = (GdkEventButton *) event;
#ifdef PANEL_WIDGET_DEBUG
			printf("the appwidget %lX\n",(long)widget);
#endif

			/* don't propagate this event */
			if (panel->currently_dragged_applet) {
				g_signal_stop_emission 
					(G_OBJECT (widget), 
					 g_signal_lookup ("event", G_OBJECT_TYPE (widget)),
					 0);
				return TRUE;
			}

			if ( ! commie_mode &&
			    bevent->button == 2) {
				/* Start drag */
				panel_widget_applet_drag_start
					(panel, widget, PW_DRAG_OFF_CURSOR);
				return TRUE;
			}

			break;

		case GDK_BUTTON_RELEASE:
			if (panel->currently_dragged_applet) {
				g_signal_stop_emission
					(G_OBJECT (widget),
					g_signal_lookup ("event", G_OBJECT_TYPE (widget)),
					0);
				panel_widget_applet_drag_end(panel);
				return TRUE;
			}

			break;
		case GDK_MOTION_NOTIFY:
			schedule_try_move(panel, FALSE);
			break;
		case GDK_KEY_PRESS:
			if (panel_applet_in_drag) {
				return gtk_bindings_activate (GTK_OBJECT (panel),
					((GdkEventKey *)event)->keyval, 
					((GdkEventKey *)event)->state);	
			}
			break;
		default:
			break;
	}

	return FALSE;
}

static int
panel_sub_event_handler(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	g_return_val_if_fail(GTK_IS_WIDGET(widget),FALSE);
	g_return_val_if_fail(event!=NULL,FALSE);

	switch (event->type) {
		GdkEventButton *bevent;
		/*pass these to the parent!*/
		case GDK_BUTTON_PRESS:
		case GDK_BUTTON_RELEASE:
		case GDK_MOTION_NOTIFY:
			bevent = (GdkEventButton *)event;
			if(bevent->button != 1 || panel_applet_in_drag)
				return gtk_widget_event(data, event);

			break;
		case GDK_KEY_PRESS:
			if (panel_applet_in_drag)
				return gtk_widget_event(data, event);
			break;
		default:
			break;
	}

	return FALSE;
}


static void
bind_applet_events(GtkWidget *widget, gpointer data)
{
	g_return_if_fail(GTK_IS_WIDGET(widget));

	/* XXX: This is more or less a hack.  We need to be able to
	 * capture events over applets so that we can drag them with
	 * the mouse and such.  So we need to force the applet's
	 * widgets to recursively send the events back to their parent
	 * until the event gets to the applet wrapper (the
	 * GtkEventBox) for processing by us.
	 */
	
	if (!GTK_WIDGET_NO_WINDOW(widget))
		g_signal_connect (G_OBJECT(widget), "event",
				  G_CALLBACK (panel_sub_event_handler),
				  data);
	
	if (GTK_IS_CONTAINER(widget))
		gtk_container_foreach (GTK_CONTAINER (widget),
				       bind_applet_events, data);
}

static void
panel_widget_applet_destroy (GtkWidget *applet, gpointer data)
{
	AppletData *ad;

	g_return_if_fail (GTK_IS_WIDGET (applet));

	ad = g_object_get_data (G_OBJECT (applet), PANEL_APPLET_DATA);

	/*if it wasn't yet removed*/
	if(applet->parent) {
		PanelWidget *panel = PANEL_WIDGET (applet->parent);

		g_signal_emit (G_OBJECT (panel),
			       panel_widget_signals[APPLET_ABOUT_TO_DIE_SIGNAL],
			       0, applet);

		if (panel->currently_dragged_applet == ad)
			panel_widget_applet_drag_end (panel);

		panel->applet_list = g_list_remove (panel->applet_list,ad);
	}

	g_free (ad);
}


static void
bind_top_applet_events(GtkWidget *widget, gboolean bind_lower)
{
	g_return_if_fail(GTK_IS_WIDGET(widget));

	g_signal_connect (G_OBJECT(widget), "destroy",
			  G_CALLBACK (panel_widget_applet_destroy),
			  NULL);

	g_signal_connect (G_OBJECT(widget),
			  "event",
			  G_CALLBACK (panel_widget_applet_event),
			  NULL);

	/* XXX: This is more or less a hack.  We need to be able to
	 * capture events over applets so that we can drag them with
	 * the mouse and such.  So we need to force the applet's
	 * widgets to recursively send the events back to their parent
	 * until the event gets to the applet wrapper (the
	 * GtkEventBox) for processing by us.
	 */

	if (bind_lower && GTK_IS_CONTAINER(widget))
		gtk_container_foreach (GTK_CONTAINER (widget),
				       bind_applet_events, widget);
}

static int
panel_widget_find_empty_pos(PanelWidget *panel, int pos)
{
	int i;
	int right=-1,left=-1;
	GList *list;

	g_return_val_if_fail(PANEL_IS_WIDGET(panel),-1);
	g_return_val_if_fail(pos>=0,-1);

	if(pos>=panel->size)
		pos = panel->size-1;

	if(!panel->applet_list)
		return pos;

	list = panel->applet_list;

	for (i = pos; i < panel->size; i++) {
		list = walk_up_to (i, list);
		if ( ! is_in_applet (i, list->data)) {
			right = i;
			break;
		}
	}

	for(i = pos; i >= 0; i--) {
		list = walk_up_to (i, list);
		if ( ! is_in_applet (i, list->data)) {
			left = i;
			break;
		}
	}

	if (left == -1) {
		if (right == -1)
			return -1;
		else
			return right;
	} else {
		if (right == -1)
			return left;
		else
			return abs (left - pos) > abs (right - pos) ?
				right : left;
	}
}

void
panel_widget_add_forbidden (PanelWidget *panel)
{
	g_return_if_fail (panel != NULL);
	g_return_if_fail (PANEL_IS_WIDGET (panel));

	add_panel_to_forbidden (panel, panel);
}

int
panel_widget_add_full (PanelWidget *panel, GtkWidget *applet, int pos,
		       gboolean bind_lower_events, gboolean insert_at_pos,
		       gboolean expand_major, gboolean expand_minor)
{
	AppletData *ad = NULL;

	g_return_val_if_fail (PANEL_IS_WIDGET (panel), -1);
	g_return_val_if_fail (GTK_IS_WIDGET (applet), -1);
	g_return_val_if_fail (pos >= 0, -1);

	ad = g_object_get_data (G_OBJECT (applet), PANEL_APPLET_DATA);

	if (ad != NULL)
		pos = ad->pos;

	if (pos < 0)
		pos = 0;
	
	if ( ! insert_at_pos) {
		if (panel->packed) {
			if (get_applet_list_pos (panel, pos)) 
				/*this is a slight hack so that this applet
				  is inserted AFTER an applet with this pos
				  number*/
				pos++;
		} else {
			int newpos = panel_widget_find_empty_pos (panel, pos);
			if (newpos >= 0)
				pos = newpos;
			else if (get_applet_list_pos (panel, pos)) 
				/*this is a slight hack so that this applet
				  is inserted AFTER an applet with this pos
				  number*/
				pos++;
		}
	}

	if(pos==-1) return -1;

	if (ad == NULL) {
		ad = g_new (AppletData, 1);
		ad->applet = applet;
		ad->cells = 1;
		ad->min_cells = 1;
		ad->pos = pos;
		ad->drag_off = 0;
		ad->dirty = FALSE;
		ad->no_die = 0;
		ad->expand_major = expand_major;
		ad->expand_minor = expand_minor;
		g_object_set_data (G_OBJECT (applet),
				   PANEL_APPLET_DATA, ad);
		
		/*this is a completely new applet, which was not yet bound*/
		bind_top_applet_events (applet, bind_lower_events);
	}

	panel->applet_list =
		g_list_insert_sorted(panel->applet_list,ad,
				     (GCompareFunc)applet_data_compare);

	/*this will get done right on size allocate!*/
	if(panel->orient == GTK_ORIENTATION_HORIZONTAL)
		gtk_fixed_put(GTK_FIXED(panel),applet,
			      pos,0);
	else
		gtk_fixed_put(GTK_FIXED(panel),applet,
			      0,pos);


	gtk_widget_queue_resize(GTK_WIDGET(panel));

	g_signal_emit (G_OBJECT(panel),
		       panel_widget_signals[APPLET_ADDED_SIGNAL],
		       0, applet);
	
	/*NOTE: forbidden list is not updated on addition, use the
	function above for the panel*/

	return pos;
}

gboolean
panel_widget_reparent (PanelWidget *old_panel,
		       PanelWidget *new_panel,
		       GtkWidget *applet,
		       int pos)
{
	AppletData *ad;

	g_return_val_if_fail(PANEL_IS_WIDGET(old_panel),-1);
	g_return_val_if_fail(PANEL_IS_WIDGET(new_panel),-1);
	g_return_val_if_fail(GTK_IS_WIDGET(applet),-1);
	g_return_val_if_fail(pos>=0,-1);

	ad = g_object_get_data (G_OBJECT (applet), PANEL_APPLET_DATA);
	g_return_val_if_fail(ad!=NULL,-1);
	
	/*we'll resize both panels anyway*/
	ad->dirty = FALSE;
	
	ad->pos = pos;

	ad->no_die++;

	gtk_widget_reparent (applet, GTK_WIDGET (new_panel));

	gdk_flush();

	ad->no_die--;

	return pos;
}

#ifdef FIXME /* This isn't used anymore but looks useful */
int
panel_widget_move (PanelWidget *panel, GtkWidget *applet, int pos)
{
	AppletData *ad;

	g_return_val_if_fail(PANEL_IS_WIDGET(panel),-1);
	g_return_val_if_fail(GTK_IS_WIDGET(applet),-1);
	g_return_val_if_fail(pos>=0,-1);

	ad = g_object_get_data (G_OBJECT (applet), PANEL_APPLET_DATA);
	ad->pos = -1;
	panel->applet_list = g_list_remove (panel->applet_list, ad);
	
	if(pos < 0)
		pos = 0;

	if(get_applet_list_pos(panel,pos)) 
		/*this is a slight hack so that this applet is
		  inserted AFTER an applet with this pos number*/
		pos++;
	if(pos==-1) return -1;

	ad->pos = pos;
	/*reset size to 1*/
	ad->cells = 1;
	ad->min_cells = 1;
	panel->applet_list =
		g_list_insert_sorted(panel->applet_list,ad,
				     (GCompareFunc)applet_data_compare);

	g_signal_emit (G_OBJECT(panel),
		       panel_widget_signals[APPLET_MOVE_SIGNAL],
		       0, ad->applet);
	gtk_widget_queue_resize(GTK_WIDGET(panel));

	return pos;
}
#endif

void
panel_widget_change_params(PanelWidget *panel,
			   GtkOrientation orient,
			   int sz,
			   PanelBackType back_type,
			   char *pixmap,
			   gboolean fit_pixmap_bg,
			   gboolean stretch_pixmap_bg,
			   gboolean rotate_pixmap_bg,
			   GdkColor *back_color)
{
	GtkOrientation oldorient;
	int oldsz;
	gboolean change_back = FALSE;

	g_return_if_fail(PANEL_IS_WIDGET(panel));
	g_return_if_fail(GTK_WIDGET_REALIZED(panel));

	oldorient = panel->orient;
	panel->orient = orient;

	oldsz = panel->sz;
	panel->sz = sz;
	
#ifdef PANEL_WIDGET_DEBUG
	printf("GOT SIZE OF %d\n",sz);
#endif

	queue_resize_on_all_applets(panel);

	if(oldorient != panel->orient) {
	   	g_signal_emit (G_OBJECT(panel),
	   		       panel_widget_signals[ORIENT_CHANGE_SIGNAL],
	   		       0, panel->orient);
	}
	if(oldsz != panel->sz) {
	   	g_signal_emit (G_OBJECT(panel),
	   		       panel_widget_signals[SIZE_CHANGE_SIGNAL],
	   		       0, panel->sz);
	}
	if(back_color) {
		/*this will allways trigger, but so what*/
		if(back_type == PANEL_BACK_COLOR)
			change_back = TRUE;
		panel->back_color = *back_color;
	} /*if we didn't pass a color, then don't set a new color!*/

	/*only change the pixmap name if we passed a non-null value*/
	if(pixmap && pixmap != panel->back_pixmap) {
		if(back_type == PANEL_BACK_PIXMAP)
			change_back = TRUE;

		g_free (panel->back_pixmap);
		panel->back_pixmap = g_strdup (pixmap);
	}

	/*clearly a signal should be sent*/
	if(panel->back_type != back_type ||
	   panel->fit_pixmap_bg != fit_pixmap_bg ||
	   panel->stretch_pixmap_bg != stretch_pixmap_bg ||
	   panel->rotate_pixmap_bg != rotate_pixmap_bg)
		change_back = TRUE;
	
	/*this bit is not optimal, it allways sets the pixmap etc etc ...
	  but this function isn't called too often*/
	panel->back_type = back_type;
	panel->fit_pixmap_bg = fit_pixmap_bg;
	panel->stretch_pixmap_bg = stretch_pixmap_bg;
	panel->rotate_pixmap_bg = rotate_pixmap_bg;
	if(back_type == PANEL_BACK_PIXMAP) {
		panel_try_to_set_pixmap (panel, panel->back_pixmap);
		/* we will queue resize and redraw anyhow */
	} else if(back_type == PANEL_BACK_COLOR) {
		panel_try_to_set_back_color(panel, &panel->back_color);
	}

	/* let the applets know we changed the background */
	if (change_back) {
		g_signal_emit (G_OBJECT (panel),
			       panel_widget_signals[BACK_CHANGE_SIGNAL],
			       0, 	
			       panel->back_type,
			       panel->back_pixmap,
			       &panel->back_color);
	}

	/* inhibit draws until we resize */
	panel->inhibit_draw = TRUE;
	gtk_widget_queue_resize (GTK_WIDGET (panel));
}

static void 
panel_widget_push_move_applet (PanelWidget     *panel,
                               GtkDirectionType dir)
{
	AppletData       *ad;
	gint increment = MOVE_INCREMENT;

	ad = panel->currently_dragged_applet;

	g_return_if_fail (ad);

	switch (dir) {
	case GTK_DIR_LEFT:
	case GTK_DIR_UP:
		increment = -increment;
		break;
	case GTK_DIR_RIGHT:
	case GTK_DIR_DOWN:
		break;
	default:
		return;
	}
	panel_widget_push_move (panel, ad, increment);
}

static void 
panel_widget_switch_move_applet (PanelWidget      *panel,
                                 GtkDirectionType  dir)
{
	AppletData       *ad;
	AppletData *nad = NULL;
	GList *list;
	gint increment;

	ad = panel->currently_dragged_applet;

	g_return_if_fail (ad);

	list = g_list_find (panel->applet_list, ad);
	g_return_if_fail (list);
	switch (dir) {
	case GTK_DIR_LEFT:
	case GTK_DIR_UP:
		if (list->prev)
			nad = list->prev->data;
		break;
	case GTK_DIR_RIGHT:
	case GTK_DIR_DOWN:
		if (list->next)
			nad = list->next->data;
		break;
	default:
		return;
	}
	if (nad) {
		increment = nad->pos - ad->pos;
		panel_widget_switch_move (panel, ad, increment);
	}
}

static void 
panel_widget_free_move_applet (PanelWidget      *panel,
                               GtkDirectionType  dir)
{
	AppletData       *ad;
	gint increment = MOVE_INCREMENT;
	gint pos;

	ad = panel->currently_dragged_applet;

	g_return_if_fail (ad);

	switch (dir) {
	case GTK_DIR_LEFT:
	case GTK_DIR_UP:
		increment = -increment;
		break;
	case GTK_DIR_RIGHT:
	case GTK_DIR_DOWN:
		break;
	default:
		return;
	}
	pos = panel_widget_get_free_spot (panel, ad, 
                                          increment + ad->pos + ad->drag_off);
	panel_widget_nice_move (panel, ad, pos);
}

static void
panel_widget_tab_move (PanelWidget *panel,
                       gboolean     next)
{
	AppletData       *ad;
	GSList *li;
	PanelWidget *new_panel = NULL;

	ad = panel->currently_dragged_applet;

	if (!ad) {
		g_return_if_fail (GTK_WIDGET_HAS_FOCUS (panel));
		
		GTK_WIDGET_UNSET_FLAGS (GTK_WIDGET (panel), GTK_CAN_FOCUS);
		gtk_widget_child_focus (GTK_WIDGET (panel),
					next ?
						GTK_DIR_TAB_FORWARD :
						GTK_DIR_TAB_BACKWARD);
		return;	
	}
	
	for (li = panels; li; li = li->next) {
		PanelWidget *previous_panel = NULL;
		PanelWidget *panel_in_list = li->data;

		if (panel_in_list == panel) {
			if (next) {
				if (li->next)
					new_panel = li->next->data;
			} else {
				new_panel = previous_panel;
			}
			break;
		} else {		
			if (!next)
				previous_panel = panel_in_list;
		}
	}
	g_return_if_fail (li);
        if (new_panel)
		panel_widget_reparent (panel, new_panel, ad->applet, 0);
}

static void
panel_widget_end_move (PanelWidget *panel)
{
	panel_widget_applet_drag_end (panel);
}

static gboolean
panel_widget_focus_in_event (GtkWidget *widget, GdkEventFocus *event)
{
	GtkWidgetClass *class;
	GtkBindingSet *binding_set;

	gtk_widget_set_state (widget, GTK_STATE_PRELIGHT);

	class = GTK_WIDGET_GET_CLASS (widget);
	binding_set = gtk_binding_set_by_class (class);
	add_tab_bindings (binding_set, 0, TRUE);
	add_tab_bindings (binding_set, GDK_SHIFT_MASK, FALSE);
	GTK_WIDGET_CLASS (panel_widget_parent_class)->focus_in_event (widget, event);
	return FALSE;
}

static gboolean
panel_widget_focus_out_event (GtkWidget *widget, GdkEventFocus *event)
{
	gtk_widget_set_state (widget, GTK_STATE_NORMAL);

	if (GTK_WINDOW (PANEL_WIDGET (widget)->panel_parent)->has_focus) {
		GtkWidgetClass *class;
		GtkBindingSet *binding_set;

		GTK_WIDGET_UNSET_FLAGS (widget, GTK_CAN_FOCUS);
		class = GTK_WIDGET_GET_CLASS (widget);
		binding_set = gtk_binding_set_by_class (class);
		remove_tab_bindings (binding_set, 0, TRUE);
		remove_tab_bindings (binding_set, GDK_SHIFT_MASK, FALSE);
	}
	GTK_WIDGET_CLASS (panel_widget_parent_class)->focus_out_event (widget, event);
	return FALSE;
}

void panel_widget_focus (PanelWidget *panel)
{
	/*
	 * Set the focus back on the panel; we unset the focus child so that
	 * the next time focus is inside the panel we do not remember the
	 * previously focused child. We also need to set GTK_CAN_FOCUS flag
	 * on the panle as it is unset when this function is called.
	 */
	gtk_container_set_focus_child (GTK_CONTAINER (panel), NULL);
	GTK_WIDGET_SET_FLAGS (panel, GTK_CAN_FOCUS);
      	gtk_widget_grab_focus (GTK_WIDGET (panel));
}

PanelOrient
panel_widget_get_applet_orient (PanelWidget *panel)
{
	g_return_val_if_fail (PANEL_IS_WIDGET (panel), PANEL_ORIENT_UP);
	g_return_val_if_fail (panel->panel_parent != NULL, PANEL_ORIENT_UP);

	if (!BASEP_IS_WIDGET (panel->panel_parent))
		return PANEL_ORIENT_DOWN;

	return basep_widget_get_applet_orient (BASEP_WIDGET (panel->panel_parent));
}
