/*
 * The GNOME terminal, using Michael Zucchi's zvt widget.
 * (C) 1998 The Free Software Foundation
 *
 * Authors: Miguel de Icaza (GNOME terminal)
 *          Erik Troan      (various configuration enhancements)
 *          Michael Zucchi (zvt widget, font code).
 */
#include <config.h>
#include <unistd.h>
#include <gnome.h>
#include <zvt/zvtterm.h>
#include <gdk/gdkprivate.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "gnome-terminal.h"

/* The program environment */
extern char **environ;		

char **env;

#define DEFAULT_FONT "-misc-fixed-medium-r-normal--20-200-75-75-c-100-iso8859-1"
#define EXTRA 2

/* Name of section to discard if --discard given.  */
static char *discard_section = NULL;

/* Initial geometry */
char *geometry = 0;

/* By default record utmp logins */
gboolean update_utmp = TRUE;

/* The color set */
enum color_set_enum {
	COLORS_WHITE_ON_BLACK,
	COLORS_BLACK_ON_WHITE,
	COLORS_GREEN_ON_BLACK,
	COLORS_BLACK_ON_LIGHT_YELLOW,
	COLORS_CUSTOM
};

enum scrollbar_position_enum {
	SCROLLBAR_LEFT   = 0,
	SCROLLBAR_RIGHT  = 1,
	SCROLLBAR_HIDDEN = 2
};

enum targets_enum {
        TARGET_STRING,
	TARGET_COLOR
};

struct terminal_config {
        int bell        :1;                     /* Do we want the bell? */
	int blink       :1; 			/* Do we want blinking cursor? */
	int scroll_key  :1;			/* Scroll on input? */
	int scroll_out  :1;			/* Scroll on output? */
	int swap_keys   :1;			/* Swap DEL/Backspace? */
	int color_type; 			/* The color mode */
	enum color_set_enum color_set;
	char *font; 				/* Font used by the terminals */
	int scrollback; 			/* Number of scrollbacklines */
	char *class;
	enum scrollbar_position_enum scrollbar_position;
	int invoke_as_login_shell; 		/* How to invoke the shell */
	GdkColor user_fore, user_back; 		/* The custom colors */
        const char *user_back_str, *user_fore_str;
	int menubar_hidden; 			/* Whether to show the menubar */
	int have_user_colors;			/* Only used for command line parsing */
	int transparent;
	int shaded;
	int background_pixmap;
	char * pixmap_file;
} ;

/* Initial command */
char **initial_command = NULL;

/* This is the terminal associated with the initial command, or NULL
   if there isn't one.  */
GtkWidget *initial_term = NULL;

/* A list of all the open terminals */
GList *terminals = 0;

typedef struct {
	GtkWidget *prop_win;
	GtkWidget *blink_checkbox;
	GtkWidget *scroll_kbd_checkbox;
	GtkWidget *scroll_out_checkbox;
	GtkWidget *swapkeys_checkbox;
	GtkWidget *pixmap_checkbox;
	GtkWidget *pixmap_file_entry;
	GtkWidget *transparent_checkbox;
	GtkWidget *shaded_checkbox;
	GtkWidget *menubar_checkbox;
        GtkWidget *bell_checkbox;
	GtkWidget *font_entry;
	GtkWidget *color_scheme;
	GtkWidget *def_fore_back;
	GtkWidget *scrollbar;
	GtkWidget *scrollback_spin;
	GtkWidget *class_box;
	GtkWidget *fore_cs;
	GtkWidget *back_cs;
	int changed;
} preferences_t;

static void new_terminal (GtkWidget *widget, ZvtTerm *term);
static void parse_an_arg (poptContext state,
			  enum poptCallbackReason reason,
			  const struct poptOption *opt,
			  const char *arg, void *data);

static void
about_terminal_cmd (void)
{
        GtkWidget *about;

        const gchar *authors[] = {
		"Zvt terminal widget: "
		"    Michael Zucchi (zucchi@zedzone.box.net.au)",
		"GNOME terminal: "
		"    Miguel de Icaza (miguel@kernel.org)",
		"    Erik Troan (ewt@redhat.com)",
		NULL
	};

        about = gnome_about_new (_("GNOME Terminal"), VERSION,
				 "(C) 1998 the Free Software Foundation",
				 authors,
				 _("The GNOME terminal emulation program."),
				 NULL);
        gtk_widget_show (about);
}

static void
close_app (GtkWidget *app)
{
        terminals = g_list_remove (terminals, app);

	/* FIXME: this should free the config info for the window */

	if (app == initial_term)
		initial_term = NULL;

	gtk_widget_destroy (app);

	if (terminals == NULL)
	       gtk_main_quit ();
}


static void
close_terminal_cmd (void *unused, void *data)
{
	GtkWidget *top = gtk_widget_get_toplevel (GTK_WIDGET (data));

	terminals = g_list_remove (terminals, top);
	if (top == initial_term)
		initial_term = NULL;
	gtk_widget_destroy (gtk_widget_get_toplevel (GTK_WIDGET (data)));
	if (terminals == NULL)
		gtk_main_quit ();
}

#if 0
static void
close_all_cmd (void)
{
	while (terminals)
		close_terminal_cmd (0, terminals->data);
}
#endif

/*
 * Keep a copy of the current font name
 */
static void
gnome_term_set_font (ZvtTerm *term, char *font_name)
{
	char *s;
	GdkFont *font;

	font = gdk_font_load (font_name);
	if (font)
		zvt_term_set_fonts  (term, font, font);
	
	s = gtk_object_get_user_data (GTK_OBJECT (term));
	if (s)
		g_free (s);
	gtk_object_set_user_data (GTK_OBJECT (term), g_strdup (font_name));
}

static GtkWidget *
aligned_label (char *str)
{
	GtkWidget *l;

	l = gtk_label_new (str);
	gtk_misc_set_alignment (GTK_MISC (l), 1.0, 0.5);
	return l;
}

/* Popular palettes */
gushort linux_red[] = { 0x0000, 0xaaaa, 0x0000, 0xaaaa, 0x0000, 0xaaaa, 0x0000, 0xaaaa,
			0x5555, 0xffff, 0x5555, 0xffff, 0x5555, 0xffff, 0x5555, 0xffff,
			0x0,    0x0 };
gushort linux_grn[] = { 0x0000, 0x0000, 0xaaaa, 0x5555, 0x0000, 0x0000, 0xaaaa, 0xaaaa,
			0x5555, 0x5555, 0xffff, 0xffff, 0x5555, 0x5555, 0xffff, 0xffff,
			0x0,    0x0 };
gushort linux_blu[] = { 0x0000, 0x0000, 0x0000, 0x0000, 0xaaaa, 0xaaaa, 0xaaaa, 0xaaaa,
			0x5555, 0x5555, 0x5555, 0x5555, 0xffff, 0xffff, 0xffff, 0xffff,
			0x0,    0x0 };

gushort xterm_red[] = { 0x0000, 0x6767, 0x0000, 0x6767, 0x0000, 0x6767, 0x0000, 0x6868,
			0x2a2a, 0xffff, 0x0000, 0xffff, 0x0000, 0xffff, 0x0000, 0xffff,
			0x0,    0x0 };

gushort xterm_grn[] = { 0x0000, 0x0000, 0x6767, 0x6767, 0x0000, 0x0000, 0x6767, 0x6868,
			0x2a2a, 0x0000, 0xffff, 0xffff, 0x0000, 0x0000, 0xffff, 0xffff,
			0x0,    0x0 };
gushort xterm_blu[] = { 0x0000, 0x0000, 0x0000, 0x0000, 0x6767, 0x6767, 0x6767, 0x6868,
			0x2a2a, 0x0000, 0x0000, 0x0000, 0xffff, 0xffff, 0xffff, 0xffff,
			0x0,    0x0 };

gushort rxvt_red[] = { 0x0000, 0xffff, 0x0000, 0xffff, 0x0000, 0xffff, 0x0000, 0xffff,
		       0x0000, 0xffff, 0x0000, 0xffff, 0x0000, 0xffff, 0x0000, 0xffff,
			0x0,    0x0 };
gushort rxvt_grn[] = { 0x0000, 0x0000, 0xffff, 0xffff, 0x0000, 0x0000, 0xffff, 0xffff,
		       0x0000, 0x0000, 0xffff, 0xffff, 0x0000, 0x0000, 0xffff, 0xffff,
			0x0,    0x0 };
gushort rxvt_blu[] = { 0x0000, 0x0000, 0x0000, 0x0000, 0xffff, 0xffff, 0xffff, 0xffff,
		       0x0000, 0x0000, 0x0000, 0x0000, 0xffff, 0xffff, 0xffff, 0xffff,
			0x0,    0x0 };

/* These point to the current table */
gushort *red, *blue, *green;

