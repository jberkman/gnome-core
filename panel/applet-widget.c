#include <config.h>
#include <string.h>
#include <X11/X.h>
#include <X11/Xlib.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <libgnomeui/libgnomeui.h>
#include <bonobo-activation/bonobo-activation.h>
#include <libbonobo.h>

#include "applet-widget.h"
#include "GNOME_Panel.h"

struct _AppletWidgetPrivate
{
	/* CORBA stuff */
        gpointer                custom_servant;

	/* something was added */
	gboolean		added_child;
	
	/*change freezing*/
	int			frozen_level;

	gboolean		frozen_got_orient;
	PanelOrientType		frozen_orient;			

	gboolean		frozen_got_size;
	int			frozen_size;			
	
	gboolean		frozen_got_back;
	PanelBackType		frozen_back_type;			
	char 			*frozen_back_pixmap;			
	GdkColor		frozen_back_color;			
	
	gboolean		frozen_got_position;
	int			frozen_position_x;
	int			frozen_position_y;

	/* for local case */
	GtkWidget		*ebox;
};

/*****************************************************************************
  CORBA STUFF
 *****************************************************************************/

typedef struct _CallbackInfo CallbackInfo;
struct _CallbackInfo {
	char			*name;
	AppletCallbackFunc	func;
	gpointer		data;
};


typedef struct {
	POA_GNOME_Applet                servant;

	AppletWidget                   *appwidget;
	GSList                         *callbacks;
	GNOME_PanelSpot                 pspot;
	GNOME_Applet                    obj;
	guint32                         winid;

	char                           *iid;
} CustomAppletServant;

static PortableServer_ServantBase__epv base_epv = {
	NULL,
	NULL,
	NULL
};

static void
server_applet_change_orient(PortableServer_Servant _servant,
			    const GNOME_Panel_OrientType orient,
			    CORBA_Environment *ev);

static void
server_applet_change_size(PortableServer_Servant _servant,
			  const CORBA_short size,
			  CORBA_Environment *ev);

static void
server_applet_do_callback(PortableServer_Servant _servant,
			  const CORBA_char * callback_name,
			  CORBA_Environment *ev);

static void
server_applet_save_session(PortableServer_Servant _servant,
			   const CORBA_char * cfgpath,
			   const CORBA_char * globcfgpath,
			   const CORBA_unsigned_long cookie,
			   CORBA_Environment *ev);

static CORBA_boolean
server_applet_session_save(PortableServer_Servant _servant,
			   const CORBA_char * cfgpath,
			   const CORBA_char * globcfgpath,
			   CORBA_Environment *ev);

static void
server_applet_back_change(PortableServer_Servant _servant,
			  const GNOME_Panel_BackInfoType *backing,
			  CORBA_Environment *ev);

static void
server_applet_draw(PortableServer_Servant _servant,
		   CORBA_Environment *ev);

static void
server_applet_set_tooltips_state(PortableServer_Servant _servant,
				 const CORBA_boolean enabled,
				 CORBA_Environment *ev);

static void
server_applet_change_position(PortableServer_Servant _servant,
			      const CORBA_short x,
			      const CORBA_short y,
			      CORBA_Environment *ev);

static CORBA_char *
server_applet__get_goad_id(PortableServer_Servant _servant,
			   CORBA_Environment *ev);

static void
server_applet_freeze_changes(PortableServer_Servant _servant,
			     CORBA_Environment *ev);

static void
server_applet_thaw_changes(PortableServer_Servant _servant,
			   CORBA_Environment *ev);

static POA_GNOME_Applet__epv applet_epv = {
	NULL,
	server_applet_change_orient,
	server_applet_do_callback,
	server_applet_session_save,
	server_applet_back_change,
	server_applet_set_tooltips_state,
	server_applet__get_goad_id,
	server_applet_draw,
	server_applet_save_session,
	server_applet_change_size,
	server_applet_change_position,
	server_applet_freeze_changes,
	server_applet_thaw_changes
};

static POA_GNOME_Applet__vepv applet_vepv = {
	&base_epv,
	&applet_epv
};

/*****************************************************************************
  WIDGET STUFF
 *****************************************************************************/
static void applet_widget_class_init    (AppletWidgetClass *klass);
static void applet_widget_instance_init	(AppletWidget      *widget);

typedef int (*SaveSignal)      (GtkObject *object,
				char      *cfgpath,
				char      *globcfgpath,
				gpointer   data);

typedef void (*BackSignal)     (GtkObject            *object,
				GNOME_Panel_BackType  type,
				char                 *pixmap,
				GdkColor             *color,
				gpointer              data);

typedef void (*PositionSignal) (GtkObject *object,
				int        x,
				int        y,
				gpointer   data);

static int applet_count = 0;

static gboolean die_on_last = FALSE;
static GtkPlugClass *parent_class;

#define CD(applet) ((CustomAppletServant *)APPLET_WIDGET(applet)->_priv->custom_servant)

GType
applet_widget_get_type (void)
{
	static GType applet_widget_type = 0;

	if (!applet_widget_type) {
		static const GtkTypeInfo applet_widget_info = {
			"AppletWidget",
			sizeof (AppletWidget),
			sizeof (AppletWidgetClass),
			(GtkClassInitFunc) applet_widget_class_init,
			(GtkObjectInitFunc) applet_widget_instance_init,
			NULL,
			NULL,
			NULL
		};

		applet_widget_type = gtk_type_unique (gtk_plug_get_type (), &applet_widget_info);
	}

	return applet_widget_type;
}

enum {
	CHANGE_ORIENT_SIGNAL,
	CHANGE_PIXEL_SIZE_SIGNAL,
	SAVE_SESSION_SIGNAL,
	BACK_CHANGE_SIGNAL,
	DO_DRAW_SIGNAL,
	TOOLTIP_STATE_SIGNAL,
	CHANGE_POSITION_SIGNAL,
	LAST_SIGNAL
};

static guint applet_widget_signals[LAST_SIGNAL] = {0};

static void
marshal_signal_save (GClosure *closure,
		     GValue *return_value,
		     guint n_param_values,
		     const GValue *param_values,
		     gpointer invocation_hint,
		     gpointer marshal_data)
{
  register SaveSignal callback;
  register GCClosure *cc = (GCClosure*) closure;
  register gpointer data1, data2;

  int v_return;

  g_return_if_fail (return_value != NULL);
  g_return_if_fail (n_param_values == 3);

  if (G_CLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }

  callback = (SaveSignal) (marshal_data ? marshal_data : cc->callback);

  v_return = callback (data1, 
		       (char *) g_value_get_string (param_values + 1),
		       (char *) g_value_get_string (param_values + 2),
		       data2);

  g_value_set_int (return_value, v_return);

  /* make applets that forget to do this not fsckup */
  gnome_config_sync ();
  gnome_config_drop_all();
}


static void
marshal_signal_back  (GClosure *closure,
		     GValue *return_value,
		     guint n_param_values,
		     const GValue *param_values,
		     gpointer invocation_hint,
		     gpointer marshal_data)
{
  register BackSignal callback;
  register GCClosure *cc = (GCClosure*) closure;
  register gpointer data1, data2;
  
  g_return_if_fail (n_param_values == 4);
  
  if (G_CLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }

  callback = (BackSignal) (marshal_data ? marshal_data : cc->callback);

  callback (data1, 
	    g_value_get_enum (param_values + 1),
	    (char *) g_value_get_string (param_values + 2),
	    g_value_get_pointer (param_values + 3),
	    data2);
}

static void
marshal_signal_position   (GClosure *closure,
			   GValue *return_value,
			   guint n_param_values,
			   const GValue *param_values,
			   gpointer invocation_hint,
			   gpointer marshal_data)
{
  register PositionSignal callback;
  register GCClosure *cc = (GCClosure*) closure;
  register gpointer data1, data2;

  g_return_if_fail (n_param_values == 3);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (PositionSignal) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_value_get_int (param_values + 1),
            g_value_get_int (param_values + 2),
            data2);
}

