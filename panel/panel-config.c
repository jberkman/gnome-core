/* GNOME panel:   Individual panel configurations
 *
 * (C) 1998 the Free Software Foundation
 * Copyright 2000 Helix Code, Inc.
 *
 * Authors: Jacob Berkman
 *          George Lebl
 */

#include <config.h>

#include <gtk/gtk.h>

#include <string.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <libart_lgpl/art_misc.h>
#include <libart_lgpl/art_affine.h>
#include <libart_lgpl/art_filterlevel.h>

#include <libgnome/libgnome.h>
#include <libgnomeui/libgnomeui.h>

#include "panel-config.h"

#include "aligned-widget.h"
#include "basep-widget.h"
#include "border-widget.h"
#include "drawer-widget.h"
#include "edge-widget.h"
#include "floating-widget.h"
#include "sliding-widget.h"
#include "panel.h"
#include "multiscreen-stuff.h"

/*
 * FIXME
 */
#include "nothing.cP"

static void config_apply (PerPanelConfig *ppc);

static GList *ppconfigs = NULL;

/* register changes */
void
panel_config_register_changes (PerPanelConfig *ppc)
{
	if (ppc->register_changes) {
		config_apply (ppc);
		if (ppc->update_function != NULL)
			ppc->update_function (ppc->update_data);
	}
}

static PerPanelConfig *
get_config_struct(GtkWidget *panel)
{
	GList *list;
	for (list = ppconfigs; list != NULL; list = list->next) {
		PerPanelConfig *ppc = list->data;
		if (ppc->panel == panel)
			return ppc;
	}
	return NULL;
}

void
kill_config_dialog (GtkWidget *panel)
{
	PerPanelConfig *ppc;

	g_return_if_fail (panel != NULL);
	g_return_if_fail (GTK_IS_WIDGET (panel));

	ppc = get_config_struct (panel);
	if (ppc != NULL &&
	    ppc->config_window != NULL)
		gtk_widget_destroy (ppc->config_window);
}

static void
update_position_toggles (PerPanelConfig *ppc)
{
	GtkWidget *toggle = ppc->toggle[ppc->edge][ppc->align];

	/* this could happen during type changes */
	if (toggle == NULL)
		return;

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), TRUE);
}

void
update_config_edge (BasePWidget *panel)
{
	PerPanelConfig *ppc;

	g_return_if_fail (panel != NULL);
	g_return_if_fail (GTK_IS_WIDGET (panel));

	ppc = get_config_struct (GTK_WIDGET (panel));

	if (ppc == NULL ||
	    ppc->ppc_origin_change)
		return;

	g_return_if_fail (BORDER_IS_WIDGET (panel));

	if (ppc->edge == BORDER_POS (panel->pos)->edge)
		return;

	ppc->edge = BORDER_POS (panel->pos)->edge;
	update_position_toggles (ppc);
}

void
update_config_floating_pos (BasePWidget *panel)
{
	PerPanelConfig *ppc;

	g_return_if_fail (panel != NULL);
	g_return_if_fail (GTK_IS_WIDGET (panel));

	ppc = get_config_struct (GTK_WIDGET (panel));

	if (ppc == NULL ||
	    ppc->ppc_origin_change)
		return;

	gtk_spin_button_set_value (GTK_SPIN_BUTTON (ppc->x_spin),
				   FLOATING_POS (panel->pos)->x);

	gtk_spin_button_set_value (GTK_SPIN_BUTTON (ppc->y_spin),
				   FLOATING_POS (panel->pos)->y);
}

void
update_config_floating_pos_limits (BasePWidget *panel)
{
	GtkWidget *widget;
	PerPanelConfig *ppc;
	int xlimit, ylimit;
	int val;
	GtkAdjustment *adj;

	g_return_if_fail (panel != NULL);
	g_return_if_fail (GTK_IS_WIDGET (panel));

	widget = GTK_WIDGET (panel);
	ppc = get_config_struct (widget);

	if (ppc == NULL ||
	    ppc->ppc_origin_change)
		return;

	xlimit = multiscreen_width(panel->screen) - widget->allocation.width;
	ylimit = multiscreen_height(panel->screen) - widget->allocation.height;

	adj = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON (ppc->x_spin));
	if((int)adj->upper == xlimit) {
		adj = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON (ppc->y_spin));
		if((int)adj->upper == ylimit)
			return;
	}


	val = FLOATING_POS (panel->pos)->x;
	if(val > xlimit)
		val = xlimit;
	adj = GTK_ADJUSTMENT(gtk_adjustment_new (val, 0, xlimit, 1, 10, 10));
	gtk_spin_button_set_adjustment (GTK_SPIN_BUTTON (ppc->x_spin), adj);
	gtk_adjustment_value_changed(adj);

	val = FLOATING_POS (panel->pos)->y;
	if(val > ylimit)
		val = ylimit;
	adj = GTK_ADJUSTMENT(gtk_adjustment_new (val, 0, ylimit, 1, 10, 10));
	gtk_spin_button_set_adjustment (GTK_SPIN_BUTTON (ppc->y_spin), adj);
	gtk_adjustment_value_changed(adj);
}

void
update_config_floating_orient (BasePWidget *panel)
{
	PerPanelConfig *ppc = get_config_struct (GTK_WIDGET (panel));
	GtkWidget *toggle;

	if (ppc == NULL ||
	    ppc->ppc_origin_change)
		return;

	toggle = (PANEL_WIDGET (panel->panel)->orient == GTK_ORIENTATION_HORIZONTAL)
		? ppc->h_orient : ppc->v_orient;

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
				      TRUE);
}

void
update_config_screen (BasePWidget *w)
{
	PerPanelConfig *ppc = get_config_struct (GTK_WIDGET (w));

	if (ppc == NULL ||
	    ppc->ppc_origin_change)
		return;

	gtk_spin_button_set_value (GTK_SPIN_BUTTON (ppc->screen_spin),
				   w->screen);

	if (FLOATING_IS_WIDGET (w))
		update_config_floating_pos_limits (w);
	else if (SLIDING_IS_WIDGET (w))
		update_config_offset_limit (w);
}

void
update_config_size (GtkWidget *panel)
{
	PerPanelConfig *ppc = get_config_struct(panel);
	int i;
	PanelWidget *p;
	GtkWidget *menuitem;

	if (ppc == NULL ||
	    ppc->ppc_origin_change)
		return;

	p = PANEL_WIDGET(BASEP_WIDGET(panel)->panel);
	switch(p->sz) {
	case PANEL_SIZE_XX_SMALL:
		i = 0;
		break;
	case PANEL_SIZE_X_SMALL:
		i = 1;
		break;
	case PANEL_SIZE_SMALL:
		i = 2;
		break;
	default:
	case PANEL_SIZE_MEDIUM:
		i = 3;
		break;
	case PANEL_SIZE_LARGE:
		i = 4;
		break;
	case PANEL_SIZE_X_LARGE:
		i = 5;
		break;
	case PANEL_SIZE_XX_LARGE:
		i = 6;
		break;
	}
	
	gtk_option_menu_set_history (GTK_OPTION_MENU (ppc->size_menu), i);
	menuitem = g_list_nth_data(GTK_MENU_SHELL(GTK_OPTION_MENU(ppc->size_menu)->menu)->children, i);
	gtk_menu_item_activate(GTK_MENU_ITEM(menuitem));
}