static void
set_color_scheme (ZvtTerm *term, struct terminal_config *color_cfg) 
{
	GdkColor c;
	
	switch (color_cfg->color_type){
	case 0:
		red = linux_red;
		green = linux_grn;
		blue = linux_blu;
		break;
	case 1:
		red = xterm_red;
		green = xterm_grn;
		blue = xterm_blu;
		break;
	default:		/* and 2 */
		red = rxvt_red;
		green = rxvt_grn;
		blue = rxvt_blu;
		break;
	}

	switch (color_cfg->color_set){
		/* White on black */
	case COLORS_WHITE_ON_BLACK:
		red   [16] = red [7];
		blue  [16] = blue [7];
		green [16] = green [7];
		red   [17] = red [0];
		blue  [17] = blue [0];
		green [17] = green [0];
		break;

		/* black on white */
	case COLORS_BLACK_ON_WHITE:
		red   [16] = red [0];
		blue  [16] = blue [0];
		green [16] = green [0];
		red   [17] = red [7];
		blue  [17] = blue [7];
		green [17] = green [7];
		break;
		
		/* Green on black */
	case COLORS_GREEN_ON_BLACK:
		red   [17] = 0;
		green [17] = 0;
		blue  [17] = 0;
		red   [16] = 0;
		green [16] = 0xffff;
		blue  [16] = 0;
		break;

		/* Black on light yellow */
	case COLORS_BLACK_ON_LIGHT_YELLOW:
		red   [16] = 0;
		green [16] = 0;
		blue  [16] = 0;
		red   [17] = 0xffff;
		green [17] = 0xffff;
		blue  [17] = 0xdddd;
		break;
		
		/* Custom foreground, custom background */
	case COLORS_CUSTOM:
		red   [16] = color_cfg->user_fore.red;
		green [16] = color_cfg->user_fore.green;
		blue  [16] = color_cfg->user_fore.blue;
		red   [17] = color_cfg->user_back.red;
		green [17] = color_cfg->user_back.green;
		blue  [17] = color_cfg->user_back.blue;
		break;
	}
	zvt_term_set_color_scheme (term, red, green, blue);
	c.pixel = term->colors [17];
	gdk_window_set_background (GTK_WIDGET (term)->parent->window, &c);
				   
	gtk_widget_queue_draw (GTK_WIDGET (term));
}

static struct terminal_config * 
load_config (char *class, gchar *prefix)
{
	char *p;
	char *fore_color = NULL;
	char *back_color = NULL;
	struct terminal_config *cfg = g_malloc (sizeof (*cfg));

	/* It's very odd that these are here */
	cfg->font = NULL;
	cfg->invoke_as_login_shell = 0;
	cfg->class = g_strdup (class);

	gnome_config_push_prefix (prefix);

	cfg->scrollback = gnome_config_get_int ("scrollbacklines=100");
	cfg->font    = gnome_config_get_string ("font=" DEFAULT_FONT);
	p = gnome_config_get_string ("scrollpos=left");
	if (strcasecmp (p, "left") == 0)
		cfg->scrollbar_position = SCROLLBAR_LEFT;
	else if (strcasecmp (p, "right") == 0)
		cfg->scrollbar_position = SCROLLBAR_RIGHT;
	else
		cfg->scrollbar_position = SCROLLBAR_HIDDEN;
	p = gnome_config_get_string ("color_scheme=linux");
	if (strcasecmp (p, "linux") == 0)
		cfg->color_type = 0;
	else if (strcasecmp (p, "xterm") == 0)
		cfg->color_type = 1;
	else
		cfg->color_type = 2;
	cfg->bell      = gnome_config_get_bool ("bell_silenced=0");
	cfg->blink     = gnome_config_get_bool ("blinking=0");
	cfg->swap_keys = gnome_config_get_bool ("swap_del_and_backspace=0");
	
	/* Default colors in the case the color set is the custom one */
	fore_color = gnome_config_get_string ("foreground=gray");
	back_color = gnome_config_get_string ("background=black");
	cfg->color_set = gnome_config_get_int ("color_set=0");

	cfg->menubar_hidden = !gnome_config_get_bool ("menubar=true");
	cfg->scroll_key = gnome_config_get_bool ("scrollonkey=true");
	cfg->scroll_out = gnome_config_get_bool ("scrollonoutput=false");

	cfg->transparent = gnome_config_get_bool ("transparent=false");
	cfg->shaded = gnome_config_get_bool ("shaded=false");
	cfg->background_pixmap = gnome_config_get_bool ("background_pixmap=false");
	cfg->pixmap_file = gnome_config_get_string ("pixmap_file");

	if (strcasecmp (fore_color, back_color) == 0)
		/* don't let them set identical foreground and background colors */
		cfg->color_set = 0;
	else {
		if (!gdk_color_parse (fore_color, &cfg->user_fore) || !gdk_color_parse (back_color, &cfg->user_back)){
			/* or illegal colors */
			cfg->color_set = 0;
		}
	}

	gnome_config_pop_prefix ();

	return cfg;
}

static struct terminal_config *
gather_changes (ZvtTerm *term)
{
	preferences_t *prefs = gtk_object_get_data (GTK_OBJECT (term), "prefs");
	gushort r, g, b;
	struct terminal_config *newcfg = g_malloc (sizeof (*newcfg));

	memset (newcfg, 0, sizeof (*newcfg));

	newcfg->bell           = GTK_TOGGLE_BUTTON (prefs->bell_checkbox)->active;
	newcfg->blink          = GTK_TOGGLE_BUTTON (prefs->blink_checkbox)->active;
	newcfg->swap_keys      = GTK_TOGGLE_BUTTON (prefs->swapkeys_checkbox)->active;
	newcfg->menubar_hidden = GTK_TOGGLE_BUTTON (prefs->menubar_checkbox)->active;
	newcfg->scroll_out     = GTK_TOGGLE_BUTTON (prefs->scroll_out_checkbox)->active;
	newcfg->scroll_key     = GTK_TOGGLE_BUTTON (prefs->scroll_kbd_checkbox)->active;
	newcfg->scrollback = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (prefs->scrollback_spin));

	newcfg->transparent = GTK_TOGGLE_BUTTON (prefs->transparent_checkbox)->active;
	newcfg->shaded = GTK_TOGGLE_BUTTON (prefs->shaded_checkbox)->active;
	newcfg->background_pixmap = GTK_TOGGLE_BUTTON (prefs->pixmap_checkbox)->active;
	newcfg->pixmap_file  = g_strdup (gtk_entry_get_text (GTK_ENTRY (prefs->pixmap_file_entry)));

	(int) newcfg->scrollbar_position = gtk_object_get_user_data (GTK_OBJECT (prefs->scrollbar));

	g_free (newcfg->font);
	newcfg->font  = g_strdup (gtk_entry_get_text (GTK_ENTRY (prefs->font_entry)));

	if (!strcmp (gtk_entry_get_text (GTK_ENTRY (GTK_COMBO (prefs->class_box)->entry)),
			_("Default")))
		newcfg->class = g_strdup (_("Config"));
	else
		newcfg->class = g_strconcat ("Class-", gtk_entry_get_text (GTK_ENTRY (GTK_COMBO (prefs->class_box)->entry)), NULL);

	newcfg->color_type = GPOINTER_TO_INT(gtk_object_get_user_data (GTK_OBJECT (prefs->color_scheme)));
	newcfg->color_set  = GPOINTER_TO_INT(gtk_object_get_user_data (GTK_OBJECT (prefs->def_fore_back)));

	gnome_color_picker_get_i16 (GNOME_COLOR_PICKER (prefs->fore_cs), &r, &g, &b, NULL);
	newcfg->user_fore.red   = r;
	newcfg->user_fore.green = g;
	newcfg->user_fore.blue  = b;
	gnome_color_picker_get_i16 (GNOME_COLOR_PICKER (prefs->back_cs), &r, &g, &b, NULL);
	newcfg->user_back.red   = r;
	newcfg->user_back.green = g;
	newcfg->user_back.blue  = b;

	return newcfg;
}

static struct terminal_config *
terminal_config_dup (struct terminal_config *cfg)
{
	struct terminal_config *n;

	n = g_malloc (sizeof (*n));
	*n = *cfg;
	n->class = g_strdup (cfg->class);
	n->font = g_strdup  (cfg->font);

	return n;
}

static void
terminal_config_free (struct terminal_config *cfg)
{
	g_free (cfg->class);
	g_free (cfg->font);
	g_free (cfg);
}

/* prototype */
static void switch_terminal_class (ZvtTerm *term, struct terminal_config *newcfg);

static int
apply_changes (ZvtTerm *term, struct terminal_config *newcfg)
{
	GtkWidget *scrollbar = gtk_object_get_data (GTK_OBJECT (term), "scrollbar");
	GtkWidget *box       = scrollbar->parent;
	GnomeApp *app = GNOME_APP (gtk_widget_get_toplevel (GTK_WIDGET (term)));
	struct terminal_config *cfg = gtk_object_get_data (GTK_OBJECT (term), "config");

	terminal_config_free (cfg);
	cfg = terminal_config_dup (newcfg);
	gtk_object_set_data (GTK_OBJECT (term), "config", cfg);

	/*zvt_term_set_font_name (term, cfg->font);*/
	gnome_term_set_font (term, cfg->font);
	zvt_term_set_bell(term, !cfg->bell);
	zvt_term_set_blink (term, cfg->blink);
	zvt_term_set_scroll_on_keystroke (term, cfg->scroll_key);
	zvt_term_set_scroll_on_output (term, cfg->scroll_out);
	zvt_term_set_scrollback (term, cfg->scrollback);
	zvt_term_set_del_key_swap (term, cfg->swap_keys);
	zvt_term_set_background (term,
				 cfg->background_pixmap?cfg->pixmap_file:NULL,
				 cfg->transparent, cfg->shaded);
	if (cfg->scrollbar_position == SCROLLBAR_HIDDEN)
		gtk_widget_hide (scrollbar);
	else {
		gtk_box_set_child_packing (GTK_BOX (box), scrollbar,
					   FALSE, TRUE, 0,
					   (cfg->scrollbar_position == SCROLLBAR_LEFT) ?
					       GTK_PACK_START : GTK_PACK_END);
		gtk_widget_show (scrollbar);
	}

	set_color_scheme (term, cfg);

	if (cfg->menubar_hidden)
		gtk_widget_hide (app->menubar);
	else
		gtk_widget_show (app->menubar);

	return 0;
}

