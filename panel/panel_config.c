#include <gtk/gtk.h>

#include <config.h>
#include <gnome.h>

#include "panel-include.h"

static GList *ppconfigs=NULL;

static PerPanelConfig *
get_config_struct(GtkWidget *panel)
{
	GList *list;
	for(list=ppconfigs;list!=NULL;list=g_list_next(list)) {
		PerPanelConfig *ppc = list->data;
		if(ppc->panel == panel)
			return ppc;
	}
	return NULL;
}

void
kill_config_dialog(GtkWidget *panel)
{
	PerPanelConfig *ppc = get_config_struct(panel);
	if(ppc && ppc->config_window)
		gtk_widget_destroy(ppc->config_window);
}

void
update_config_orient(GtkWidget *panel)
{
	PerPanelConfig *ppc = get_config_struct(panel);
	if(!ppc)
		return;
	if(IS_SNAPPED_WIDGET(panel)) {
		switch(SNAPPED_WIDGET(panel)->pos) {
		case SNAPPED_TOP:
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ppc->t_button),
						    TRUE);
			break;
		case SNAPPED_BOTTOM:
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ppc->b_button),
						    TRUE);
			break;
		case SNAPPED_LEFT:
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ppc->l_button),
						    TRUE);
			break;
		case SNAPPED_RIGHT:
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ppc->r_button),
						    TRUE);
			break;
		}
	} else if(IS_CORNER_WIDGET(panel)) {
		switch(CORNER_WIDGET(panel)->pos) {
		case CORNER_NE:
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ppc->t_button),
						    TRUE);
			break;
		case CORNER_SW:
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ppc->b_button),
						    TRUE);
			break;
		case CORNER_NW:
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ppc->l_button),
						    TRUE);
			break;
		case CORNER_SE:
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ppc->r_button),
						    TRUE);
			break;
		}
	}
}

void
update_config_size(GtkWidget *panel)
{
	PerPanelConfig *ppc = get_config_struct(panel);
	PanelWidget *p;
	if(!ppc)
		return;
	p = PANEL_WIDGET(BASEP_WIDGET(panel)->panel);
	switch(p->sz) {
	case SIZE_TINY:
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ppc->s_tiny),
					     TRUE);
		break;
	case SIZE_STANDARD:
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ppc->s_std),
					     TRUE);
		break;
	case SIZE_LARGE:
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ppc->s_large),
					     TRUE);
		break;
	case SIZE_HUGE:
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ppc->s_huge),
					     TRUE);
		break;
	}
}

void
update_config_back(PanelWidget *pw)
{
	GtkWidget *t;
	GtkWidget *panelw = gtk_object_get_data(GTK_OBJECT(pw), PANEL_PARENT);
	PerPanelConfig *ppc = get_config_struct(panelw);

	if(!ppc)
		return;
	switch(pw->back_type) {
	case PANEL_BACK_NONE:
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ppc->non),
					    TRUE);
		break;
	case PANEL_BACK_COLOR:
		gnome_color_picker_set_i16(GNOME_COLOR_PICKER(ppc->backsel),
					   pw->back_color.red,
					   pw->back_color.green,
					   pw->back_color.blue,
					   65535);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ppc->col),
					    TRUE);
		break;
	case PANEL_BACK_PIXMAP:
		t=gnome_pixmap_entry_gtk_entry(GNOME_PIXMAP_ENTRY(ppc->pix_entry));
		gtk_entry_set_text(GTK_ENTRY(t),
				   pw->back_pixmap?pw->back_pixmap:"");
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ppc->pix),
					    TRUE);
		break;
	}
}

static void
config_destroy(GtkWidget *widget, gpointer data)
{
	PerPanelConfig *ppc = data;
	
	ppconfigs = g_list_remove(ppconfigs,ppc);
	
	g_free(ppc->back_pixmap);
	g_free(ppc);
}

static void
config_apply (GtkWidget *widget, int page, gpointer data)
{
	PerPanelConfig *ppc = data;
	
	if(page != -1)
		return;
	
	if(IS_SNAPPED_WIDGET(ppc->panel))
		snapped_widget_change_params(SNAPPED_WIDGET(ppc->panel),
					     ppc->snapped_pos,
					     ppc->snapped_mode,
					     SNAPPED_WIDGET(ppc->panel)->state,
					     ppc->sz,
					     ppc->hidebuttons,
					     ppc->hidebutton_pixmaps,
					     ppc->back_type,
					     ppc->back_pixmap,
					     ppc->fit_pixmap_bg,
					     &ppc->back_color);
	else if(IS_CORNER_WIDGET(ppc->panel))
		corner_widget_change_params(CORNER_WIDGET(ppc->panel),
					    ppc->corner_pos,
					    ppc->corner_orient,
					    ppc->corner_mode,
					    CORNER_WIDGET(ppc->panel)->state,
					    ppc->sz,
					    ppc->hidebuttons,
					    ppc->hidebutton_pixmaps,
					    ppc->back_type,
					    ppc->back_pixmap,
					    ppc->fit_pixmap_bg,
					    &ppc->back_color);
	else if(IS_DRAWER_WIDGET(ppc->panel)) {
	        DrawerWidget *dw = DRAWER_WIDGET(ppc->panel);
		drawer_widget_change_params(dw,
					    dw->orient,
					    dw->state, 
					    ppc->sz,
					    ppc->back_type,
					    ppc->back_pixmap,
					    ppc->fit_pixmap_bg,
					    &ppc->back_color,
					    ppc->hidebutton_pixmaps,
					    ppc->hidebuttons);
	}
	gtk_widget_queue_draw (ppc->panel);
}

