/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
#ifndef __CAPLET_WIDGET_H__
#define __CAPLET_WIDGET_H__

#include <gtk/gtk.h>
#include <gnome.h>

#ifdef __cplusplus
extern "C" {
#pragma }
#endif /* __cplusplus */


#define CAPLET_WIDGET(obj)          GTK_CHECK_CAST (obj, caplet_widget_get_type (), CapletWidget)
#define CAPLET_WIDGET_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, control_center_widget_get_type (), ControlCenterWidgetClass)
#define IS_CAPLET_WIDGET(obj)       GTK_CHECK_TYPE (obj, caplet_widget_get_type ())

typedef struct _CapletWidget		        CapletWidget;
typedef struct _CapletWidgetClass		CapletWidgetClass;

struct _CapletWidget
{
	GtkPlug			window;
	int			control_center_id;
};

struct _CapletWidgetClass
{
	GtkPlugClass 		parent_class;
};

guint           caplet_widget_get_type       	(void);
GtkWidget*      caplet_widget_new            	(void);



void		caplet_gtk_main  		(void);
error_t		caplet_init		(char *app_id,
                                         struct argp *app_parser,
                                         int argc,
                                         char **argv,
                                         unsigned int flags,
                                         int *arg_index);



#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __CAPLET_WIDGET_H__ */