static void
apply_changes_cmd (GtkWidget *widget, int page, ZvtTerm *term){
	struct terminal_config *newcfg, *cfg;

	if (page != -1) return;

	cfg = gtk_object_get_data (GTK_OBJECT (term), "config");
	newcfg = gather_changes (term);

	if (strcmp (cfg->class, newcfg->class)){
		switch_terminal_class (term, newcfg);
	} else {
		apply_changes (term, newcfg);
		terminal_config_free (newcfg);
	}
}

static void
switch_terminal_cb (GnomeMessageBox *mbox, gint button, void *term)
{
	struct terminal_config *newcfg, *loaded_cfg, *cfg;

	newcfg = gtk_object_get_data (GTK_OBJECT (term), "newcfg");
	cfg = gtk_object_get_data (GTK_OBJECT (term), "config");

	if (button == 0){
		/* yes */
         	char *prefix = alloca (strlen (newcfg->class) + 20);
	        sprintf (prefix, "/Terminal/%s/", newcfg->class);
		loaded_cfg = load_config (newcfg->class,prefix);
		apply_changes (term, loaded_cfg);
		terminal_config_free (loaded_cfg);
	} else if (button == 1){
		/* no */
		apply_changes (term, newcfg);
	} else {
		/* cancel -- don't do anything about the 'apply' at all */
	}
}

static void
switch_terminal_closed (GtkWidget *w, void *data)
{
	ZvtTerm *term = ZVT_TERM (data);
	struct terminal_config *newcfg;

	newcfg = gtk_object_get_data (GTK_OBJECT (term), "newcfg");
	terminal_config_free (newcfg);
	gtk_object_set_data (GTK_OBJECT (term), "newcfg", NULL);
}

static void
switch_terminal_class (ZvtTerm *term, struct terminal_config *newcfg) 
{
	GtkWidget *mbox;
	preferences_t *prefs = gtk_object_get_data (GTK_OBJECT (term), "prefs");

	gtk_object_set_data (GTK_OBJECT (term), "newcfg", newcfg);

	if (!prefs->changed){
	    /* just do it! */
	    switch_terminal_cb (NULL, 0, term);
	    free (newcfg);
	    gtk_object_set_data (GTK_OBJECT (term), "newcfg", NULL);
	    return;
	}

	mbox = gnome_message_box_new (_("You have switched the class of this window. Do you\n "
				       "want to reconfigure this window to match the default\n"
				       "configuration of the new class?"),
				 GNOME_MESSAGE_BOX_QUESTION,
                                 GNOME_STOCK_BUTTON_YES,
                                 GNOME_STOCK_BUTTON_NO, GNOME_STOCK_BUTTON_CANCEL, NULL);

	gtk_signal_connect (GTK_OBJECT (mbox), "clicked", GTK_SIGNAL_FUNC (switch_terminal_cb),
			   term);
	gtk_signal_connect (GTK_OBJECT (mbox), "destroy", GTK_SIGNAL_FUNC (switch_terminal_closed),
			   term);

	gtk_widget_show (mbox);

}

static void
window_closed (GtkWidget *w, void *data)
{
	ZvtTerm *term = ZVT_TERM (data);
	preferences_t *prefs;

	prefs = gtk_object_get_data (GTK_OBJECT (term), "prefs");
	gtk_signal_disconnect_by_data(GTK_OBJECT(prefs->pixmap_file_entry),
				      prefs);
	g_free (prefs);
	
	gtk_object_set_data (GTK_OBJECT (term), "prefs", NULL);
}

static int
window_closed_event (GtkWidget *w, GdkEvent *event, void *data)
{
	ZvtTerm *term = ZVT_TERM (data);
	
	window_closed (w, term);
	return FALSE;
}

/*
 * Called when something has changed on the properybox
 */
static void
prop_changed (GtkWidget *w, preferences_t *prefs)
{
	if (w != GTK_WIDGET (GTK_COMBO (prefs->class_box)->entry)) 
		prefs->changed = 1;
	gnome_property_box_changed (GNOME_PROPERTY_BOX (prefs->prop_win));
}

static void
color_changed (GtkWidget *w, int r, int g, int b, int a, preferences_t *prefs)
{
	prop_changed (w, prefs);
}

static void
font_changed (GtkWidget *w, preferences_t *prefs)
{
	char *font;
	GtkWidget *peer;
	if (GNOME_IS_FONT_PICKER(w)) {
		font = gnome_font_picker_get_font_name (GNOME_FONT_PICKER(w));
		peer = gtk_object_get_user_data (GTK_OBJECT(w));
		gtk_entry_set_text (GTK_ENTRY(peer), font);
	} else {
		font = gtk_entry_get_text (GTK_ENTRY(w));
		peer = gtk_object_get_user_data (GTK_OBJECT(w));
		gnome_font_picker_set_font_name (GNOME_FONT_PICKER(peer), font);
		prop_changed (w, prefs);
	}
}


/*
static void
prop_changed_zvt (void *data, char *font_name) 
{ 
	ZvtTerm *term = ZVT_TERM (data); 
	preferences_t *prefs; 
	 
	prefs = gtk_object_get_data (GTK_OBJECT (term), "prefs"); 
	gtk_entry_set_text (GTK_ENTRY (prefs->font_entry), font_name); 
	gtk_entry_set_position (GTK_ENTRY (prefs->font_entry), 0); 
} 
*/ 

typedef struct {
	GtkWidget        *menu;
	GnomePropertyBox *box;
	preferences_t	 *prefs;
	int              idx;
	void             *data1, *data2;
} lambda_t;

static void
set_active_data (GtkWidget *omenu, int idx, GtkWidget *sens1, GtkWidget *sens2)
{
	gtk_object_set_user_data (GTK_OBJECT (omenu), GINT_TO_POINTER(idx));

	if (sens1) gtk_widget_set_sensitive (GTK_WIDGET (sens1), idx == COLORS_CUSTOM);
	if (sens2) gtk_widget_set_sensitive (GTK_WIDGET (sens2), idx == COLORS_CUSTOM);
}


static void
set_active (GtkWidget *widget, lambda_t *t)
{
	set_active_data (t->menu, t->idx, t->data1, t->data2);
	t->prefs->changed = 1;
	gnome_property_box_changed (GNOME_PROPERTY_BOX (t->box));
}

static void
free_lambda (GtkWidget *w, void *l)
{
	g_free (l);
}
		    
static GtkWidget *
create_option_menu_data (GnomePropertyBox *box, preferences_t *prefs, char **menu_list, int item,
			 GtkSignalFunc func, void *data1, void *data2)
{
	GtkWidget *omenu;
	GtkWidget *menu;
	lambda_t *t;
	int i = 0;
       
	omenu = gtk_option_menu_new ();
	menu = gtk_menu_new ();
	while (*menu_list){
		GtkWidget *entry;

		t = g_new (lambda_t, 1);
		t->idx   = i;
		t->menu  = omenu;
		t->box   = box;
		t->data1 = data1;
		t->data2 = data2;
		t->prefs = prefs;
		entry = gtk_menu_item_new_with_label (_(*menu_list));
		gtk_signal_connect (GTK_OBJECT (entry), "activate", func, t);
		gtk_signal_connect (GTK_OBJECT (entry), "destroy",
				    GTK_SIGNAL_FUNC (free_lambda), t);
		gtk_widget_show (entry);
		gtk_menu_append (GTK_MENU (menu), entry);
		menu_list++;
		i++;
	}
	gtk_option_menu_set_menu (GTK_OPTION_MENU (omenu), menu);
	gtk_option_menu_set_history (GTK_OPTION_MENU (omenu), item);
	gtk_widget_show (omenu);

	return omenu;
}

static GtkWidget *
create_option_menu (GnomePropertyBox *box, preferences_t *prefs, char **menu_list, int item, GtkSignalFunc func)
{
	return create_option_menu_data (box, prefs, menu_list, item, func, NULL, NULL);
}

char *color_scheme [] = {
	N_("Linux console"),
	N_("Color Xterm"),
	N_("rxvt"),
	NULL
};

char *fore_back_table [] = {
	N_("White on black"),
	N_("Black on white"),
	N_("Green on black"),
	N_("Black on light yellow"),
	N_("Custom colors"),
	NULL
};

char *scrollbar_position_list [] = {
	N_("Left"),
	N_("Right"),
	N_("Hidden"),
	NULL
};

enum {
	COLORPAL_ROW    = 1,
	FOREBACK_ROW    = 2,
	FORECOLOR_ROW   = 3,
	BACKCOLOR_ROW   = 4,
	CLASS_ROW       = 1,
	FONT_ROW        = 2,
	BLINK_ROW       = 3,
	MENUBAR_ROW     = 4,
	BELL_ROW        = 5,
	SWAPKEYS_ROW    = 6,
	PIXMAP_ROW	= 7,
	PIXMAP_FILE_ROW	= 8,
	TRANSPARENT_ROW = 9,
	SHADED_ROW      = 10,
	SCROLL_ROW      = 1,
	SCROLLBACK_ROW  = 2,
	KBDSCROLL_ROW   = 3,
	OUTSCROLL_ROW   = 4
};

/* called back to free the ColorSelector */
static void
free_cs (GtkWidget *widget, GnomeColorSelector *cs)
{
	/*gnome_color_selector_destroy (cs);*/
}
	 
