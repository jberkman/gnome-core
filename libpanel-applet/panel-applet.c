/*
 * panel-applet.c: panel applet writing library.
 *
 * Copyright (C) 2001 Sun Microsystems, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Authors:
 *     Mark McLoughlin <mark@skynet.ie>
 */

#include <unistd.h>
#include <string.h>
#include <bonobo/bonobo-ui-util.h>
#include <bonobo/bonobo-types.h>
#include <bonobo/bonobo-property-bag.h>
#include <bonobo/bonobo-item-handler.h>
#include <bonobo/bonobo-shlib-factory.h>
#include <libgnome/gnome-program.h>
#include <libgnomeui/gnome-ui-init.h>
#include <gconf/gconf.h>
#include <gconf/gconf-client.h>

#include "panel-applet.h"
#include "panel-applet-private.h"
#include "panel-applet-shell.h"
#include "panel-applet-marshal.h"
#include "panel-applet-enums.h"

struct _PanelAppletPrivate {
	PanelAppletShell           *shell;
	BonoboControl              *control;
	BonoboItemHandler          *item_handler;

	gchar                      *iid;
	GClosure                   *closure;
	gboolean                    bound;
	gchar                      *prefs_key;

	PanelAppletOrient           orient;
	guint                       size;
	gchar                      *background;

	gboolean                    expand_major;
	gboolean                    expand_minor;
};

static GObjectClass *parent_class;

enum {
        CHANGE_ORIENT,
        CHANGE_SIZE,
        CHANGE_BACKGROUND,
        SAVE_YOURSELF,
        LAST_SIGNAL
};

static guint panel_applet_signals [LAST_SIGNAL];

#define PROPERTY_ORIENT     "panel-applet-orient"
#define PROPERTY_SIZE       "panel-applet-size"
#define PROPERTY_BACKGROUND "panel-applet-background"

enum {
	PROPERTY_ORIENT_IDX,
	PROPERTY_SIZE_IDX,
	PROPERTY_BACKGROUND_IDX
};

static void
panel_applet_associate_schemas_in_dir (GConfEngine  *engine,
				       const gchar  *prefs_key,
				       const gchar  *schema_dir,
				       GError      **error)
{
	GSList *list;

	list = gconf_engine_all_entries (engine, schema_dir, error);

	g_return_if_fail (list && !*error);

	for (; list; list = list->next) {
		GConfEntry *entry = list->data;
		gchar      *key;
		gchar      *tmp;

		tmp = g_path_get_basename (gconf_entry_get_key (entry));

		key = g_strdup_printf ("%s/%s", prefs_key, tmp);

		g_free (tmp);

		gconf_engine_associate_schema (engine, key, gconf_entry_get_key (entry), error);

		g_free (key);

		g_return_if_fail (list && !*error);
	}

	list = gconf_engine_all_dirs (engine, schema_dir, error);

	for (; list; list = list->next) {
		gchar *subdir = list->data;
		gchar *prefs_subdir;
		gchar *schema_subdir;

		prefs_subdir  = g_strdup_printf ("%s/%s", prefs_key, subdir);
		schema_subdir = g_strdup_printf ("%s/%s", schema_dir, subdir);

		panel_applet_associate_schemas_in_dir (engine, prefs_subdir, schema_subdir, error);

		g_free (prefs_subdir);
		g_free (schema_subdir);
		g_free (subdir);

		g_return_if_fail (list && !*error);
	}
}

void
panel_applet_add_preferences (PanelApplet  *applet,
			      const gchar  *schema_dir,
			      GError      **opt_error)
{
	GConfEngine  *engine;
	GConfClient  *client;
	GError      **error = NULL;
	GError       *our_error = NULL;

	g_return_if_fail (applet && PANEL_IS_APPLET (applet));

	if (opt_error)
		error = opt_error;
	else
		error = &our_error;

	engine = gconf_engine_get_default ();

	panel_applet_associate_schemas_in_dir (engine,
					       applet->priv->prefs_key,
					       schema_dir,
					       error);

	if (!opt_error && our_error) {
		g_warning (G_STRLOC ": failed to add preferences from '%s' : '%s'",
			   schema_dir, our_error->message);
		g_error_free (our_error);
	}

	client = gconf_client_get_default ();

	gconf_client_add_dir (client,
			      applet->priv->prefs_key,
			      GCONF_CLIENT_PRELOAD_NONE,
			      NULL);
}