void
update_config_back (PanelWidget *pw)
{
	GtkWidget *item = NULL;
	int history = 0;
	PerPanelConfig *ppc;
	
	g_return_if_fail (pw);
	g_return_if_fail (PANEL_IS_WIDGET(pw));
	g_return_if_fail (pw->panel_parent);

	ppc = get_config_struct (pw->panel_parent);

	if (ppc == NULL ||
	    ppc->ppc_origin_change)
		return;

	switch(pw->back_type) {
	default:
	case PANEL_BACK_NONE:
		item = ppc->non;
		history = 0;
		break;
	case PANEL_BACK_COLOR:
		gnome_color_picker_set_i16(GNOME_COLOR_PICKER(ppc->backsel),
					   pw->back_color.red,
					   pw->back_color.green,
					   pw->back_color.blue,
					   65535);
		item = ppc->col;
		history = 1;
		break;
	case PANEL_BACK_PIXMAP: {
		GtkWidget   *t;
		const gchar *text;

		t = gnome_file_entry_gtk_entry (GNOME_FILE_ENTRY (ppc->pix_entry));

		text = gtk_entry_get_text (GTK_ENTRY (t));

		if (strcmp (sure_string (pw->back_pixmap), text))
			gtk_entry_set_text (GTK_ENTRY (t),
					    sure_string (pw->back_pixmap));

		item = ppc->pix;
		history = 2;
		}
		break;
	}

	gtk_option_menu_set_history(GTK_OPTION_MENU(ppc->back_om), history);
	gtk_menu_item_activate(GTK_MENU_ITEM(item));
}

void
update_config_anchor (BasePWidget *w)
{
	PerPanelConfig *ppc = get_config_struct (GTK_WIDGET (w));
	g_return_if_fail (SLIDING_IS_WIDGET (w));

	if (ppc == NULL ||
	    ppc->ppc_origin_change)
		return;
	
	ppc->align = SLIDING_POS (w->pos)->anchor;
	update_position_toggles (ppc);
}

void
update_config_offset (BasePWidget *w)
{
	PerPanelConfig *ppc = get_config_struct (GTK_WIDGET (w));

	g_return_if_fail (SLIDING_IS_WIDGET (w));

	if (ppc == NULL ||
	    ppc->ppc_origin_change)
		return;

	gtk_spin_button_set_value (GTK_SPIN_BUTTON (ppc->offset_spin),
				   SLIDING_POS (w->pos)->offset);
}

void
update_config_offset_limit (BasePWidget *panel)
{
	GtkWidget *widget = GTK_WIDGET(panel);
	PerPanelConfig *ppc = get_config_struct (widget);
	int range, val;
	GtkAdjustment *adj;

	if (ppc == NULL ||
	    ppc->ppc_origin_change)
		return;

	if(ppc->edge == BORDER_LEFT || ppc->edge == BORDER_RIGHT)
		range = multiscreen_height (panel->screen)
			- widget->allocation.height;
	else
		range = multiscreen_width (panel->screen)
			- widget->allocation.width;

	adj = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON (ppc->offset_spin));
	if((int)adj->upper == range)
		return;

	val = SLIDING_POS (panel->pos)->offset;
	if(val > range) val = range;
	adj = GTK_ADJUSTMENT(gtk_adjustment_new (val, 0, range, 1, 10, 10));
	gtk_spin_button_set_adjustment (GTK_SPIN_BUTTON (ppc->offset_spin), adj);
	gtk_adjustment_value_changed(adj);
}


void
update_config_align (BasePWidget *w)
{
	PerPanelConfig *ppc = get_config_struct (GTK_WIDGET (w));
	g_return_if_fail (ALIGNED_IS_WIDGET (w));

	if (ppc == NULL ||
	    ppc->ppc_origin_change)
		return;

	if (ppc->align == ALIGNED_POS (w->pos)->align)
		return;

	ppc->align = ALIGNED_POS (w->pos)->align;
	update_position_toggles (ppc);
}


static void
config_destroy(GtkWidget *widget, gpointer data)
{
	PerPanelConfig *ppc = data;
	
	ppconfigs = g_list_remove (ppconfigs, ppc);
	
	g_free (ppc->back_pixmap);
	ppc->back_pixmap = NULL;
	g_free (ppc);
}

static void
config_apply (PerPanelConfig *ppc)
{
	/* don't update selves, all changes coming from ME */
	ppc->ppc_origin_change = TRUE;

	if(EDGE_IS_WIDGET(ppc->panel))
		border_widget_change_params(BORDER_WIDGET(ppc->panel),
					    ppc->screen,
					    ppc->edge,
					    ppc->sz,
					    ppc->mode,
					    BASEP_WIDGET(ppc->panel)->state,
					    ppc->hidebuttons,
					    ppc->hidebutton_pixmaps,
					    ppc->back_type,
					    ppc->back_pixmap,
					    ppc->fit_pixmap_bg,
					    ppc->stretch_pixmap_bg,
					    ppc->rotate_pixmap_bg,
					    &ppc->back_color);
	else if(SLIDING_IS_WIDGET(ppc->panel))
		sliding_widget_change_params(SLIDING_WIDGET(ppc->panel),
					     ppc->screen,
					     ppc->align,
					     ppc->offset,
					     ppc->edge,
					     ppc->sz,
					     ppc->mode,
					     BASEP_WIDGET(ppc->panel)->state,
					     ppc->hidebuttons,
					     ppc->hidebutton_pixmaps,
					     ppc->back_type,
					     ppc->back_pixmap,
					     ppc->fit_pixmap_bg,
					     ppc->stretch_pixmap_bg,
					     ppc->rotate_pixmap_bg,
					     &ppc->back_color);
	else if (ALIGNED_IS_WIDGET (ppc->panel))
		aligned_widget_change_params (ALIGNED_WIDGET (ppc->panel),
					      ppc->screen,
					      ppc->align,
					      ppc->edge,
					      ppc->sz,
					      ppc->mode,
					      BASEP_WIDGET(ppc->panel)->state,
					      ppc->hidebuttons,
					      ppc->hidebutton_pixmaps,
					      ppc->back_type,
					      ppc->back_pixmap,
					      ppc->fit_pixmap_bg,
					      ppc->stretch_pixmap_bg,
					      ppc->rotate_pixmap_bg,
					      &ppc->back_color);
	else if (FLOATING_IS_WIDGET (ppc->panel))
		floating_widget_change_params (FLOATING_WIDGET (ppc->panel), 
					       ppc->screen,
					       ppc->x,
					       ppc->y,
					       ppc->orient,
					       ppc->mode,
					       BASEP_WIDGET (ppc->panel)->state,
					       ppc->sz,
					       ppc->hidebuttons,
					       ppc->hidebutton_pixmaps,
					       ppc->back_type,
					       ppc->back_pixmap,
					       ppc->fit_pixmap_bg,
					       ppc->stretch_pixmap_bg,
					       ppc->rotate_pixmap_bg,
					       &ppc->back_color);
	else if(DRAWER_IS_WIDGET(ppc->panel)) {
	        DrawerPos *dp = DRAWER_POS (BASEP_WIDGET (ppc->panel)->pos);
		drawer_widget_change_params(DRAWER_WIDGET (ppc->panel),
					    dp->orient,
					    ppc->mode,
					    BASEP_WIDGET (ppc->panel)->state, 
					    ppc->sz,
					    ppc->hidebuttons,
					    ppc->hidebutton_pixmaps,
					    ppc->back_type,
					    ppc->back_pixmap,
					    ppc->fit_pixmap_bg,
					    ppc->stretch_pixmap_bg,
					    ppc->rotate_pixmap_bg,
					    &ppc->back_color);
	}

	/* start registering changes again */
	ppc->ppc_origin_change = FALSE;

	gtk_widget_queue_draw (ppc->panel);
}

static void
set_toggle (GtkWidget *widget, gpointer data)
{
	PerPanelConfig *ppc = g_object_get_data (G_OBJECT (widget), "PerPanelConfig");
	int *the_toggle = data;

	*the_toggle = GTK_TOGGLE_BUTTON(widget)->active;

	panel_config_register_changes (ppc);
}

static void
set_sensitive_toggle (GtkWidget *widget, GtkWidget *widget2)
{
	gtk_widget_set_sensitive(widget2,GTK_TOGGLE_BUTTON(widget)->active);
}

static void
basep_set_auto_hide (GtkWidget *widget, gpointer data)
{
	PerPanelConfig *ppc = g_object_get_data (G_OBJECT (widget), "PerPanelConfig");
	
	ppc->mode = (GTK_TOGGLE_BUTTON (widget)->active)
		? BASEP_AUTO_HIDE
		: BASEP_EXPLICIT_HIDE;


	panel_config_register_changes (ppc);
}