static void
applet_widget_class_init (AppletWidgetClass *class)
{
	GtkObjectClass *object_class;

	object_class = (GtkObjectClass*) class;

	parent_class = gtk_type_class (gtk_plug_get_type ());

	applet_widget_signals[CHANGE_ORIENT_SIGNAL] =
		gtk_signal_new("change_orient",
			       GTK_RUN_FIRST,
			       GTK_CLASS_TYPE (object_class),
			       GTK_SIGNAL_OFFSET(AppletWidgetClass,
			       			 change_orient),
			       gtk_marshal_VOID__ENUM, /* FIXME: void isn't right here...should by gtk_marshal_NONE */
			       GTK_TYPE_NONE,
			       1,
			       GTK_TYPE_ENUM);
	applet_widget_signals[CHANGE_PIXEL_SIZE_SIGNAL] =
		gtk_signal_new("change_pixel_size",
			       GTK_RUN_FIRST,
			       GTK_CLASS_TYPE (object_class),
			       GTK_SIGNAL_OFFSET(AppletWidgetClass,
			       			 change_pixel_size),
			       gtk_marshal_NONE__INT,
			       GTK_TYPE_NONE,
			       1,
			       GTK_TYPE_INT);
	applet_widget_signals[SAVE_SESSION_SIGNAL] =
		gtk_signal_new("save_session",
			       GTK_RUN_LAST,
			       GTK_CLASS_TYPE (object_class),
			       GTK_SIGNAL_OFFSET(AppletWidgetClass,
			       			 save_session),
			       marshal_signal_save,
			       GTK_TYPE_BOOL,
			       2,
			       GTK_TYPE_STRING,
			       GTK_TYPE_STRING);
	applet_widget_signals[BACK_CHANGE_SIGNAL] =
		gtk_signal_new("back_change",
			       GTK_RUN_LAST,
			       GTK_CLASS_TYPE (object_class),
			       GTK_SIGNAL_OFFSET(AppletWidgetClass,
			       			 back_change),
			       marshal_signal_back,
			       GTK_TYPE_NONE,
			       3,
			       GTK_TYPE_ENUM,
			       GTK_TYPE_POINTER,
			       GTK_TYPE_POINTER);
	applet_widget_signals[DO_DRAW_SIGNAL] =
		gtk_signal_new("do_draw",
			       GTK_RUN_LAST,
			       GTK_CLASS_TYPE (object_class),
			       GTK_SIGNAL_OFFSET(AppletWidgetClass,
			       			 do_draw),
			       gtk_signal_default_marshaller,
			       GTK_TYPE_NONE,
			       0);
	applet_widget_signals[TOOLTIP_STATE_SIGNAL] =
		gtk_signal_new("tooltip_state",
			       GTK_RUN_LAST,
			       GTK_CLASS_TYPE (object_class),
			       GTK_SIGNAL_OFFSET(AppletWidgetClass,
			       			 tooltip_state),
			       gtk_marshal_NONE__INT,
			       GTK_TYPE_NONE,
			       1,
			       GTK_TYPE_INT);
	applet_widget_signals[CHANGE_POSITION_SIGNAL] =
		gtk_signal_new("change_position",
			       GTK_RUN_LAST,
			       GTK_CLASS_TYPE (object_class),
			       GTK_SIGNAL_OFFSET(AppletWidgetClass,
			       			 change_position),
			       marshal_signal_position,
			       GTK_TYPE_NONE,
			       2,
			       GTK_TYPE_INT,
			       GTK_TYPE_INT);

	class->change_orient = NULL;
	class->save_session = NULL;
	class->back_change = NULL;
	class->do_draw = NULL;
	class->tooltip_state = NULL;
	class->change_position = NULL;
}

static void
applet_widget_instance_init (AppletWidget *applet)
{
	g_return_if_fail (applet);
	g_return_if_fail (IS_APPLET_WIDGET (applet));
	
	applet->orient = ORIENT_UP;
	applet->size   = PIXEL_SIZE_STANDARD;

	applet->_priv = g_new0 (AppletWidgetPrivate, 1);

	applet->_priv->custom_servant      = NULL;
	applet->_priv->added_child         = FALSE;
	applet->_priv->frozen_level        = 0;
	applet->_priv->frozen_got_orient   = FALSE;
	applet->_priv->frozen_got_size     = FALSE;
	applet->_priv->frozen_got_back     = FALSE;
	applet->_priv->frozen_got_position = FALSE;
}

static void
custom_servant_destroy (CustomAppletServant *servant)
{
	PortableServer_POA       poa;
	PortableServer_ObjectId *objid;
	CORBA_Environment        env;
	GNOME_Applet             applet;
	GSList                  *l;

	CORBA_exception_init (&env);

	for (l = servant->callbacks; l; l = l->next) {
		CallbackInfo *info = (CallbackInfo *)l->data;

		g_free (info->name);
		info->name = NULL;

		g_free (info);

		l->data = NULL;
	}

	g_slist_free (servant->callbacks);
	servant->callbacks = NULL;

	poa = bonobo_poa ();
	
	objid  = PortableServer_POA_servant_to_id (poa, servant, &env);

	PortableServer_POA_deactivate_object (poa, objid, &env);

	CORBA_free (objid);

	bonobo_activation_active_server_unregister (servant->iid, servant->obj);

	g_free (servant->iid);
	servant->iid = NULL;

	CORBA_Object_release (servant->pspot, &env);
	servant->pspot = NULL;

	CORBA_Object_release (servant->obj, &env);
	servant->obj = NULL;

	POA_GNOME_Applet__fini (servant, &env);

	g_free (servant);

	CORBA_exception_free (&env);
}

static void
applet_widget_destroy(GtkWidget *w, gpointer data)
{
	AppletWidget *applet;
	GtkPlug *plug;

	g_return_if_fail(w != NULL);
	g_return_if_fail(IS_APPLET_WIDGET(w));

	applet = APPLET_WIDGET(w);
	plug = GTK_PLUG(w);

	/* XXX: hackaround to broken gtkplug/gtksocket, we kill the references
	   to ourselves on the socket and our references to the socket and
	   destroy the socket */
	if(plug->same_app && plug->socket_window) {
		GtkSocket *socket;
		gdk_window_get_user_data (plug->socket_window,
					  (gpointer *)&socket);
		if(socket) {
			GtkWidget *toplevel = gtk_widget_get_toplevel (GTK_WIDGET(socket));
			if (toplevel && GTK_IS_WINDOW (toplevel))
				gtk_window_remove_embedded_xid (GTK_WINDOW (toplevel), 
								GDK_WINDOW_XWINDOW (socket->plug_window));
			socket->plug_window = NULL;
			socket->same_app = FALSE;
			plug->socket_window = NULL;
			plug->same_app = FALSE;
			gtk_widget_destroy(GTK_WIDGET(socket));
		}
	}

	if(applet->_priv->ebox && GTK_BIN(applet->_priv->ebox)->child) {
		GtkWidget *child = GTK_BIN(applet->_priv->ebox)->child;
		/* disconnect the destroy handler */
		gtk_signal_disconnect_by_data(GTK_OBJECT(child), applet);
		gtk_widget_destroy(child);
	}


	if (applet->privcfgpath) {
		CORBA_Environment env;

		CORBA_exception_init (&env);

		g_free (applet->privcfgpath);
		g_free (applet->globcfgpath);

		applet->privcfgpath = NULL;
		applet->globcfgpath = NULL;

		/*
		 * if nothing has been added as our child, this 
		 * means we have not yet fully completed load, so 
		 * notify the panel that we are going to die
		 */
		if (!applet->_priv->added_child)
			GNOME_PanelSpot_abort_load (CD (applet)->pspot, &env);

		CORBA_exception_free (&env);
	}

	custom_servant_destroy (applet->_priv->custom_servant);
	applet->_priv->custom_servant = NULL;

	applet_count--;

	if(die_on_last && applet_count == 0)
		applet_widget_gtk_main_quit();

	g_free(applet->_priv);
	applet->_priv = NULL;
}

/**
 * applet_widget_abort_load:
 * @applet: #AppletWidget to work on.
 *
 * Description:  Abort the applet loading, once applet has been created, this is
 * a way to tell the panel to forget about us if we decide we want to quit
 * before we add the actual applet to the applet-widget.  This is only useful
 * to abort after #applet_widget_new was called but before #applet_widget_add
 * is called.
 **/
void
applet_widget_abort_load (AppletWidget *applet)
{
	CORBA_Environment env;

	CORBA_exception_init (&env);

	GNOME_PanelSpot_abort_load (CD (applet)->pspot, &env);

	CORBA_exception_free (&env);
}