gchar *
panel_applet_get_preferences_key (PanelApplet *applet)
{
	g_return_val_if_fail (applet && PANEL_IS_APPLET (applet), NULL);

	if (!applet->priv->prefs_key)
		return NULL;

	return g_strdup (applet->priv->prefs_key);
}

void
panel_applet_get_expand_flags (PanelApplet *applet,
			       gboolean    *expand_major,
			       gboolean    *expand_minor)
{
	g_return_if_fail (applet && PANEL_IS_APPLET (applet));

	*expand_major = applet->priv->expand_major;
	*expand_minor = applet->priv->expand_minor;
}

void
panel_applet_set_expand_flags (PanelApplet *applet,
			       gboolean     expand_major,
			       gboolean     expand_minor)
{
	g_return_if_fail (applet && PANEL_IS_APPLET (applet));

	applet->priv->expand_major = expand_major;
	applet->priv->expand_minor = expand_minor;
}

/**
 * panel_applet_get_size:
 * @applet: A #PanelApplet.
 *
 * Returns the current size of the panel which
 * the applet is contained in.
 *
 * Return value: a #guint value.
 */
guint
panel_applet_get_size (PanelApplet *applet)
{
	g_return_val_if_fail (applet && PANEL_IS_APPLET (applet), 0);

	return applet->priv->size;
}

/**
 * panel_applet_get_orient
 * @applet: A #PanelApplet.
 *
 * Returns the current orientation of the panel which
 * the applet is contained in.
 *
 * Return value: a #PanelAppletOrient value.
 */
PanelAppletOrient
panel_applet_get_orient (PanelApplet *applet)
{
	g_return_val_if_fail (applet && PANEL_IS_APPLET (applet), 0);

	return applet->priv->orient;
}

/**
 * panel_applet_setup_menu:
 * @applet: A #PanelApplet.
 * @xml: The xml character string describing the popup menu.
 * @verb_list: The list of #BonoboUIVerbs for the menu.
 * @user_data: The user data pointer for the menu.
 *
 * Sets up a popup menu for @applet described by the xml 
 * string, @xml. See <link linkend="applet-writing">Applet Writing
 * </link> section for a description of the format of the xml.
 */
void
panel_applet_setup_menu (PanelApplet        *applet,
			 const gchar        *xml,
			 const BonoboUIVerb *verb_list,
			 gpointer            user_data)
{
	BonoboUIComponent *popup_component;

	g_return_if_fail (applet && xml && verb_list);

	popup_component = panel_applet_get_popup_component (applet);

	bonobo_ui_component_set (popup_component, "/", "<popups/>", NULL);

	bonobo_ui_component_set_translate (popup_component, "/popups", xml, NULL);

	bonobo_ui_component_add_verb_list_with_data (popup_component, verb_list, user_data);
}

/**
 * panel_applet_setup_menu_from_file:
 * @applet: A #PanelApplet.
 * @opt_datadir: The data directory - i.e. ${prefix}/share (optional).
 * @file: The file's name.
 * @opt_app_name: The application's name (optional).
 * @verb_list: The list of #BonoboUIVerbs for the menu.
 * @user_data: The user data pointer for the menu.
 *
 * Sets up a popup menu for @applet described by the xml 
 * file, @file. See &applet-writing for a description of
 * the format of the xml.
 */
void
panel_applet_setup_menu_from_file (PanelApplet        *applet, 
				   const gchar        *opt_datadir,
				   const gchar        *file,
				   const gchar        *opt_app_name,
				   const BonoboUIVerb *verb_list,
				   gpointer            user_data)
{
	BonoboUIComponent *popup_component;
	gchar             *app_name = NULL;

	g_return_if_fail (applet && file && verb_list);

	if (!opt_datadir)
		opt_datadir = GNOME_DATADIR;

	if (!opt_app_name)
		opt_app_name = app_name = g_strdup_printf ("%d", getpid ());

	popup_component = panel_applet_get_popup_component (applet);

	bonobo_ui_util_set_ui (popup_component, opt_datadir, file, opt_app_name, NULL);

	bonobo_ui_component_add_verb_list_with_data (popup_component, verb_list, user_data);

	if (app_name)
		g_free (app_name);
}

