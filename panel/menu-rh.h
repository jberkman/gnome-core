#ifndef MENU_RH_H
#define MENU_RH_H

G_BEGIN_DECLS

void create_rh_menu(int dofork);
void rh_submenu_to_display(GtkWidget *menuw, GtkMenuItem *menuitem);

#define REDHAT_MENUDIR "/etc/X11/wmconfig"

G_END_DECLS

#endif