/**
 * applet_widget_remove:
 * @applet: #AppletWidget to work on.
 *
 * Description:  Remove the plug from the panel, this will destroy the applet.
 * You can only call this once for each applet.
 **/
void
applet_widget_remove (AppletWidget *applet)
{
	CustomAppletServant *servant;
	CORBA_Environment    env;

	g_return_if_fail (applet);
	g_return_if_fail (IS_APPLET_WIDGET (applet));

	CORBA_exception_init (&env);

	servant = applet->_priv->custom_servant;

	bonobo_activation_active_server_unregister (servant->iid, servant->obj);

	GNOME_PanelSpot_unregister_us (CD (applet)->pspot, &env);

	CORBA_exception_free (&env);
}

/**
 * applet_widget_sync_config:
 * @applet: #AppletWidget to work on.
 *
 * Description:  Tell the panel to save our session here (just saves, no
 * shutdown).  This should be done when you change some of your config and want
 * the panel to save it's config, you should NOT call this in the session_save
 * handler as it will result in a locked panel, as it will actually trigger
 * another session_save signal for you.  However it also asks for a complete
 * panel save, so you should not do this too often, and only when the user
 * has changed some preferences and you want to sync them to disk. 
 * Theoretically you don't even need to do that if you don't mind loosing
 * settings on a panel crash or when the user kills the session without
 * logging out properly, since the panel will always save your session when
 * it exists.
 **/
void
applet_widget_sync_config (AppletWidget *applet)
{
	CORBA_Environment env;

	g_return_if_fail (applet);
	g_return_if_fail (IS_APPLET_WIDGET (applet));

	CORBA_exception_init (&env);

	GNOME_PanelSpot_sync_config (CD (applet)->pspot, &env);

	CORBA_exception_free (&env);
}

static const char*
make_sane_name(const char *name)
{
	if(!name)
		return NULL;
	while(*name=='/')
		name++;
	if(*name)
		return name;
	return NULL;
}

static CallbackInfo *
get_callback_info(GtkWidget *applet, const char *name)
{
	GSList *list;
	CallbackInfo *info;

	for(list=CD(applet)->callbacks;
	    list!=NULL;
	    list=g_slist_next(list)) {
		info = (CallbackInfo *)list->data;
		if(strcmp(name,info->name)==0)
			return info;
	}

	return NULL;
}


/*
 * adds a callback to the callback hash
 */
static void
gnome_panel_applet_register_callback (GtkWidget          *applet,
				      const char         *name,
				      const char         *stock_item,
				      const char         *menutext,
				      AppletCallbackFunc  func,
				      gpointer            data)
{
	CallbackInfo *info;

	/*skip over leading '/'s*/
	name = make_sane_name(name);

	g_return_if_fail(name!=NULL);
	
	info = get_callback_info(applet,name);
	if(!info) {
		info = g_new0(CallbackInfo,1);
		CD(applet)->callbacks = g_slist_prepend(CD(applet)->callbacks,
							info);
	} else
		g_free(info->name);

	info->name = g_strdup(name);
	info->func = func;
	info->data = data;

	{
		CORBA_Environment env;

		CORBA_exception_init (&env);

		GNOME_PanelSpot_add_callback (CD (applet)->pspot,
					      name, stock_item,
					      menutext, &env);

		CORBA_exception_free (&env);
	}
}

/**
 * applet_widget_register_callback:
 * @applet: #AppletWidget to work on.
 * @name: path to the menu item.
 * @menutext: text for the menu item.
 * @func: #AppletCallbacFunc to call when the menu item is activated.
 * @data: data passed to @func.
 *
 * Description:  Adds a menu item to the applet's context menu.  The name
 * should be a path that is separated by '/' and ends in the name of this
 * item.  You need to add any submenus with
 * #applet_widget_register_callback_dir.
 **/
void
applet_widget_register_callback(AppletWidget *applet,
				const char *name,
				const char *menutext,
				AppletCallbackFunc func,
				gpointer data)
{
	g_return_if_fail(applet != NULL);
	g_return_if_fail(IS_APPLET_WIDGET(applet));
	g_return_if_fail(name != NULL);
	g_return_if_fail(menutext != NULL);
	g_return_if_fail(func != NULL);

	gnome_panel_applet_register_callback (GTK_WIDGET(applet),name,
					      "",menutext,func,data);
}

/**
 * applet_widget_register_stock_callback:
 * @applet: #AppletWidget to work on.
 * @name: path to the menu item.
 * @stock_type: GNOME_STOCK string to use for the pixmap
 * @menutext: text for the menu item.
 * @func: #AppletCallbacFunc to call when the menu item is activated.
 * @data: data passed to @func.
 *
 * Description:  Adds a menu item to the applet's context menu with a stock 
 * GNOME pixmap.  This works almost exactly the same as
 * #applet_widget_register_callback.
 **/
void
applet_widget_register_stock_callback(AppletWidget *applet,
				      const char *name,
				      const char *stock_type,
				      const char *menutext,
				      AppletCallbackFunc func,
				      gpointer data)
{
	g_return_if_fail(applet != NULL);
	g_return_if_fail(IS_APPLET_WIDGET(applet));
	g_return_if_fail(name != NULL);
	g_return_if_fail(stock_type != NULL);
	g_return_if_fail(menutext != NULL);
	g_return_if_fail(func != NULL);

	gnome_panel_applet_register_callback (GTK_WIDGET(applet),name,
					      stock_type,menutext,func,data);
}


/**
 * applet_widget_unregister_callback:
 * @applet: #AppletWidget to work on.
 * @name: path to the menu item.
 *
 * Description:  Remove a menu item from the applet's context menu.  The
 * @name should be the full path to the menu item.  This will not remove
 * any submenus.
 **/
void
applet_widget_unregister_callback(AppletWidget *applet,
				  const char *name)
{
	GSList *li;
	CallbackInfo *cbi = NULL;

	g_return_if_fail(applet != NULL);
	g_return_if_fail(IS_APPLET_WIDGET(applet));

	/*skip over leading '/'s*/
	name = make_sane_name(name);

	g_return_if_fail(name!=NULL);

	for(li = CD(applet)->callbacks; li; li = g_slist_next(li)) {
		if(!strcmp(((CallbackInfo*)li->data)->name, name)) {
			cbi = li->data;
			break;
		}
	}

	if(!cbi) return;
	CD(applet)->callbacks = g_slist_remove(CD(applet)->callbacks, cbi);

	{
		CORBA_Environment env;

		CORBA_exception_init (&env);

		GNOME_PanelSpot_remove_callback (CD (applet)->pspot, name, &env);

		CORBA_exception_free (&env);
	}
}

static void
gnome_panel_applet_register_callback_dir(GtkWidget *applet,
					 const char *name,
					 const char *stock_item,
					 const char *menutext)
{
	char *n;

	/*skip over leading '/'s*/
	name = make_sane_name(name);
	g_return_if_fail(name!=NULL);

	if(name[strlen(name)-1]!='/')
		n = g_strconcat(name,"/",NULL);
	else
		n = g_strdup(name);

	{
		CORBA_Environment env;

		CORBA_exception_init (&env);

		GNOME_PanelSpot_add_callback (CD (applet)->pspot,
					      n, stock_item,
					      menutext, &env);

		CORBA_exception_free (&env);
	}

	g_free(n);
}


/**
 * applet_widget_register_callback_dir:
 * @applet: #AppletWidget to work on.
 * @name: path to the menu item.
 * @menutext: text for the menu item.
 *
 * Description:  Adds a submenu to the applet's context menu.  The @name
 * should be the full path of the new submenu with the name of the new
 * submenu as the last part of the path.  The @name can, but doesn't
 * have to be terminated with a '/'.
 **/
void
applet_widget_register_callback_dir(AppletWidget *applet,
				    const char *name,
				    const char *menutext)
{
	g_return_if_fail(applet != NULL);
	g_return_if_fail(IS_APPLET_WIDGET(applet));
	g_return_if_fail(name != NULL);
	g_return_if_fail(menutext != NULL);

	gnome_panel_applet_register_callback_dir (GTK_WIDGET(applet),name,
						  "",menutext);
}