static void
preferences_cmd (GtkWidget *widget, ZvtTerm *term)
{
	GtkWidget *l, *table, *picker, *label, *b1, *b2, *e;
	preferences_t *prefs;
	GtkAdjustment *adj;
	GList *class_list = NULL;
	void *iter;
	char *some_class;
	struct terminal_config *cfg;

	/* Is a property window for this terminal already running? */
	if (gtk_object_get_data (GTK_OBJECT (term), "prefs") ||
	    gtk_object_get_data (GTK_OBJECT (term), "newcfg"))
		return;

	cfg = gtk_object_get_data (GTK_OBJECT (term), "config");

	prefs = g_new0 (preferences_t, 1);
	prefs->changed = 0;

	prefs->prop_win = gnome_property_box_new ();
	gtk_object_set_data (GTK_OBJECT (term), "prefs", prefs);

	/* general page */
	table = gtk_table_new (3, 3, FALSE);
	gnome_property_box_append_page (GNOME_PROPERTY_BOX (prefs->prop_win),
					table, gtk_label_new (_("General")));

	/* Font */
	l = aligned_label (_("Font:"));
	gtk_table_attach (GTK_TABLE (table), l,
			  1, 2, FONT_ROW, FONT_ROW+1, GTK_FILL, 0, GNOME_PAD, GNOME_PAD);
	prefs->font_entry = gtk_entry_new ();
	gtk_entry_set_text (GTK_ENTRY (prefs->font_entry),
			    gtk_object_get_user_data (GTK_OBJECT (term)));
	gtk_entry_set_position (GTK_ENTRY (prefs->font_entry), 0);
	gtk_signal_connect (GTK_OBJECT (prefs->font_entry), "changed",
			    GTK_SIGNAL_FUNC (font_changed), prefs);
		gtk_table_attach (GTK_TABLE (table), prefs->font_entry,
			  2, 3, FONT_ROW, FONT_ROW+1, GTK_FILL, 0, GNOME_PAD, GNOME_PAD);

	picker = gnome_font_picker_new();
	gnome_font_picker_set_font_name(GNOME_FONT_PICKER(picker),
					gtk_entry_get_text(GTK_ENTRY (prefs->font_entry)));
	gnome_font_picker_set_mode(GNOME_FONT_PICKER (picker),
				   GNOME_FONT_PICKER_MODE_USER_WIDGET);
	
	gtk_signal_connect (GTK_OBJECT (picker), "font_set",
			    GTK_SIGNAL_FUNC (font_changed), prefs);
	label = gtk_label_new (_("Browse..."));
	gnome_font_picker_uw_set_widget(GNOME_FONT_PICKER(picker), GTK_WIDGET(label));
	gtk_object_set_user_data(GTK_OBJECT(picker), GTK_OBJECT(prefs->font_entry)); 
	gtk_object_set_user_data (GTK_OBJECT(prefs->font_entry), GTK_OBJECT(picker)); 

	gtk_table_attach (GTK_TABLE (table), picker,
			  3, 5, FONT_ROW, FONT_ROW+1, GTK_FILL, 0, GNOME_PAD, GNOME_PAD);
	/* Terminal class */
	l = aligned_label (_("Terminal Class"));
	gtk_table_attach (GTK_TABLE (table), l,
			  1, 2, CLASS_ROW, CLASS_ROW+1, GTK_FILL, 0, GNOME_PAD, GNOME_PAD);
	prefs->class_box = gtk_combo_new ();
	iter = gnome_config_init_iterator_sections ("/Terminal");
	while (gnome_config_iterator_next (iter, &some_class, NULL)){
		if (!strcmp (some_class, "Config") || !strncmp (some_class, "Class-", 6)){
		    if (!strcmp (some_class, "Config")) 
			    some_class = _("Default");
		    else
			    some_class += 6;

		    class_list = g_list_append (class_list, some_class);
		}
	}
	if(class_list)
		gtk_combo_set_popdown_strings (GTK_COMBO (prefs->class_box), class_list);
	if (!strcmp (cfg->class, "Config")) 
		some_class = _("Default");
	else
		some_class = cfg->class + 6;
	gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (prefs->class_box)->entry), some_class);
	gtk_signal_connect (GTK_OBJECT (GTK_COMBO (prefs->class_box)->entry), "changed",
			    GTK_SIGNAL_FUNC (prop_changed), prefs);
	gtk_table_attach (GTK_TABLE (table), prefs->class_box,
			  2, 3, CLASS_ROW, CLASS_ROW+1, GTK_FILL, 0, GNOME_PAD, GNOME_PAD);
	
	/* Blinking status */
	prefs->blink_checkbox = gtk_check_button_new_with_label (_("Blinking cursor"));
	gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (prefs->blink_checkbox),
				     term->blink_enabled ? 1 : 0);
	gtk_signal_connect (GTK_OBJECT (prefs->blink_checkbox), "toggled",
			    GTK_SIGNAL_FUNC (prop_changed), prefs);
	gtk_table_attach (GTK_TABLE (table), prefs->blink_checkbox,
			  2, 3, BLINK_ROW, BLINK_ROW+1, GTK_FILL, 0, GNOME_PAD, GNOME_PAD);

	/* Show menu bar */
	prefs->menubar_checkbox = gtk_check_button_new_with_label (_("Hide menu bar"));
	gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (prefs->menubar_checkbox),
				     cfg->menubar_hidden ? 1 : 0);
	gtk_signal_connect (GTK_OBJECT (prefs->menubar_checkbox), "toggled",
			    GTK_SIGNAL_FUNC (prop_changed), prefs);
	gtk_table_attach (GTK_TABLE (table), prefs->menubar_checkbox,
			  2, 3, MENUBAR_ROW, MENUBAR_ROW+1, GTK_FILL, 0, GNOME_PAD, GNOME_PAD);

	/* Toggle the bell */
	prefs->bell_checkbox = gtk_check_button_new_with_label (_("Silence Terminal bell"));
	gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (prefs->bell_checkbox),
				     zvt_term_get_bell(term) ? 0 : 1);
	gtk_signal_connect (GTK_OBJECT (prefs->bell_checkbox), "toggled",
			    GTK_SIGNAL_FUNC (prop_changed), prefs);
	gtk_table_attach (GTK_TABLE (table), prefs->bell_checkbox,
			  2, 3, BELL_ROW, BELL_ROW+1, GTK_FILL, 0, GNOME_PAD, GNOME_PAD);

	/* Swap keys */
	prefs->swapkeys_checkbox = gtk_check_button_new_with_label (_("Swap DEL/Backspace"));
	gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (prefs->swapkeys_checkbox),
				     cfg->swap_keys ? 1 : 0);
	gtk_signal_connect (GTK_OBJECT (prefs->swapkeys_checkbox), "toggled",
			    GTK_SIGNAL_FUNC (prop_changed), prefs);
	gtk_table_attach (GTK_TABLE (table), prefs->swapkeys_checkbox,
			  2, 3, SWAPKEYS_ROW, SWAPKEYS_ROW+1, GTK_FILL, 0, GNOME_PAD, GNOME_PAD);
	
	/* Background Pixmap checkbox */
	prefs->pixmap_checkbox = gtk_check_button_new_with_label (_("Background pixmap"));
	gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (prefs->pixmap_checkbox),
				     term->pixmap_filename ? 1 : 0);
	gtk_signal_connect (GTK_OBJECT (prefs->pixmap_checkbox), "toggled",
			    GTK_SIGNAL_FUNC (prop_changed), prefs);
	gtk_table_attach (GTK_TABLE (table), prefs->pixmap_checkbox,
			  2, 3, PIXMAP_ROW, PIXMAP_ROW+1, GTK_FILL, 0, GNOME_PAD, GNOME_PAD);

	/* Background pixmap filename */
	l = aligned_label (_("Pixmap file:"));
	gtk_table_attach (GTK_TABLE (table), l,
			  1, 2, PIXMAP_FILE_ROW, PIXMAP_FILE_ROW+1, GTK_FILL, 0, GNOME_PAD, GNOME_PAD);
	e = gnome_file_entry_new ("pixmap",_("Browse"));
	prefs->pixmap_file_entry =
		gnome_file_entry_gtk_entry(GNOME_FILE_ENTRY(e));
	gtk_entry_set_text (GTK_ENTRY (prefs->pixmap_file_entry),
			    cfg->pixmap_file?cfg->pixmap_file:"");
	gtk_entry_set_position (GTK_ENTRY (prefs->pixmap_file_entry), 0);
	gtk_signal_connect (GTK_OBJECT (prefs->pixmap_file_entry), "changed",
			    GTK_SIGNAL_FUNC (prop_changed), prefs);
	gtk_table_attach (GTK_TABLE (table), e,
			  2, 5, PIXMAP_FILE_ROW, PIXMAP_FILE_ROW+1, GTK_FILL, 0, GNOME_PAD, GNOME_PAD);

	/* Transparency */
	prefs->transparent_checkbox = gtk_check_button_new_with_label (_("Transparent"));
	gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (prefs->transparent_checkbox),
				     term->transparent ? 1 : 0);
	gtk_signal_connect (GTK_OBJECT (prefs->transparent_checkbox), "toggled",
			    GTK_SIGNAL_FUNC (prop_changed), prefs);
	gtk_table_attach (GTK_TABLE (table), prefs->transparent_checkbox,
			  2, 3, TRANSPARENT_ROW, TRANSPARENT_ROW+1, GTK_FILL, 0, GNOME_PAD, GNOME_PAD);

	/* Shaded */
	prefs->shaded_checkbox = gtk_check_button_new_with_label (_("Background should be shaded (slow)"));
	gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (prefs->shaded_checkbox),
				     term->shaded ? 1 : 0);
	gtk_signal_connect (GTK_OBJECT (prefs->shaded_checkbox), "toggled",
			    GTK_SIGNAL_FUNC (prop_changed), prefs);
	gtk_table_attach (GTK_TABLE (table), prefs->shaded_checkbox,
			  2, 3, SHADED_ROW, SHADED_ROW+1, GTK_FILL, 0, GNOME_PAD, GNOME_PAD);


	/* Color page */
	table = gtk_table_new (4, 4, FALSE);
	gnome_property_box_append_page (GNOME_PROPERTY_BOX (prefs->prop_win), table, gtk_label_new (_("Colors")));
	
	/* Color palette */
	l = aligned_label (_("Color palette:"));
	gtk_table_attach (GTK_TABLE (table), l,
			  1, 2, COLORPAL_ROW, COLORPAL_ROW+1, GTK_FILL, 0, GNOME_PAD, GNOME_PAD);
	prefs->color_scheme = create_option_menu (GNOME_PROPERTY_BOX (prefs->prop_win), prefs,
						  color_scheme, cfg->color_type, GTK_SIGNAL_FUNC (set_active));
	gtk_object_set_user_data (GTK_OBJECT (prefs->color_scheme), GINT_TO_POINTER (cfg->color_type));
	gtk_table_attach (GTK_TABLE (table), prefs->color_scheme,
			  2, 6, COLORPAL_ROW, COLORPAL_ROW+1, GTK_FILL, 0, GNOME_PAD, GNOME_PAD);

	/* Foreground, background buttons */
	l = aligned_label (_("Foreground color:"));
	gtk_table_attach (GTK_TABLE (table), l,
			  1, 2, FORECOLOR_ROW, FORECOLOR_ROW+1, GTK_FILL, 0, GNOME_PAD, GNOME_PAD);
	prefs->fore_cs = gnome_color_picker_new ();
	gtk_table_attach (GTK_TABLE (table), b1 = prefs->fore_cs,
			  2, 3, FORECOLOR_ROW, FORECOLOR_ROW+1, 0, 0, GNOME_PAD, GNOME_PAD);
	gtk_signal_connect (GTK_OBJECT (b1), "destroy", GTK_SIGNAL_FUNC (free_cs), prefs->fore_cs);
	gnome_color_picker_set_i16 (GNOME_COLOR_PICKER (prefs->fore_cs), cfg->user_fore.red, cfg->user_fore.green, cfg->user_fore.blue, 0);
	
	l = aligned_label (_("Background color:"));
	gtk_table_attach (GTK_TABLE (table), l,
			  1, 2, BACKCOLOR_ROW, BACKCOLOR_ROW+1, GTK_FILL, 0, GNOME_PAD, GNOME_PAD);
	prefs->back_cs = gnome_color_picker_new ();
	gtk_table_attach (GTK_TABLE (table), b2 = prefs->back_cs,
			  2, 3, BACKCOLOR_ROW, BACKCOLOR_ROW+1, 0, 0, GNOME_PAD, GNOME_PAD);
	gtk_signal_connect (GTK_OBJECT (b2), "destroy", GTK_SIGNAL_FUNC (free_cs), prefs->back_cs);
	gnome_color_picker_set_i16 (GNOME_COLOR_PICKER (prefs->back_cs), cfg->user_back.red, cfg->user_back.green, cfg->user_back.blue, 0);
	gtk_signal_connect (GTK_OBJECT (prefs->fore_cs), "color_set", GTK_SIGNAL_FUNC (color_changed), prefs);
	gtk_signal_connect (GTK_OBJECT (prefs->back_cs), "color_set", GTK_SIGNAL_FUNC (color_changed), prefs);

	/* default foreground/backgorund selector */
	l = aligned_label (_("Colors:"));
	gtk_table_attach (GTK_TABLE (table), l,
			  1, 2, FOREBACK_ROW, FOREBACK_ROW+1, GTK_FILL, 0, GNOME_PAD, GNOME_PAD);
	prefs->def_fore_back = create_option_menu_data (GNOME_PROPERTY_BOX (prefs->prop_win), prefs,
							fore_back_table, cfg->color_set, GTK_SIGNAL_FUNC (set_active),
							b1, b2);
	set_active_data (prefs->def_fore_back, cfg->color_set, b2, b2);
	gtk_table_attach (GTK_TABLE (table), prefs->def_fore_back,
			  2, 6, FOREBACK_ROW, FOREBACK_ROW+1, GTK_FILL, 0, GNOME_PAD, GNOME_PAD);

	/* Scrolling page */
	table = gtk_table_new (4, 4, FALSE);
	gnome_property_box_append_page (GNOME_PROPERTY_BOX (prefs->prop_win), table, 
					gtk_label_new (_("Scrolling")));

	/* Scrollbar position */
	l = aligned_label (_("Scrollbar position"));
	gtk_table_attach (GTK_TABLE (table), l,
			  1, 2, SCROLL_ROW, SCROLL_ROW+1, GTK_FILL, 0, GNOME_PAD, GNOME_PAD);
	prefs->scrollbar = create_option_menu (GNOME_PROPERTY_BOX (prefs->prop_win), prefs,
					       scrollbar_position_list,
					       cfg->scrollbar_position, GTK_SIGNAL_FUNC (set_active));
	gtk_object_set_user_data(GTK_OBJECT(prefs->scrollbar), GINT_TO_POINTER(cfg->scrollbar_position));
	gtk_table_attach (GTK_TABLE (table), prefs->scrollbar,
			  2, 3, SCROLL_ROW, SCROLL_ROW+1, GTK_FILL, 0, GNOME_PAD, GNOME_PAD);

	/* Scroll back */
	l = aligned_label (_("Scrollback lines"));
        gtk_table_attach (GTK_TABLE (table), l, 1, 2, SCROLLBACK_ROW, SCROLLBACK_ROW+1, 
			  GTK_FILL, 0, GNOME_PAD, GNOME_PAD);
	adj = (GtkAdjustment *) gtk_adjustment_new ((gfloat)cfg->scrollback, 1.0, 
						    1000.0, 1.0, 5.0, 0.0);
	prefs->scrollback_spin = gtk_spin_button_new (adj, 0, 0);
	gtk_signal_connect (GTK_OBJECT (prefs->scrollback_spin), "changed",
			    GTK_SIGNAL_FUNC (prop_changed), prefs);
	gtk_table_attach (GTK_TABLE (table), prefs->scrollback_spin, 2, 3, SCROLLBACK_ROW, 
			  SCROLLBACK_ROW+1, GTK_FILL, 0, GNOME_PAD, GNOME_PAD);

	/* Scroll on keystroke checkbox */
	prefs->scroll_kbd_checkbox = gtk_check_button_new_with_label (_("Scroll on keystroke"));
	gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (prefs->scroll_kbd_checkbox),
				     cfg->scroll_key);
	gtk_signal_connect (GTK_OBJECT (prefs->scroll_kbd_checkbox), "toggled",
			    GTK_SIGNAL_FUNC (prop_changed), prefs);
	gtk_table_attach (GTK_TABLE (table), prefs->scroll_kbd_checkbox, 2, 3, 
			  KBDSCROLL_ROW, KBDSCROLL_ROW+1, GTK_FILL, 0, GNOME_PAD, GNOME_PAD);

	/* Scroll on output checkbox */
	prefs->scroll_out_checkbox = gtk_check_button_new_with_label (_("Scroll on output"));
	gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (prefs->scroll_out_checkbox),
				     cfg->scroll_out);
	gtk_signal_connect (GTK_OBJECT (prefs->scroll_out_checkbox), "toggled",
			    GTK_SIGNAL_FUNC (prop_changed), prefs);
	gtk_table_attach (GTK_TABLE (table), prefs->scroll_out_checkbox, 2, 3, 
			  OUTSCROLL_ROW, OUTSCROLL_ROW+1, GTK_FILL, 0, GNOME_PAD, GNOME_PAD);

	/* connect the property box signals */
	gtk_signal_connect (GTK_OBJECT (prefs->prop_win), "apply",
			    GTK_SIGNAL_FUNC (apply_changes_cmd), term);
	gtk_signal_connect (GTK_OBJECT (prefs->prop_win), "delete_event",
			    GTK_SIGNAL_FUNC (window_closed_event), term);
	gtk_signal_connect (GTK_OBJECT (prefs->prop_win), "destroy",
			    GTK_SIGNAL_FUNC (window_closed), term);
	gtk_widget_show_all (prefs->prop_win);
}

