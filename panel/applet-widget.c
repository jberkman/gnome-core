#include <gtk/gtk.h>
#include <gnome.h>
#include <applet-widget.h>

static void applet_widget_class_init	(AppletWidgetClass *klass);
static void applet_widget_init		(AppletWidget      *applet_widget);

typedef void (*AppletWidgetOrientSignal) (GtkObject * object,
					  PanelOrientType orient,
					  gpointer data);

typedef void (*AppletWidgetSaveSignal) (GtkObject * object,
				        char *cfgpath,
				        char *globcfgpath,
				        gpointer data);

static GList *applet_widgets=NULL;

guint
applet_widget_get_type ()
{
	static guint applet_widget_type = 0;

	if (!applet_widget_type) {
		GtkTypeInfo applet_widget_info = {
			"AppletWidget",
			sizeof (AppletWidget),
			sizeof (AppletWidgetClass),
			(GtkClassInitFunc) applet_widget_class_init,
			(GtkObjectInitFunc) applet_widget_init,
			(GtkArgSetFunc) NULL,
			(GtkArgGetFunc) NULL,
		};

		applet_widget_type = gtk_type_unique (gtk_plug_get_type (), &applet_widget_info);
	}

	return applet_widget_type;
}

enum {
	CHANGE_ORIENT_SIGNAL,
	SESSION_SAVE_SIGNAL,
	LAST_SIGNAL
};

static gint applet_widget_signals[LAST_SIGNAL] = {0,0};

static void
gtk_applet_widget_marshal_signal_orient (GtkObject * object,
				         GtkSignalFunc func,
				         gpointer func_data,
				         GtkArg * args)
{
	AppletWidgetOrientSignal rfunc;

	rfunc = (AppletWidgetOrientSignal) func;

	(*rfunc) (object, GTK_VALUE_ENUM (args[0]),
		  func_data);
}

static void
gtk_applet_widget_marshal_signal_save (GtkObject * object,
				       GtkSignalFunc func,
				       gpointer func_data,
				       GtkArg * args)
{
	AppletWidgetSaveSignal rfunc;

	rfunc = (AppletWidgetSaveSignal) func;

	(*rfunc) (object, GTK_VALUE_STRING (args[0]),
		  GTK_VALUE_STRING (args[0]),
		  func_data);
}

static void
applet_widget_class_init (AppletWidgetClass *class)
{
	GtkObjectClass *object_class;

	object_class = (GtkObjectClass*) class;

	applet_widget_signals[CHANGE_ORIENT_SIGNAL] =
		gtk_signal_new("change_orient",
			       GTK_RUN_LAST,
			       object_class->type,
			       GTK_SIGNAL_OFFSET(AppletWidgetClass,
			       			 change_orient),
			       gtk_applet_widget_marshal_signal_orient,
			       GTK_TYPE_NONE,
			       1,
			       GTK_TYPE_ENUM);
	applet_widget_signals[SESSION_SAVE_SIGNAL] =
		gtk_signal_new("session_save",
			       GTK_RUN_LAST,
			       object_class->type,
			       GTK_SIGNAL_OFFSET(AppletWidgetClass,
			       			 session_save),
			       gtk_applet_widget_marshal_signal_save,
			       GTK_TYPE_NONE,
			       2,
			       GTK_TYPE_STRING,
			       GTK_TYPE_STRING);

	gtk_object_class_add_signals(object_class,applet_widget_signals,
				     LAST_SIGNAL);

	class->change_orient = NULL;
	class->session_save = NULL;
}

static void
applet_widget_init (AppletWidget *applet_widget)
{
	applet_widget->applet_id = -1;
}

static gint
applet_widget_destroy(GtkWidget *w,gpointer data)
{
	AppletWidget *applet = APPLET_WIDGET(w);
	g_free(applet->cfgpath);
	g_free(applet->globcfgpath);
	return FALSE;
}

GtkWidget *
applet_widget_new(gchar *argv0)
{
	AppletWidget *applet;
	char *result;
	char *cfgpath;
	char *globcfgpath;
	guint32 winid;
	char *myinvoc;
	gint applet_id;

	myinvoc = get_full_path(argv0);
	if(!myinvoc)
		return 0;

	if (!gnome_panel_applet_init_corba())
		g_error("Could not communicate with the panel\n");

	result = gnome_panel_applet_request_id(myinvoc, &applet_id,
					       &cfgpath, &globcfgpath,
					       &winid);
	if (result)
		g_error("Could not talk to the panel: %s\n", result);

	g_free(myinvoc);

	applet = APPLET_WIDGET (gtk_type_new (applet_widget_get_type ()));

	GTK_PLUG(applet)->socket_window = gdk_window_foreign_new (winid);
	GTK_PLUG(applet)->same_app = FALSE;

	applet->applet_id = applet_id;
	applet->cfgpath = cfgpath;
	applet->globcfgpath = globcfgpath;

	gtk_signal_connect(GTK_OBJECT(applet),"destroy",
			   GTK_SIGNAL_FUNC(applet_widget_destroy),
			   NULL);

	applet_widgets = g_list_prepend(applet_widgets,applet);

	return GTK_WIDGET(applet);
}

void
applet_widget_add(AppletWidget *applet, GtkWidget *widget)
{
	char *result;

	gtk_container_add(GTK_CONTAINER(applet),widget);

	result = gnome_panel_applet_register(GTK_WIDGET(applet),
					     applet->applet_id);
	if (result)
		g_error("Could not talk to the Panel: %s\n", result);
}

void
applet_widget_set_tooltip(AppletWidget *applet, gchar *text)
{
	gnome_panel_applet_add_tooltip (applet->applet_id, text);
}

void
applet_widget_gtk_main(void)
{
	applet_corba_gtk_main("IDL:GNOME/Applet:1.0");
}

void
change_orient(int applet_id, int orient)
{
	GList *list;
	PanelOrientType o = (PanelOrientType) orient;

	for(list = applet_widgets;list!=NULL;list=g_list_next(list)) {
		AppletWidget *applet = list->data;
		if(applet->applet_id == applet_id) {
			gtk_signal_emit(GTK_OBJECT(applet),
					applet_widget_signals[
						CHANGE_ORIENT_SIGNAL],
					o);
			return;
		}
	}
}

int
session_save(int applet_id, const char *cfgpath, const char *globcfgpath)
{
	GList *list;

	char *cfg = g_strdup(cfgpath);
	char *globcfg = g_strdup(globcfgpath);

	for(list = applet_widgets;list!=NULL;list=g_list_next(list)) {
		AppletWidget *applet = list->data;
		if(applet->applet_id == applet_id) {
			gtk_signal_emit(GTK_OBJECT(applet),
					applet_widget_signals[
						SESSION_SAVE_SIGNAL],
					cfg,globcfg);
			break;
		}
	}
	g_free(cfg);
	g_free(globcfg);
	/*need to implement a return type from widget!*/
	return TRUE;
}
