/*
 * GNOME panel utils
 * (C) 1997, 1998, 1999, 2000 The Free Software Foundation
 * Copyright 2000 Helix Code, Inc.
 * Copyright 2000,2001 Eazel, Inc.
 * Copyright 2001 George Lebl
 *
 * Authors: George Lebl
 *          Jacob Berkman
 */

#include <config.h>
#include <string.h>
#include <glib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <dirent.h>

#include <libgnomeui/libgnomeui.h>

#include "panel-include.h"

extern GlobalConfig global_config;

extern GSList *applets;

void
panel_show_help (const char *doc_name, const char *linkid)
{
	gnome_help_display (NULL /* program */,
			    doc_name,
			    linkid,
			    NULL);

	/* FIXME: handle error */
}

static char *
panel_gnome_help_path (const char *docpath)
{
#ifdef FIXME
	char *fullpath, *app, *p, *path, *uri;

	app = g_strdup (docpath);

	p = strchr (app, '/');

	if (p == NULL) {
		g_free (app);
		return NULL;
	}

	path = p+1;
	*p = '\0';

	fullpath = gnome_help_file_path (app, path);

	g_free (app);

	if ( ! g_file_test (fullpath, G_FILE_TEST_EXISTS)) {
		g_free (fullpath);
		fullpath = NULL;
	}

	uri = g_strconcat ("ghelp:", fullpath, NULL);
	g_free (fullpath);

	return uri;
#else
	return NULL;
#endif
}

static char *
panel_kde_help_path (const char *docpath)
{
	const GList *li;

	if ( ! g_file_test (KDE_DOCDIR, G_FILE_TEST_EXISTS))
		return NULL;

	for (li = gnome_i18n_get_language_list ("LC_MESSAGES");
	     li != NULL;
	     li = li->next) {
		char *fullpath = g_strdup_printf ("%s/HTML/%s/%s",
						  KDE_DOCDIR,
						  (char *)li->data,
						  docpath);
		if (g_file_test (fullpath, G_FILE_TEST_EXISTS)) {
			char *uri = g_strconcat ("ghelp:", fullpath, NULL);
			g_free (fullpath);
			return uri;
		}
		g_free (fullpath);
	}

	return NULL;
}

char *
panel_gnome_kde_help_path (const char *docpath)
{
#ifdef FIXME
	char *path;

	if (string_empty (docpath))
		return NULL;

	if (panel_is_url (docpath))
		return g_strdup (docpath);

	path = panel_gnome_help_path (docpath);

	if (path == NULL)
		path = panel_kde_help_path (docpath);

	return path;
#else
	return NULL;
#endif
}

gboolean
string_is_in_list(const GSList *list, const char *text)
{
	for(;list != NULL; list = list->next)
		if(strcmp(text,(char *)list->data) == 0)
			return TRUE;
	return FALSE;
}

static void
updated (GtkWidget *w, gpointer data)
{
	UpdateFunction func = gtk_object_get_data (GTK_OBJECT (w), "update_function");

	func (data);
}

GtkWidget *
create_text_entry(GtkWidget *table,
		  const char *history_id,
		  int row,
		  const char *label,
		  const char *text,
		  UpdateFunction func,
		  gpointer data)
{
	GtkWidget *wlabel;
	GtkWidget *entry;
	GtkWidget *t;

	wlabel = gtk_label_new(label);
	gtk_misc_set_alignment(GTK_MISC(wlabel), 0.0, 0.5);
	gtk_table_attach(GTK_TABLE(table), wlabel,
			 0, 1, row, row + 1,
			 GTK_EXPAND | GTK_FILL | GTK_SHRINK,
			 GTK_FILL | GTK_SHRINK,
			 GNOME_PAD_SMALL, GNOME_PAD_SMALL);
	gtk_widget_show(wlabel);

	entry = gnome_entry_new(history_id);
	t = gnome_entry_gtk_entry (GNOME_ENTRY (entry));
	if (text)
		gtk_entry_set_text(GTK_ENTRY(t), text);
	gtk_table_attach(GTK_TABLE(table), entry,
			 1, 2, row, row + 1,
			 GTK_EXPAND | GTK_FILL | GTK_SHRINK,
			 GTK_FILL | GTK_SHRINK,
			 GNOME_PAD_SMALL, GNOME_PAD_SMALL);

	if(func) {
		gtk_object_set_data (GTK_OBJECT (t), "update_function", func);
		gtk_signal_connect (GTK_OBJECT (t), "changed",
				    GTK_SIGNAL_FUNC (updated), 
				    data);
	}
	return entry;
}