static void
color_ok (GtkWidget *w)
{
	gtk_widget_destroy (gtk_widget_get_toplevel (w));
}

static void
color_cmd (void)
{
	GtkWidget *c;

	c = gtk_color_selection_dialog_new (_("Color selector"));
	gtk_signal_connect (GTK_OBJECT (GTK_COLOR_SELECTION_DIALOG (c)->ok_button),
			    "clicked", GTK_SIGNAL_FUNC (color_ok), c);
	gtk_widget_hide (GTK_COLOR_SELECTION_DIALOG (c)->cancel_button);
	gtk_widget_hide (GTK_COLOR_SELECTION_DIALOG (c)->help_button);
	gtk_widget_show (c);
}

static char *
get_color_string (GdkColor c)
{
	static char buffer [40];

	sprintf (buffer, "rgb:%04x/%04x/%04x", c.red, c.green , c.blue);
	return buffer;
}

static void
save_preferences (GtkWidget *widget, ZvtTerm *term, struct terminal_config *cfg, gchar *prefix)
{
	gnome_config_push_prefix (prefix);

	if (cfg->font)
		gnome_config_set_string ("font", cfg->font);
	gnome_config_set_string ("scrollpos",
				 cfg->scrollbar_position == SCROLLBAR_LEFT ? "left" :
				 cfg->scrollbar_position == SCROLLBAR_RIGHT ? "right" : "hidden");
	gnome_config_set_bool   ("bell_silenced", cfg->bell);
	gnome_config_set_bool   ("blinking", cfg->blink);
	gnome_config_set_bool   ("swap_del_and_backspace", cfg->swap_keys);
	gnome_config_set_int    ("scrollbacklines", cfg->scrollback);
	gnome_config_set_int    ("color_set", cfg->color_set);
	gnome_config_set_string ("color_scheme",
		     cfg->color_type == 0 ? "linux" : (cfg->color_type == 1 ? "xterm" : "rxvt"));
	gnome_config_set_string ("foreground", get_color_string (cfg->user_fore));
	gnome_config_set_string ("background", get_color_string (cfg->user_back));
	gnome_config_set_bool   ("menubar", !cfg->menubar_hidden);
	gnome_config_set_bool   ("scrollonkey", cfg->scroll_key);
	gnome_config_set_bool   ("scrollonoutput", cfg->scroll_out);
	gnome_config_set_bool   ("transparent", cfg->transparent);
	gnome_config_set_bool   ("shaded", cfg->shaded);
	gnome_config_set_bool   ("background_pixmap", cfg->background_pixmap);
	gnome_config_set_string ("pixmap_file", cfg->pixmap_file);
	gnome_config_sync ();

	gnome_config_pop_prefix ();
}

