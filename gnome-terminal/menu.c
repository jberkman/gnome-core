/*  menu.c - Zed's Virtual Terminal
 *  Copyright (C) 1998  Michael Zucchi
 *
 *  Menu and menu handling
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
  does main and pop-up menu's n stuff

  The stuff here is still in development.
*/

#include <gtk/gtk.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <stdlib.h>

#include <errno.h>
#include <gdk/gdkkeysyms.h>
#include <gdk/gdkx.h>		/* need the display for the font stuff */
#include <zvt/zvtterm.h>
/*#include <zvt/forkpty.h> */

/* debug macro */
#define d(x)

void vt_popup_filemenu  (ZvtTerm *, char *name, int mode);
void vt_popup_fontsmenu (ZvtTerm *);

/*
 * reads in all suitable fonts for the font-list
 * (fixed width fonts)
 *
 * These patterns are the ones passed to XListFonts().
 * Some of them are a bit useless ...
 */

char *fontlist_patterns[] = {
	"-*-clean-*-r-*-1",
	"-*-courier-*-r-*-1",
	"-*-terminal-*-r-*-1",
	"-*-fixed-*-r-*-1",
};

/*
 *  need list of foundaries
 *   with lists of families
 *      with lists of sizes (sorted?)
*/

struct _fontsizelist {
	struct _fontsizelist *next;
	struct _fontsizelist *prev;

	ZvtTerm *term;
	void    *f;
	
	char size[8];			/* size as a string */
	char name[1];			/* complete X font name */
};

struct _fontlist {
	struct _fontlist *next;
	struct _fontlist *prev;

	struct vt_list subfont;	/* sub font part */

	char name[1];			/* name of this font part */
};

struct vt_list font_list;	/* top level */

/*
  FIXME: should be calling a set-font function*/

void popup_font_callback(GtkWidget *widget, struct _fontsizelist *swn)
{
	ZvtTerm *term;
	void (*f)(ZvtTerm *, char *) = swn->f;
	
	term = swn->term;
	(*f)(term, swn->name);
#if 0

/*	???? */
	if (vx->font)
		gdk_font_unref(vx->font);
	if (vx->font_bold)
		gdk_font_unref(vx->font_bold);      

	/* currently bold/non bold is same font */
	vx->font = gdk_font_load(swn->name);
	vx->font_bold = gdk_font_load(swn->name);

	if (vx->font==0 || vx->font_bold==0) {
		printf("ARGH!, failure to get font(s) (quitting!)...\n");
		gtk_exit(1);
	}

	/* now resize window ... */
	vx->charwidth = gdk_string_width(vx->font, "M"); /* set fixed width character */
	vx->charheight = vx->font->ascent+vx->font->descent;

	/* FIXME: how can i resize this without having to know how big
	   the scrollbar is!?!?! */
	/* FIXME: this needs to be removed to other code (update.c?) ... */
	gdk_window_resize(window->window,
			  vx->charwidth * vx->vt.width + scrollbar->allocation.width,
			  vx->charheight * vx->vt.height);

	d(printf("selected font %s\n", swn->name));
#endif
}