static void
destroy_egg(GtkWidget *widget, int **pages)
{
	if(pages)
		*pages = 0;
}

/*thy evil easter egg*/
static int
config_event(GtkWidget *widget,GdkEvent *event,GtkNotebook *nbook)
{
	GtkWidget *w;
	char *file;
	static int clicks=0;
	static int pages=0;
	GdkEventButton *bevent;
	
	if(event->type != GDK_BUTTON_PRESS)
		return FALSE;
	
	bevent = (GdkEventButton *)event;
	if(bevent->button != 3)
		clicks = 0;
	else
		clicks++;
	
	if(clicks<3)
		return FALSE;
	clicks = 0;
	
	if(pages==0) {
		file = gnome_unconditional_pixmap_file("gnome-gegl.png");
		if (file && g_file_exists (file))
			w = gnome_pixmap_new_from_file (file);
		else
			w = gtk_label_new("<insert picture of goat here>");
		g_free(file);
		gtk_widget_show(w);
		/*the GEGL shall not be translated*/
		gtk_notebook_append_page (nbook, w,
					  gtk_label_new ("GEGL"));
		gtk_notebook_set_page(nbook,-1);
		pages = 1;
		gtk_signal_connect(GTK_OBJECT(widget),"destroy",
				   GTK_SIGNAL_FUNC(destroy_egg),&pages);
	} else {
		gtk_notebook_set_page(nbook,-1);
	}
	return FALSE;
}

static void
set_toggle (GtkWidget *widget, gpointer data)
{
	PerPanelConfig *ppc = gtk_object_get_user_data(GTK_OBJECT(widget));
	int *the_toggle = data;

	*the_toggle = GTK_TOGGLE_BUTTON(widget)->active;
	if (ppc->register_changes)
		gnome_property_box_changed (GNOME_PROPERTY_BOX (ppc->config_window));
}

static void
set_sensitive_toggle (GtkWidget *widget, GtkWidget *widget2)
{
	gtk_widget_set_sensitive(widget2,GTK_TOGGLE_BUTTON(widget)->active);
}

static void
snapped_set_pos (GtkWidget *widget, gpointer data)
{
	SnappedPos pos = (SnappedPos) data;
	PerPanelConfig *ppc = gtk_object_get_user_data(GTK_OBJECT(widget));

	if(!(GTK_TOGGLE_BUTTON(widget)->active))
		return;
	
	ppc->snapped_pos = pos;
	if (ppc->register_changes)
		gnome_property_box_changed (GNOME_PROPERTY_BOX (ppc->config_window));
}

static void
snapped_set_auto_hide (GtkWidget *widget, gpointer data)
{
	PerPanelConfig *ppc = gtk_object_get_user_data(GTK_OBJECT(widget));

	if(GTK_TOGGLE_BUTTON(widget)->active)
		ppc->snapped_mode = SNAPPED_AUTO_HIDE;
	else
		ppc->snapped_mode = SNAPPED_EXPLICIT_HIDE;
	
	if (ppc->register_changes)
		gnome_property_box_changed (GNOME_PROPERTY_BOX (ppc->config_window));
}


