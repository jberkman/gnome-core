/*
 * grdb: prime xrdb settings with gtk theme colors
 *
 * This is a public release of grdb, a tool to apply gtk theme
 * color schemes to not gtk programs.
 * 
 * (C) Copyright 2000 Samuel Hunter <shunter@bigsky.net>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
 *
 * Author: Sam Hunter <shunter@bigsky.net>
 */

#include <config.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkmain.h>
#include <gtk/gtkfontsel.h>

#include "grdb-filtered-font-picker.h"

static void grdb_filtered_font_picker_class_init (GrdbFilteredFontPickerClass *class);
static void grdb_filtered_font_picker_init (GrdbFilteredFontPicker *cp);
static void grdb_filtered_font_picker_destroy (GtkObject *object);
static void grdb_filtered_font_picker_clicked (GtkButton *button);

static GnomeFontPickerClass *parent_class;

GtkType
grdb_filtered_font_picker_get_type (void)
{
	static GtkType ffp_type = 0;

	if (!ffp_type) {
		GtkTypeInfo ffp_info = {
			"GrdbFilteredFontPicker",
			sizeof (GrdbFilteredFontPicker),
			sizeof (GrdbFilteredFontPickerClass),
			(GtkClassInitFunc)grdb_filtered_font_picker_class_init,
			(GtkObjectInitFunc)grdb_filtered_font_picker_init,
			NULL, /* reserved_1 */
			NULL, /* reserved_2 */
			(GtkClassInitFunc)NULL
		};
		ffp_type = gtk_type_unique( gnome_font_picker_get_type (),
					    &ffp_info);
	}
	return ffp_type;
}

static void
grdb_filtered_font_picker_class_init (GrdbFilteredFontPickerClass *class)
{
	GtkObjectClass       *object_class;
	GtkButtonClass       *button_class;

	object_class = (GtkObjectClass *)class;
	button_class = (GtkButtonClass *)class;

	parent_class = gtk_type_class (gnome_font_picker_get_type ());
       
	/* no new signals */


	object_class->destroy = grdb_filtered_font_picker_destroy;

	button_class->clicked = grdb_filtered_font_picker_clicked;
}

static void
grdb_filtered_font_picker_init (GrdbFilteredFontPicker *gffp)
{
	/* Initialize fields */
	gffp->filtered    = FALSE;
	gffp->filter_type = GTK_FONT_FILTER_BASE;
	gffp->font_type   = GTK_FONT_ALL;
       	gffp->foundries   = NULL;
	gffp->weights     = NULL;
	gffp->slants      = NULL;
	gffp->setwidths   = NULL;
	gffp->spacings    = NULL;
	gffp->charsets    = NULL;
}

static void
grdb_filtered_font_picker_destroy (GtkObject *object)
{
	GrdbFilteredFontPicker *gffp;
	g_return_if_fail (object != NULL);
	g_return_if_fail (GRDB_IS_FILTERED_FONT_PICKER (object));

	gffp = GRDB_FILTERED_FONT_PICKER(object);

	if (gffp->foundries)
		g_strfreev(gffp->foundries);
	if (gffp->weights)
		g_strfreev(gffp->weights);
	if (gffp->slants)
		g_strfreev(gffp->slants);
	if (gffp->setwidths)
		g_strfreev(gffp->setwidths);
	if (gffp->spacings)
		g_strfreev(gffp->spacings);
	if (gffp->charsets)
		g_strfreev(gffp->charsets);


	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS(parent_class)->destroy) (object);
}

static char **copy_strv (char **vec)
{
	if (!vec) {
		return NULL;
	}

	return g_copy_vector (vec);
}

GtkWidget *
grdb_filtered_font_picker_new (void)
{
	return GTK_WIDGET (gtk_type_new
			   (grdb_filtered_font_picker_get_type ()));
}

void grdb_filtered_font_picker_set_filter (GrdbFilteredFontPicker *gffp,
					   GtkFontFilterType filter_type,
					   GtkFontType font_type,
					   gchar **foundries,
					   gchar **weights,
					   gchar **slants,
					   gchar **setwidths,
					   gchar **spacings,
					   gchar **charsets)
{
	GtkFontSelectionDialog *font_dialog = NULL;

	gffp->filtered    = TRUE;
	gffp->filter_type = filter_type;
	gffp->font_type   = font_type;
	gffp->foundries   = copy_strv (foundries);
	gffp->weights     = copy_strv (weights);
	gffp->slants      = copy_strv (slants);
	gffp->setwidths   = copy_strv (setwidths);
	gffp->spacings    = copy_strv (spacings);
	gffp->charsets    = copy_strv (charsets);

	/* quash warning about NULL cast */
	if (GNOME_FONT_PICKER (gffp)->font_dialog) {
		font_dialog = GTK_FONT_SELECTION_DIALOG (
			GNOME_FONT_PICKER (gffp)->font_dialog);
	}
	if ( font_dialog ) {
		gtk_font_selection_dialog_set_filter (font_dialog, 
						      filter_type,
						      font_type,
						      foundries,    
						      weights,
						      slants,
						      setwidths,
						      spacings,
						      charsets);
	}
}

static void
grdb_filtered_font_picker_clicked(GtkButton *button)
{
	GtkFontSelectionDialog *font_dialog;
	GrdbFilteredFontPicker *gffp = GRDB_FILTERED_FONT_PICKER (button);

	if (GTK_BUTTON_CLASS (parent_class)->clicked) {
		GTK_BUTTON_CLASS (parent_class)->clicked (button);
	}

	if (gffp->filtered) {
		font_dialog = GTK_FONT_SELECTION_DIALOG (
			GNOME_FONT_PICKER (button)->font_dialog);
		gtk_font_selection_dialog_set_filter (font_dialog,
						      gffp->filter_type,
						      gffp->font_type,
						      gffp->foundries,
						      gffp->weights,
						      gffp->slants,
						      gffp->setwidths,
						      gffp->spacings,
						      gffp->charsets);
	}
}