static GtkWidget *
make_hidebuttons_widget (PerPanelConfig *ppc)
{
	GtkWidget *frame;
	GtkWidget *box;
	GtkWidget *button;
	GtkWidget *w;

	frame = gtk_frame_new (_("Hiding"));
	gtk_container_set_border_width (GTK_CONTAINER (frame), GNOME_PAD_SMALL);

	box = gtk_vbox_new (FALSE, GNOME_PAD_SMALL);
	gtk_container_set_border_width (GTK_CONTAINER (box), GNOME_PAD_SMALL);
	gtk_container_add (GTK_CONTAINER (frame), box);
	
	/* Auto-hide */
	button = gtk_check_button_new_with_label(_("Enable Auto-hide"));
	ppc->autohide_button = button;
	g_object_set_data (G_OBJECT (button), "PerPanelConfig", ppc);
	if (ppc->mode == BASEP_AUTO_HIDE)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
	g_signal_connect (G_OBJECT (button), "toggled", 
			  G_CALLBACK (basep_set_auto_hide), 
			  NULL);
	gtk_box_pack_start (GTK_BOX (box), button, TRUE, TRUE,0);

	/* Hidebuttons enable */
	w = button = gtk_check_button_new_with_label (_("Show hide buttons"));
	ppc->hidebuttons_button = button;
	g_object_set_data (G_OBJECT (button), "PerPanelConfig", ppc);
	if (ppc->hidebuttons)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
	g_signal_connect (G_OBJECT (button), "toggled", 
			  G_CALLBACK (set_toggle),
			  &ppc->hidebuttons);
	gtk_box_pack_start (GTK_BOX (box), button, TRUE, TRUE,0);

	/* Arrow enable */
	button = gtk_check_button_new_with_label (_("Show arrows on hide button"));
	ppc->hidebutton_pixmaps_button = button;
	g_signal_connect (G_OBJECT (w), "toggled", 
			  G_CALLBACK (set_sensitive_toggle),
			  button);
	if (!ppc->hidebuttons)
		gtk_widget_set_sensitive(button,FALSE);
	g_object_set_data (G_OBJECT (button), "PerPanelConfig", ppc);
	if (ppc->hidebutton_pixmaps)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
	g_signal_connect (G_OBJECT (button), "toggled", 
			  G_CALLBACK (set_toggle),
			  &ppc->hidebutton_pixmaps);
	gtk_box_pack_start (GTK_BOX (box), button, TRUE, TRUE,0);	

	return frame;
}

static void
screen_set (GtkWidget *widget, gpointer data)
{
	PerPanelConfig *ppc = g_object_get_data (G_OBJECT (widget), "PerPanelConfig");

	ppc->screen = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (widget));
	panel_config_register_changes (ppc);	
}

static GtkWidget *
make_misc_widget (PerPanelConfig *ppc, gboolean avoiding)
{
	GtkWidget *frame;
	GtkWidget *box, *hbox;
	GtkWidget *button, *label;
	GtkObject *range;

	if ( ! avoiding &&
	    multiscreen_screens () <= 1)
		return NULL;

	frame = gtk_frame_new (_("Miscellaneous"));
	gtk_container_set_border_width (GTK_CONTAINER (frame), GNOME_PAD_SMALL);

	box = gtk_vbox_new (FALSE, GNOME_PAD_SMALL);
	gtk_container_set_border_width (GTK_CONTAINER (box), GNOME_PAD_SMALL);
	gtk_container_add (GTK_CONTAINER (frame), box);
	
	if (multiscreen_screens () > 1) {
		hbox = gtk_hbox_new (FALSE, 0);
		gtk_box_pack_start (GTK_BOX (box), hbox, FALSE, FALSE, 0);

		label = gtk_label_new (_("Current screen:"));
		gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

		range = gtk_adjustment_new (ppc->screen,
					    0, multiscreen_screens () - 1,
					    1, 1, 1);
		ppc->screen_spin = button =
			gtk_spin_button_new (GTK_ADJUSTMENT (range), 1, 0);
		gtk_widget_set_size_request (GTK_WIDGET (button), 65, -1);
		g_object_set_data (G_OBJECT (button), "PerPanelConfig", ppc);
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (button), ppc->screen);
		g_signal_connect (G_OBJECT (button), "changed",
				  G_CALLBACK (screen_set), NULL);
		gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
	}

	return frame;
}

static void
border_set_edge (GtkWidget *widget, gpointer data)
{
	int edge = GPOINTER_TO_INT (data);
	PerPanelConfig *ppc = g_object_get_data (G_OBJECT (widget), "PerPanelConfig");

	if (ppc->edge == edge)
		return;

	ppc->edge = edge;
	
	panel_config_register_changes (ppc);
}

static void
border_set_align (GtkWidget *widget, gpointer data)
{
	int align = GPOINTER_TO_INT (data);
	PerPanelConfig *ppc = g_object_get_data (G_OBJECT (widget), "PerPanelConfig");

	if (ppc->align == align)
		return;

	ppc->align = align;
	
	panel_config_register_changes (ppc);
}

static GtkWidget *
make_position_widget (PerPanelConfig *ppc, int aligns)
{
	GtkWidget *pos_box;
	GtkWidget *table;
	GtkWidget *w = NULL;
	int align;

	pos_box = gtk_vbox_new (FALSE, GNOME_PAD_SMALL);
	gtk_container_set_border_width (GTK_CONTAINER (pos_box),
					GNOME_PAD_SMALL);

	w = gtk_label_new (_("Panel Position"));
	gtk_box_pack_start (GTK_BOX (pos_box), w, FALSE, FALSE, 0);

	w = gtk_alignment_new (0.5, 0.5, 0, 0);	
	gtk_box_pack_start (GTK_BOX (pos_box), w, FALSE, FALSE, 0);

	table = gtk_table_new (2 + aligns, 2 + aligns, FALSE);
	gtk_container_add (GTK_CONTAINER (w), table);

	w = NULL;

	/* LEFT */
	for (align = 0; align < aligns; ++align) {
		w = w 
			? gtk_radio_button_new_from_widget (
				GTK_RADIO_BUTTON (w))
			: gtk_radio_button_new (NULL);
		ppc->toggle[BORDER_LEFT][align] = w;
		gtk_widget_set_size_request (w, 18, 18);
		gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (w), FALSE);
		g_object_set_data (G_OBJECT (w), "PerPanelConfig", ppc);
		gtk_table_attach (GTK_TABLE (table), w, 0, 1,
				  1 + align, 2 + align,
				  GTK_FILL,
				  GTK_EXPAND | GTK_FILL, 0, 0);
		g_signal_connect (G_OBJECT (w), "toggled",
				  G_CALLBACK (border_set_edge),
				  GINT_TO_POINTER (BORDER_LEFT));
		g_signal_connect (G_OBJECT (w), "toggled",
				  G_CALLBACK (border_set_align),
				  GINT_TO_POINTER (align));
	}
	

	/* TOP */
	for (align = 0; align < aligns; ++align) {
		w = gtk_radio_button_new_from_widget (
			GTK_RADIO_BUTTON (w));
		ppc->toggle[BORDER_TOP][align] = w;
		gtk_widget_set_size_request (w, 18, 18);
		gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (w), FALSE);
		g_object_set_data (G_OBJECT (w), "PerPanelConfig", ppc);
		gtk_table_attach (GTK_TABLE (table), w,
				  1 + align, 2 + align, 0, 1,
				  GTK_EXPAND | GTK_FILL,
				  0, 0, 0);
		g_signal_connect (G_OBJECT (w), "toggled",
				  G_CALLBACK (border_set_edge),
				  GINT_TO_POINTER (BORDER_TOP));
		g_signal_connect (G_OBJECT (w), "toggled",
				  G_CALLBACK (border_set_align),
				  GINT_TO_POINTER (align));
	}


	/* RIGHT */
	for (align = 0; align < aligns; ++align) {
		w = gtk_radio_button_new_from_widget (
			GTK_RADIO_BUTTON (w));
		ppc->toggle[BORDER_RIGHT][align] = w;
		gtk_widget_set_size_request (w, 18, 18);
		gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (w), FALSE);
		g_object_set_data (G_OBJECT (w), "PerPanelConfig", ppc);
		gtk_table_attach (GTK_TABLE (table), w,
				  1 + aligns, 2 + aligns,
				  1 + align, 2 + align,
				  GTK_FILL,
				  GTK_FILL | GTK_EXPAND, 0, 0);
		g_signal_connect (G_OBJECT (w), "toggled",
				  G_CALLBACK (border_set_edge),
				  GINT_TO_POINTER (BORDER_RIGHT));
		g_signal_connect (G_OBJECT (w), "toggled",
				  G_CALLBACK (border_set_align),
				  GINT_TO_POINTER (align));
	}


	/* BOTTOM */
	for (align = 0; align < aligns; ++align) {
		w = gtk_radio_button_new_from_widget (
			GTK_RADIO_BUTTON (w));
		ppc->toggle[BORDER_BOTTOM][align] = w;
		gtk_widget_set_size_request (w, 18, 18);
		gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (w), FALSE);
		g_object_set_data (G_OBJECT (w), "PerPanelConfig", ppc);
		gtk_table_attach (GTK_TABLE (table), w,
				  1 + align, 2 + align,
				  1 + aligns, 2 + aligns,
				  GTK_EXPAND | GTK_FILL,
				  0, 0, 0);
		g_signal_connect (G_OBJECT (w), "toggled",
				  G_CALLBACK (border_set_edge),
				  GINT_TO_POINTER (BORDER_BOTTOM));
		g_signal_connect (G_OBJECT (w), "toggled",
				  G_CALLBACK (border_set_align),
				  GINT_TO_POINTER (align));
	}

	update_position_toggles (ppc);

	w = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(w), GTK_SHADOW_IN);
	gtk_table_attach (GTK_TABLE (table), w, 
			  1, 1 + aligns,
			  1, 1 + aligns,
			  GTK_FILL, GTK_FILL, 0, 0);
	gtk_widget_set_size_request (w, 103, 77);

	return pos_box;
}
	
