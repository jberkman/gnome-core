#include <config.h>
#include <gtk/gtk.h>
#include <gnome.h>
#include "tasklist_applet.h"

GtkWidget *get_popup_menu (TasklistTask *task);
void add_menu_item (gchar *name, GtkWidget *menu);
gboolean cb_menu (GtkWidget *widget, gpointer data);

/* Callback for menus */
gboolean
cb_menu (GtkWidget *widget, gpointer data)
{
	g_print ("Menu Callback\n");
	return FALSE;
}

/* Open a popup menu with window operations */
void
menu_popup (TasklistTask *task, guint button, guint32 activate_time)
{
	GtkWidget *menu;
	
	menu = get_popup_menu (task);

	gtk_menu_popup (GTK_MENU (menu), NULL, NULL,
			NULL, NULL,
			button, activate_time);
}

/* Add a menu item to the popup menu */
void add_menu_item (gchar *name, GtkWidget *menu)
{
	GtkWidget *menuitem;

	menuitem = gtk_menu_item_new_with_label (name);
	gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
			    GTK_SIGNAL_FUNC (cb_menu), NULL);

	gtk_widget_show (menuitem);
	gtk_menu_append (GTK_MENU (menu), menuitem);

}

/* Create a popup menu */
GtkWidget 
*get_popup_menu (TasklistTask *task)
{
	GtkWidget *menu, *menuitem;

	menu = gtk_menu_new ();
	gtk_widget_show (menu);

	add_menu_item (GWMH_TASK_ICONIFIED (task->gwmh_task)
		       ? _("Restore") : _("Minimize"), menu);
	add_menu_item (GWMH_TASK_SHADED (task->gwmh_task)
		       ? _("Unshade") : _("Shade"), menu);
	add_menu_item (GWMH_TASK_STICKY (task->gwmh_task)
		       ? _("Unstick") : _("Stick"), menu);
	add_menu_item (_("To Desktop"), menu);
	add_menu_item (_("Close"), menu);

	menuitem = gtk_menu_item_new ();
	gtk_widget_show (menuitem);
	gtk_menu_append (GTK_MENU (menu), menuitem);
	
	add_menu_item (_("Kill"), menu);
	
	return menu;
}