/**
 * panel_applet_get_control:
 * @applet: A #PanelApplet.
 *
 * Retrieves the #BonoboControl associated with @applet.
 *
 * Return value: A #BonobControl on success, %NULL on failure.
 */
BonoboControl *
panel_applet_get_control (PanelApplet *applet)
{
	g_return_val_if_fail (applet && PANEL_IS_APPLET (applet), NULL);

	return applet->priv->control;
}

/**
 * panel_applet_get_popup_component:
 * @applet: A #PanelApplet.
 *
 * Retrieves the #BonoboUIComponent used for popup menus associated
 * with @applet. 
 *
 * Return value: A #BonoboUIComponent on success, or %NULL on failure.
 */
BonoboUIComponent *
panel_applet_get_popup_component (PanelApplet *applet)
{
	g_return_val_if_fail (applet && PANEL_IS_APPLET (applet), NULL);

	return bonobo_control_get_popup_ui_component (applet->priv->control);
}

static void
panel_applet_finalize (GObject *object)
{
	PanelApplet *applet = PANEL_APPLET (object);

	if (applet->priv->prefs_key)
		g_free (applet->priv->prefs_key);

	if (applet->priv->iid)
		g_free (applet->priv->iid);

	g_free (applet->priv);
	applet->priv = NULL;

	parent_class->finalize (object);
}

static gboolean
panel_applet_button_press (GtkWidget      *widget,
			   GdkEventButton *event)
{
	PanelApplet *applet = PANEL_APPLET (widget);

	if (event->button == 3) {
		bonobo_control_do_popup (applet->priv->control,
					 event->button,
					 event->time);
		return TRUE;
	}

	return FALSE;
}

static gboolean
panel_applet_parse_color (const gchar *color_str,
			   GdkColor    *color)
{
	int r, g, b;

	g_assert (color_str && color);

	if (color_str [0] != '#')
		return FALSE;

	if (sscanf (color_str + 1, "%4x%4x%4x", &r, &g, &b) != 3)
		return FALSE;

	color->red   = r;
	color->green = g;
	color->blue  = b;
		
	return TRUE;
}

static void
panel_applet_get_prop (BonoboPropertyBag *sack,
                       BonoboArg         *arg,
		       guint              arg_id,
		       CORBA_Environment *ev,
		       gpointer           user_data)
{
	PanelApplet *applet = PANEL_APPLET (user_data);

	switch (arg_id) {
	case PROPERTY_ORIENT_IDX:
		BONOBO_ARG_SET_SHORT (arg, applet->priv->orient);
		break;
	case PROPERTY_SIZE_IDX:
		BONOBO_ARG_SET_SHORT (arg, applet->priv->size);
		break;
	case PROPERTY_BACKGROUND_IDX:
		BONOBO_ARG_SET_STRING (arg, applet->priv->background);
		break;
	default:
		g_assert_not_reached ();
		break;
	}
}