static GtkWidget *
edge_notebook_page (PerPanelConfig *ppc)
{
	GtkWidget *vbox, *box, *vvbox;
	GtkWidget *w, *f;

	vbox = gtk_vbox_new (FALSE, GNOME_PAD_SMALL);
	
	f = gtk_frame_new (_("Size and Position"));
	gtk_container_set_border_width (GTK_CONTAINER (f), GNOME_PAD_SMALL);
	gtk_box_pack_start (GTK_BOX (vbox), f, TRUE, TRUE, 0);

	vvbox = gtk_vbox_new (FALSE, GNOME_PAD_SMALL);
	gtk_container_set_border_width (GTK_CONTAINER (vvbox), GNOME_PAD_SMALL);
	gtk_container_add (GTK_CONTAINER (f), vvbox);
	
	box = gtk_hbox_new (TRUE, GNOME_PAD_SMALL);
	gtk_box_pack_start (GTK_BOX (vvbox), box, TRUE, TRUE, 0);
	
	w = make_size_widget (ppc);
	gtk_box_pack_start (GTK_BOX (box), w, TRUE, TRUE, 0);

	w = make_position_widget (ppc, 1);
	gtk_box_pack_start (GTK_BOX (box), w,  TRUE, TRUE, 0);

	w = make_hidebuttons_widget (ppc);
	gtk_box_pack_start (GTK_BOX (vbox), w, FALSE, FALSE, 0);

	w = make_misc_widget (ppc, TRUE);
	if (w != NULL)
		gtk_box_pack_start (GTK_BOX (vbox), w, FALSE, FALSE, 0);
	
	return vbox;
}

static GtkWidget *
aligned_notebook_page (PerPanelConfig *ppc)
{
	GtkWidget *vbox, *vvbox, *box;
	GtkWidget *w, *f;

	vbox = gtk_vbox_new (FALSE, GNOME_PAD_SMALL);
	
	f = gtk_frame_new (_("Size and Position"));
	gtk_container_set_border_width (GTK_CONTAINER (f), GNOME_PAD_SMALL);
	gtk_box_pack_start (GTK_BOX (vbox), f, FALSE, TRUE, 0);

	vvbox = gtk_vbox_new (FALSE, GNOME_PAD_SMALL);
	gtk_container_set_border_width (GTK_CONTAINER (vvbox), GNOME_PAD_SMALL);
	gtk_container_add (GTK_CONTAINER (f), vvbox);
	
	box = gtk_hbox_new (TRUE, GNOME_PAD_SMALL);
	gtk_box_pack_start (GTK_BOX (vvbox), box, TRUE, TRUE, 0);
	
	w = make_size_widget (ppc);
	gtk_box_pack_start (GTK_BOX (box), w, FALSE, FALSE, 0);

	w = make_position_widget (ppc, 3);
	gtk_box_pack_start (GTK_BOX (box), w,  FALSE, FALSE, 0);

	w = make_hidebuttons_widget (ppc);
	gtk_box_pack_start (GTK_BOX (vbox), w, FALSE, FALSE, 0);

	w = make_misc_widget (ppc, TRUE);
	if (w != NULL)
		gtk_box_pack_start (GTK_BOX (vbox), w, FALSE, FALSE, 0);
	
	return vbox;
}

static void
floating_set_orient (GtkWidget *widget, gpointer data)
{
	GtkOrientation orient = GPOINTER_TO_INT (data);
	PerPanelConfig *ppc = g_object_get_data (G_OBJECT (widget), "PerPanelConfig");

	if (!(GTK_TOGGLE_BUTTON (widget)->active))
		return;

	ppc->orient = orient;

	panel_config_register_changes (ppc);
}

static void
floating_set_xy (GtkWidget *widget, gpointer data)
{
	gint16 *xy = data;
	PerPanelConfig *ppc = g_object_get_data (G_OBJECT (widget), "PerPanelConfig");

	*xy = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (widget));
	panel_config_register_changes (ppc);	
}