/**
 * applet_widget_register_stock_callback_dir:
 * @applet: #AppletWidget to work on.
 * @name: path to the menu item.
 * @stock_type: GNOME_STOCK string to use for the pixmap
 * @menutext: text for the menu item.
 *
 * Description:  Adds a submenu to the applet's context menu with a stock 
 * GNOME pixmap.  This is similiar to #applet_widget_register_callback_dir.
 **/
void
applet_widget_register_stock_callback_dir(AppletWidget *applet,
					  const char *name,
					  const char *stock_type,
					  const char *menutext)
{
	g_return_if_fail(applet != NULL);
	g_return_if_fail(IS_APPLET_WIDGET(applet));
	g_return_if_fail(name != NULL);
	g_return_if_fail(stock_type != NULL);
	g_return_if_fail(menutext != NULL);

	gnome_panel_applet_register_callback_dir (GTK_WIDGET(applet),name,
						  stock_type,menutext);
}

/**
 * applet_widget_unregister_callback_dir:
 * @applet: #AppletWidget to work on.
 * @name: path to the menu item.
 *
 * Description:  Removes a submenu from the applet's context menu.  Use
 * this instead of #applet_widget_unregister_callback to remove submenus.
 * The @name can be, but doesn't have to be terminated with a '/'.  If you
 * have not removed the subitems of this menu, it will still be shown but
 * without it's title or icon.  So make sure to first remove any items and
 * submenus before calling this function.
 **/
void
applet_widget_unregister_callback_dir(AppletWidget *applet, const char *name)
{
	char *n;
	g_return_if_fail(applet != NULL);
	g_return_if_fail(IS_APPLET_WIDGET(applet));

	/*skip over leading '/'s*/
	name = make_sane_name(name);
	if(name[strlen(name)-1]!='/')
		n = g_strconcat(name,"/",NULL);
	else
		n = g_strdup(name);

	g_return_if_fail(name!=NULL);

	{
		CORBA_Environment env;

		CORBA_exception_init (&env);

		GNOME_PanelSpot_remove_callback (CD (applet)->pspot,
						 n, &env);
		CORBA_exception_free (&env);
	}

	g_free(n);
}

/**
 * applet_widget_callback_set_sensitive:
 * @applet: #AppletWidget to work on.
 * @name: path to the menu item.
 * @sensitive: whether menu item should be sensitive.
 *
 * Description:  Sets the sensitivity of a menu item in the applet's 
 * context menu.
 **/
void
applet_widget_callback_set_sensitive(AppletWidget *applet,
				     const char *name,
				     gboolean sensitive)
{

	g_return_if_fail(applet != NULL);
	g_return_if_fail(IS_APPLET_WIDGET(applet));

	/*skip over leading '/'s*/
	name = make_sane_name(name);

	g_return_if_fail(name!=NULL);

	{
		CORBA_Environment env;

		CORBA_exception_init (&env);

		GNOME_PanelSpot_callback_set_sensitive (CD (applet)->pspot,
							name,
							sensitive,
							&env);
		CORBA_exception_free (&env);
	}
}

static GNOME_Panel
gnome_panel_client (CORBA_Environment *ev)
{
	static GNOME_Panel panel_client = CORBA_OBJECT_NIL;

	if (panel_client == CORBA_OBJECT_NIL)
		panel_client = bonobo_activation_activate_from_id (
						    "OAFIID:GNOME_Panel",
						    0, NULL, ev);
}

static CustomAppletServant *
custom_servant_activate (AppletWidget *applet,
			 const char   *iid)
{
	CORBA_Environment          env;
	CustomAppletServant       *custom_servant;
	POA_GNOME_Applet          *applet_servant;
	GNOME_Applet               applet_obj;
	Bonobo_RegistrationResult  result;
	GNOME_Panel                panel;
	GNOME_PanelSpot            pspot;
	CORBA_char                *privcfg;
	CORBA_char                *globcfg;

	CORBA_exception_init (&env);

	bonobo_activate ();

	custom_servant = g_new0 (CustomAppletServant, 1);

	applet_servant = (POA_GNOME_Applet *)custom_servant;

	applet_servant->vepv = &applet_vepv;

	POA_GNOME_Applet__init (applet_servant, &env);
	
	applet_obj = PortableServer_POA_servant_to_reference (bonobo_poa (),
							      applet_servant,
							      &env);
	if (BONOBO_EX (&env)) {
		CORBA_exception_free (&env);
		fprintf (stderr, "custom_servant_activate: failed to activate servant\n");
		g_free (custom_servant);
		return NULL;
	}

	result = bonobo_activation_active_server_register (iid, applet_obj);
	switch (result) {
        case Bonobo_ACTIVATION_REG_SUCCESS:
                break;
        case Bonobo_ACTIVATION_REG_ALREADY_ACTIVE:
		fprintf (stderr, "custom_servant_activate: applet already active\n");
		g_free (custom_servant);
		return NULL;
		break;
        default:
		fprintf (stderr, "custom_servant_activate: bonobo activation error\n");
		g_free (custom_servant);
		return NULL;
		break;
        }

	panel = gnome_panel_client (&env);
	if (BONOBO_EX (&env) || panel == CORBA_OBJECT_NIL) {
		CORBA_exception_free (&env);
		g_warning (_("Cannot activate a panel object"));
		g_free (custom_servant);
		return NULL;
	}

	pspot = GNOME_Panel_add_applet (panel,
					applet_obj,
					(char *)iid,
					&privcfg,
					&globcfg,
					&custom_servant->winid,
					&env);
	if (BONOBO_EX (&env) || pspot == CORBA_OBJECT_NIL) {
		CORBA_exception_free (&env);
		g_free (custom_servant);
		return NULL;
	}

	custom_servant->obj       = applet_obj;
	custom_servant->pspot     = pspot;

	/*
	 * we need to do this as 1.0 panel will crap out otherwise,
	 * it NEEDS to know the applet as it's doing orient change
	 * signals during this
	 */
	custom_servant->appwidget = applet;

	/*
	 * this is just for consistency with the above
	 */
	custom_servant->iid       = g_strdup (iid);


	if (privcfg && *privcfg)
		applet->privcfgpath = g_strdup (privcfg);
	else
		applet->privcfgpath = NULL;
	CORBA_free (privcfg);

	if (globcfg && *globcfg)
		applet->globcfgpath = g_strdup (globcfg);
	else
		applet->globcfgpath = NULL;
	CORBA_free (globcfg);

	applet->orient = GNOME_PanelSpot__get_parent_orient (pspot, &env);
	if (BONOBO_EX (&env)) {
		fprintf (stderr, "custom_servant_activate: failed to get panel orient\n");

		CORBA_exception_free (&env);
		CORBA_exception_init (&env);

		applet->orient = ORIENT_UP;
	}

	applet->size = GNOME_PanelSpot__get_parent_size (pspot, &env);
	if (BONOBO_EX (&env)) {
		fprintf (stderr, "custom_servant_activate: failed to get panel size\n");

		applet->size = PIXEL_SIZE_STANDARD;
	}

	CORBA_exception_free (&env);

	return custom_servant;
}

/**
 * applet_widget_new:
 * @iid: The implementation id of the applet we are starting
 *
 * Description: Make a new applet and register us with the panel, if you
 * decide to cancel the load before calling #applet_widget_add, you should
 * call #applet_widget_abort_load.  This widget is a simple container but you
 * should always use only #applet_widget_add to add a child and you should
 * only use it once.
 *
 * Returns: A pointer to a new widget of type #AppletWidget, or %NULL if 
 * something went wrong.
 **/
GtkWidget *
applet_widget_new (const char *iid)
{
	AppletWidget *applet;

	applet = APPLET_WIDGET (gtk_type_new (applet_widget_get_type()));
	applet_widget_construct (applet, iid);

	return GTK_WIDGET (applet);
}

/**
 * applet_widget_construct:
 * @applet: #AppletWidget to work on
 * @id: the implementation id of the applet to construct
 *
 * Description: For bindings and subclassing only
 **/