static GtkWidget *
snapped_notebook_page(PerPanelConfig *ppc)
{
	GtkWidget *frame;
	GtkWidget *button;
	GtkWidget *box;
	GtkWidget *hbox;
	GtkWidget *vbox;
        GtkWidget *table;
        GtkWidget *w;

	/* main vbox */
	vbox = gtk_vbox_new (FALSE, 0);
	
	/* main hbox */
	hbox = gtk_hbox_new (FALSE, GNOME_PAD_SMALL);
	gtk_container_set_border_width(GTK_CONTAINER (hbox), GNOME_PAD_SMALL);
	
	/* Position frame */
	frame = gtk_frame_new (_("Position"));
	gtk_container_set_border_width(GTK_CONTAINER (frame), GNOME_PAD_SMALL);
	gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE,0);
	
	/* table for frame */
	table = gtk_table_new(3, 3, TRUE);
	gtk_table_set_row_spacings(GTK_TABLE(table), GNOME_PAD_SMALL);
	gtk_table_set_col_spacings(GTK_TABLE(table), GNOME_PAD_SMALL);
	gtk_container_set_border_width(GTK_CONTAINER(table), GNOME_PAD_SMALL);
	gtk_container_add (GTK_CONTAINER (frame), table);
	
	/* Top Position */
	ppc->t_button = gtk_radio_button_new_with_label (NULL, _("Top"));
	gtk_object_set_user_data(GTK_OBJECT(ppc->t_button),ppc);
	gtk_signal_connect (GTK_OBJECT (ppc->t_button), "toggled", 
			    GTK_SIGNAL_FUNC (snapped_set_pos), 
			    (gpointer)SNAPPED_TOP);
        gtk_table_attach(GTK_TABLE(table), ppc->t_button, 1, 2, 0, 1,
			 GTK_FILL | GTK_SHRINK, GTK_EXPAND | GTK_SHRINK, 0, 0);
	
	/* Bottom Position */
	ppc->b_button = gtk_radio_button_new_with_label (
			  gtk_radio_button_group (GTK_RADIO_BUTTON (ppc->t_button)),
			  _("Bottom"));
	gtk_object_set_user_data(GTK_OBJECT(ppc->b_button),ppc);
	gtk_signal_connect (GTK_OBJECT (ppc->b_button), "toggled", 
			    GTK_SIGNAL_FUNC (snapped_set_pos), 
			    (gpointer)SNAPPED_BOTTOM);
        gtk_table_attach(GTK_TABLE(table), ppc->b_button, 1, 2, 2, 3,
			 GTK_FILL | GTK_SHRINK, GTK_EXPAND | GTK_SHRINK, 0, 0);
	
	/* Left Position */
	ppc->l_button = gtk_radio_button_new_with_label (
			  gtk_radio_button_group (GTK_RADIO_BUTTON (ppc->t_button)),
			  _("Left"));
	gtk_object_set_user_data(GTK_OBJECT(ppc->l_button),ppc);
	gtk_signal_connect (GTK_OBJECT (ppc->l_button), "toggled", 
			    GTK_SIGNAL_FUNC (snapped_set_pos), 
			    (gpointer)SNAPPED_LEFT);
        gtk_table_attach(GTK_TABLE(table), ppc->l_button, 0, 1, 1, 2,
			 GTK_FILL | GTK_SHRINK, GTK_EXPAND | GTK_SHRINK, 0, 0);

	/* Right Position */
	ppc->r_button = gtk_radio_button_new_with_label (
			  gtk_radio_button_group (GTK_RADIO_BUTTON (ppc->t_button)),
			  _("Right"));
	gtk_object_set_user_data(GTK_OBJECT(ppc->r_button),ppc);
	gtk_signal_connect (GTK_OBJECT (ppc->r_button), "toggled", 
			    GTK_SIGNAL_FUNC (snapped_set_pos), 
			    (gpointer)SNAPPED_RIGHT);
        gtk_table_attach(GTK_TABLE(table), ppc->r_button, 2, 3, 1, 2,
			 GTK_FILL | GTK_SHRINK, GTK_EXPAND | GTK_SHRINK, 0, 0);

	switch(ppc->snapped_pos) {
	case SNAPPED_TOP:
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ppc->t_button),
					     TRUE);
		break;
	case SNAPPED_BOTTOM:
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ppc->b_button),
					     TRUE);
		break;
	case SNAPPED_LEFT:
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ppc->l_button),
					     TRUE);
		break;
	case SNAPPED_RIGHT:
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ppc->r_button),
					    TRUE);
		break;
	}

	/* Auto-hide/stayput frame */
	frame = gtk_frame_new (_("Minimize options"));
	gtk_container_set_border_width(GTK_CONTAINER (frame), GNOME_PAD_SMALL);
	gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE,0);

	/* vbox for frame */
	box = gtk_vbox_new (FALSE, GNOME_PAD_SMALL);
	gtk_container_set_border_width(GTK_CONTAINER (box), GNOME_PAD_SMALL);
	gtk_container_add (GTK_CONTAINER (frame), box);
	
	/* Auto-hide */
	button = gtk_check_button_new_with_label(_("Auto hide"));
	gtk_object_set_user_data(GTK_OBJECT(button),ppc);
	if (ppc->snapped_mode == SNAPPED_AUTO_HIDE)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
	gtk_signal_connect (GTK_OBJECT (button), "toggled", 
			    GTK_SIGNAL_FUNC (snapped_set_auto_hide), 
			    NULL);
	gtk_box_pack_start (GTK_BOX (box), button, TRUE, TRUE,0);

	/* Hidebuttons enable */
	w = button = gtk_check_button_new_with_label (_("Enable hidebuttons"));
	gtk_object_set_user_data(GTK_OBJECT(button),ppc);
	if (ppc->hidebuttons)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
	gtk_signal_connect (GTK_OBJECT (button), "toggled", 
			    GTK_SIGNAL_FUNC (set_toggle),
			    &ppc->hidebuttons);
	gtk_box_pack_start (GTK_BOX (box), button, TRUE, TRUE,0);

	/* Arrow enable */
	button = gtk_check_button_new_with_label (_("Enable hidebutton arrows"));
	gtk_signal_connect (GTK_OBJECT (w), "toggled", 
			    GTK_SIGNAL_FUNC (set_sensitive_toggle),
			    button);
	if (!ppc->hidebuttons)
		gtk_widget_set_sensitive(button,FALSE);
	gtk_object_set_user_data(GTK_OBJECT(button),ppc);
	if (ppc->hidebutton_pixmaps)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
	gtk_signal_connect (GTK_OBJECT (button), "toggled", 
			    GTK_SIGNAL_FUNC (set_toggle),
			    &ppc->hidebutton_pixmaps);
	gtk_box_pack_start (GTK_BOX (box), button, TRUE, TRUE,0);

	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

	return vbox;
}

static void
size_set_size (GtkWidget *widget, gpointer data)
{
	PanelSizeType sz = (PanelSizeType) data;
	PerPanelConfig *ppc = gtk_object_get_user_data(GTK_OBJECT(widget));

	if(!(GTK_TOGGLE_BUTTON(widget)->active))
		return;
	
	ppc->sz = sz;
	if (ppc->register_changes)
		gnome_property_box_changed (GNOME_PROPERTY_BOX (ppc->config_window));
}