static void
save_preferences_cmd (GtkWidget *widget, ZvtTerm *term)
{
	struct terminal_config *cfg = gtk_object_get_data (GTK_OBJECT (term), "config");
 	char *prefix = alloca (strlen (cfg->class) + 20);

	sprintf (prefix, "/Terminal/%s/", cfg->class);
	save_preferences (widget, term, cfg, prefix);
}

static void
show_menu_cmd (GtkWidget *widget, ZvtTerm *term)
{
	GnomeApp *app;
	struct terminal_config *cfg;

	app = GNOME_APP (gtk_widget_get_toplevel (GTK_WIDGET (term)));
	cfg = gtk_object_get_data (GTK_OBJECT (term), "config");
	cfg->menubar_hidden = 0;
	gtk_widget_show (app->menubar);
}

static void
hide_menu_cmd (GtkWidget *widget, ZvtTerm *term)
{
	GnomeApp *app;
	struct terminal_config *cfg;

	app = GNOME_APP (gtk_widget_get_toplevel (GTK_WIDGET (term)));
	cfg = gtk_object_get_data (GTK_OBJECT (term), "config");
	cfg->menubar_hidden = 1;
	gtk_widget_hide (app->menubar);
}

#define DEFINE_TERMINAL_MENU(name,text,cmd) \
\
static GnomeUIInfo name [] = {								\
	GNOMEUIINFO_ITEM_NONE (N_("_New terminal"), NULL, new_terminal),\
	GNOMEUIINFO_ITEM_NONE (N_("_Save properties as Defaults"), NULL, save_preferences_cmd),	\
	GNOMEUIINFO_SEPARATOR,\
	GNOMEUIINFO_ITEM_NONE (text, NULL, cmd),\
	GNOMEUIINFO_ITEM_STOCK (N_("_Properties..."), NULL, preferences_cmd, GNOME_STOCK_MENU_PROP), \
	/* GNOMEUIINFO_ITEM_NONE (N_("C_olor selector..."), NULL, color_cmd), */\
	GNOMEUIINFO_SEPARATOR, \
	GNOMEUIINFO_ITEM_NONE (N_("_Close terminal"),    NULL, close_terminal_cmd), 	\
	GNOMEUIINFO_END \
}

DEFINE_TERMINAL_MENU (gnome_terminal_terminal_menu_hide_menubar, N_("_Hide menubar"), hide_menu_cmd);
DEFINE_TERMINAL_MENU (gnome_terminal_terminal_menu_show_menubar, N_("_Show menubar"), show_menu_cmd);
	
static GnomeUIInfo gnome_terminal_about_menu [] = {
	GNOMEUIINFO_ITEM_STOCK (N_("_About..."), NULL,
			       about_terminal_cmd, GNOME_STOCK_MENU_ABOUT),
#if 0
	GNOMEUIINFO_SEPARATOR, 
	GNOMEUIINFO_HELP ("_Terminal"),
#endif
	GNOMEUIINFO_END
};

static GnomeUIInfo gnome_terminal_menu [] = {
	GNOMEUIINFO_SUBTREE (N_("Terminal"), &gnome_terminal_terminal_menu_hide_menubar),
	GNOMEUIINFO_SUBTREE (N_("Help"),     &gnome_terminal_about_menu),
	GNOMEUIINFO_END
};

static void
set_shell_to (char *the_shell, char **shell, char **name, int isLogin)
{
	char *only_name;
	int len;

	only_name = strrchr (the_shell, '/');
	only_name++;
	
	if (isLogin){
		len = strlen (only_name);
		
		*name  = g_malloc (len + 2);
		**name = '-';
		strcpy ((*name)+1, only_name); 
	} else
		*name = only_name;
}

/*
 * Puts in *shell a pointer to the full shell pathname
 * Puts in *name the invocation name for the shell
 * *shell is allocated on the heap.
 */
static void
get_shell_name (char **shell, char **name, int isLogin)
{
	*shell = gnome_util_user_shell ();
	set_shell_to (*shell, shell, name, isLogin);
}

static void
terminal_kill (GtkWidget *widget, void *data)
{
	GnomeApp *app = GNOME_APP (gtk_widget_get_toplevel (GTK_WIDGET (data)));

	close_terminal_cmd (widget, app);
}

/* called for "title_changed" event.  Use it to change the window title */
static void
title_changed(ZvtTerm *term, VTTITLE_TYPE type, char *newtitle)
{
  GnomeApp *window = GNOME_APP (gtk_widget_get_toplevel (GTK_WIDGET (term)));

  switch(type) {
  case VTTITLE_WINDOW:
  case VTTITLE_WINDOWICON:
    gtk_window_set_title((GtkWindow *)window, newtitle);
    break;
  default:
    break;
  }
}


static void  
drag_data_received  (GtkWidget *widget, GdkDragContext *context, 
		     gint x, gint y,
		     GtkSelectionData *selection_data, guint info,
		     guint time)
{
	ZvtTerm *term = ZVT_TERM (widget);
	int len, col, row, the_char;
	struct terminal_config *cfg;

	cfg = gtk_object_get_data (GTK_OBJECT (term), "config");

	switch (info) {
	case TARGET_STRING:
	{
		char *p = selection_data->data;
		int count = selection_data->length;
		
		do {
			len = 1 + strlen (p);
			count -= len;
			
			vt_writechild (&term->vx->vt, p, len - 1);
			vt_writechild (&term->vx->vt, " ", 1);
			p += len;
		} while (count > 0);
		break;
	}
	case TARGET_COLOR:
	{
		guint16 *data = (guint16 *)selection_data->data;

		if (selection_data->length != 8)
			return;

		col = x / term->charwidth;
		row = y / term->charheight;

		/* Switch to custom colors and */
		cfg->color_set = COLORS_CUSTOM;
		
		the_char = vt_get_attr_at (term->vx, col, row) & 0xff;
		if (the_char == ' ' || the_char == 0){
			/* copy the current foreground color */
			cfg->user_fore.red   = red [16];
			cfg->user_fore.green = green [16];
			cfg->user_fore.blue  = blue [16];

			/* Accept the dropped colors */
			cfg->user_back.red   = data [0];
			cfg->user_back.green = data [1];
			cfg->user_back.blue  = data [2];
		} else {
			/* copy the current background color */
			cfg->user_back.red   = red [17];
			cfg->user_back.green = green [17];
			cfg->user_back.blue  = blue [17];
			
			/* Accept the dropped colors */
			cfg->user_fore.red   = data [0];
			cfg->user_fore.green = data [1];
			cfg->user_fore.blue  = data [2];
		}
		set_color_scheme (term, cfg);
	}
	}
}

static void
configure_term_dnd (ZvtTerm *term)
{
	static GtkTargetEntry target_table[] = {
		{ "STRING",     0, TARGET_STRING },
		{ "text/plain", 0, TARGET_STRING },
		{ "application/x-color", 0, TARGET_COLOR }
	};

	gtk_signal_connect (GTK_OBJECT (term), "drag_data_received",
			    GTK_SIGNAL_FUNC(drag_data_received), NULL);

	gtk_drag_dest_set (GTK_WIDGET (term),
			   GTK_DEST_DEFAULT_MOTION |
			   GTK_DEST_DEFAULT_HIGHLIGHT |
			   GTK_DEST_DEFAULT_DROP,
			   target_table, 3,
			   GDK_ACTION_COPY);
}

/*
 * Performs signal connection as appropriate for interpreters or native bindings
 *
 * ALl of this nonsense is due to the limited features of gnome_app_fill_menus
 *
 */
static void
do_ui_signal_connect (GnomeUIInfo *uiinfo, gchar *signal_name, GnomeUIBuilderData *uibdata)
{
	gtk_object_set_data (GTK_OBJECT (uiinfo->widget),
			     GNOMEUIINFO_KEY_UIDATA,
			     uiinfo->user_data);

	gtk_object_set_data (GTK_OBJECT (uiinfo->widget),
			     GNOMEUIINFO_KEY_UIBDATA,
			     uibdata->data);

	gtk_signal_connect (GTK_OBJECT (uiinfo->widget), signal_name,
			    uiinfo->moreinfo,
			    uibdata->data ? uibdata->data : uiinfo->user_data);
}