void
applet_widget_construct (AppletWidget *applet,
			 const char   *iid)
{
	CustomAppletServant *custom_servant;
	GdkWindow           *win;
	
	g_return_if_fail (iid);

	custom_servant = custom_servant_activate (applet, iid);
	if (!custom_servant) {
		g_warning (_("Failed to activate CORBA object."));
		return;
	}

	applet->_priv->custom_servant = custom_servant;

	win = gdk_window_lookup (custom_servant->winid);

	gtk_plug_construct (GTK_PLUG (applet), custom_servant->winid);

	/*
	 * after doing all that we just take the 
	 * socket and put it in limbo
	 */
	if (win) {
		GtkWidget *socket;

		gdk_window_get_user_data (win, (gpointer *)&socket);
		if (socket) {
			GtkWidget *temp_window;

			temp_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

			applet->_priv->ebox = socket->parent;

			gtk_widget_set_uposition (GTK_WIDGET (temp_window),
						  gdk_screen_width ()  + 1,
						  gdk_screen_height () + 1);
			gtk_widget_realize (temp_window);
			gtk_widget_reparent (GTK_WIDGET (socket),
					     temp_window);
			gtk_signal_connect_object (GTK_OBJECT (applet->_priv->ebox),
						   "destroy",
						   GTK_SIGNAL_FUNC (gtk_widget_destroy),
						   GTK_OBJECT (temp_window));
		}
	}
	
	gtk_signal_connect (GTK_OBJECT (applet), "destroy",
			    GTK_SIGNAL_FUNC (applet_widget_destroy),
			    NULL);

	applet_count++;
}

/**
 * applet_widget_get_applet_count:
 *
 * Description:  Gets the number of applets loaded in this this process.  If
 * this is a shared lib applet it will return the total number of shared lib
 * applets loaded.
 *
 * Returns:  The number of applets loaded.
 **/
int
applet_widget_get_applet_count(void)
{
	return applet_count;
}

static gboolean
applet_event(GtkWidget *w, GdkEvent *event, AppletWidget *aw)
{
	GdkEventButton *bevent = (GdkEventButton *)event;
	if(event->type == GDK_BUTTON_PRESS && 
	   (bevent->button == 3 || bevent->button == 2)) {
		XButtonEvent ev;
		GtkWidget *wi;
		GtkPlug *plug = GTK_PLUG(aw);

		/* on local case */
		if(aw->_priv->ebox)
			return gtk_widget_event(aw->_priv->ebox, event);

		if((wi = gtk_grab_get_current()))
			gtk_grab_remove(wi);
		gdk_pointer_ungrab(GDK_CURRENT_TIME);
		gdk_keyboard_ungrab(GDK_CURRENT_TIME);
		gdk_flush();
		gtk_signal_emit_stop_by_name(GTK_OBJECT(w),
					     "event");
		ev.type = ButtonPress;
		ev.send_event = True;
		ev.display = GDK_DISPLAY();
		ev.window = GDK_WINDOW_XWINDOW(plug->socket_window);
		ev.subwindow = None;
		ev.time = bevent->time;
		ev.x = bevent->x;
		ev.y = bevent->y;
		ev.x_root = bevent->x_root;
		ev.y_root = bevent->y_root;
		ev.state = bevent->state;
		ev.button = bevent->button;
		ev.same_screen = True;
		/* in the local case send it to our ebox */
		if(aw->_priv->ebox && aw->_priv->ebox->window) {
			XSendEvent(GDK_DISPLAY(),
				   GDK_WINDOW_XWINDOW(aw->_priv->ebox->window),
				   True,NoEventMask,(XEvent *)&ev);
		} else if(plug->socket_window)
			XSendEvent(GDK_DISPLAY(),
				   GDK_WINDOW_XWINDOW(plug->socket_window),
				   True,NoEventMask,(XEvent *)&ev);
		
		return TRUE;
	}
	return FALSE;
}

static void
bind_applet_events(GtkWidget *widget, gpointer data)
{
	if (!GTK_WIDGET_NO_WINDOW(widget)) {
		gtk_signal_connect(GTK_OBJECT(widget), "event",
				   (GtkSignalFunc) applet_event,
				   data);
	}
	
	if (GTK_IS_CONTAINER(widget))
		gtk_container_foreach (GTK_CONTAINER (widget),
				       bind_applet_events, data);
}

static void
destroy_the_applet(GtkWidget *w, AppletWidget *applet)
{
	applet->_priv->ebox = NULL;
	gtk_widget_destroy(GTK_WIDGET(applet));
}

/**
 * applet_widget_add_full:
 * @applet: the #AppletWidget to work with
 * @widget: the child to add
 * @bind_events: bind 2nd and 3rd button events over the applet if %TRUE
 *
 * Description:  Add a child (@widget) to the @applet.  This finishes the
 * handshaking with the panel started in @applet_widget_new.  You should never
 * call this function twice for the same @applet and you should always use
 * this function rather then #gtk_container_add.  If you have already created
 * an applet widget with #applet_widget_new, but need to cancel the loading
 * of the applet, use #applet_widget_abort_load.  This function is only for
 * special applets and you should use #applet_widget_bind_events on some
 * internal widget if @bind_events was %FALSE.  Normally you'll just want to
 * use #applet_widget_add.
 **/
void
applet_widget_add_full (AppletWidget *applet,
			GtkWidget    *widget,
			gboolean      bind_events)
{

	g_return_if_fail(applet != NULL);
	g_return_if_fail(IS_APPLET_WIDGET(applet));
	g_return_if_fail(widget != NULL);
	g_return_if_fail(GTK_IS_WIDGET(widget));

	if(applet->_priv->ebox) {
		gtk_container_add(GTK_CONTAINER(applet->_priv->ebox), widget);
		gtk_signal_connect(GTK_OBJECT(widget), "destroy",
				   GTK_SIGNAL_FUNC(destroy_the_applet),
				   applet);
	} else
		gtk_container_add(GTK_CONTAINER(applet), widget);


	{
		CORBA_Environment env;

		CORBA_exception_init (&env);
  
		GNOME_PanelSpot_register_us (CD (applet)->pspot, &env);
		if (BONOBO_EX (&env)) {
			g_warning (_("CORBA Exception"));
			CORBA_exception_free (&env);
			gtk_widget_destroy (widget);
			return;
		}

		CORBA_exception_free (&env);
	}

	if(bind_events) {
		if(applet->_priv->ebox)
			bind_applet_events(widget, applet);
		else
			bind_applet_events(GTK_WIDGET(applet), applet);
	}

	applet->_priv->added_child = TRUE;
}

/**
 * applet_widget_add:
 * @applet: the #AppletWidget to work with
 * @widget: the child to add
 *
 * Description:  Add a child (@widget) to the @applet.  This finishes the
 * handshaking with the panel started in @applet_widget_new.  You should never
 * call this function twice for the same @applet and you should always use
 * this function rather then #gtk_container_add.  If you have already created
 * an applet widget with #applet_widget_new, but need to cancel the loading
 * of the applet, use #applet_widget_abort_load.
 **/
void
applet_widget_add(AppletWidget *applet, GtkWidget *widget)
{
	applet_widget_add_full(applet, widget, TRUE);
}

/**
 * applet_widget_bind_events:
 * @applet: the #AppletWidget to work with
 * @widget: the widget over which to bind events
 *
 * Description:  Binds the 2nd and 3rd button clicks over this widget. 
 * Normally this is done during #applet_widget_add, but if you need to
 * bind events over a widget which you added later, use this function.
 **/
void
applet_widget_bind_events(AppletWidget *applet, GtkWidget *widget)
{
	g_return_if_fail(applet != NULL);
	g_return_if_fail(IS_APPLET_WIDGET(applet));
	g_return_if_fail(widget != NULL);
	g_return_if_fail(IS_APPLET_WIDGET(widget));
	
	if(applet->_priv->ebox && GTK_WIDGET(applet) == widget) {
		GtkWidget *child = GTK_BIN(applet->_priv->ebox)->child;
		if(child) bind_applet_events(child, applet);
	} else
		bind_applet_events(GTK_WIDGET(widget), applet);
}

static GtkTooltips *
applet_widget_tooltips (void)
{
	static GtkTooltips *applet_tooltips = NULL;

	if (!applet_tooltips)
		applet_tooltips = gtk_tooltips_new();

	return applet_tooltips;
}

/**
 * applet_widget_set_widget_tooltip:
 * @applet: the #AppletWidget to work with
 * @widget: the widget to set tooltip on
 * @text: the tooltip text
 *
 * Description:  Set a tooltip on the @widget that will follow the tooltip
 * setting from the panel configuration.
 **/
