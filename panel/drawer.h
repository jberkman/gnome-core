#ifndef DRAWER_H
#define DRAWER_H

#include "panel.h"

BEGIN_GNOME_DECLS

#define DRAWER_PANEL_KEY "drawer_panel"

typedef struct {
	char *pixmap;
	char *tooltip;
	GtkWidget *button;
	GtkWidget *drawer;
	PanelOrientType orient;
} Drawer;

void load_drawer_applet(char *params, char *pixmap, char *tooltip,
			PanelWidget *panel, int pos);

void set_drawer_applet_orient(Drawer *drawer, PanelOrientType orient);

void reposition_drawer(Drawer *drawer);
void add_drawer_properties_page(GtkWidget *dialog, Drawer *drawer);

END_GNOME_DECLS

#endif
