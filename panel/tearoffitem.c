#include "config.h"

#include <libgnome/libgnome.h>
#include <gconf/gconf-client.h>

#include "main.h"
#include "tearoffitem.h"

static void tearoff_item_class_init	(TearoffItemClass	*klass);
static void tearoff_item_init		(TearoffItem	*tearoff_menu_item);

GType
tearoff_item_get_type (void)
{
	static GType tearoff_menu_item_type = 0;

	if (!tearoff_menu_item_type) {
		static const GtkTypeInfo tearoff_menu_item_info = {
			"TearoffItem",
			sizeof (TearoffItem),
			sizeof (TearoffItemClass),
			(GtkClassInitFunc) tearoff_item_class_init,
			(GtkObjectInitFunc) tearoff_item_init,
			/* reserved_1 */ NULL,
			/* reserved_2 */ NULL,
			(GtkClassInitFunc) NULL,
		};

		tearoff_menu_item_type =
			gtk_type_unique(gtk_tearoff_menu_item_get_type(),
					&tearoff_menu_item_info);
	}

	return tearoff_menu_item_type;
}

GtkWidget*
tearoff_item_new (void)
{
	g_return_val_if_fail (gconf_client_get_bool (panel_main_gconf_client (),
						     "/desktop/gnome/interface/menus-have-tearoff",
						     NULL),
			      NULL);

	return GTK_WIDGET (gtk_type_new(tearoff_item_get_type()));
}

static void
tearoff_item_class_init(TearoffItemClass *klass)
{
	GtkMenuItemClass *menu_item_class;

	menu_item_class = (GtkMenuItemClass*) klass;

	menu_item_class->activate = NULL;
}

static void
tearoff_item_init(TearoffItem *tearoff_menu_item)
{
}