static int
button_press (GtkWidget *widget, GdkEventButton *event, ZvtTerm *term)
{
	GtkWidget *menu;
	struct terminal_config *cfg;

	cfg = gtk_object_get_data (GTK_OBJECT (term), "config");

	/* FIXME: this should popup a menu instead */
	if (event->state & GDK_CONTROL_MASK && event->button == 3){
		GnomeUIInfo *uiinfo;
		GnomeUIBuilderData uib;
		menu = gtk_menu_new (); 

		if (cfg->menubar_hidden)
			uiinfo = gnome_terminal_terminal_menu_show_menubar;
		else
			uiinfo = gnome_terminal_terminal_menu_hide_menubar;

		/* All of this magic is required just to pass a *data to the menu entries */
		uib.connect_func = do_ui_signal_connect;
		uib.data = term;
		uib.is_interp = FALSE;
		uib.relay_func = NULL;
		uib.destroy_func = NULL;
		
		gtk_object_set_user_data (GTK_OBJECT (menu), term);
		gnome_app_fill_menu_custom (GTK_MENU_SHELL (menu), uiinfo, &uib, NULL, FALSE, TRUE, 0);

		gtk_menu_popup (GTK_MENU (menu), NULL, NULL, 0, NULL, 3, event->time);

		return 1;
	}

	return 0;
}

static void
term_change_pos(GtkWidget *widget)
{
	static int x=-999;
	static int y=-999;
	int nx,ny;
	
	if(!widget->window ||
	   !ZVT_TERM(widget)->transparent)
		return;
	
	gdk_window_get_position(widget->window,&nx,&ny);
	
	if(nx!=x || ny!=y)
		gtk_widget_queue_draw(widget);
}

static void
new_terminal_cmd (char **cmd, struct terminal_config *cfg_in, gchar *geometry)
{
	GtkWidget *app, *hbox, *scrollbar;
	ZvtTerm   *term;
	static char **env_copy;
	static int winid_pos;
	char buffer [40];
	char *shell, *name;
	int i = 0;
	struct terminal_config *cfg;

	/* FIXME: is seems like a lot of this stuff should be done by apply_changes instead */

	cfg = terminal_config_dup (cfg_in);

	/* Setup the environment for the gnome-terminals:
	 *
	 * TERM is set to xterm-color (which is what zvt emulates)
	 * COLORTERM is set for slang-based applications to auto-detect color
	 * WINDOWID spot is reserved for the xterm compatible variable.
	 * COLS is removed
	 * LINES is removed
	 */
	if (!env_copy){
		char **p;
		
		for (p = env; *p; p++)
			;
		i = p - env;
		env_copy = (char **) g_malloc (sizeof (char *) * (i + 1 + EXTRA));
		for (i = 0, p = env; *p; p++){
			if (strncmp (*p, "TERM=", 5) == 0)
				env_copy [i++] = "TERM=xterm";
			else if ((strncmp (*p, "COLUMNS=", 8) == 0)
				 || (strncmp (*p, "LINES=", 6) == 0)){
				/* nothing: do not copy those */
			} else
				env_copy [i++] = *p;
		}
		env_copy [i++] = "COLORTERM=gnome-terminal";
		winid_pos = i++;
		env_copy [winid_pos] = "TEST";
		env_copy [i] = NULL;
	}

	app = gnome_app_new ("Terminal", "Terminal");
	gtk_window_set_wmclass (GTK_WINDOW (app), "GnomeTerminal", "GnomeTerminal");
	if (cmd != NULL)
		initial_term = app;
#ifdef ZVT_USES_MINIMAL_ALLOC
	gtk_window_set_policy  (GTK_WINDOW (app), 1, 1, 1);
#else
	gtk_window_set_policy  (GTK_WINDOW (app), 0, 1, 1);
#endif
	gtk_widget_realize (app);
	terminals = g_list_append (terminals, app);

	/* Setup the Zvt widget */
	term = ZVT_TERM (zvt_term_new ());
	gtk_object_set_data(GTK_OBJECT(app), "term", term);
	gtk_widget_show (GTK_WIDGET (term));
	gtk_signal_connect_object(GTK_OBJECT(app),"configure_event",
				  GTK_SIGNAL_FUNC(term_change_pos),
				  GTK_OBJECT(term));
#if ZVT_USES_MINIMAL_ALLOC
	gtk_widget_set_usize (GTK_WIDGET (term),
			      80 * term->charwidth,
			      25 * term->charheight);
#endif
	zvt_term_set_scrollback (term, cfg->scrollback);
	gnome_term_set_font (term, cfg->font);
	zvt_term_set_bell  (term, !cfg->bell);
	zvt_term_set_blink (term, cfg->blink);
	zvt_term_set_scroll_on_keystroke (term, cfg->scroll_key);
	zvt_term_set_scroll_on_output (term, cfg->scroll_out);

	gtk_signal_connect (GTK_OBJECT (term), "child_died",
			    GTK_SIGNAL_FUNC (terminal_kill), term);

	gtk_signal_connect (GTK_OBJECT (term), "title_changed",
			    (GtkSignalFunc) title_changed, term);

	gtk_signal_connect (GTK_OBJECT (app), "delete_event",
			   GTK_SIGNAL_FUNC (close_app), term);

	gtk_signal_connect (GTK_OBJECT (term), "button_press_event",
			    GTK_SIGNAL_FUNC (button_press), term);
	
	gnome_app_create_menus_with_data (GNOME_APP (app), gnome_terminal_menu, term);
	if (cfg->menubar_hidden)
		gtk_widget_hide (GNOME_APP (app)->menubar);
	
	/* Decorations */
	hbox = gtk_hbox_new (0, 0);
	gtk_container_border_width (GTK_CONTAINER (hbox), 2);
	gtk_box_set_spacing (GTK_BOX (hbox), 3);
	gtk_widget_show (hbox);
	get_shell_name (&shell, &name, cfg->invoke_as_login_shell);

	scrollbar = gtk_vscrollbar_new (GTK_ADJUSTMENT (term->adjustment));
	gtk_object_set_data (GTK_OBJECT (term), "scrollbar", scrollbar);
	gtk_object_set_data (GTK_OBJECT (term), "config", cfg);
	GTK_WIDGET_UNSET_FLAGS (scrollbar, GTK_CAN_FOCUS);
	
	if (cfg->scrollbar_position == SCROLLBAR_LEFT)
		gtk_box_pack_start (GTK_BOX (hbox), scrollbar, 0, 1, 0);
	else
		gtk_box_pack_end (GTK_BOX (hbox), scrollbar, 0, 1, 0);
	if (cfg->scrollbar_position != SCROLLBAR_HIDDEN)
		gtk_widget_show (scrollbar);

	gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (term), 1, 1, 0);
	
	gnome_app_set_contents (GNOME_APP (app), hbox);

	/*
	 * Handle geometry specification, this is not quite ok, as the
	 * geometry for terminals is usually specified in terms of
	 * lines/columns
	 */
	if (geometry){
		int xpos, ypos, width, height;
		
		gnome_parse_geometry (geometry, &xpos, &ypos, &width, &height);
		if (xpos != -1 && ypos != -1)
			gtk_widget_set_uposition (GTK_WIDGET (app), xpos, ypos);
		if (width != -1 && height != -1)
			gtk_widget_set_usize (GTK_WIDGET (app), width, height);
		
		/* Only the first window gets --geometry treatment for now */
		geometry = NULL;
	}
	configure_term_dnd (term);
	zvt_term_set_background (term,
				 cfg->background_pixmap?cfg->pixmap_file:NULL,
				 cfg->transparent, cfg->shaded);
	gtk_widget_show (app);
	set_color_scheme (term, cfg);

	gdk_window_set_hints (((GtkWidget *)app)->window,
			      0, 0, 50, 50, 0, 0, GDK_HINT_MIN_SIZE);

	switch (zvt_term_forkpty (term, update_utmp)){
	case -1:
		perror ("Error: unable to fork");
		return;
		
	case 0: {
		sprintf (buffer, "WINDOWID=%d",(int) ((GdkWindowPrivate *)app->window)->xwindow);
		env_copy [winid_pos] = buffer;
		if (cmd){
			environ = env_copy;
			execvp (cmd[0], cmd);
		} else
			execle (shell, name, NULL, env_copy);
		perror ("Could not exec\n");
		_exit (127);
	}
	}

	g_free (shell);
}

static void
new_terminal (GtkWidget *widget, ZvtTerm *term)
{
	struct terminal_config *cfg;

	cfg = gtk_object_get_data (GTK_OBJECT (term), "config");

	new_terminal_cmd (NULL, cfg, NULL);
}

static gboolean
load_session (GnomeClient *client)
{
	int num_terms, i;
	gboolean def;
	gchar *section;

	section=gnome_client_get_config_prefix (client);
	gnome_config_push_prefix (section);
	num_terms = gnome_config_get_int_with_default ("dummy/num_terms", &def);
	if (def || ! num_terms)
		return FALSE;

	for (i = 0; i < num_terms; ++i){
		char buffer[50], *geom, **argv;
		struct terminal_config *cfg;
		char *class;
		int argc;
		gchar *prefix;

		sprintf (buffer, "%d/geometry", i);
		geom = gnome_config_get_string (buffer);

		sprintf (buffer, "%d/command", i);
		gnome_config_get_vector (buffer, &argc, &argv);

		sprintf (buffer, "%d/class=Default", i);
		class=gnome_config_get_string(buffer);

		sprintf (buffer, "%d/",i);
		prefix=alloca(strlen(section) + strlen(buffer) + 1);
		strcpy(prefix,section);
		strcat(prefix,buffer);
		printf("loading configuration from %s\n",prefix);
		cfg=load_config(class,prefix);
		new_terminal_cmd (argv, cfg,geom);
		g_free(cfg);
		g_free(class);

		g_free (geom);
		g_strfreev (argv);
		gnome_config_push_prefix(section);
	}

	gnome_config_pop_prefix();
	return TRUE;
}

/* Save terminal in this session.  FIXME: should save all terminal
   windows, but currently does not.  */