static GtkWidget *
size_notebook_page(PerPanelConfig *ppc)
{
	GtkWidget *frame;
	GtkWidget *button;
	GtkWidget *box;
	GtkWidget *hbox;
	GtkWidget *vbox;
        GtkWidget *w;

	/* main vbox */
	vbox = gtk_vbox_new (FALSE, 0);
	
	/* main hbox */
	hbox = gtk_hbox_new (FALSE, GNOME_PAD_SMALL);
	gtk_container_set_border_width(GTK_CONTAINER (hbox), GNOME_PAD_SMALL);
	
	/* Position frame */
	frame = gtk_frame_new (_("Panel size"));
	gtk_container_set_border_width(GTK_CONTAINER (frame), GNOME_PAD_SMALL);
	gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE,0);
	
	/* box for radio buttons */
	box = gtk_vbox_new (FALSE, GNOME_PAD_SMALL);
	gtk_container_add (GTK_CONTAINER (frame), box);

	gtk_box_pack_start (GTK_BOX (box),
			    gtk_label_new(_("Note: The panel will size itself to the\n"
					    "largest applet. The applets get notified of\n"
					    "the size, but they may choose not to follow it.\n"
					    "All standard panel icons follow this size.")),
			    FALSE, FALSE,0);
	
	/* Tiny Size */
	ppc->s_tiny = gtk_radio_button_new_with_label (NULL, _("Tiny (24 pixels)"));
	gtk_object_set_user_data(GTK_OBJECT(ppc->s_tiny),ppc);
	gtk_signal_connect (GTK_OBJECT (ppc->s_tiny), "toggled", 
			    GTK_SIGNAL_FUNC (size_set_size), 
			    (gpointer)SIZE_TINY);
	gtk_box_pack_start (GTK_BOX (box), ppc->s_tiny, FALSE, FALSE,0);
	
	/* Standard Size */
	ppc->s_std = gtk_radio_button_new_with_label (
			  gtk_radio_button_group (GTK_RADIO_BUTTON (ppc->s_tiny)),
			  _("Standard (48 pixels)"));
	gtk_object_set_user_data(GTK_OBJECT(ppc->s_std),ppc);
	gtk_signal_connect (GTK_OBJECT (ppc->s_std), "toggled", 
			    GTK_SIGNAL_FUNC (size_set_size), 
			    (gpointer)SIZE_STANDARD);
	gtk_box_pack_start (GTK_BOX (box), ppc->s_std, FALSE, FALSE,0);

	/* Large Size */
	ppc->s_large = gtk_radio_button_new_with_label (
			  gtk_radio_button_group (GTK_RADIO_BUTTON (ppc->s_tiny)),
			  _("Large (64 pixels)"));
	gtk_object_set_user_data(GTK_OBJECT(ppc->s_large),ppc);
	gtk_signal_connect (GTK_OBJECT (ppc->s_large), "toggled", 
			    GTK_SIGNAL_FUNC (size_set_size), 
			    (gpointer)SIZE_LARGE);
	gtk_box_pack_start (GTK_BOX (box), ppc->s_large, FALSE, FALSE,0);
	
	/* Huge Size */
	ppc->s_huge = gtk_radio_button_new_with_label (
			  gtk_radio_button_group (GTK_RADIO_BUTTON (ppc->s_tiny)),
			  _("Huge (80 pixels)"));
	gtk_object_set_user_data(GTK_OBJECT(ppc->s_huge),ppc);
	gtk_signal_connect (GTK_OBJECT (ppc->s_huge), "toggled", 
			    GTK_SIGNAL_FUNC (size_set_size), 
			    (gpointer)SIZE_HUGE);
	gtk_box_pack_start (GTK_BOX (box), ppc->s_huge, FALSE, FALSE,0);
	
	switch(ppc->sz) {
	case SIZE_TINY:
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ppc->s_tiny),
					     TRUE);
		break;
	case SIZE_STANDARD:
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ppc->s_std),
					     TRUE);
		break;
	case SIZE_LARGE:
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ppc->s_large),
					     TRUE);
		break;
	case SIZE_HUGE:
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ppc->s_huge),
					     TRUE);
		break;
	}

	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

	return vbox;
}

static void
corner_set_pos (GtkWidget *widget, gpointer data)
{
	CornerPos pos = (CornerPos) data;
	PerPanelConfig *ppc = gtk_object_get_user_data(GTK_OBJECT(widget));

	if(!(GTK_TOGGLE_BUTTON(widget)->active))
		return;
	
	ppc->corner_pos = pos;
	if (ppc->register_changes)
		gnome_property_box_changed (GNOME_PROPERTY_BOX (ppc->config_window));
}

static void
corner_set_orient (GtkWidget *widget, gpointer data)
{
	PanelOrientation orient = (PanelOrientation) data;
	PerPanelConfig *ppc = gtk_object_get_user_data(GTK_OBJECT(widget));

	if(!(GTK_TOGGLE_BUTTON(widget)->active))
		return;
	
	ppc->corner_orient = orient;
	if (ppc->register_changes)
		gnome_property_box_changed (GNOME_PROPERTY_BOX (ppc->config_window));
}

static void
corner_set_auto_hide (GtkWidget *widget, gpointer data)
{
        PerPanelConfig *ppc = gtk_object_get_user_data(GTK_OBJECT(widget));
	if (GTK_TOGGLE_BUTTON(widget)->active)
	        ppc->corner_mode = CORNER_AUTO_HIDE;
	else
	        ppc->corner_mode = CORNER_EXPLICIT_HIDE;
	
	if (ppc->register_changes)
	        gnome_property_box_changed(GNOME_PROPERTY_BOX(ppc->config_window));
}