static GtkWidget *
floating_notebook_page (PerPanelConfig *ppc)
{
	GtkWidget *vbox;
	GtkWidget *vvbox;
	GtkWidget *box;
	GtkWidget *button, *label;
	GtkWidget *orientbox;
	GtkObject *range;
	GtkWidget *f, *w;
	GtkWidget *hbox;
	int xlimit, ylimit;

	xlimit = multiscreen_width (ppc->screen)
		- ppc->panel->allocation.width;
	ylimit = multiscreen_height (ppc->screen)
		- ppc->panel->allocation.height;
	
	vbox = gtk_vbox_new (FALSE, GNOME_PAD_SMALL);
	
	f = gtk_frame_new (_("Size and Position"));
	gtk_container_set_border_width (GTK_CONTAINER (f), GNOME_PAD_SMALL);
	gtk_box_pack_start (GTK_BOX (vbox), f, FALSE, TRUE, 0);

	vvbox = gtk_vbox_new (FALSE, GNOME_PAD_SMALL);
	gtk_container_set_border_width (GTK_CONTAINER (vvbox), GNOME_PAD_SMALL);
	gtk_container_add (GTK_CONTAINER (f), vvbox);
	
	box = gtk_hbox_new (TRUE, GNOME_PAD_SMALL);
	gtk_box_pack_start (GTK_BOX (vvbox), box, TRUE, TRUE, 0);
	
	w = make_size_widget (ppc);
	gtk_box_pack_start (GTK_BOX (box), w, FALSE, FALSE, 0);

	/****** bleh ********/
	orientbox = gtk_vbox_new (FALSE, GNOME_PAD_SMALL);
	gtk_box_pack_start (GTK_BOX (box), orientbox, FALSE, FALSE, 0);

	ppc->h_orient = button = gtk_radio_button_new_with_label (
		NULL, _("Orient panel horizontally"));
	if(ppc->orient == GTK_ORIENTATION_HORIZONTAL)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
	g_object_set_data (G_OBJECT (button), "PerPanelConfig", ppc);
	g_signal_connect (G_OBJECT (button), "toggled",
			  G_CALLBACK (floating_set_orient),
			  GINT_TO_POINTER (GTK_ORIENTATION_HORIZONTAL));
	gtk_box_pack_start (GTK_BOX (orientbox), button, FALSE, FALSE, 0);

	ppc->v_orient = button = 
		gtk_radio_button_new_with_label_from_widget (
			GTK_RADIO_BUTTON (ppc->h_orient), 
			_("Orient panel vertically"));
	if(ppc->orient == GTK_ORIENTATION_VERTICAL)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
	g_object_set_data (G_OBJECT (button), "PerPanelConfig", ppc);
	g_signal_connect (G_OBJECT (button), "toggled",
			  G_CALLBACK (floating_set_orient),
			  GINT_TO_POINTER (GTK_ORIENTATION_VERTICAL));
	gtk_box_pack_start (GTK_BOX (orientbox), button, FALSE, FALSE, 0);

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (orientbox), hbox, FALSE, FALSE, 0);
	
	label = gtk_label_new (_("Top left corner's position: X"));
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

	range = gtk_adjustment_new (ppc->x, 0, xlimit, 1, 10, 10);
	ppc->x_spin = button =
		gtk_spin_button_new (GTK_ADJUSTMENT (range), 1, 0);
	gtk_widget_set_size_request (GTK_WIDGET (button), 65, -1);
	g_object_set_data (G_OBJECT (button), "PerPanelConfig", ppc);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (button), ppc->x);
	g_signal_connect (G_OBJECT (button), "changed",
			  G_CALLBACK (floating_set_xy),
			  &ppc->x);
	gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);

	label = gtk_label_new (_("Y"));
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	
	range = gtk_adjustment_new (ppc->y, 0, ylimit, 1, 10, 10);
	ppc->y_spin = button =
		gtk_spin_button_new (GTK_ADJUSTMENT (range), 1, 0);
	gtk_widget_set_size_request (GTK_WIDGET (button), 65, -1);
	g_object_set_data (G_OBJECT (button), "PerPanelConfig", ppc);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (button), ppc->y);
	g_signal_connect (G_OBJECT (button), "changed",
			  G_CALLBACK (floating_set_xy),
			  &ppc->y);
	gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);

	w = make_hidebuttons_widget (ppc);
	gtk_box_pack_start (GTK_BOX (vbox), w, FALSE, FALSE, 0);

	w = make_misc_widget (ppc, FALSE);
	if (w != NULL)
		gtk_box_pack_start (GTK_BOX (vbox), w, FALSE, FALSE, 0);
	
	return vbox;
}


static void
sliding_set_offset (GtkWidget *widget, gpointer data)
{
	PerPanelConfig *ppc = data;

	ppc->offset = 
		gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (ppc->offset_spin));

	panel_config_register_changes (ppc);
}
	

static GtkWidget *
sliding_notebook_page (PerPanelConfig *ppc)
{
	GtkWidget *vvbox, *vbox, *box, *hbox;
	GtkWidget *w, *f;
	GtkWidget *l;
	GtkWidget *button;
	GtkAdjustment *adj;
	int range;
	
	vbox = gtk_vbox_new (FALSE, GNOME_PAD_SMALL);
	
	f = gtk_frame_new (_("Size and Position"));
	gtk_container_set_border_width (GTK_CONTAINER (f), GNOME_PAD_SMALL);
	gtk_box_pack_start (GTK_BOX (vbox), f, FALSE, TRUE, 0);
	
	vvbox = gtk_vbox_new (FALSE, GNOME_PAD_SMALL);
	gtk_container_set_border_width (GTK_CONTAINER (vvbox), GNOME_PAD_SMALL);
	gtk_container_add (GTK_CONTAINER (f), vvbox);
	
	box = gtk_hbox_new (TRUE, GNOME_PAD_SMALL);

	w = make_size_widget (ppc);
	gtk_box_pack_start (GTK_BOX (box), w, FALSE, FALSE, 0);

	hbox = gtk_hbox_new (TRUE, GNOME_PAD_SMALL);
	gtk_box_pack_start (GTK_BOX (vvbox), hbox, TRUE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), GNOME_PAD_SMALL);

	gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 0);

	w = make_position_widget (ppc, 2);
	gtk_box_pack_start (GTK_BOX (hbox), w, TRUE, TRUE, 0);

	hbox = gtk_hbox_new (FALSE, GNOME_PAD_SMALL);
	gtk_box_pack_start (GTK_BOX (vvbox), hbox, TRUE, TRUE, 0);

	l = gtk_label_new (_("Offset from screen edge:"));
	gtk_box_pack_start (GTK_BOX (hbox), l, FALSE, FALSE, 0);

	if(ppc->edge == BORDER_LEFT || ppc->edge == BORDER_RIGHT)
		range = multiscreen_height (ppc->screen)
			- ppc->panel->allocation.height;
	else
		range = multiscreen_width (ppc->screen)
			- ppc->panel->allocation.width;
	adj = GTK_ADJUSTMENT(gtk_adjustment_new (ppc->offset, 0, range, 1, 10, 10));
	ppc->offset_spin = button = 
		gtk_spin_button_new (adj, 1, 0);
	gtk_widget_set_size_request (button, 100, -1);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (button), ppc->offset);
	g_signal_connect (G_OBJECT (button), "changed",
			  G_CALLBACK (sliding_set_offset), ppc);
	gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);

	w = make_hidebuttons_widget (ppc);
	gtk_box_pack_start (GTK_BOX (vbox), w, FALSE, FALSE, 0);
	
	w = make_misc_widget (ppc, TRUE);
	if (w != NULL)
		gtk_box_pack_start (GTK_BOX (vbox), w, FALSE, FALSE, 0);
	
	return vbox;
}

static void
size_set_size (GtkWidget *widget, gpointer data)
{
	int sz = GPOINTER_TO_INT(data);
	PerPanelConfig *ppc = g_object_get_data (G_OBJECT (widget), "PerPanelConfig");

	if (ppc->sz == sz)
		return;

	ppc->sz = sz;
	
	panel_config_register_changes (ppc);
}

/* XXX: until this is fixed in GTK+ */
static void
activate_proper_item (GtkMenuShell *menu)
{
#ifdef FIXME
	GtkWidget *active;
	active = gtk_menu_shell_get_active (menu);
	if (active != NULL)
		gtk_menu_item_activate (GTK_MENU_ITEM (active));
#endif
}