GtkWidget *
create_icon_entry(GtkWidget *table,
		  const char *history_id,
		  int cols, int cole,
		  const char *label,
		  const char *subdir,
		  const char *text,
		  UpdateFunction func,
		  gpointer data)
{
	GtkWidget *wlabel;
	GtkWidget *entry;

	wlabel = gtk_label_new(label);
	gtk_misc_set_alignment(GTK_MISC(wlabel), 0.0, 0.5);
	gtk_table_attach(GTK_TABLE(table), wlabel,
			 cols, cole, 2, 3,
			 GTK_SHRINK,
			 GTK_EXPAND | GTK_FILL | GTK_SHRINK,
			 GNOME_PAD_SMALL, GNOME_PAD_SMALL);
	gtk_widget_show(wlabel);

	entry = gnome_icon_entry_new (history_id, _("Browse"));

	gnome_icon_entry_set_pixmap_subdir (GNOME_ICON_ENTRY(entry), subdir);
	if (text != NULL)
		gnome_icon_entry_set_filename (GNOME_ICON_ENTRY(entry), text);

	gtk_table_attach(GTK_TABLE(table), entry,
			 cols, cole, 1, 2,
			 GTK_EXPAND | GTK_FILL | GTK_SHRINK,
			 GTK_FILL | GTK_SHRINK,
			 GNOME_PAD_SMALL, GNOME_PAD_SMALL);

	if(func) {
		gtk_object_set_data (GTK_OBJECT (entry), "update_function", func);
		gtk_signal_connect (GTK_OBJECT (entry), "changed",
				    GTK_SIGNAL_FUNC (updated), 
				    data);
	}

	return entry;
}

GList *
my_g_list_swap_next (GList *list, GList *dl)
{
	GList *t;

	if(!dl->next)
		return list;
	if(dl->prev)
		dl->prev->next = dl->next;
	t = dl->prev;
	dl->prev = dl->next;
	dl->next->prev = t;
	if(dl->next->next)
		dl->next->next->prev = dl;
	t = dl->next->next;
	dl->next->next = dl;
	dl->next = t;

	if(list == dl)
		return dl->prev;
	return list;
}

GList *
my_g_list_swap_prev (GList *list, GList *dl)
{
	GList *t;

	if(!dl->prev)
		return list;
	if(dl->next)
		dl->next->prev = dl->prev;
	t = dl->next;
	dl->next = dl->prev;
	dl->prev->next = t;
	if(dl->prev->prev)
		dl->prev->prev->next = dl;
	t = dl->prev->prev;
	dl->prev->prev = dl;
	dl->prev = t;

	if(list == dl->next)
		return dl;
	return list;
}

/*maybe this should be a glib function?
 it resorts a single item in the list*/
GList *
my_g_list_resort_item(GList *list, gpointer data, GCompareFunc func)
{
	GList *dl;

	if(!list)
		return NULL;

	dl = g_list_find(list,data);

	g_return_val_if_fail(dl!=NULL,list);

	while(dl->next &&
	      (*func)(dl->data,dl->next->data)>0)
		list=my_g_list_swap_next(list,dl);
	while(dl->prev &&
	      (*func)(dl->data,dl->prev->data)<0)
		list=my_g_list_swap_prev(list,dl);
	return list;
}

/*following code shamelessly stolen from gtk*/
static void
rgb_to_hls (gdouble *r,
	    gdouble *g,
	    gdouble *b)
{
  gdouble min;
  gdouble max;
  gdouble red;
  gdouble green;
  gdouble blue;
  gdouble h, l, s;
  gdouble delta;

  red = *r;
  green = *g;
  blue = *b;

  if (red > green)
    {
      if (red > blue)
	max = red;
      else
	max = blue;

      if (green < blue)
	min = green;
      else
	min = blue;
    }
  else
    {
      if (green > blue)
	max = green;
      else
	max = blue;

      if (red < blue)
	min = red;
      else
	min = blue;
    }

  l = (max + min) / 2;
  s = 0;
  h = 0;

  if (max != min)
    {
      if (l <= 0.5)
	s = (max - min) / (max + min);
      else
	s = (max - min) / (2 - max - min);

      delta = max -min;
      if (red == max)
	h = (green - blue) / delta;
      else if (green == max)
	h = 2 + (blue - red) / delta;
      else if (blue == max)
	h = 4 + (red - green) / delta;

      h *= 60;
      if (h < 0.0)
	h += 360;
    }

  *r = h;
  *g = l;
  *b = s;
}