static GtkWidget *
corner_notebook_page(PerPanelConfig *ppc)
{
	GtkWidget *frame;
	GtkWidget *button;
	GtkWidget *w;
	GtkWidget *box;
	GtkWidget *hbox;
	GtkWidget *vbox;
        GtkWidget *table;
	
	/* main hbox */
	hbox = gtk_hbox_new (FALSE, GNOME_PAD_SMALL);
	gtk_container_set_border_width(GTK_CONTAINER (hbox), GNOME_PAD_SMALL);
	
	/* Position frame */
	frame = gtk_frame_new (_("Position"));
	gtk_container_set_border_width(GTK_CONTAINER (frame), GNOME_PAD_SMALL);
	gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE,0);
	
	/* table for frame */
	table = gtk_table_new(3, 3, TRUE);
	gtk_table_set_row_spacings(GTK_TABLE(table), GNOME_PAD_SMALL);
	gtk_table_set_col_spacings(GTK_TABLE(table), GNOME_PAD_SMALL);
	gtk_container_set_border_width(GTK_CONTAINER(table), GNOME_PAD_SMALL);
	gtk_container_add (GTK_CONTAINER (frame), table);
	
	/* North East Position */
	ppc->t_button = gtk_radio_button_new_with_label (NULL, _("North east"));
	gtk_object_set_user_data(GTK_OBJECT(ppc->t_button),ppc);
	gtk_signal_connect (GTK_OBJECT (ppc->t_button), "toggled", 
			    GTK_SIGNAL_FUNC (corner_set_pos), 
			    (gpointer)CORNER_NE);
        gtk_table_attach(GTK_TABLE(table), ppc->t_button, 2, 3, 0, 1,
			 GTK_FILL | GTK_SHRINK, GTK_EXPAND | GTK_SHRINK, 0, 0);
	
	/* Bottom Position */
	ppc->b_button = gtk_radio_button_new_with_label (
		 gtk_radio_button_group (GTK_RADIO_BUTTON (ppc->t_button)),
		 _("South west"));
	gtk_object_set_user_data(GTK_OBJECT(ppc->b_button),ppc);
	gtk_signal_connect (GTK_OBJECT (ppc->b_button), "toggled", 
			    GTK_SIGNAL_FUNC (corner_set_pos), 
			    (gpointer)CORNER_SW);
        gtk_table_attach(GTK_TABLE(table), ppc->b_button, 0, 1, 2, 3,
			 GTK_FILL | GTK_SHRINK, GTK_EXPAND | GTK_SHRINK, 0, 0);
	
	/* North West Position */
	ppc->l_button = gtk_radio_button_new_with_label (
		  gtk_radio_button_group (GTK_RADIO_BUTTON (ppc->t_button)),
		  _("North west"));
	gtk_object_set_user_data(GTK_OBJECT(ppc->l_button),ppc);
	gtk_signal_connect (GTK_OBJECT (ppc->l_button), "toggled", 
			    GTK_SIGNAL_FUNC (corner_set_pos), 
			    (gpointer)CORNER_NW);
        gtk_table_attach(GTK_TABLE(table), ppc->l_button, 0, 1, 0, 1,
			 GTK_FILL | GTK_SHRINK, GTK_EXPAND | GTK_SHRINK, 0, 0);

	/* South East Position */
	ppc->r_button = gtk_radio_button_new_with_label (
		 gtk_radio_button_group (GTK_RADIO_BUTTON (ppc->t_button)),
		 _("South east"));
	gtk_object_set_user_data(GTK_OBJECT(ppc->r_button),ppc);
	gtk_signal_connect (GTK_OBJECT (ppc->r_button), "toggled", 
			    GTK_SIGNAL_FUNC (corner_set_pos), 
			    (gpointer)CORNER_SE);
        gtk_table_attach(GTK_TABLE(table), ppc->r_button, 2, 3, 2, 3,
			 GTK_FILL | GTK_SHRINK, GTK_EXPAND | GTK_SHRINK, 0, 0);

	switch(ppc->corner_pos) {
	case CORNER_NE:
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ppc->t_button),
					    TRUE);
		break;
	case CORNER_SW:
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ppc->b_button),
					    TRUE);
		break;
	case CORNER_NW:
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ppc->l_button),
					    TRUE);
		break;
	case CORNER_SE:
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ppc->r_button),
					    TRUE);
		break;
	}

	vbox = gtk_vbox_new (FALSE, GNOME_PAD_SMALL);
	gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE,0);

	/* Orientation frame */
	frame = gtk_frame_new (_("Orientation"));
	gtk_container_set_border_width(GTK_CONTAINER (frame), GNOME_PAD_SMALL);
	gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE,0);

	/* vbox for frame */
	box = gtk_vbox_new (FALSE, GNOME_PAD_SMALL);
	gtk_container_set_border_width(GTK_CONTAINER (box), GNOME_PAD_SMALL);
	gtk_container_add (GTK_CONTAINER (frame), box);
	
	/* Horizontal */
	button = gtk_radio_button_new_with_label (NULL, _("Horizontal"));
	gtk_object_set_user_data(GTK_OBJECT(button),ppc);
	if (ppc->corner_orient == PANEL_HORIZONTAL)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
	gtk_signal_connect (GTK_OBJECT (button), "toggled", 
			    GTK_SIGNAL_FUNC (corner_set_orient), 
			    (gpointer)PANEL_HORIZONTAL);
	gtk_box_pack_start (GTK_BOX (box), button, TRUE, TRUE,0);
	
	/* Vertical */
	button = gtk_radio_button_new_with_label (
			  gtk_radio_button_group (GTK_RADIO_BUTTON (button)),
			  _("Vertical"));
	gtk_object_set_user_data(GTK_OBJECT(button),ppc);
	if (ppc->corner_orient == PANEL_VERTICAL)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
	gtk_signal_connect (GTK_OBJECT (button), "toggled", 
			    GTK_SIGNAL_FUNC (corner_set_orient), 
			    (gpointer)PANEL_VERTICAL);
	gtk_box_pack_start (GTK_BOX (box), button, TRUE, TRUE,0);

	/* Hidebuttons frame */
	frame = gtk_frame_new (_("Minimize options"));
	gtk_container_set_border_width(GTK_CONTAINER (frame), GNOME_PAD_SMALL);
	gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE,0);

	/* vbox for frame */
	box = gtk_vbox_new (FALSE, GNOME_PAD_SMALL);
	gtk_container_set_border_width(GTK_CONTAINER (box), GNOME_PAD_SMALL);
	gtk_container_add (GTK_CONTAINER (frame), box);
	
	/* Auto-hide */
	button = gtk_check_button_new_with_label(_("Auto hide"));
	gtk_object_set_user_data(GTK_OBJECT(button),ppc);
	if (ppc->corner_mode == CORNER_AUTO_HIDE)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
	gtk_signal_connect (GTK_OBJECT (button), "toggled", 
			    GTK_SIGNAL_FUNC (corner_set_auto_hide), 
			    NULL);
	gtk_box_pack_start (GTK_BOX (box), button, TRUE, TRUE,0);

	/* Hidebuttons enable */
	w = button = gtk_check_button_new_with_label (_("Enable hidebuttons"));
	gtk_object_set_user_data(GTK_OBJECT(button),ppc);
	if (ppc->hidebuttons)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
	gtk_signal_connect (GTK_OBJECT (button), "toggled", 
			    GTK_SIGNAL_FUNC (set_toggle),
			    &ppc->hidebuttons);
	gtk_box_pack_start (GTK_BOX (box), button, TRUE, TRUE,0);

	/* Arrow enable */
	button = gtk_check_button_new_with_label (_("Enable hidebutton arrows"));
	gtk_signal_connect (GTK_OBJECT (w), "toggled", 
			    GTK_SIGNAL_FUNC (set_sensitive_toggle),
			    button);
	if (!ppc->hidebuttons)
		gtk_widget_set_sensitive(button,FALSE);
	gtk_object_set_user_data(GTK_OBJECT(button),ppc);
	if (ppc->hidebutton_pixmaps)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
	gtk_signal_connect (GTK_OBJECT (button), "toggled", 
			    GTK_SIGNAL_FUNC (set_toggle),
			    &ppc->hidebutton_pixmaps);
	gtk_box_pack_start (GTK_BOX (box), button, TRUE, TRUE,0);

	return (hbox);
}