/* sets up the font-list pop-up menu

   Creates a hierarchy of menu's based on
     Foundary
       -> Family-Weight
          -> Pixel size.

   Some of the font names have the same pixel
   size though, so some entried appear twice.
*/
GtkWidget *
create_font_menu (ZvtTerm *term, void *f)
{
	int i, j, k;
	char **fonts;
	int count;
	char *foundary, *family, *size;
	char name[256];
	int index;

	struct _fontlist *wn, *nn, *fwn, *fnn;
	struct _fontsizelist *swn, *snn;

	GtkWidget *family_menu;
	GtkWidget *size_menu;
	GtkWidget *foundary_item, *family_item, *size_item;
	GtkWidget *popup_fonts;
	
	vt_list_new(&font_list);

	/* find all fonts which match the suitable types */
	for (i=0;i<sizeof(fontlist_patterns)/sizeof(char *);i++) {
		fonts = XListFonts(GDK_DISPLAY(), fontlist_patterns[i], 64, &count);
		/* found some fonts ... */
		if (!fonts)
			continue;
		
		for (j=0;j<count;j++) {
			index=0;
			foundary = 0;
			family = 0;
			size = 0;
			strcpy(name, fonts[j]);
			for (k=0;name[k];k++) {
				if (name[k]=='-') {
					name[k]=0;
					index++;
				}
				switch(index) {
				case 1:
					if (!foundary)
						foundary = &name[k+1];
					break;
				case 2:
					if (!family)
						family = &name[k+1];
					break;
				case 3:		/* makes family-weight come out as one word */
					if (name[k]==0)
						name[k]='-';
					break;
				case 7:
					if (!size)
						size =  &name[k+1];
					break;
				}
			}
			if (index>7){
				/* add font to list ... */
				/* first, scan foundary list */
				wn = (struct _fontlist *)font_list.head;
				nn = wn->next;
				while (nn) {
					if (!strcmp(wn->name, foundary))
						break;
					wn = nn;
					nn = nn->next;
				}
				if (!nn) {
					wn = (struct _fontlist *)malloc(sizeof(struct _fontlist)+strlen(foundary));
					strcpy(wn->name, foundary);
					vt_list_new(&wn->subfont);
					vt_list_addtail(&font_list, (struct vt_listnode *)wn);
					d(printf("allocating new foundary %s\n", foundary));
				}

				/* found the foundary, find the family entry */
				fwn = (struct _fontlist *)wn->subfont.head;
				fnn = fwn->next;
				while (fnn) {
					if (!strcmp(fwn->name, family))
						break;
					fwn = fnn;
					fnn = fnn->next;
				}
				if (!fnn) {
					fwn = (struct _fontlist *)malloc(sizeof(struct _fontlist)+strlen(family));
					strcpy(fwn->name, family);
					vt_list_new(&fwn->subfont);
					vt_list_addtail(&wn->subfont, (struct vt_listnode *)fwn);
					d(printf("adding new family %s\n", family));
				}

				/* finally, append this size ... as a full font name entry */
				swn = (struct _fontsizelist *)malloc(sizeof(struct _fontsizelist)+strlen(fonts[j]));
				strcpy(swn->name, fonts[j]);
				strcpy(swn->size, size);
				swn->term = term;
				swn->f = f;
				vt_list_addtail(&fwn->subfont, (struct vt_listnode *)swn);

				/*printf("font %s %s %s\n", foundary, family, size);
				  printf("from %s\n", fonts[j]);*/
			}
		}
		XFreeFontNames(fonts);
	}

	/* Scan all fonts and build the font menu's from them */
	popup_fonts = gtk_menu_new();

	wn = (struct _fontlist *)font_list.head;
	nn = wn->next;
	while (nn) {

		foundary_item = gtk_menu_item_new_with_label(wn->name);
		gtk_widget_show(foundary_item);

		family_menu = gtk_menu_new();

		fwn = (struct _fontlist *)wn->subfont.head;
		fnn = fwn->next;
		while (fnn) {

			family_item = gtk_menu_item_new_with_label(fwn->name);
			gtk_widget_show(family_item);

			size_menu = gtk_menu_new();

			swn = (struct _fontsizelist *)fwn->subfont.head;
			snn = swn->next;
			while (snn) {

				size_item = gtk_menu_item_new_with_label(swn->size);
				gtk_menu_append(GTK_MENU (size_menu), size_item);
				gtk_signal_connect_object(GTK_OBJECT(size_item), "activate",
							  GTK_SIGNAL_FUNC(popup_font_callback), (GtkObject *)swn);
				gtk_widget_show(size_item);
				swn = snn;
				snn = snn->next;
			}

			/* set the sub-popup for the size submenu */
			gtk_menu_item_set_submenu( GTK_MENU_ITEM(family_item), size_menu);
			gtk_menu_append(GTK_MENU (family_menu), family_item);

			fwn = fnn;
			fnn = fnn->next;
		}

		/* set the sub-popup for the family submenu */
		gtk_menu_item_set_submenu( GTK_MENU_ITEM(foundary_item), family_menu);
		gtk_menu_append(GTK_MENU (popup_fonts), foundary_item);

		wn = nn;
		nn = nn->next;
	}

	return popup_fonts;
}


