/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
#ifndef __CONTROL_CENTER_WIDGET_H__
#define __CONTROL_CENTER_WIDGET_H__

#include <gtk/gtk.h>
#include <gnome.h>

#ifdef __cplusplus
extern "C" {
#pragma }
#endif /* __cplusplus */


#define CONTROL_CENTER_WIDGET(obj)          GTK_CHECK_CAST (obj, control_center_widget_get_type (), ControlCenterWidget)
#define CONTROL_CENTER_WIDGET_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, control_center_widget_get_type (), ControlCenterWidgetClass)
#define IS_CONTROL_CENTER_WIDGET(obj)       GTK_CHECK_TYPE (obj, control_center_widget_get_type ())

typedef struct _ControlCenterWidget	        ControlCenterWidget;
typedef struct _ControlCenterWidgetClass	ControlCenterWidgetClass;

struct _ControlCenterWidget
{
	GtkPlug			window;
	int			control_center_id;
};

struct _ControlCenterWidgetClass
{
	GtkPlugClass 		parent_class;
};

guint           control_center_widget_get_type       	(void);
GtkWidget*      control_center_widget_new            	(void);



void		control_center_gtk_main  		(void);
error_t		control_center_init		(char *app_id,
                                                 struct argp *app_parser,
                                                 int argc,
                                                 char **argv,
                                                 unsigned int flags,
                                                 int *arg_index);
 


#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __CONTROL_CENTER_WIDGET_H__ */