static void
hls_to_rgb (gdouble *h,
	    gdouble *l,
	    gdouble *s)
{
  gdouble hue;
  gdouble lightness;
  gdouble saturation;
  gdouble m1, m2;
  gdouble r, g, b;

  lightness = *l;
  saturation = *s;

  if (lightness <= 0.5)
    m2 = lightness * (1 + saturation);
  else
    m2 = lightness + saturation - lightness * saturation;
  m1 = 2 * lightness - m2;

  if (saturation == 0)
    {
      *h = lightness;
      *l = lightness;
      *s = lightness;
    }
  else
    {
      hue = *h + 120;
      while (hue > 360)
	hue -= 360;
      while (hue < 0)
	hue += 360;

      if (hue < 60)
	r = m1 + (m2 - m1) * hue / 60;
      else if (hue < 180)
	r = m2;
      else if (hue < 240)
	r = m1 + (m2 - m1) * (240 - hue) / 60;
      else
	r = m1;

      hue = *h;
      while (hue > 360)
	hue -= 360;
      while (hue < 0)
	hue += 360;

      if (hue < 60)
	g = m1 + (m2 - m1) * hue / 60;
      else if (hue < 180)
	g = m2;
      else if (hue < 240)
	g = m1 + (m2 - m1) * (240 - hue) / 60;
      else
	g = m1;

      hue = *h - 120;
      while (hue > 360)
	hue -= 360;
      while (hue < 0)
	hue += 360;

      if (hue < 60)
	b = m1 + (m2 - m1) * hue / 60;
      else if (hue < 180)
	b = m2;
      else if (hue < 240)
	b = m1 + (m2 - m1) * (240 - hue) / 60;
      else
	b = m1;

      *h = r;
      *l = g;
      *s = b;
    }
}

static void
gtk_style_shade (GdkColor *a,
		 GdkColor *b,
		 gdouble   k)
{
  gdouble red;
  gdouble green;
  gdouble blue;

  red = (gdouble) a->red / 65535.0;
  green = (gdouble) a->green / 65535.0;
  blue = (gdouble) a->blue / 65535.0;

  rgb_to_hls (&red, &green, &blue);

  green *= k;
  if (green > 1.0)
    green = 1.0;
  else if (green < 0.0)
    green = 0.0;

  blue *= k;
  if (blue > 1.0)
    blue = 1.0;
  else if (blue < 0.0)
    blue = 0.0;

  hls_to_rgb (&red, &green, &blue);

  b->red = red * 65535.0;
  b->green = green * 65535.0;
  b->blue = blue * 65535.0;
}

#define LIGHTNESS_MULT  1.3
#define DARKNESS_MULT   0.7

static void
set_color_back (GtkWidget *widget, PanelWidget *panel)
{
	GtkStyle *ns;
	int i;

	gtk_widget_set_rc_style (widget);
	ns = gtk_style_copy (gtk_widget_get_style (widget));

	ns->bg[GTK_STATE_NORMAL] =
		panel->back_color;
	gtk_style_shade (&panel->back_color,
			 &ns->bg[GTK_STATE_PRELIGHT],1.5);
	gtk_style_shade (&panel->back_color,
			 &ns->bg[GTK_STATE_ACTIVE],0.8);
	ns->bg[GTK_STATE_INSENSITIVE] = 
		panel->back_color;

	for (i = 0; i < 5; i++) {
		gtk_style_shade (&ns->bg[i], &ns->light[i], LIGHTNESS_MULT);
		gtk_style_shade (&ns->bg[i], &ns->dark[i], DARKNESS_MULT);

		ns->mid[i].red = (ns->light[i].red + ns->dark[i].red) / 2;
		ns->mid[i].green = (ns->light[i].green + ns->dark[i].green) / 2;
		ns->mid[i].blue = (ns->light[i].blue + ns->dark[i].blue) / 2;
	}
	gtk_widget_set_style (widget, ns);
	gtk_style_unref (ns);
}