void
applet_widget_set_widget_tooltip (AppletWidget *applet,
				  GtkWidget    *widget,
				  const char   *text)
{
	g_return_if_fail (applet);
	g_return_if_fail (IS_APPLET_WIDGET (applet));
	g_return_if_fail (widget);
	g_return_if_fail (GTK_IS_WIDGET (widget));

	gtk_tooltips_set_tip (applet_widget_tooltips (),
			      widget, text, NULL);
}

/**
 * applet_widget_set_tooltip:
 * @applet: the #AppletWidget to work with
 * @text: the tooltip text
 *
 * Description:  Set a tooltip on the entire applet that will follow the
 * tooltip setting from the panel configuration.
 **/
void
applet_widget_set_tooltip (AppletWidget *applet,
			   const char   *text)
{
	CORBA_Environment env;

	g_return_if_fail (applet);
	g_return_if_fail (IS_APPLET_WIDGET (applet));

	CORBA_exception_init (&env);

	GNOME_PanelSpot__set_tooltip (CD (applet)->pspot,
				      text ? text : "",
				      &env);
	if(BONOBO_EX (&env)) {
		g_warning (_("CORBA Exception"));
		CORBA_exception_free (&env);
		return;
	}

	CORBA_exception_free (&env);
}

/**
 * applet_widget_get_panel_orient:
 * @applet: the #AppletWidget to work with
 *
 * Description:  Gets the orientation of the panel this widget is on.
 * it can be one of ORIENT_UP, ORIENT_DOWN, ORIENT_LEFT and ORIENT_RIGHT.
 * This is not the position of the panel, but rather the direction that the
 * applet should be "reaching out".  So any arrows should for example point
 * in this direction.  It will be ORIENT_UP or ORIENT_DOWN for horizontal
 * panels and ORIENT_LEFT or ORIENT_RIGHT for vertical panels
 *
 * Returns:  PanelOrientType enum of the orientation
 **/
PanelOrientType
applet_widget_get_panel_orient(AppletWidget *applet)
{
	g_return_val_if_fail(applet != NULL,ORIENT_UP);
	g_return_val_if_fail(IS_APPLET_WIDGET(applet), ORIENT_UP);

	return applet->orient;
}

/**
 * applet_widget_get_panel_pixel_size:
 * @applet: the #AppletWidget to work with
 *
 * Description:  Gets the width of the panel in pixels.  This is not the
 * actual size, but the recomended one.  The panel may be streched if the
 * applets use larger sizes then this.
 *
 * Returns:  Size of panel in pixels
 **/
int
applet_widget_get_panel_pixel_size(AppletWidget *applet)
{
	g_return_val_if_fail(applet != NULL, PIXEL_SIZE_STANDARD);
	g_return_val_if_fail(IS_APPLET_WIDGET(applet), PIXEL_SIZE_STANDARD);

	return applet->size;
}

/**
 * applet_widget_get_free_space:
 * @applet: the #AppletWidget to work with
 *
 * Description:  Gets the free space left that you can use for your applet.
 * This is the number of pixels around your applet to both sides.  If you
 * strech by this amount you will not disturb any other applets.  If you
 * are on a packed panel 0 will be returned.
 *
 * Returns:  Free space left for your applet.
 **/
int
applet_widget_get_free_space (AppletWidget *applet)
{
	CORBA_Environment env;
	int               retval;

	g_return_val_if_fail (applet, 0);
	g_return_val_if_fail (IS_APPLET_WIDGET (applet), 0);
	
	CORBA_exception_init (&env);

	retval = GNOME_PanelSpot__get_free_space (CD (applet)->pspot, &env);
	if (BONOBO_EX (&env)) {
		g_warning (_("CORBA Exception"));
		CORBA_exception_free (&env);
		return 0;
	}

	CORBA_exception_free (&env);

	return retval;
}

/**
 * applet_widget_send_position:
 * @applet: the #AppletWidget to work with
 * @enable: whether to enable or disable change_position signal
 *
 * Description:  If you need to get a signal everytime this applet changes
 * position relative to the screen, you need to run this function with %TRUE
 * for @enable and bind the change_position signal on the applet.  This signal
 * can be quite CPU/bandwidth consuming so only applets which need it should
 * use it.  By default change_position is not sent.
 **/
void
applet_widget_send_position (AppletWidget *applet,
			     gboolean      enable)
{
	CORBA_Environment env;

	g_return_if_fail (applet);
	g_return_if_fail (IS_APPLET_WIDGET (applet));
	
	CORBA_exception_init (&env);

	GNOME_PanelSpot__set_send_position (CD (applet)->pspot,
					    enable,
					    &env);
	if (BONOBO_EX (&env)) {
		g_warning (_("CORBA Exception"));
		CORBA_exception_free (&env);
		return;
	}

	CORBA_exception_free (&env);
}

/**
 * applet_widget_send_draw:
 * @applet: the #AppletWidget to work with
 * @enable: whether to enable or disable do_draw signal
 *
 * Description:  If you are using rgb background drawing, call this function
 * with %TRUE for @enable, and then bind the do_draw signal.  Inside that
 * signal you can get an RGB buffer to draw on with #applet_widget_get_rgb_bg.
 * The do_draw signal will only be sent when the RGB truly changed.
 **/
void
applet_widget_send_draw (AppletWidget *applet,
			 gboolean      enable)
{
	CORBA_Environment env;

	g_return_if_fail (applet);
	g_return_if_fail (IS_APPLET_WIDGET (applet));
	
	CORBA_exception_init (&env);
	GNOME_PanelSpot__set_send_draw (CD (applet)->pspot,
					enable,
					&env);
	if (BONOBO_EX (&env)) {
		g_warning (_("CORBA Exception"));
		CORBA_exception_free (&env);
		return;
	}

	CORBA_exception_free (&env);
}

/**
 * applet_widget_get_rgb_bg:
 * @applet: the #AppletWidget to work with
 * @rgb: pointer to a pointer to which the rgb buffer will be returned
 * @w: pointer to an integer in which the width will be stored
 * @h: pointer to an integer in which the height will be stored
 * @rowstride: pointer to an integer in which the rowstride will be stored
 *
 * Description:  Gets an rgb buffer that you can draw your applet on.  Useful
 * in conjunction with the do_draw signal and the #applet_widget_send_draw
 * method.  The rgb should be freed after use with g_free.
 **/
void
applet_widget_get_rgb_bg (AppletWidget  *applet,
			  guchar       **rgb,
			  int           *w,
			  int           *h,
			  int           *rowstride)
{
	GNOME_Panel_RgbImage *image;

	g_return_if_fail (applet);
	g_return_if_fail (IS_APPLET_WIDGET (applet));
	g_return_if_fail (rgb && w && h && rowstride);

	{
		CORBA_Environment env;
	
		CORBA_exception_init (&env);

		image = GNOME_PanelSpot__get_rgb_background (CD (applet)->pspot,
							     &env);
		if (BONOBO_EX (&env)) {
			g_warning (_("CORBA Exception"));
			CORBA_exception_free (&env);
			return;
		}

		CORBA_exception_free (&env);
	}

	*w = image->width;
	*h = image->height;
	if(!image->color_only)
		*rowstride = image->rowstride;
	else
		*rowstride = (*w)*3;

	if(image->data._buffer) {
		*rgb = g_new(guchar, (*h)*(*rowstride));
		if(!image->color_only) {
			int size = (*h)*(*rowstride);
			if(image->data._length<size)
				size = image->data._length;
			memcpy(*rgb,image->data._buffer,
			       sizeof(guchar)*size);
		} else {
			int i;
			int r;
			int g;
			int b;
			guchar *p;

			r = *(image->data._buffer);
			g = *(image->data._buffer+1);
			b = *(image->data._buffer+2);

			p = *rgb;
			for(i=0;i<(*w)*(*h);i++) {
				*(p++) = r;
				*(p++) = g;
				*(p++) = b;
			}
		}
	} else {
		/* this will make a black background */
		*rgb = g_new0(guchar, (*h)*(*rowstride));
	}
	
	CORBA_free (image);
}