static int
value_changed (GtkWidget *w, gpointer data)
{
	PerPanelConfig *ppc = data;

	g_free(ppc->back_pixmap);
	ppc->back_pixmap = gnome_pixmap_entry_get_filename(GNOME_PIXMAP_ENTRY(ppc->pix_entry));
	if (ppc->register_changes)
		gnome_property_box_changed (GNOME_PROPERTY_BOX (ppc->config_window));
	return FALSE;
}

static void
set_fit_pixmap_bg (GtkToggleButton *toggle, gpointer data)
{
	PerPanelConfig *ppc = data;
	ppc->fit_pixmap_bg = toggle->active;

	if (ppc->register_changes)
		gnome_property_box_changed (GNOME_PROPERTY_BOX (ppc->config_window));
}

static void
color_set_cb(GtkWidget *w, int r, int g, int b, int a, gpointer data)
{
	PerPanelConfig *ppc = data;

	ppc->back_color.red = r;
	ppc->back_color.green = g;
	ppc->back_color.blue = b;
	
	if (ppc->register_changes)
		gnome_property_box_changed (GNOME_PROPERTY_BOX (ppc->config_window));
}
			   
static int
set_back (GtkWidget *widget, gpointer data)
{
	GtkWidget *pixf,*colf;
	PerPanelConfig *ppc = gtk_object_get_user_data(GTK_OBJECT(widget));
	PanelBackType back_type = GPOINTER_TO_INT(data);

	if(!GTK_TOGGLE_BUTTON(widget)->active)
		return FALSE;

	pixf = gtk_object_get_data(GTK_OBJECT(widget),"pix");
	colf = gtk_object_get_data(GTK_OBJECT(widget),"col");
	
	if(back_type == PANEL_BACK_NONE) {
		gtk_widget_set_sensitive(pixf,FALSE);
		gtk_widget_set_sensitive(colf,FALSE);
	} else if(back_type == PANEL_BACK_COLOR) {
		gtk_widget_set_sensitive(pixf,FALSE);
		gtk_widget_set_sensitive(colf,TRUE);
	} else  {
		gtk_widget_set_sensitive(pixf,TRUE);
		gtk_widget_set_sensitive(colf,FALSE);
	}
	
	ppc->back_type = back_type;

	if (ppc->register_changes)
		gnome_property_box_changed (GNOME_PROPERTY_BOX (ppc->config_window));
	return FALSE;
}