void
set_frame_colors (PanelWidget *panel, GtkWidget *frame,
		  GtkWidget *but1, GtkWidget *but2,
		  GtkWidget *but3, GtkWidget *but4)
{
	if (panel->back_type == PANEL_BACK_COLOR) {
		set_color_back (frame, panel);
		set_color_back (but1, panel);
		set_color_back (but2, panel);
		set_color_back (but3, panel);
		set_color_back (but4, panel);
	} else {
		gtk_widget_set_rc_style (frame);
		gtk_widget_set_rc_style (but1);
		gtk_widget_set_rc_style (but2);
		gtk_widget_set_rc_style (but3);
		gtk_widget_set_rc_style (but4);
	}
}


void
remove_directory(const char *dirname, gboolean just_clean)
{
	DIR *dir;
	struct dirent *dent;
	char *oldcwd;

	dir = opendir (dirname);
	if(!dir) return;
	oldcwd = g_get_current_dir();

	chdir(dirname);
	while((dent = readdir (dir)) != NULL) {
		if(strcmp(dent->d_name,".")==0 ||
		   strcmp(dent->d_name,"..")==0)
			continue;
		if(g_file_test(dent->d_name,G_FILE_TEST_IS_DIR))
			remove_directory(dent->d_name, FALSE);
		else
			unlink(dent->d_name);
	}
	closedir(dir);
	chdir(oldcwd);

	if(!just_clean)
		rmdir(dirname);
	g_free(oldcwd);
}

char *
strtok_with_escape(char *string, const char *seps, gboolean empty)
{
	char *our_string;
	static char *p = NULL;

	g_return_val_if_fail(seps != NULL, NULL);

	if(string)
		p = string;
	else if(!p)
		return NULL;

	our_string = p;
	while(*p) {
		if(*p == '\\' && *(p+1)) {
			strcpy(p,p+1);
		} else if(strchr(seps,*p)) {
			*p = '\0';
			if(empty || *our_string) {
				p++;
				return our_string;
			}

			/* this was an empty part and
			   we don't want to return empty
			   parts, so skip it */
			our_string = p+1;
		}
		p++;
	}
	p = NULL;

	if(!empty && !*our_string)
		return NULL;
	return our_string;
}

/* return a newly allocated string that escapes / and 'special' */
char *
escape_string(const char *string, const char *special)
{
	int count;
	const char *p;
	char *ret;
	int i;

	if(!string) return NULL;

	g_return_val_if_fail(special != NULL, NULL);

	for(p=string,count=0; *p; p++,count++) {
		if(*p == '\\' ||
		   strchr(special,*p))
			count++;
	}

	ret = g_new(char,count+1);
	i = 0;
	for(p=string; *p; p++) {
		if(*p == '\\' ||
		   strchr(special,*p))
			ret[i++] = '\\';
		ret[i++] = *p;
	}
	ret[i] = '\0';

	return ret;
}

gboolean
convert_string_to_keysym_state(const char *string,
			       guint *keysym,
			       guint *state)
{
	char *s, *p;

	g_return_val_if_fail (keysym != NULL, FALSE);
	g_return_val_if_fail (state != NULL, FALSE);
	
	*state = 0;
	*keysym = 0;

	if(string_empty (string) ||
	   strcmp (string, "Disabled") == 0 ||
	   strcmp (string, _("Disabled")) == 0)
		return FALSE;

	s = g_strdup (string);

	gdk_error_trap_push ();

	p = strtok (s, "-");
	while (p != NULL) {
		if(strcmp(p, "Control")==0) {
			*state |= GDK_CONTROL_MASK;
		} else if(strcmp(p, "Lock")==0) {
			*state |= GDK_LOCK_MASK;
		} else if(strcmp(p, "Shift")==0) {
			*state |= GDK_SHIFT_MASK;
		} else if(strcmp(p, "Mod1")==0) {
			*state |= GDK_MOD1_MASK;
		} else if(strcmp(p, "Mod2")==0) {
			*state |= GDK_MOD2_MASK;
		} else if(strcmp(p, "Mod3")==0) {
			*state |= GDK_MOD3_MASK;
		} else if(strcmp(p, "Mod4")==0) {
			*state |= GDK_MOD4_MASK;
		} else if(strcmp(p, "Mod5")==0) {
			*state |= GDK_MOD5_MASK;
		} else {
			*keysym = gdk_keyval_from_name(p);
			if(*keysym == 0) {
				gdk_flush();
				gdk_error_trap_pop();
				g_free(s);
				return FALSE;
			}
		} 
		p = strtok(NULL, "-");
	}

	gdk_flush ();
	gdk_error_trap_pop ();

	g_free (s);

	if (*keysym == 0)
		return FALSE;

	return TRUE;
}