GtkWidget *
make_size_widget (PerPanelConfig *ppc)
{
	GtkWidget *box;
	GtkWidget *vbox;
	GtkWidget *menu;
	GtkWidget *menuitem;
	GtkWidget *w;
	gchar *s;
	int i;

	/* main vbox */
	vbox = gtk_vbox_new (FALSE, GNOME_PAD_SMALL);
	
	box = gtk_hbox_new (FALSE, GNOME_PAD_SMALL);
	gtk_box_pack_start (GTK_BOX (vbox), box, FALSE, FALSE, 0);

	gtk_box_pack_start (GTK_BOX (box),
			    gtk_label_new (_("Panel size:")),
			    FALSE, FALSE, 0);

	
	menu = gtk_menu_new ();
	g_signal_connect (G_OBJECT (menu), "deactivate", 
			  G_CALLBACK (activate_proper_item), 
			  NULL);

	menuitem = gtk_menu_item_new_with_label (_("XX Small Tiny (12 pixels)"));
	gtk_widget_show(menuitem);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
	g_object_set_data (G_OBJECT (menuitem), "PerPanelConfig", ppc);
	g_signal_connect (G_OBJECT (menuitem), "activate",
			  G_CALLBACK (size_set_size),
			  GINT_TO_POINTER (PANEL_SIZE_XX_SMALL));

	menuitem = gtk_menu_item_new_with_label (_("X Small (24 pixels)"));
	gtk_widget_show(menuitem);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
	g_object_set_data (G_OBJECT (menuitem), "PerPanelConfig", ppc);
	g_signal_connect (G_OBJECT (menuitem), "activate",
			  G_CALLBACK (size_set_size),
			  GINT_TO_POINTER (PANEL_SIZE_X_SMALL));
	
	menuitem = gtk_menu_item_new_with_label (_("Small (36 pixels)"));
	gtk_widget_show(menuitem);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
	g_object_set_data (G_OBJECT (menuitem), "PerPanelConfig", ppc);
	g_signal_connect (G_OBJECT (menuitem), "activate",
			  G_CALLBACK (size_set_size),
			  GINT_TO_POINTER (PANEL_SIZE_SMALL));
	
	menuitem = gtk_menu_item_new_with_label (_("Medium (48 pixels)"));
	gtk_widget_show(menuitem);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
	g_object_set_data (G_OBJECT (menuitem), "PerPanelConfig", ppc);
	g_signal_connect (G_OBJECT (menuitem), "activate",
			  G_CALLBACK (size_set_size),
			  GINT_TO_POINTER (PANEL_SIZE_MEDIUM));

	menuitem = gtk_menu_item_new_with_label (_("Large (64 pixels)"));
	gtk_widget_show(menuitem);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
	g_object_set_data (G_OBJECT (menuitem), "PerPanelConfig", ppc);
	g_signal_connect (G_OBJECT (menuitem), "activate",
			  G_CALLBACK (size_set_size),
			  GINT_TO_POINTER (PANEL_SIZE_LARGE));

	menuitem = gtk_menu_item_new_with_label (_("X Large (80 pixels)"));
	gtk_widget_show(menuitem);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
	g_object_set_data (G_OBJECT (menuitem), "PerPanelConfig", ppc);
	g_signal_connect (G_OBJECT (menuitem), "activate",
			  G_CALLBACK (size_set_size),
			  GINT_TO_POINTER (PANEL_SIZE_X_LARGE));

	menuitem = gtk_menu_item_new_with_label (_("XX Large (128 pixels)"));
	gtk_widget_show(menuitem);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
	g_object_set_data (G_OBJECT (menuitem), "PerPanelConfig", ppc);
	g_signal_connect (G_OBJECT (menuitem), "activate",
			  G_CALLBACK (size_set_size),
			  GINT_TO_POINTER (PANEL_SIZE_XX_LARGE));

	ppc->size_menu = gtk_option_menu_new ();
	gtk_option_menu_set_menu (GTK_OPTION_MENU (ppc->size_menu), menu);
	
	gtk_box_pack_start (GTK_BOX (box), ppc->size_menu,
			    FALSE, FALSE, 0);
	
	s = _("Note: The panel will size itself to the\n"
	      "largest applet in the panel, and that\n"
	      "not all applets obey these sizes.");

	w = gtk_label_new (s);
	gtk_label_set_justify (GTK_LABEL (w), GTK_JUSTIFY_LEFT);
	gtk_box_pack_start (GTK_BOX (vbox), w, FALSE, FALSE, 0);

	switch(ppc->sz) {
	case PANEL_SIZE_XX_SMALL:
		i = 0;
		break;
	case PANEL_SIZE_X_SMALL:
		i = 1;
		break;
	case PANEL_SIZE_SMALL:
		i = 2;
		break;
	default:
	case PANEL_SIZE_MEDIUM:
		i = 3;
		break;
	case PANEL_SIZE_LARGE:
		i = 4;
		break;
	case PANEL_SIZE_X_LARGE:
		i = 5;
		break;
	case PANEL_SIZE_XX_LARGE:
		i = 6;
		break;
	}

	gtk_option_menu_set_history (GTK_OPTION_MENU (ppc->size_menu), i);
	menuitem = g_list_nth_data(GTK_MENU_SHELL(GTK_OPTION_MENU(ppc->size_menu)->menu)->children, i);
	gtk_menu_item_activate(GTK_MENU_ITEM(menuitem));

	return vbox;
}

static void
value_changed (GtkWidget *w, gpointer data)
{
	PerPanelConfig *ppc = data;

	g_free(ppc->back_pixmap);
	ppc->back_pixmap = gnome_pixmap_entry_get_filename (
				GNOME_PIXMAP_ENTRY (ppc->pix_entry));

	panel_config_register_changes (ppc);
}

static void
set_fit_pixmap_bg (GtkToggleButton *toggle, gpointer data)
{
	PerPanelConfig *ppc = data;
	ppc->fit_pixmap_bg = toggle->active;

	panel_config_register_changes (ppc);
}

static void
set_stretch_pixmap_bg (GtkToggleButton *toggle, gpointer data)
{
	PerPanelConfig *ppc = data;
	ppc->stretch_pixmap_bg = toggle->active;

	panel_config_register_changes (ppc);
}

static void
set_rotate_pixmap_bg (GtkToggleButton *toggle, gpointer data)
{
	PerPanelConfig *ppc = data;
	ppc->rotate_pixmap_bg = toggle->active;

	panel_config_register_changes (ppc);
}

static void
color_set_cb (GtkWidget *w, int r, int g, int b, int a, gpointer data)
{
	PerPanelConfig *ppc = data;

	ppc->back_color.red = r;
	ppc->back_color.green = g;
	ppc->back_color.blue = b;
	
	panel_config_register_changes (ppc);
}
			   
static void
set_back (GtkWidget *widget, gpointer data)
{
	GtkWidget *pixf,*colf;
	PerPanelConfig *ppc = g_object_get_data (G_OBJECT (widget), "PerPanelConfig");
	PanelBackType back_type = GPOINTER_TO_INT (data);

	if (ppc->back_type == back_type)
		return;

	pixf = g_object_get_data (G_OBJECT (widget), "pix");
	colf = g_object_get_data (G_OBJECT (widget), "col");
	
	if (back_type == PANEL_BACK_NONE) {
		gtk_widget_set_sensitive (pixf, FALSE);
		gtk_widget_set_sensitive (colf, FALSE);
	} else if (back_type == PANEL_BACK_COLOR) {
		gtk_widget_set_sensitive (pixf, FALSE);
		gtk_widget_set_sensitive (colf, TRUE);
	} else  {
		gtk_widget_set_sensitive (pixf, TRUE);
		gtk_widget_set_sensitive (colf, FALSE);
	}
	
	ppc->back_type = back_type;

	panel_config_register_changes (ppc);
}