static GtkWidget *
background_page (PerPanelConfig *ppc)
{
	GtkWidget *box, *f, *t;
	GtkWidget *vbox;
	GtkWidget *w;

	vbox = gtk_vbox_new (FALSE, GNOME_PAD_SMALL);
	gtk_container_set_border_width(GTK_CONTAINER (vbox), GNOME_PAD_SMALL);

	/*selector frame*/
	f = gtk_frame_new (_("Background"));
	gtk_container_set_border_width(GTK_CONTAINER (f), GNOME_PAD_SMALL);
	gtk_box_pack_start (GTK_BOX (vbox), f, FALSE, FALSE, 0);

	box = gtk_hbox_new (0, 0);
	gtk_container_set_border_width(GTK_CONTAINER (box), GNOME_PAD_SMALL);
	gtk_container_add (GTK_CONTAINER (f), box);
	
	/*standard background*/
	ppc->non = gtk_radio_button_new_with_label (NULL, _("Standard"));
	gtk_box_pack_start (GTK_BOX (box), ppc->non, FALSE, FALSE,0);
	gtk_object_set_user_data(GTK_OBJECT(ppc->non),ppc);

	/* pixmap */
	ppc->pix = gtk_radio_button_new_with_label (
		gtk_radio_button_group (GTK_RADIO_BUTTON (ppc->non)),
		_("Pixmap"));
	gtk_box_pack_start (GTK_BOX (box), ppc->pix, FALSE, FALSE,0);
	gtk_object_set_user_data(GTK_OBJECT(ppc->pix),ppc);
	
	/* color */
	ppc->col = gtk_radio_button_new_with_label (
		gtk_radio_button_group (GTK_RADIO_BUTTON (ppc->non)),
		_("Color"));
	gtk_box_pack_start (GTK_BOX (box), ppc->col, FALSE, FALSE,0);
	gtk_object_set_user_data(GTK_OBJECT(ppc->col),ppc);

	/*image frame*/
	f = gtk_frame_new (_("Image file"));
	if(ppc->back_type == PANEL_BACK_PIXMAP) {
		gtk_widget_set_sensitive(f,TRUE);
	} else  {
		gtk_widget_set_sensitive(f,FALSE);
	}
	gtk_object_set_data(GTK_OBJECT(ppc->pix),"pix",f);
	gtk_object_set_data(GTK_OBJECT(ppc->col),"pix",f);
	gtk_object_set_data(GTK_OBJECT(ppc->non),"pix",f);
	gtk_container_set_border_width(GTK_CONTAINER (f), GNOME_PAD_SMALL);
	gtk_box_pack_start (GTK_BOX (vbox), f, FALSE, FALSE, 0);

	box = gtk_vbox_new (0, 0);
	gtk_container_set_border_width(GTK_CONTAINER (box), GNOME_PAD_SMALL);
	gtk_container_add (GTK_CONTAINER (f), box);

	ppc->pix_entry = gnome_pixmap_entry_new ("pixmap", _("Browse"),TRUE);

	t = gnome_pixmap_entry_gtk_entry (GNOME_PIXMAP_ENTRY (ppc->pix_entry));
	gtk_signal_connect_while_alive (GTK_OBJECT (t), "changed",
					GTK_SIGNAL_FUNC (value_changed), ppc,
					GTK_OBJECT (ppc->pix_entry));
	gtk_box_pack_start (GTK_BOX (box), ppc->pix_entry, FALSE, FALSE, 0);
	
	gtk_entry_set_text (GTK_ENTRY(t),
			    ppc->back_pixmap?ppc->back_pixmap:"");

	w = gtk_check_button_new_with_label (_("Scale image to fit panel"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w),
				     ppc->fit_pixmap_bg);
	gtk_signal_connect (GTK_OBJECT (w), "toggled",
			    GTK_SIGNAL_FUNC (set_fit_pixmap_bg),
			    ppc);
	gtk_box_pack_start (GTK_BOX (box), w, FALSE, FALSE,0);


	/*color frame*/
	box = gtk_hbox_new (0, 0);
	gtk_box_pack_start (GTK_BOX (vbox), box, FALSE, FALSE, 0);
	f = gtk_frame_new (_("Background color"));
	if(ppc->back_type == PANEL_BACK_COLOR) {
		gtk_widget_set_sensitive(f,TRUE);
	} else  {
		gtk_widget_set_sensitive(f,FALSE);
	}
	gtk_object_set_data(GTK_OBJECT(ppc->pix),"col",f);
	gtk_object_set_data(GTK_OBJECT(ppc->col),"col",f);
	gtk_object_set_data(GTK_OBJECT(ppc->non),"col",f);
	gtk_container_set_border_width(GTK_CONTAINER (f), GNOME_PAD_SMALL);
	gtk_box_pack_start (GTK_BOX (box), f, FALSE, FALSE, 0);

	box = gtk_vbox_new (0, 0);
	gtk_container_set_border_width(GTK_CONTAINER (box), GNOME_PAD_SMALL);
	gtk_container_add (GTK_CONTAINER (f), box);

	ppc->backsel = gnome_color_picker_new();
	gtk_signal_connect(GTK_OBJECT(ppc->backsel),"color_set",
			   GTK_SIGNAL_FUNC(color_set_cb), ppc);
        gnome_color_picker_set_i16(GNOME_COLOR_PICKER(ppc->backsel),
				   ppc->back_color.red,
				   ppc->back_color.green,
				   ppc->back_color.blue,
				   65535);

	gtk_box_pack_start (GTK_BOX (box), ppc->backsel, FALSE, FALSE, 0);

	gtk_signal_connect (GTK_OBJECT (ppc->non), "toggled", 
			    GTK_SIGNAL_FUNC (set_back), 
			    GINT_TO_POINTER(PANEL_BACK_NONE));
	gtk_signal_connect (GTK_OBJECT (ppc->pix), "toggled", 
			    GTK_SIGNAL_FUNC (set_back), 
			    GINT_TO_POINTER(PANEL_BACK_PIXMAP));
	gtk_signal_connect (GTK_OBJECT (ppc->col), "toggled", 
			    GTK_SIGNAL_FUNC (set_back), 
			    GINT_TO_POINTER(PANEL_BACK_COLOR));
	
	if(ppc->back_type == PANEL_BACK_NONE)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ppc->non), TRUE);
	else if(ppc->back_type == PANEL_BACK_COLOR)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ppc->col), TRUE);
	else
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ppc->pix), TRUE);

	return vbox;
}