static gint
save_session (GnomeClient *client, gint phase, GnomeSaveStyle save_style,
	      gint is_shutdown, GnomeInteractStyle interact_style,
	      gint is_fast, gpointer client_data)
{
	if (gnome_client_get_id (client)){
		char *section = gnome_client_get_config_prefix (client);
		char *args[11];
		int argc = 0, i;
		GList *list;

		gnome_config_clean_file (section);

		i = 0;
		for (list = terminals; list != NULL; list = list->next){
		  
		        char buffer [50], *geom;
			char *prefix;
			struct terminal_config *cfg;			
			ZvtTerm *term;

			GtkWidget *top = gtk_widget_get_toplevel (GTK_WIDGET (list->data));
			
			gnome_config_push_prefix (section);
			sprintf (buffer, "%d/geometry", i);
			geom = gnome_geometry_string (top->window);
			gnome_config_set_string (buffer, geom);
			g_free (geom);

			sprintf (buffer, "%d/", i);
			prefix = alloca(strlen(section) + strlen(buffer) + 1);
			strcpy(prefix,section);
			strcat(prefix,buffer);
			term=ZVT_TERM(gtk_object_get_data(GTK_OBJECT(list->data), "term"));
			cfg=gtk_object_get_data (GTK_OBJECT (term), "config");

			sprintf(buffer, "%d/class", i);
			gnome_config_set_string(buffer,cfg->class);

			if (top == initial_term){
				int n;
				for (n = 0; initial_command[n]; ++n);
				sprintf (buffer, "%d/command", i);
				gnome_config_set_vector (buffer, n,
							 initial_command);
			}

			save_preferences(list->data,term,cfg,prefix);
			++i;
		}
		gnome_config_set_int ("dummy/num_terms", i);

#if 0 /* save_prefences above implicitly pops the prefix.  
	 This is broken, but oh well */
		gnome_config_pop_prefix ();
#endif
		gnome_config_sync ();


		args[argc++] = (char *) client_data;
/* FIXME */
#if 0
		args[argc++] = "--font";
		args[argc++] = cfg.font;
		args[argc++] = cfg.invoke_as_login_shell ? "--login" : "--nologin";
		args[argc++] = "--foreground";
		args[d1 = argc++] = g_strdup (get_color_string (cfg.user_fore));
		args[argc++] = "--background";
		args[d2 = argc++] = g_strdup (get_color_string (cfg.user_back));

		args[argc] = NULL;
#endif

		gnome_client_set_restart_command (client, argc, args);
/* Remember to put me in when above is fixed :) */
#if 0 
		g_free (args[d1]);
		g_free (args[d2]);
#endif
		args[0] = (char*)client_data;
		args[1] = "--discard";
		args[2] = section;
		args[3] = NULL;

		gnome_client_set_discard_command (client, 3, args);
	}

	return TRUE;
}

static gint
die (GnomeClient *client, gpointer client_data)
{
#if 0
	close_all_cmd ();
#endif
	return TRUE;
}

/* Keys for the ARGP parser, should be negative */
enum {
	FONT_KEY     = -1,
	NOLOGIN_KEY  = -2,
	LOGIN_KEY    = -3,
	GEOMETRY_KEY = -4,
	COMMAND_KEY  = 'e',
	FORE_KEY     = -6,
	BACK_KEY     = -7,
	DISCARD_KEY  = -8,
	CLASS_KEY    = -9,
	DOUTMP_KEY   = -10,
	DONOUTMP_KEY = -11
};

static struct poptOption cb_options [] = {
	{ NULL, '\0', POPT_ARG_CALLBACK, parse_an_arg, 0},

	{ "class", '\0', POPT_ARG_STRING, NULL, CLASS_KEY,
	  N_("Terminal class name"), N_("CLASS")},

	{ "font", '\0', POPT_ARG_STRING, NULL, FONT_KEY,
	  N_("Specifies font name"), N_("FONT")},

	{ "nologin", '\0', POPT_ARG_NONE, NULL, NOLOGIN_KEY,
	  N_("Do not start up shells as logins shells"), NULL},

	{ "login", '\0', POPT_ARG_NONE, NULL, LOGIN_KEY,
	  N_("Start up shells as logins shells"), NULL},

	{ "geometry", '\0', POPT_ARG_STRING, NULL, GEOMETRY_KEY,
	  N_("Specifies the geometry for the main window"), N_("GEOMETRY")},

	{ "command", 'e', POPT_ARG_STRING, NULL, COMMAND_KEY,
	  N_("Execute this program instead of a shell"), N_("COMMAND")},

	{ "foreground", '\0', POPT_ARG_STRING, NULL, FORE_KEY,
	  N_("Foreground color"), N_("COLOR")},

	{ "background", '\0', POPT_ARG_STRING, NULL, BACK_KEY,
	  N_("Background color"), N_("COLOR")},

	{ "discard", '\0', POPT_ARG_STRING, NULL, DISCARD_KEY,
	  NULL, N_("ID")},

	{ "utmp", '\0', POPT_ARG_NONE, NULL, DOUTMP_KEY,
	  N_("Update utmp/wtmp entries"), N_("UTMP") },

	{ "noutmp", '\0', POPT_ARG_NONE, NULL, DONOUTMP_KEY,
	  N_("Do not update utmp/wtmp entries"), N_("NOUTMP") },
	
	{ NULL, '\0', 0, NULL, 0}
};

static void
parse_an_arg (poptContext state,
	      enum poptCallbackReason reason,
	      const struct poptOption *opt,
	      const char *arg, void *data)
{
	struct terminal_config *cfg = data;

	int key = opt->val;

	switch (key){
	case CLASS_KEY:
		free (cfg->class);
		cfg->class = g_strconcat ("Class-", arg, NULL);
		break;
	case FONT_KEY:
		cfg->font = (char *)arg;
		break;
	case LOGIN_KEY:
		cfg->invoke_as_login_shell = 1;
		break;
	case NOLOGIN_KEY:
	        cfg->invoke_as_login_shell = 0;
	        break;
	case GEOMETRY_KEY:
		geometry = (char *)arg;
		break;
	case COMMAND_KEY:
	        initial_command = poptGetArgs(state);
		break;
	case FORE_KEY:
	        cfg->user_fore_str = arg;
		cfg->have_user_colors = 1;
		break;
	case BACK_KEY:
	        cfg->user_back_str = arg;
		cfg->have_user_colors = 1;
		break;
	case DISCARD_KEY:
		gnome_client_disable_master_connection ();
		discard_section = (char *)arg;
		break;
	case DOUTMP_KEY:
		update_utmp = TRUE;
		break;
	case DONOUTMP_KEY:
		update_utmp = FALSE;
		break;
	default:
	}
}

static void
session_die (gpointer client_data)
{
        gtk_main_quit ();
}

static int
main_terminal_program (int argc, char *argv [], char **environ)
{
	GnomeClient *client, *clone;
	char *program_name;
	char *class;
	struct terminal_config *default_config, *cmdline_config;
	poptContext ctx;

	env = environ;
	
	bindtextdomain (PACKAGE, GNOMELOCALEDIR);
	textdomain (PACKAGE);

	program_name = strrchr (argv[0], '/');
	if (!program_name) 
		program_name = argv[0];
	else
		program_name++;

	if (getenv ("GNOME_TERMINAL_CLASS"))
		class = g_strconcat ("Class-", getenv ("GNOME_TERMINAL_CLASS"), 
				     NULL);
	else if (strcmp (program_name, "gnome-terminal"))
		class = g_strconcat ("Class-", program_name, NULL);
	else
		class = g_strdup ("Config");
	
	cmdline_config = g_new0 (struct terminal_config, 1);
	
	cb_options[0].descrip = (char *)cmdline_config;
	
	gnome_init_with_popt_table("Terminal", VERSION, argc, argv,
				   cb_options, 0, &ctx);

	if (discard_section){
		gnome_config_clean_file (discard_section);
		return 0;
	}
	
	if(cmdline_config->user_back_str)
		gdk_color_parse(cmdline_config->user_back_str,
				&cmdline_config->user_back);
	
	if(cmdline_config->user_fore_str)
		gdk_color_parse(cmdline_config->user_fore_str,
				&cmdline_config->user_fore);
	
	if (cmdline_config->class){
		free (class);
		class = g_strdup (cmdline_config->class);
	}
	
	client = gnome_master_client ();
	gtk_signal_connect (GTK_OBJECT (client), "save_yourself",
			    GTK_SIGNAL_FUNC (save_session), argv[0]);
	gtk_signal_connect (GTK_OBJECT (client), "die",
			    GTK_SIGNAL_FUNC (session_die), NULL);
	
	{
		char *prefix = g_malloc (strlen (class) + 20);
		sprintf (prefix, "/Terminal/%s/", class);
		default_config = load_config (class,prefix);
		g_free (class);
		g_free (prefix);
	}
	
	/* now to override the defaults */
	if (cmdline_config->font){
		free (default_config->font);
		default_config->font = g_strdup (cmdline_config->font);
	}
	
	if (cmdline_config->have_user_colors){
		default_config->color_set = cmdline_config->color_set;
		default_config->user_fore = cmdline_config->user_fore;
		default_config->user_back = cmdline_config->user_back;
		default_config->color_set = COLORS_CUSTOM;
	}
	
	default_config->invoke_as_login_shell =
		cmdline_config->invoke_as_login_shell;
	
	terminal_config_free (cmdline_config);
	
	clone = gnome_cloned_client ();
	if (! clone || ! load_session (clone))
		new_terminal_cmd (initial_command, default_config, geometry);

	terminal_config_free (default_config);

	gtk_main ();
	return 0;
}

int
main (int argc, char *argv [], char **environ)
{
	return main_terminal_program (argc, argv, environ);
}