static GtkWidget *
background_page (PerPanelConfig *ppc)
{
	GtkWidget *box, *f, *t;
	GtkWidget *vbox, *noscale, *fit, *stretch;
	GtkWidget *w, *m;

	vbox = gtk_vbox_new (FALSE, GNOME_PAD_SMALL);
	gtk_container_set_border_width(GTK_CONTAINER (vbox), GNOME_PAD_SMALL);

	box = gtk_hbox_new (FALSE, GNOME_PAD_SMALL);
	gtk_box_pack_start (GTK_BOX (vbox), box, FALSE, FALSE, 0);

	w = gtk_label_new(_("Background Type: "));
	gtk_box_pack_start (GTK_BOX (box), w, FALSE, FALSE, 0);

	/*background type option menu*/
	m = gtk_menu_new ();
	g_signal_connect (G_OBJECT (m), "deactivate", 
			  G_CALLBACK (activate_proper_item), 
			  NULL);
	ppc->non = gtk_menu_item_new_with_label (_("Standard"));
	g_object_set_data (G_OBJECT (ppc->non), "PerPanelConfig", ppc);
	gtk_widget_show (ppc->non);
	gtk_menu_shell_append (GTK_MENU_SHELL (m), ppc->non);
	ppc->col = gtk_menu_item_new_with_label (_("Color"));
	g_object_set_data (G_OBJECT (ppc->col), "PerPanelConfig", ppc);
	gtk_widget_show (ppc->col);
	gtk_menu_shell_append (GTK_MENU_SHELL (m), ppc->col);
	ppc->pix = gtk_menu_item_new_with_label (_("Pixmap"));
	g_object_set_data (G_OBJECT (ppc->pix), "PerPanelConfig", ppc);
	gtk_widget_show (ppc->pix);
	gtk_menu_shell_append (GTK_MENU_SHELL (m), ppc->pix);

	ppc->back_om = gtk_option_menu_new ();
	gtk_option_menu_set_menu (GTK_OPTION_MENU (ppc->back_om), m);
	gtk_box_pack_start (GTK_BOX (box), ppc->back_om, FALSE, FALSE, 0);


	/*color frame*/
	f = gtk_frame_new (_("Color"));
	gtk_widget_set_sensitive (f, ppc->back_type == PANEL_BACK_COLOR);
	g_object_set_data (G_OBJECT (ppc->pix), "col", f);
	g_object_set_data (G_OBJECT (ppc->col), "col", f);
	g_object_set_data (G_OBJECT (ppc->non), "col", f);
	gtk_container_set_border_width(GTK_CONTAINER (f), GNOME_PAD_SMALL);
	gtk_box_pack_start (GTK_BOX (vbox), f, FALSE, FALSE, 0);

	box = gtk_hbox_new (0, 0);
	gtk_container_set_border_width(GTK_CONTAINER (box), GNOME_PAD_SMALL);
	gtk_container_add (GTK_CONTAINER (f), box);

	gtk_box_pack_start (GTK_BOX (box), gtk_label_new (_("Color to use:")),
			    FALSE, FALSE, GNOME_PAD_SMALL);

	ppc->backsel = gnome_color_picker_new();

	g_signal_connect (G_OBJECT(ppc->backsel),"color_set",
			  G_CALLBACK(color_set_cb), ppc);
        gnome_color_picker_set_i16(GNOME_COLOR_PICKER(ppc->backsel),
				   ppc->back_color.red,
				   ppc->back_color.green,
				   ppc->back_color.blue,
				   65535);

	gtk_box_pack_start (GTK_BOX (box), ppc->backsel, FALSE, FALSE, 0);

	/*image frame*/
	f = gtk_frame_new (_("Image"));
	gtk_widget_set_sensitive (f, ppc->back_type == PANEL_BACK_PIXMAP);
	g_object_set_data (G_OBJECT (ppc->pix), "pix", f);
	g_object_set_data (G_OBJECT (ppc->col), "pix", f);
	g_object_set_data (G_OBJECT (ppc->non), "pix", f);
	gtk_container_set_border_width(GTK_CONTAINER (f), GNOME_PAD_SMALL);
	gtk_box_pack_start (GTK_BOX (vbox), f, FALSE, FALSE, 0);

	box = gtk_vbox_new (0, 0);
	gtk_container_set_border_width(GTK_CONTAINER (box), GNOME_PAD_SMALL);
	gtk_container_add (GTK_CONTAINER (f), box);

	ppc->pix_entry = gnome_pixmap_entry_new ("pixmap", _("Browse"), TRUE);
	if (gdk_screen_height () < 600)
		gnome_pixmap_entry_set_preview_size (GNOME_PIXMAP_ENTRY (ppc->pix_entry),
						     0,50);
	t = gnome_file_entry_gtk_entry (GNOME_FILE_ENTRY (ppc->pix_entry));
	panel_signal_connect_while_alive (G_OBJECT (t), "changed",
					  G_CALLBACK (value_changed), ppc,
					  G_OBJECT (ppc->pix_entry));
	gtk_box_pack_start (GTK_BOX (box), ppc->pix_entry, FALSE, FALSE, 0);
	
	gtk_entry_set_text (GTK_ENTRY (t),
			    sure_string (ppc->back_pixmap));
	
	noscale = gtk_radio_button_new_with_label (
		NULL, _("Don't scale image to fit"));
		
	gtk_box_pack_start (GTK_BOX (box), noscale, FALSE, FALSE,0);

	fit = gtk_radio_button_new_with_label (
		gtk_radio_button_get_group(GTK_RADIO_BUTTON(noscale)),
		_("Scale image (keep proportions)"));
	gtk_box_pack_start (GTK_BOX (box), fit, FALSE, FALSE,0);

	stretch = gtk_radio_button_new_with_label (
		gtk_radio_button_get_group(GTK_RADIO_BUTTON(noscale)),
		_("Stretch image (change proportions)"));
	gtk_box_pack_start (GTK_BOX (box), stretch, FALSE, FALSE,0);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (noscale),
				      ppc->stretch_pixmap_bg &&
				      ppc->stretch_pixmap_bg);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (stretch),
				      ppc->stretch_pixmap_bg);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (fit),
				      ppc->fit_pixmap_bg);
	g_signal_connect (G_OBJECT (fit), "toggled",
			  G_CALLBACK (set_fit_pixmap_bg), ppc);
	g_signal_connect (G_OBJECT (stretch), "toggled",
			  G_CALLBACK (set_stretch_pixmap_bg), ppc);

	w = gtk_check_button_new_with_label (_("Rotate image for vertical panels"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w),
				      ppc->rotate_pixmap_bg);
	g_signal_connect (G_OBJECT (w), "toggled",
			  G_CALLBACK (set_rotate_pixmap_bg), ppc);
	gtk_box_pack_start (GTK_BOX (box), w, FALSE, FALSE,0);

	g_signal_connect (G_OBJECT (ppc->non), "activate", 
			  G_CALLBACK (set_back), 
			  GINT_TO_POINTER(PANEL_BACK_NONE));
	g_signal_connect (G_OBJECT (ppc->pix), "activate", 
			  G_CALLBACK (set_back), 
			  GINT_TO_POINTER(PANEL_BACK_PIXMAP));
	g_signal_connect (G_OBJECT (ppc->col), "activate", 
			  G_CALLBACK (set_back), 
			  GINT_TO_POINTER(PANEL_BACK_COLOR));
	
	if(ppc->back_type == PANEL_BACK_NONE) {
		gtk_option_menu_set_history(GTK_OPTION_MENU(ppc->back_om), 0);
		gtk_menu_item_activate(GTK_MENU_ITEM(ppc->non));
	} else if(ppc->back_type == PANEL_BACK_COLOR) {
		gtk_option_menu_set_history(GTK_OPTION_MENU(ppc->back_om), 1);
		gtk_menu_item_activate(GTK_MENU_ITEM(ppc->col));
	} else {
		gtk_option_menu_set_history(GTK_OPTION_MENU(ppc->back_om), 2);
		gtk_menu_item_activate(GTK_MENU_ITEM(ppc->pix));
	}

	return vbox;
}

static void
setup_pertype_defs (BasePWidget *basep, PerPanelConfig *ppc)
{
	if (BORDER_IS_WIDGET (basep))
		ppc->edge = BORDER_POS (basep->pos)->edge;
	
	if (ALIGNED_IS_WIDGET(basep)) {
		ppc->align = ALIGNED_POS (basep->pos)->align;
	} else if (FLOATING_IS_WIDGET (basep)) {
		FloatingPos *pos = FLOATING_POS (basep->pos);
		ppc->x = pos->x;
		ppc->y = pos->y;
		ppc->orient = PANEL_WIDGET(basep->panel)->orient;
	} else if (SLIDING_IS_WIDGET(basep)) {
		SlidingPos *pos = SLIDING_POS (basep->pos);
		ppc->offset = pos->offset;
		ppc->align = pos->anchor;
	} else {
		ppc->align = 0;
	}
}

void
update_config_type (BasePWidget *w)
{
	PerPanelConfig *ppc = get_config_struct (GTK_WIDGET (w));
	int i,j;
	GtkWidget *page;

	if (ppc == NULL ||
	    ppc->ppc_origin_change)
		return;

	g_return_if_fail(ppc->type_tab);

	ppc->register_changes = FALSE;
	gtk_widget_destroy(GTK_BIN(ppc->type_tab)->child);

	for(i = 0; i < POSITION_EDGES; i++)
		for(j = 0; j < POSITION_ALIGNS; j++)
			ppc->toggle[i][j] = NULL;

	setup_pertype_defs (BASEP_WIDGET(w), ppc);

	if(EDGE_IS_WIDGET(w)) {
		/* edge notebook page */
		page = edge_notebook_page(ppc);
		gtk_label_set_text (GTK_LABEL (ppc->type_tab_label),
				    _("Edge panel"));
		gtk_container_add(GTK_CONTAINER(ppc->type_tab), page);
	} else if(ALIGNED_IS_WIDGET(w)) {
		/* aligned notebook page */
		page = aligned_notebook_page(ppc);
		gtk_container_add(GTK_CONTAINER(ppc->type_tab), page);
		gtk_label_set_text (GTK_LABEL (ppc->type_tab_label),
				    _("Corner panel"));
	} else if(SLIDING_IS_WIDGET(w)) {
		/* sliding notebook page */
		page = sliding_notebook_page(ppc);
		gtk_container_add(GTK_CONTAINER(ppc->type_tab), page);
		gtk_label_set_text (GTK_LABEL (ppc->type_tab_label),
				    _("Sliding panel"));
	} else if(FLOATING_IS_WIDGET(w)) {
		/* floating notebook page */
		page = floating_notebook_page(ppc);
		gtk_container_add(GTK_CONTAINER(ppc->type_tab), page);
		gtk_label_set_text (GTK_LABEL (ppc->type_tab_label),
				    _("Floating panel"));
 	}
	gtk_widget_show_all (ppc->type_tab);
	ppc->register_changes = TRUE;
}

