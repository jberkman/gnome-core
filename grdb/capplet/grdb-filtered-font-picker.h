#ifndef GRDB_FILTERED_FONT_PICKER_H_
#define GRDB_FILTERED_FONT_PICKER_H_

/*
 * grdb: prime xrdb settings with gtk theme colors
 *
 * This is a public release of grdb, a tool to apply gtk theme
 * color schemes to other than gtk programs.
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

#include <gnome.h>

BEGIN_GNOME_DECLS

#define GRDB_TYPE_FILTERED_FONT_PICKER \
  (grdb_filtered_font_picker_get_type())
#define GRDB_FILTERED_FONT_PICKER(obj)  \
  (GTK_CHECK_CAST((obj), GRDB_TYPE_FILTERED_FONT_PICKER, \
		 GrdbFilteredFontPicker))
#define GRDB_FILTERED_FONT_PICKER_CLASS(klass) \
  (GTK_CHECK_CAST_CLASS((klass), GRDB_TYPE_FILTERED_FONT_PICKER, \
			GrdbFilteredFontPickerClass))
#define GRDB_IS_FILTERED_FONT_PICKER(obj) \
  (GTK_CHECK_TYPE((obj), GRDB_TYPE_FILTERED_FONT_PICKER))
#define GRDB_IS_FILTERED_FONT_PICKER_CLASS(klass) \
  (GTK_CHECK_CLASS_TYPE((klass), GRDB_IS_FILTERED_FONT_PICKER))

typedef struct _GrdbFilteredFontPicker GrdbFilteredFontPicker;
typedef struct _GrdbFilteredFontPickerClass GrdbFilteredFontPickerClass;

struct _GrdbFilteredFontPicker {
	GnomeFontPicker parent;

	gboolean          filtered;
	GtkFontFilterType filter_type;
	GtkFontType       font_type;
	gchar**           foundries;
	gchar**           weights;
	gchar**           slants;
        gchar**           setwidths;
	gchar**           spacings;
	gchar**           charsets;
};

struct _GrdbFilteredFontPickerClass {
	GnomeFontPickerClass parent_class;
};

GtkType grdb_filtered_font_picker_get_type (void);

GtkWidget *grdb_filtered_font_picker_new (void);

void grdb_filtered_font_picker_set_filter (GrdbFilteredFontPicker *gffp,
					   GtkFontFilterType filter_type,
					   GtkFontType font_type,
					   gchar **foundries,
					   gchar **weights,
					   gchar **slants,
					   gchar **setwidths,
					   gchar **spacings,
					   gchar **charsets);	   

END_GNOME_DECLS

#endif /* GRDB_FILTERED_FONT_PICKER_H_ */