static void
panel_applet_set_prop (BonoboPropertyBag *sack,
		       const BonoboArg   *arg,
		       guint              arg_id,
		       CORBA_Environment *ev,
		       gpointer           user_data)
{
	PanelApplet *applet = PANEL_APPLET (user_data);
	
	switch (arg_id) {
	case PROPERTY_ORIENT_IDX: {
		PanelAppletOrient orient;

		orient = BONOBO_ARG_GET_SHORT (arg);

		if (applet->priv->orient != orient) {
			applet->priv->orient = orient;

			g_signal_emit (G_OBJECT (applet),
				       panel_applet_signals [CHANGE_ORIENT],
				       0, orient);
		}
		}
		break;
	case PROPERTY_SIZE_IDX: {
		guint size;

		size = BONOBO_ARG_GET_SHORT (arg);

		if (applet->priv->size != size) {
			applet->priv->size = size;

			g_signal_emit (G_OBJECT (applet),
                                       panel_applet_signals [CHANGE_SIZE],
                                       0, size);
		}
		}
		break;
	case PROPERTY_BACKGROUND_IDX: {
		gchar  *bg_str;
		gchar **elements;

		bg_str = BONOBO_ARG_GET_STRING (arg);

		elements = g_strsplit (bg_str, ":", -1);

		if (elements [0] && !strcmp (elements [0], "none" )) {
			applet->priv->background = NULL;

			g_signal_emit (G_OBJECT (applet),
				       panel_applet_signals [CHANGE_BACKGROUND],
				       0, PANEL_NO_BACKGROUND, NULL, NULL);
		
		} else if (elements [0] && !strcmp (elements [0], "color")) {
			GdkColor color;

			if (!elements [1] || !panel_applet_parse_color (elements [1], &color)) {

				g_warning (_("panel_applet_set_prop: Incomplete '%s'"
					     " background type received"), elements [0]);

				g_strfreev (elements);
				return;
			}

			applet->priv->background = bg_str;

			g_signal_emit (G_OBJECT (applet),
				       panel_applet_signals [CHANGE_BACKGROUND],
				       0, PANEL_COLOR_BACKGROUND, &color, NULL);

		} else if (elements [0] && !strcmp (elements [0], "pixmap")) {
			gchar *pixmap = elements [1];

			if (!pixmap) {
				g_warning (_("panel_applet_set_prop: Incomplete '%s'"
					     " background type received"), elements [0]);

				g_strfreev (elements);
				return;
			}

			applet->priv->background = bg_str;

			g_signal_emit (G_OBJECT (applet),
				       panel_applet_signals [CHANGE_BACKGROUND],
				       0, PANEL_PIXMAP_BACKGOUND, NULL, pixmap);

		} else {
			g_warning (_("panel_applet_set_prop: Unknown backgound type received"));
			return;
		}

		g_strfreev (elements);
		}
		break;
	default:
		g_assert_not_reached ();
		break;
	}
}

static BonoboPropertyBag *
panel_applet_property_bag (PanelApplet *applet)
{
	BonoboPropertyBag *sack;

	sack = bonobo_property_bag_new (panel_applet_get_prop, 
					panel_applet_set_prop,
					applet);

	bonobo_property_bag_add (sack,
				 PROPERTY_ORIENT,
				 PROPERTY_ORIENT_IDX,
				 BONOBO_ARG_SHORT,
				 NULL,
				 _("The Applet's containing Panel's orientation"),
				 Bonobo_PROPERTY_READABLE | Bonobo_PROPERTY_WRITEABLE);

	bonobo_property_bag_add (sack,
				 PROPERTY_SIZE,
				 PROPERTY_SIZE_IDX,
				 BONOBO_ARG_SHORT,
				 NULL,
				 _("The Applet's containing Panel's size in pixels"),
				 Bonobo_PROPERTY_READABLE | Bonobo_PROPERTY_WRITEABLE);

	bonobo_property_bag_add (sack,
				 PROPERTY_BACKGROUND,
				 PROPERTY_BACKGROUND_IDX,
				 BONOBO_ARG_STRING,
				 NULL,
				 _("The Applet's containing Panel's background color or pixmap"),
				 Bonobo_PROPERTY_READABLE | Bonobo_PROPERTY_WRITEABLE);

	return sack;
}

static void
panel_applet_control_bound (BonoboControl *control,
			    PanelApplet   *applet)
{
	gboolean ret;

	g_return_if_fail (applet && PANEL_IS_APPLET (applet));
	g_return_if_fail (applet->priv->iid && applet->priv->closure);

	if (applet->priv->bound)
		return;

	bonobo_closure_invoke (applet->priv->closure,
			       G_TYPE_BOOLEAN, &ret,
			       PANEL_TYPE_APPLET, applet,
			       G_TYPE_STRING, applet->priv->iid,
			       0);


	if (!ret) { /* FIXME */
		g_warning ("need to free the control here");

		return;
	}

	applet->priv->bound = TRUE;
}

static Bonobo_Unknown
panel_applet_item_handler_get_object (BonoboItemHandler *handler,
				      const char        *item_name,
				      gboolean           only_if_exists,
				      gpointer           user_data,
				      CORBA_Environment *ev)
{
	PanelApplet *applet = user_data;
	GSList      *options;
	GSList      *l;

	g_return_val_if_fail (applet && PANEL_IS_APPLET (applet), CORBA_OBJECT_NIL);

	options = bonobo_item_option_parse (item_name);

	for (l = options; l; l = l->next) {
		BonoboItemOption *option = l->data;

		if (!strcmp (option->key, "prefs_key") && !applet->priv->prefs_key &&
		    option->value && option->value [0])
			applet->priv->prefs_key = g_strdup (option->value);
	}

	bonobo_item_options_free (options);

	return bonobo_object_dup_ref (BONOBO_OBJREF (applet->priv->control), ev);
}
			   