char *
convert_keysym_state_to_string(guint keysym,
			       guint state)
{
	GString *gs;
	char *sep = "";
	char *key;

	if(keysym == 0)
		return g_strdup(_("Disabled"));

	gdk_error_trap_push();
	key = gdk_keyval_name(keysym);
	gdk_flush();
	gdk_error_trap_pop();
	if(!key) return NULL;

	gs = g_string_new(NULL);

	if(state & GDK_CONTROL_MASK) {
		/*g_string_append(gs, sep);*/
		g_string_append(gs, "Control");
		sep = "-";
	}
	if(state & GDK_LOCK_MASK) {
		g_string_append(gs, sep);
		g_string_append(gs, "Lock");
		sep = "-";
	}
	if(state & GDK_SHIFT_MASK) {
		g_string_append(gs, sep);
		g_string_append(gs, "Shift");
		sep = "-";
	}
	if(state & GDK_MOD1_MASK) {
		g_string_append(gs, sep);
		g_string_append(gs, "Mod1");
		sep = "-";
	}
	if(state & GDK_MOD2_MASK) {
		g_string_append(gs, sep);
		g_string_append(gs, "Mod2");
		sep = "-";
	}
	if(state & GDK_MOD3_MASK) {
		g_string_append(gs, sep);
		g_string_append(gs, "Mod3");
		sep = "-";
	}
	if(state & GDK_MOD4_MASK) {
		g_string_append(gs, sep);
		g_string_append(gs, "Mod4");
		sep = "-";
	}
	if(state & GDK_MOD5_MASK) {
		g_string_append(gs, sep);
		g_string_append(gs, "Mod5");
		sep = "-";
	}

	g_string_append(gs, sep);
	g_string_append(gs, key);

	{
		char *ret = gs->str;
		g_string_free(gs, FALSE);
		return ret;
	}
}

static GSList *layered_dialogs = NULL;

static void
panel_dialog_realized (GtkWidget *dialog)
{
#if FIXME
	if ( ! global_config.keep_bottom &&
	     ! global_config.normal_layer)
		gnome_win_hints_set_layer (GTK_WIDGET(dialog),
					   WIN_LAYER_ABOVE_DOCK);
	else
		gnome_win_hints_set_layer (GTK_WIDGET (dialog),
					   WIN_LAYER_NORMAL);
#endif
}

static void
remove_from_layered (GtkWidget *w)
{
	layered_dialogs = g_slist_remove (layered_dialogs, w);
}

void
panel_set_dialog_layer (GtkWidget *dialog)
{
#if FIXME
	if (g_slist_find (layered_dialogs, dialog) == NULL) {
		layered_dialogs = g_slist_prepend (layered_dialogs, dialog);
		gtk_signal_connect (GTK_OBJECT (dialog), "destroy",
				    GTK_SIGNAL_FUNC (remove_from_layered),
				    NULL);
	}

	if (GTK_WIDGET_REALIZED (dialog) &&
	    ! global_config.normal_layer &&
	    ! global_config.keep_bottom)
		gnome_win_hints_set_layer (GTK_WIDGET (dialog),
					   WIN_LAYER_ABOVE_DOCK);

	gtk_signal_connect (GTK_OBJECT (dialog), "realize",
			    GTK_SIGNAL_FUNC (panel_dialog_realized),
			    NULL);
#endif
}

void
panel_reset_dialog_layers (void)
{
#if FIXME
	GSList *li;

	for (li = layered_dialogs; li != NULL; li = li->next) {
		GtkWidget *dialog = li->data;

		if ( ! GTK_WIDGET_REALIZED (dialog))
			continue;

		if ( ! global_config.normal_layer &&
		     ! global_config.keep_bottom)
			gnome_win_hints_set_layer (GTK_WIDGET (dialog),
						   WIN_LAYER_ABOVE_DOCK);
		else
			gnome_win_hints_set_layer (GTK_WIDGET (dialog),
						   WIN_LAYER_NORMAL);
	}
#endif
}