/**
 * applet_widget_init:
 * @app_id: applet id
 * @app_version: applet version
 * @argc: the argc passed to main
 * @argv: the argv passed to main
 * @options: extra popt options to use
 * @flags: extra popt flags
 * @return_ctx: return popt context
 *
 * Description: Initialize the applet library, gnome and corba.
 * Don't call this if your app has an applet, but your process is not
 * simply an applet process.  This will 1) disconnect the session
 * manager and 2) setup stuff to call gtk_main_quit when the last applet
 * you create exists.  And that's all really.
 *
 * Returns: A boolean, %TRUE if we succeed, %FALSE if an error occured
 **/
gboolean
applet_widget_init (const char         *app_id,
		    const char         *app_version,
		    int                 argc,
		    char              **argv,
		    struct poptOption  *options,
		    unsigned int        flags,
		    poptContext        *return_ctx)
{

	/*
	 * this is not called for shlib applets
	 * so we set it to true here.
	 */
	die_on_last = TRUE;

	gnome_client_disable_master_connection ();

	gnome_program_init (app_id, app_version,
			    LIBGNOMEUI_MODULE,
			    argc, argv,
			    GNOME_PARAM_POPT_TABLE,
			    options, GNOME_PARAM_NONE);

	return TRUE;
}

/*****************************************************************************
  CORBA STUFF
 *****************************************************************************/

/**
 * applet_widget_gtk_main:
 *
 * Description: Run the main loop, just like #gtk_main
 **/
void
applet_widget_gtk_main(void)
{
	gtk_main();
}

/**
 * applet_widget_gtk_main_quit:
 *
 * Description: Quit the main loop, just like #gtk_main_quit
 **/
void
applet_widget_gtk_main_quit (void)
{
	gtk_main_quit();
}

/**
 * applet_widget_panel_quit:
 *
 * Description: Trigger 'Log out' on the panel.  This shouldn't be
 * used in normal applets, as it is not normal for applets to trigger
 * a logout.
 **/
void
applet_widget_panel_quit (void)
{
	CORBA_Environment env;

	CORBA_exception_init (&env);

	GNOME_Panel_quit (gnome_panel_client (&env), &env);
	if (BONOBO_EX (&env)) {
		CORBA_exception_free (&env);
		return;
	}

	CORBA_exception_free (&env);
}

/**
 * applet_widget_queue_resize:
 * @applet: #AppletWidget to work on
 *
 * Description:  For shared library applets this calls #gtk_widget_queue_resize
 * on the internal panel eventbox, for external applets this just calls this on
 * the #AppletWidget itself, but in both cases it forces a resize of the socket
 * on the panel
 **/
void
applet_widget_queue_resize(AppletWidget *applet)
{
	GtkPlug *plug;

	g_return_if_fail(applet != NULL);
	g_return_if_fail(IS_APPLET_WIDGET(applet));

	plug = GTK_PLUG(applet);

	if(applet->_priv->ebox)
		gtk_widget_queue_resize(applet->_priv->ebox);
	else
		gtk_widget_queue_resize(GTK_WIDGET(applet));
}

static void
server_applet_change_orient(PortableServer_Servant _servant,
			    const GNOME_Panel_OrientType orient,
			    CORBA_Environment *ev)
{
	CustomAppletServant *servant = (CustomAppletServant *)_servant;
	servant->appwidget->orient = orient;
	if (servant->appwidget->_priv->frozen_level > 0) {
		servant->appwidget->_priv->frozen_got_orient = TRUE;
		servant->appwidget->_priv->frozen_orient = (GNOME_Panel_OrientType)orient;
	} else {
		gtk_signal_emit(GTK_OBJECT(servant->appwidget),
				applet_widget_signals[CHANGE_ORIENT_SIGNAL],
				(GNOME_Panel_OrientType)orient);
	}
}

static void
server_applet_change_size (PortableServer_Servant _servant,
			   const CORBA_short size,
			   CORBA_Environment *ev)
{
	CustomAppletServant *servant = (CustomAppletServant *)_servant;
	servant->appwidget->size = size;
	if (servant->appwidget->_priv->frozen_level > 0) {
		servant->appwidget->_priv->frozen_got_size = TRUE;
		servant->appwidget->_priv->frozen_size = size;
	} else {
		gtk_signal_emit(GTK_OBJECT(servant->appwidget),
				applet_widget_signals[CHANGE_PIXEL_SIZE_SIGNAL],
				size);
	}
}

static void
server_applet_do_callback(PortableServer_Servant _servant,
			  const CORBA_char * callback_name,
			  CORBA_Environment *ev)
{
	CustomAppletServant *servant = (CustomAppletServant *)_servant;
	GSList *list;
	CallbackInfo *info;

	for(list = servant->callbacks;
	    list!=NULL;list = g_slist_next (list)) {
		info = (CallbackInfo *)list->data;
		if(strcmp(info->name,(char *)callback_name)==0)	{
			(*(info->func)) (servant->appwidget,
					 info->data);
			return;
		}
	}
}

/*
 * this is the new session saving call
 * and the one which should be used.
 */
static void
server_applet_save_session (PortableServer_Servant     servant,
			    const CORBA_char          *cfgpath,
			    const CORBA_char          *globcfgpath,
			    const CORBA_unsigned_long  cookie,
			    CORBA_Environment         *ev)
{
	CustomAppletServant *custom_servant = (CustomAppletServant *)servant;
	AppletWidget        *applet;
	char                *cfg;
	char                *globcfg;
	gboolean             retval = FALSE;

	cfg     = g_strdup (cfgpath);
	globcfg = g_strdup (globcfgpath);

	applet = custom_servant->appwidget;

	gtk_signal_emit (GTK_OBJECT (applet),
			 applet_widget_signals [SAVE_SESSION_SIGNAL],
			 cfg, globcfg, &retval);

	g_free (cfg);
	g_free (globcfg);

	/*
	 * retval of true would mean that the applet handled
	 * the session saving itself, therefore we pass the 
	 * reverse to the corba function
	 */
	GNOME_PanelSpot_done_session_save (CD (applet)->pspot,
					   !retval,
					   cookie,
					   ev);
}

/* 
 * this is here just that if an applet uses the 
 * new lib with the old panel it will still work.
 */
static CORBA_boolean
server_applet_session_save (PortableServer_Servant  servant,
			    const CORBA_char       *cfgpath,
			    const CORBA_char       *globcfgpath,
			    CORBA_Environment      *ev)
{
	CustomAppletServant *custom_servant = (CustomAppletServant *)servant;
	AppletWidget        *applet;
	char                *cfg;
	char                *globcfg;
	gboolean             retval = FALSE;

	cfg     = g_strdup (cfgpath);
	globcfg = g_strdup (globcfgpath);

	applet = custom_servant->appwidget;

	gtk_signal_emit (GTK_OBJECT (applet),
			 applet_widget_signals [SAVE_SESSION_SIGNAL],
			 cfg, globcfg, &retval);

	g_free (cfg);
	g_free (globcfg);

	return !retval;
}

static void
server_applet_back_change (PortableServer_Servant         servant,
			  const GNOME_Panel_BackInfoType *backing,
			  CORBA_Environment              *ev)
{
	CustomAppletServant *custom_servant = (CustomAppletServant *)servant;
	AppletWidget        *applet;
	GdkColor             color = {0, 0, 0, 0};
	GdkColor            *cptr = NULL;
	char                *pptr = NULL;

	switch (backing->_d) {
	case GNOME_Panel_BACK_COLOR:
		color.red   = backing->_u.c.red;
		color.green = backing->_u.c.green;
		color.blue  = backing->_u.c.blue;
		cptr = &color;
		break;
	case GNOME_Panel_BACK_PIXMAP:
		pptr = backing->_u.pmap;
		break;
	default:
		break;
	}

	applet = custom_servant->appwidget;

	if (applet->_priv->frozen_level > 0) {
		applet->_priv->frozen_got_back   = TRUE;
		applet->_priv->frozen_back_type  = (GNOME_Panel_BackType)backing->_d;
		applet->_priv->frozen_back_color = color;

		if (applet->_priv->frozen_back_pixmap) {
			g_free (applet->_priv->frozen_back_pixmap);
			applet->_priv->frozen_back_pixmap = g_strdup (pptr);
		}
	} 
	else {
		gtk_signal_emit (GTK_OBJECT (applet),
				 applet_widget_signals [BACK_CHANGE_SIGNAL],
				 (GNOME_Panel_BackType)backing->_d,
				 pptr, cptr);
	}
}