static void
panel_applet_class_init (PanelAppletClass *klass,
			 gpointer          dummy)
{
	GObjectClass   *gobject_class = (GObjectClass *) klass;
	GtkWidgetClass *widget_class = (GtkWidgetClass *) klass;

	parent_class = g_type_class_peek_parent (klass);

	widget_class->button_press_event = panel_applet_button_press;

	gobject_class->finalize = panel_applet_finalize;

	panel_applet_signals [CHANGE_ORIENT] =
                g_signal_new ("change_orient",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (PanelAppletClass, change_orient),
                              NULL,
			      NULL,
                              panel_applet_marshal_VOID__INT,
                              G_TYPE_NONE,
			      1,
			      G_TYPE_INT);

	panel_applet_signals [CHANGE_SIZE] =
                g_signal_new ("change_size",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (PanelAppletClass, change_size),
                              NULL,
			      NULL,
                              panel_applet_marshal_VOID__INT,
                              G_TYPE_NONE,
			      1,
			      G_TYPE_INT);

	panel_applet_signals [CHANGE_BACKGROUND] =
                g_signal_new ("change_background",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (PanelAppletClass, change_background),
                              NULL,
			      NULL,
                              panel_applet_marshal_VOID__ENUM_POINTER_STRING,
                              G_TYPE_NONE,
			      3,
			      PANEL_TYPE_PANEL_APPLET_BACKGROUND_TYPE,
			      G_TYPE_POINTER,
			      G_TYPE_STRING);
}