static void
window_response (GtkWidget *w, int response, gpointer data)
{
	GtkWidget *notebook = data;
	const char *help_path = g_object_get_data (G_OBJECT (w), "help_path");
	const char *help_linkid = g_object_get_data (G_OBJECT (w), "help_linkid");

	if (response == GTK_RESPONSE_HELP) {
		int tab;

		tab = gtk_notebook_get_current_page (GTK_NOTEBOOK (notebook));

		if (tab == 1)
			panel_show_help ("panelproperties", "PANELBACKTAB");
		else
			panel_show_help (help_path, help_linkid);
	} else {
		gtk_widget_destroy (w);
	}
}
	     
void 
panel_config (GtkWidget *panel)
{
	GtkWidget *page;
	PerPanelConfig *ppc;
	GtkWidget *prop_nbook;
	BasePWidget *basep = BASEP_WIDGET (panel);
	PanelWidget *pw = PANEL_WIDGET (basep->panel);
	char *help_path = "";
	char *help_linkid = NULL;
	ppc = get_config_struct(panel);
	
	/* return if the window is already up. */
	if (ppc != NULL) {
		g_assert (ppc->config_window != NULL);

		gtk_window_present (GTK_WINDOW (ppc->config_window));
		return;
	}
	
	ppc = g_new0 (PerPanelConfig, 1);
	ppconfigs = g_list_prepend(ppconfigs, ppc);
	ppc->register_changes = FALSE; /*don't notify property box of changes
					 until everything is all set up*/
	ppc->ppc_origin_change = FALSE; /* default state */

	ppc->sz = pw->sz;
	ppc->screen = basep->screen;
	ppc->hidebuttons = basep->hidebuttons_enabled;
	ppc->hidebutton_pixmaps = basep->hidebutton_pixmaps_enabled;
	ppc->fit_pixmap_bg = pw->fit_pixmap_bg;
	ppc->stretch_pixmap_bg = pw->stretch_pixmap_bg;
	ppc->rotate_pixmap_bg = pw->rotate_pixmap_bg;
	ppc->back_pixmap = g_strdup(pw->back_pixmap);
	ppc->back_color = pw->back_color;
	ppc->back_type = pw->back_type;
	ppc->mode = basep->mode;
	ppc->update_function = NULL;
	ppc->update_data = NULL;

	setup_pertype_defs (basep, ppc);

	ppc->panel = panel;
	
	/* main window */
	ppc->config_window = gtk_dialog_new_with_buttons (_("Panel properties"),
							  NULL, 0,
							  GTK_STOCK_HELP,
							  GTK_RESPONSE_HELP,
							  GTK_STOCK_CLOSE,
							  GTK_RESPONSE_CLOSE,
							  NULL);
	gtk_widget_add_events (ppc->config_window, GDK_KEY_PRESS_MASK);
	g_signal_connect (G_OBJECT (ppc->config_window), "event",
			  G_CALLBACK (panel_dialog_window_event), NULL);
	gtk_window_set_wmclass (GTK_WINDOW (ppc->config_window),
				"panel_properties", "Panel");
	gtk_widget_set_events (ppc->config_window,
			       gtk_widget_get_events (ppc->config_window) |
			       GDK_BUTTON_PRESS_MASK);

	g_signal_connect (G_OBJECT (ppc->config_window), "destroy",
			  G_CALLBACK (config_destroy), ppc);
	
	prop_nbook = gtk_notebook_new ();

	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (ppc->config_window)->vbox),
			    prop_nbook, TRUE, TRUE, 0);

	if(EDGE_IS_WIDGET(panel)) {
		/* edge notebook page */
		help_path = "panelproperties";
		help_linkid = "EDGETAB";
		page = edge_notebook_page(ppc);
		ppc->type_tab = gtk_event_box_new();
		gtk_container_add(GTK_CONTAINER(ppc->type_tab), page);
		ppc->type_tab_label = gtk_label_new (_("Edge panel"));
		gtk_notebook_append_page(GTK_NOTEBOOK(prop_nbook),
					 ppc->type_tab,
					 ppc->type_tab_label);
	} else if(ALIGNED_IS_WIDGET(panel)) {
		/* aligned notebook page */
		help_path = "panelproperties";
		help_linkid = "EDGETAB";
		page = aligned_notebook_page(ppc);
		ppc->type_tab = gtk_event_box_new();
		gtk_container_add(GTK_CONTAINER(ppc->type_tab), page);
		ppc->type_tab_label = gtk_label_new (_("Corner panel"));
		gtk_notebook_append_page(GTK_NOTEBOOK(prop_nbook),
					 ppc->type_tab,
					 ppc->type_tab_label);
	} else if(SLIDING_IS_WIDGET(panel)) {
		/* sliding notebook page */
		help_path = "panelproperties";
		help_linkid = "EDGETAB";
		page = sliding_notebook_page(ppc);
		ppc->type_tab = gtk_event_box_new();
		gtk_container_add(GTK_CONTAINER(ppc->type_tab), page);
		ppc->type_tab_label = gtk_label_new (_("Sliding panel"));
		gtk_notebook_append_page(GTK_NOTEBOOK (prop_nbook),
					 ppc->type_tab,
					 ppc->type_tab_label);
	} else if(FLOATING_IS_WIDGET(panel)) {
		/* floating notebook page */
		help_path = "panelproperties";
		help_linkid = "EDGETAB";
		page = floating_notebook_page(ppc);
		ppc->type_tab = gtk_event_box_new();
		gtk_container_add(GTK_CONTAINER(ppc->type_tab), page);
		ppc->type_tab_label = gtk_label_new (_("Floating panel"));
		gtk_notebook_append_page(GTK_NOTEBOOK (prop_nbook),
					 ppc->type_tab,
					 ppc->type_tab_label);
	} else if(DRAWER_IS_WIDGET(panel)) {
		BasePWidget *basep = BASEP_WIDGET(panel);
		GtkWidget *applet = PANEL_WIDGET(basep->panel)->master_widget;
		AppletInfo *info =
			g_object_get_data (G_OBJECT (applet), "applet_info");
		add_drawer_properties_page(ppc, GTK_NOTEBOOK (prop_nbook), info->data);
		help_path = "drawers";
		help_linkid = NULL;
		/* we can't change to/from drawers anyhow */
		ppc->type_tab = NULL;
 	}

	/* Backing configuration */
	page = background_page (ppc);
	gtk_notebook_append_page (GTK_NOTEBOOK(prop_nbook),
				  page, gtk_label_new (_("Background")));

	g_object_set_data (G_OBJECT (ppc->config_window), "help_path",
			   help_path);
	g_object_set_data (G_OBJECT (ppc->config_window), "help_linkid",
			   help_linkid);
	g_signal_connect (G_OBJECT (ppc->config_window), "response",
			  G_CALLBACK (window_response),
			  prop_nbook);
	
	g_signal_connect (G_OBJECT (ppc->config_window), "event",
			  G_CALLBACK (config_event),
			  prop_nbook);
	
	ppc->register_changes = TRUE;

	/* show main window */
	gtk_widget_show_all (ppc->config_window);

	/* grab the focus on the size menu always, if it exists */
	if (ppc->size_menu != NULL)
		gtk_widget_grab_focus (ppc->size_menu);
}