static void
server_applet_draw (PortableServer_Servant   servant,
		    CORBA_Environment       *ev)
{
	CustomAppletServant *custom_servant = (CustomAppletServant *)servant;

	gtk_signal_emit (GTK_OBJECT (custom_servant->appwidget),
			 applet_widget_signals [DO_DRAW_SIGNAL]);
}

static void
server_applet_set_tooltips_state (PortableServer_Servant  servant,
				  const CORBA_boolean     enabled,
				  CORBA_Environment      *ev)
{
	CustomAppletServant *custom_servant = (CustomAppletServant *)servant;

	if (enabled)
		gtk_tooltips_enable (applet_widget_tooltips ());
	else
		gtk_tooltips_disable (applet_widget_tooltips ());

	gtk_signal_emit (GTK_OBJECT (custom_servant->appwidget),
			 applet_widget_signals [TOOLTIP_STATE_SIGNAL],
			 enabled);
}

static void
server_applet_change_position (PortableServer_Servant  servant,
			       const CORBA_short       x,
			       const CORBA_short       y,
			       CORBA_Environment      *ev)
{
	AppletWidget *applet = ((CustomAppletServant *)servant)->appwidget;

	if (applet->_priv->frozen_level > 0) {
		applet->_priv->frozen_got_position = TRUE;
		applet->_priv->frozen_position_x   = x;
		applet->_priv->frozen_position_y   = y;
	}
	else {
		gtk_signal_emit (GTK_OBJECT (applet),
				 applet_widget_signals [CHANGE_POSITION_SIGNAL],
				 (int)x,
				 (int)y);
	}
}

static CORBA_char *
server_applet__get_goad_id (PortableServer_Servant  servant,
			    CORBA_Environment      *env)
{
	CustomAppletServant *custom_servant = (CustomAppletServant *)servant;

	return CORBA_string_dup (custom_servant->iid);
}

static void
server_applet_freeze_changes (PortableServer_Servant  servant,
			      CORBA_Environment      *ev)
{
	AppletWidget *applet = ((CustomAppletServant *)servant)->appwidget;

	applet->_priv->frozen_level++;
}

static void
server_applet_thaw_changes (PortableServer_Servant  servant,
			    CORBA_Environment      *ev)
{
	AppletWidget *applet = ((CustomAppletServant *)servant)->appwidget;

	if (applet->_priv->frozen_level > 0)
		applet->_priv->frozen_level--;
	
	if (applet->_priv->frozen_level > 0)
		return;

	if (applet->_priv->frozen_got_orient) {
		applet->_priv->frozen_got_orient = FALSE;

		gtk_signal_emit (GTK_OBJECT (applet),
				 applet_widget_signals [CHANGE_ORIENT_SIGNAL],
				 applet->_priv->frozen_orient);
	}

	if (applet->_priv->frozen_got_size) {
		applet->_priv->frozen_got_size = FALSE;

		gtk_signal_emit (GTK_OBJECT (applet),
				 applet_widget_signals [CHANGE_PIXEL_SIZE_SIGNAL],
				 applet->_priv->frozen_size);
	}

	if (applet->_priv->frozen_got_back) {
		applet->_priv->frozen_got_back = FALSE;

		gtk_signal_emit (GTK_OBJECT (applet),
				 applet_widget_signals[BACK_CHANGE_SIGNAL],
				 applet->_priv->frozen_back_type,
				 applet->_priv->frozen_back_pixmap,
				 &applet->_priv->frozen_back_color);

		g_free (applet->_priv->frozen_back_pixmap);
	}

	if (applet->_priv->frozen_got_position) {
		applet->_priv->frozen_got_position = FALSE;

		gtk_signal_emit (GTK_OBJECT (applet),
				 applet_widget_signals [CHANGE_POSITION_SIGNAL],
				 applet->_priv->frozen_position_x,
				 applet->_priv->frozen_position_y);
	}
}

/**
 * applet_widget_corba_activate:
 * @applet: widget to embed.
 * @poa: the POA to use.
 * @iid: the implementation id of the applet.
 * @params: params passed when the applet is activated.
 * @impl_ptr:
 * @ev: CORBA environment to use for errors.
 *
 * Description: Duplicates the applet's CORBA object.  This should
 * be called when a shared library applet is activated.
 *
 * Returns: the duplication CORBA object to use.
 **/
CORBA_Object
applet_widget_corba_activate (GtkWidget            *applet,
			      PortableServer_POA    poa,
			      const char           *iid,
			      const char          **params,
			      gpointer             *impl_ptr,
			      CORBA_Environment    *ev)
{
	return CORBA_Object_duplicate (CD (applet)->obj, ev);
}

/**
 * applet_widget_corba_deactivate:
 * @poa: the POA to use.
 * @iid: the implementation id of the applet.
 * @impl_ptr:
 * @ev: CORBA environment to use for errors.
 * 
 * Description:
 **/
void
applet_widget_corba_deactivate (PortableServer_POA  poa,
				const char         *iid,
				gpointer            impl_ptr,
				CORBA_Environment  *ev)
{
	/*
	 * FIXME: fill this in
	 */
}

typedef struct {
	POA_GNOME_GenericFactory        servant;

	AppletFactoryActivator          afunc;
	AppletFactoryQuerier            qfunc;

	CORBA_Object                    fobj;
	PortableServer_ObjectId        *objid;
} AppletFactory;

static CORBA_boolean
server_applet_factory_supports (PortableServer_Servant  servant,
				const CORBA_char       *iid,
				CORBA_Environment      *ev)
{
	AppletFactory *factory_servant = (AppletFactory *)servant;

	if (factory_servant->qfunc)
		return factory_servant->qfunc (iid);

	g_message ("No AppletFactoryQuerier to check on %s in panel applet", iid);

	return CORBA_FALSE;
}

static CORBA_Object
server_applet_factory_create_object (PortableServer_Servant  servant,
				     const CORBA_char       *iid,
				     const GNOME_stringlist *params,
				     CORBA_Environment      *ev)
{
	AppletFactory *factory_servant = (AppletFactory *)_servant;
	GtkWidget     *applet;

	applet = servant->afunc (iid, params->_buffer, params->_length);
	if (!applet) {
		g_warning (_("Activator failed to create an object."));
		return CORBA_OBJECT_NIL;
	}

	if (!IS_APPLET_WIDGET (applet)) {
		g_warning (_("Activator failed to create an AppletWidget."));
		gtk_widget_destroy (applet);
		return CORBA_OBJECT_NIL;
	}

	return CORBA_Object_duplicate (CD(applet)->obj, ev);
}

static POA_GNOME_GenericFactory__epv applet_factory_epv = {
	NULL,
	server_applet_factory_supports,
	server_applet_factory_create_object
};

static POA_GNOME_GenericFactory__vepv applet_factory_vepv = {
	&base_epv,
	&applet_factory_epv
};

/**
 * applet_factory_new:
 * @iid: the implementation id of the factory to be registered.
 * @qfunc: #AppletFactoryQuerier to determine whether an applet with
 * a specified GOAD ID can be created.
 * @afunc: #AppletFactoryActivator to activate a specified GOAD ID.
 *
 * Description: create a new applet factory.  It is used for applets
 * that can run many applets from one process.
 **/
void
applet_factory_new (const char             *iid,
		    AppletFactoryQuerier    qfunc,
		    AppletFactoryActivator  afunc)
{
	POA_GNOME_GenericFactory *factory;
	AppletFactory            *applet_factory;
	CORBA_Environment         env;
	CORBA_Object              obj;

	g_return_if_fail (afunc);

	CORBA_exception_init (&env);

	poa = bonobo_poa ();

	applet_factory = g_new0 (AppletFactory, 1);

	factory = (POA_GNOME_GenericFactory *)applet_factory

	factory->servant.vepv = &applet_factory_vepv;


	POA_GNOME_GenericFactory__init (factory, &env);

	obj = PortableServer_POA_servant_to_reference (bonobo_poa (),  
						       factory, &env);
	if (BONOBO_EX (&env)) {
		CORBA_exception_free (&env);
		g_free (applet_factory);
		return;
	}

	applet_factory->afunc = afunc;
	applet_factory->qfunc = qfunc;
	applet_factory->fobj  = obj;

	bonobo_activation_active_server_register (iid, obj);

	CORBA_Object_release (obj, &env);

	CORBA_exception_free (&env);
}