GtkWidget *
panel_error_dialog (const char *class,
		    const char *format,
		    ...)
{
	GtkWidget *w;
	char *s;
	va_list ap;

	if (format == NULL) {
		g_warning ("NULL error dialog");
		s = g_strdup ("(null)");
	} else {
		va_start (ap, format);
		s = g_strdup_vprintf (format, ap);
		va_end (ap);
	}

	w = gtk_message_dialog_new (NULL, 0, GTK_MESSAGE_ERROR,
				    GTK_BUTTONS_OK, s);
	gtk_window_set_wmclass (GTK_WINDOW (w),
				class, "Panel");
	g_free (s);

	gtk_widget_show_all (w);
	panel_set_dialog_layer (w);

	return w;
}

gboolean
string_empty (const char *string)
{
	if (string == NULL ||
	    string[0] == '\0')
		return TRUE;
	else
		return FALSE;
}

gboolean
is_ext (const char *file, const char *ext)
{
	const char *p = strrchr (file, '.');

	if (p != NULL &&
	    strcmp (p, ext) == 0)
		return TRUE;
	else
		return FALSE;
}

/* Do strcasecmp but ignore locale */
int
strcasecmp_no_locale (const char *s1, const char *s2)
{
	int i;

	/* Error, but don't make them equal then */
	g_return_val_if_fail (s1 != NULL, G_MAXINT);
	g_return_val_if_fail (s2 != NULL, G_MININT);

	for (i = 0; s1[i] != '\0' && s2[i] != '\0'; i++) {
		char a = s1[i];
		char b = s2[i];

		if (a >= 'A' && a <= 'Z')
			a -= 'A' - 'a';
		if (b >= 'A' && b <= 'Z')
			b -= 'A' - 'a';

		if (a < b)
			return -1;
		else if (a > b)
			return 1;
	}

	/* find out which string is smaller */
	if (s2[i] != '\0')
		return -1; /* s1 is smaller */
	else if (s1[i] != '\0')
		return 1; /* s2 is smaller */
	else
		return 0; /* equal */
}

int
find_applet (GtkWidget *widget)
{
	int i;
	GSList *li;

	for (i = 0, li = applets; li != NULL; i++, li = li->next) {
		AppletInfo *info = li->data;

		if (info->widget == widget)
			return i;
	}

	return i;
}

int
get_requisition_width (GtkWidget *widget)
{
	GtkRequisition req;

	gtk_widget_get_child_requisition (widget, &req);

	return req.width;
}

int
get_requisition_height (GtkWidget *widget)
{
	GtkRequisition req;

	gtk_widget_get_child_requisition (widget, &req);

	return req.height;
}

/* is url showable by gnome_url_show */
gboolean
panel_is_url (const char *url)
{
	if (strncmp (url, "http://", strlen ("http://")) == 0 ||
	    strncmp (url, "https://", strlen ("https://")) == 0 ||
	    strncmp (url, "gopher://", strlen ("gopher://")) == 0 ||
	    strncmp (url, "ftp://", strlen ("ftp://")) == 0 ||
	    strncmp (url, "file:", strlen ("file:")) == 0 ||
	    strncmp (url, "ghelp:", strlen ("ghelp:")) == 0 ||
	    strncmp (url, "help:", strlen ("help:")) == 0 ||
	    strncmp (url, "man:", strlen ("man:")) == 0 ||
	    strncmp (url, "info:", strlen ("info:")) == 0)
		return TRUE;
	else
		return FALSE;
}

static gboolean
no_need_to_quote (const char *str)
{
	const char *p;
	
	/* empty string needs quoting */
	if (string_empty (str))
		return TRUE;

	for (p = str; *p != '\0'; p++) {
		if ((*p >= 'a' && *p <= 'z') ||
		    (*p >= 'A' && *p <= 'Z') ||
		    strchr ("_-./", *p) != NULL)
			continue;

		return FALSE;
	}
	return TRUE;
}

/* Quote string so that it could be on a shell like
 * quoted line as an argument */
char *
panel_quote_string (const char *str)
{
	GString *gs;
	const char *p;

	if (string_empty (str))
		return g_strdup ("''");

	if (no_need_to_quote (str))
		return g_strdup (str);

	gs = g_string_new ("'");
	for (p = str; *p != '\0'; p++) {
		if (*p == '\'')
			g_string_append (gs, "'\\''");
		else
			g_string_append_c (gs, *p);
	}
	g_string_append_c (gs, '\'');

	return g_string_free (gs, FALSE);
}
