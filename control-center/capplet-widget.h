/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
#ifndef __CAPPLET_WIDGET_H__
#define __CAPPLET_WIDGET_H__

#include <gtk/gtk.h>
#include <gnome.h>

#ifdef __cplusplus
extern "C" {
#pragma }
#endif /* __cplusplus */


#define CAPPLET_WIDGET(obj)          GTK_CHECK_CAST (obj, capplet_widget_get_type (), CappletWidget)
#define CAPPLET_WIDGET_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, control_center_widget_get_type (), ControlCenterWidgetClass)
#define IS_CAPPLET_WIDGET(obj)       GTK_CHECK_TYPE (obj, capplet_widget_get_type ())

typedef struct _CappletWidget		        CappletWidget;
typedef struct _CappletWidgetClass		CappletWidgetClass;

struct _CappletWidget
{
	GtkPlug			window;
	int			control_center_id;
        int			capid;
        gboolean 		changed;
};

struct _CappletWidgetClass
{
	GtkPlugClass 		parent_class;

        void (* try) 		(CappletWidget *capplet);
        void (* revert) 	(CappletWidget *capplet);
        void (* ok) 		(CappletWidget *capplet);
        void (* help)	 	(CappletWidget *capplet);
};

guint           capplet_widget_get_type       	(void);
GtkWidget*      capplet_widget_new            	(void);



void		capplet_gtk_main  		(void);
error_t		gnome_capplet_init	(char *app_id,
                                         struct argp *app_parser,
                                         int argc,
                                         char **argv,
                                         unsigned int flags,
                                         int *arg_index);
void 		capplet_widget_state_changed 	(CappletWidget *cap, gboolean undoable);


#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __CAPPLET_WIDGET_H__ */