static void
panelcfg_help(void)
{
    gchar *tmp;

    tmp = gnome_help_file_find_file ("users-guide", "confthis.html");
    if (tmp) {
       gnome_help_goto(0, tmp);
       g_free(tmp);
    }
}
	     
void 
panel_config(GtkWidget *panel)
{
/*      static GnomeHelpMenuEntry help_entry = { NULL, "properties" }; */
	GtkWidget *page;
	PerPanelConfig *ppc;
	GtkWidget *prop_nbook;
	BasePWidget *basep = BASEP_WIDGET(panel);
	PanelWidget *pw = PANEL_WIDGET(basep->panel);
	
	ppc = get_config_struct(panel);
	
	/* return if the window is already up. */
	if (ppc) {
		gdk_window_raise(ppc->config_window->window);
		gtk_widget_show(ppc->config_window);
		return;
	}
	
	ppc = g_new(PerPanelConfig,1);
	ppconfigs = g_list_prepend(ppconfigs,ppc);
	ppc->register_changes = FALSE; /*don't notify property box of changes
					 until everything is all set up*/

	ppc->hidebuttons = basep->hidebuttons_enabled;
	ppc->hidebutton_pixmaps = basep->hidebutton_pixmaps_enabled;
	ppc->fit_pixmap_bg = pw->fit_pixmap_bg;
	ppc->back_pixmap = g_strdup(pw->back_pixmap);
	ppc->back_color = pw->back_color;
	ppc->back_type = pw->back_type;

	if(IS_SNAPPED_WIDGET(panel)) {
		SnappedWidget *snapped = SNAPPED_WIDGET(panel);
		ppc->snapped_pos = snapped->pos;
		ppc->snapped_mode = snapped->mode;
	} else if(IS_CORNER_WIDGET(panel)) {
		CornerWidget *corner = CORNER_WIDGET(panel);
		ppc->corner_pos = corner->pos;
		ppc->corner_orient = pw->orient;
		ppc->corner_mode = corner->mode;
	}

	ppc->panel = panel;
	
	/* main window */
	ppc->config_window = gnome_property_box_new ();
	gtk_window_set_wmclass(GTK_WINDOW(ppc->config_window),
			       "panel_properties","Panel");
	gtk_widget_set_events(ppc->config_window,
			      gtk_widget_get_events(ppc->config_window) |
			      GDK_BUTTON_PRESS_MASK);
	/*gtk_window_set_position(GTK_WINDOW(ppc->config_window), GTK_WIN_POS_CENTER);*/
	gtk_window_set_policy(GTK_WINDOW(ppc->config_window), FALSE, FALSE, TRUE);

	gtk_signal_connect(GTK_OBJECT(ppc->config_window), "destroy",
			   GTK_SIGNAL_FUNC (config_destroy), ppc);
	gtk_window_set_title (GTK_WINDOW(ppc->config_window),
			      _("Panel properties"));
	gtk_container_set_border_width (GTK_CONTAINER(ppc->config_window),
					GNOME_PAD_SMALL);
	
	prop_nbook = GNOME_PROPERTY_BOX (ppc->config_window)->notebook;
	
	if(IS_SNAPPED_WIDGET(panel)) {
		/* Snapped notebook page */
		page = snapped_notebook_page (ppc);
		gtk_notebook_append_page (GTK_NOTEBOOK(prop_nbook),
					  page,
					  gtk_label_new (_("Edge panel")));
	} else if(IS_CORNER_WIDGET(panel)) {
		/* Corner notebook page */
		page = corner_notebook_page (ppc);
		gtk_notebook_append_page (GTK_NOTEBOOK(prop_nbook),
					  page,
					  gtk_label_new (_("Corner panel")));
	} else if(IS_DRAWER_WIDGET(panel)) {
		BasePWidget *basep = BASEP_WIDGET(panel);
		GtkWidget *applet = PANEL_WIDGET(basep->panel)->master_widget;
		AppletInfo *info =
			gtk_object_get_data(GTK_OBJECT(applet), "applet_info");
		add_drawer_properties_page(ppc, info->data);
	}
						
	/* Size configuration */
	page = size_notebook_page (ppc);
	gtk_notebook_append_page (GTK_NOTEBOOK(prop_nbook),
				  page, gtk_label_new (_("Size")));

	/* Backing configuration */
	page = background_page (ppc);
	gtk_notebook_append_page (GTK_NOTEBOOK(prop_nbook),
				  page, gtk_label_new (_("Background")));
	
	gtk_signal_connect (GTK_OBJECT (ppc->config_window), "apply",
			    GTK_SIGNAL_FUNC (config_apply), ppc);

/*	help_entry.name = gnome_app_id; 
	gtk_signal_connect (GTK_OBJECT (ppc->config_window), "help",
			    GTK_SIGNAL_FUNC (gnome_help_pbox_display),
			    &help_entry);
*/
	gtk_signal_connect (GTK_OBJECT (ppc->config_window), "help",
			    GTK_SIGNAL_FUNC (panelcfg_help),
			    NULL);

	gtk_signal_connect (GTK_OBJECT (ppc->config_window), "event",
			    GTK_SIGNAL_FUNC (config_event),
			    prop_nbook);
	
	ppc->register_changes = TRUE;

	/* show main window */
	gtk_widget_show_all (ppc->config_window);
}