static void
panel_applet_instance_init (PanelApplet      *applet,
			    PanelAppletClass *klass)
{
	applet->priv = g_new0 (PanelAppletPrivate, 1);

	applet->priv->bound        = FALSE;

	applet->priv->orient       = PANEL_APPLET_ORIENT_UP;
	applet->priv->size         = GNOME_Vertigo_PANEL_MEDIUM;

	applet->priv->expand_major = FALSE;
	applet->priv->expand_minor = FALSE;

	gtk_widget_set_events (GTK_WIDGET (applet), 
			       GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
}

GType
panel_applet_get_type (void)
{
	static GType type = 0;

	if (!type) {
		static const GTypeInfo info = {
			sizeof (PanelAppletClass),
			NULL,
			NULL,
			(GClassInitFunc) panel_applet_class_init,
			NULL,
			NULL,
			sizeof (PanelApplet),
			0,
			(GInstanceInitFunc) panel_applet_instance_init,
			NULL
		};

		type = g_type_register_static (GTK_TYPE_EVENT_BOX, "PanelApplet",
					       &info, 0);
	}

	return type;
}

void
panel_applet_construct (PanelApplet *applet)
{
	PanelAppletPrivate *priv;

	priv = applet->priv;

	priv->control = bonobo_control_new (GTK_WIDGET (applet));

	g_signal_connect (G_OBJECT (priv->control),
			  "set_frame",
			  G_CALLBACK (panel_applet_control_bound),
			  applet);

	bonobo_control_set_properties (priv->control,
				       BONOBO_OBJREF (panel_applet_property_bag (applet)),
				       NULL);

	priv->shell = panel_applet_shell_new (applet);

	bonobo_object_add_interface (BONOBO_OBJECT (priv->control),
				     BONOBO_OBJECT (priv->shell));

	priv->item_handler = bonobo_item_handler_new (NULL,
						      panel_applet_item_handler_get_object,
						      applet);

	bonobo_object_add_interface (BONOBO_OBJECT (priv->control),
				     BONOBO_OBJECT (priv->item_handler));
}

/**
 * panel_applet_new:
 * @widget: The widget the contains all the widgetry the applet
 *          wishes to expose.
 *
 * Creates a new #PanelApplet which exposes @widget.
 *
 * Return value: A #GtkWidget on success, %NULL on failure.
 */
GtkWidget *
panel_applet_new (void)
{
	PanelApplet *applet;

	applet = g_object_new (PANEL_TYPE_APPLET, NULL);

	panel_applet_construct (applet);

	return GTK_WIDGET (applet);
}

static BonoboObject *
panel_applet_factory_callback (BonoboGenericFactory *factory,
			       const char           *iid,
			       gpointer              user_data)
{
	PanelApplet        *applet;

	applet = PANEL_APPLET (panel_applet_new ());

	applet->priv->iid       = g_strdup (iid);
	applet->priv->closure   = g_closure_ref (user_data);

	return BONOBO_OBJECT (applet->priv->control);
}

/**
 * panel_applet_factory_main_closure:
 * @argc: The number of commmand line arguments contained in @argv.
 * @argv: The array of command line argument strings.
 * @iid: The bonobo-activation iid of the factory.
 * @name: The applet ID string.
 * @version: The applet version string.
 * @closure: The factory callback closure.
 *
 * A generic 'main' routine for applets. This should not normally be
 * used directly because it is invoked by #PANEL_APPLET_BONOBO_FACTORY.
 *
 * Return value: 0 on success, 1 on failure.
 */
int
panel_applet_factory_main_closure (int                          argc,
				   char                       **argv,
				   const gchar                 *iid,
				   const gchar                 *name,
				   const gchar                 *version,
				   GClosure                    *closure)
{
	GnomeProgram       *program;
	int                 retval;

	/* FIXME: other precondition checks */

	/* FIXME: EVIL, what about the per app domain things and all that,
	 * we shouldn't really be calling gnome_program_init here, the applet
	 * should call it itself.  We need to fix this */
	program = gnome_program_init (name, version,
				      LIBGNOMEUI_MODULE,
				      argc, argv,
				      GNOME_PARAM_NONE);

	closure = bonobo_closure_store (closure, panel_applet_marshal_BOOLEAN__STRING);

	retval = bonobo_generic_factory_main (iid, panel_applet_factory_callback, closure);

	g_closure_unref (closure);

	return retval;
}

/**
 * panel_applet_factory_main:
 * @argc: The number of commmand line arguments contained in @argv.
 * @argv: The array of command line argument strings.
 * @iid: The bonobo-activation iid of the factory.
 * @name: The applet ID string.
 * @version: The applet version string.
 * @callback: The factory callback.
 * @data: The factory user data pointer.
 *
 * A generic 'main' routine for applets. This should not normally be
 * used directly because it is invoked by #PANEL_APPLET_BONOBO_FACTORY.
 *
 * Return value: 0 on success, 1 on failure.
 */
int
panel_applet_factory_main (int                          argc,
			   char                       **argv,
			   const gchar                 *iid,
			   const gchar                 *name,
			   const gchar                 *version,
			   PanelAppletFactoryCallback   callback,
			   gpointer                     data)
{
	GClosure *closure;

	/* FIXME: precondition checks */

	closure = g_cclosure_new (G_CALLBACK (callback), data, NULL);

	return panel_applet_factory_main_closure (argc, argv, iid, name,
						  version, closure);
}

Bonobo_Unknown
panel_applet_shlib_factory_closure (const char                 *iid,
				    PortableServer_POA          poa,
				    gpointer                    impl_ptr,
				    GClosure                   *closure,
				    CORBA_Environment          *ev)
{
	BonoboShlibFactory *factory;

	/* FIXME: other precondition checks */
	g_return_val_if_fail (closure != NULL, CORBA_OBJECT_NIL);

	closure = bonobo_closure_store (closure, panel_applet_marshal_BOOLEAN__STRING);
       
	factory = bonobo_shlib_factory_new_closure
		(iid, poa, impl_ptr,
		 g_cclosure_new (G_CALLBACK (panel_applet_factory_callback),
				 closure,
				 (GClosureNotify)g_closure_unref));

        return CORBA_Object_duplicate (BONOBO_OBJREF (factory), ev);
}

Bonobo_Unknown
panel_applet_shlib_factory (const char                 *iid,
			    PortableServer_POA          poa,
			    gpointer                    impl_ptr,
			    PanelAppletFactoryCallback  callback,
			    gpointer                    user_data,
			    CORBA_Environment          *ev)
{
	/* FIXME: precondition checks */

	return panel_applet_shlib_factory_closure
		(iid, poa, impl_ptr,
		 g_cclosure_new (G_CALLBACK (callback),
				 user_data, NULL),
		 ev);
}
