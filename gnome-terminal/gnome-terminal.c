/*
 * The GNOME terminal, using Michael Zucchi's zvt widget.
 * (C) 1998, 1999 The Free Software Foundation
 *
 * Authors: Miguel de Icaza (GNOME terminal)
 *          Erik Troan      (various configuration enhancements)
 *          Michael Zucchi  (zvt widget, various updates and enhancements)
 *
 * Other contributors: George Lebl, Jeff Garzik, Jay Painter,
 * Christopher Blizzard, Jens Lautenbacher, Tom Tromey, Tristan Tarant,
 * Jonathan Blandford, Cody Russell, and Nat Friedman
 */
#include <config.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>

#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gdk/gdkprivate.h>
#include <gnome.h>
#include <zvt/zvtterm.h>

#include <X11/Xatom.h>

#include "gnome-terminal.h"

/* The program environment */
extern char **environ;		

/* Initial geometry */
static char *initial_global_geometry = NULL;

char **env;

#define DEFAULT_FONT "-misc-fixed-medium-r-normal--20-200-75-75-c-100-iso8859-1"
#define EXTRA 6

/* is there pixmap compiled into zvt */
static gboolean zvt_pixmap_support = FALSE;

/* small hack? whether they specified --login 
   or --nologin on the command line */
gboolean cmdline_login;

/* The color set */
enum color_set_enum {
	COLORS_WHITE_ON_BLACK,
	COLORS_BLACK_ON_WHITE,
	COLORS_GREEN_ON_BLACK,
	COLORS_BLACK_ON_LIGHT_YELLOW,
	COLORS_CUSTOM
};

enum palette_enum {
	PALETTE_LINUX,
	PALETTE_XTERM,
	PALETTE_RXVT,
	PALETTE_CUSTOM
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
	int keyboard_secured :1;		/* Does this terminal have the keyboard secured? */
        int bell             :1;                /* Do we want the bell? */
	int blink            :1; 		/* Do we want blinking cursor? */
	int scroll_key       :1;       		/* Scroll on input? */
	int scroll_out       :1;       		/* Scroll on output? */
	int swap_keys        :1;       		/* Swap DEL/Backspace? */
	int login_by_default :1;                /* do --login as default */
#ifdef ZVT_BACKGROUND_SCROLL
	int scroll_background:1; 		/* background will scroll */
#endif
	enum palette_enum color_type; 			/* The color mode */
	enum color_set_enum color_set;
	char *font; 				/* Font used by the terminals */
	int scrollback; 			/* Number of scrollbacklines */
	char *class;
	enum scrollbar_position_enum scrollbar_position;
	int invoke_as_login_shell; 		/* How to invoke the shell */
	int update_records;
	int update_records_and, update_records_xor;
	const char *user_back_str, *user_fore_str;
	int menubar_hidden; 			/* Whether to show the menubar */
	int have_user_colors;			/* Only used for command line parsing */
	int transparent;
	int shaded;
	int background_pixmap;
	char *pixmap_file;
        char *window_title;                     /* the window title */
	char *wordclass;			/* select-by-word character class */
	char *termname;				/* TERM variable setting, store as TERM=xxx */
	GdkColor palette[18];			/* the full palette */
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
	GtkWidget *login_by_default_checkbox;
	GtkWidget *wordclass_entry;
	GtkWidget *pixmap_checkbox;
	GtkWidget *pixmap_file_entry;
	GtkWidget *pixmap_label;
	GtkWidget *pixmap_entry;
	GtkWidget *transparent_checkbox;
	GtkWidget *shaded_checkbox;
#ifdef ZVT_BACKGROUND_SCROLL
	GtkWidget *pixmap_scrollable_checkbox;
#endif
	GtkWidget *menubar_checkbox;
        GtkWidget *bell_checkbox;
	GtkWidget *font_entry;
	GtkWidget *color_scheme;
	GtkWidget *def_fore_back;
	GtkWidget *scrollbar;
	GtkWidget *scrollback_spin;
	GtkWidget *class_box;
	GtkWidget *fore_label;
	GtkWidget *back_label;
	GtkWidget *palette_label;
	GtkWidget *palette[18];
	int changed;
} preferences_t;

/*
 * These are the indices for the toggle items in the popup menu.  If
 * you change the popup menus, these macros MUST be updated to reflect
 * the changes.
 */
#define POPUP_MENU_TOGGLE_INDEX_MENUBAR 2
#define POPUP_MENU_TOGGLE_INDEX_SECURE  3
/* eek, multi-conditaional for backward compatability *sigh* */
#ifdef ZVT_TERM_MATCH_SUPPORT
# ifdef HAVE_ZVT_TERM_RESET
#  define POPUP_MENU_DYNAMIC_INDEX 6
#  define POPUP_MENU_LAST_INDEX 7
# else
#  define POPUP_MENU_DYNAMIC_INDEX 4
#  define POPUP_MENU_LAST_INDEX 5
# endif
#endif

/*
 * Exported interfaces, for Gtk modules that hook
 * into the gnome terminal
 */
void close_terminal_cmd   (void *unused, void *data);
void save_preferences_cmd (GtkWidget *widget, ZvtTerm *term);
void color_cmd            (void);
void toggle_menubar_cmd   (GtkWidget *widget, ZvtTerm *term);
void paste_cmd            (GtkWidget *widget, ZvtTerm *term);
void preferences_cmd      (GtkWidget *widget, ZvtTerm *term);
void toggle_secure_keyboard_cmd (GtkWidget *w, ZvtTerm *term);

static int popup_menu_cmd (ZvtTerm *term, GdkEventButton *event,
			   GtkWidget *menu, GnomeUIInfo *uiinfo);

GtkWidget *new_terminal_cmd (char **cmd, struct terminal_config *cfg_in, gchar *geometry);
GtkWidget *new_terminal     (GtkWidget *widget, ZvtTerm *term);

static void parse_an_arg (poptContext state,
			  enum poptCallbackReason reason,
			  const struct poptOption *opt,
			  const char *arg, void *data);

static void set_hints (GtkWidget *widget);

static void
about_terminal_cmd (void)
{
        GtkWidget *about;

        const gchar *authors[] = {
		"Zvt terminal widget: "
		"    Michael Zucchi (zucchi@zedzone.mmc.com.au)",
		"GNOME terminal: "
		"    Miguel de Icaza (miguel@kernel.org)",
		"    Erik Troan (ewt@redhat.com)",
		NULL
	};

        about = gnome_about_new (_("GNOME Terminal"), VERSION,
				 "(C) 1998, 1999 the Free Software Foundation",
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


void
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
	if (font) {
#ifdef ZVT_TERM_EMBOLDEN_SUPPORT
		if (zvt_term_get_capabilities(term) & ZVT_TERM_EMBOLDEN_SUPPORT)
			zvt_term_set_fonts  (term, font, 0);
		else
#endif
			zvt_term_set_fonts  (term, font, font);
	}

	if (GTK_WIDGET_REALIZED (term))
		set_hints (GTK_WIDGET (term));

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

/* This really should be in GTK+
 */
static gint
option_menu_get_history (GtkOptionMenu *option_menu)
{
	GtkWidget *active_widget;
	
	g_return_val_if_fail (GTK_IS_OPTION_MENU (option_menu), -1);
	
	active_widget = gtk_menu_get_active (GTK_MENU (option_menu->menu));

	if (active_widget)
		return g_list_index (GTK_MENU_SHELL (option_menu->menu)->children,
				     active_widget);
	else
		return -1;
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

/* point to the other tables */
gushort *scheme_red[] = { linux_red, xterm_red, rxvt_red, rxvt_red };
gushort *scheme_blue[] = { linux_blu, xterm_blu, rxvt_blu, rxvt_blu };
gushort *scheme_green[] = { linux_grn, xterm_grn, rxvt_grn, rxvt_grn };

static void
set_color_scheme (ZvtTerm *term, struct terminal_config *color_cfg) 
{
	GdkColor c;
	gushort red[18],green[18],blue[18];
	int i;
	gushort *r, *b, *g;

	switch (color_cfg->color_type){
	default:		/* and 0 */
		color_cfg->color_type = 0;
	case PALETTE_LINUX:
	case PALETTE_XTERM:
	case PALETTE_RXVT:
		r = scheme_red[color_cfg->color_type];
		g = scheme_green[color_cfg->color_type];
		b = scheme_blue[color_cfg->color_type];
		for (i=0;i<18;i++) {
			red[i] = r[i];
			green[i] = g[i];
			blue[i] = b[i];
		}
		break;
	case PALETTE_CUSTOM:
		for (i=0;i<18;i++) {
			red[i] = color_cfg->palette[i].red;
			green[i] = color_cfg->palette[i].green;
			blue[i] = color_cfg->palette[i].blue;
		}
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
		for (i=16;i<18;i++) {
			red[i] = color_cfg->palette[i].red;
			green[i] = color_cfg->palette[i].green;
			blue[i] = color_cfg->palette[i].blue;
		}
		break;
	}
	zvt_term_set_color_scheme (term, red, green, blue);
	c.pixel = term->colors [17];

	gdk_window_set_background (GTK_WIDGET (term)->window, &c);
	gtk_widget_queue_draw (GTK_WIDGET (term));
}

static struct terminal_config * 
load_config (char *class)
{
	char *p;
	char *fore_color = NULL;
	char *back_color = NULL;
	struct terminal_config *cfg = g_malloc (sizeof (*cfg));
	int colour_count;
	char **colours;
	int i;

	/* It's very odd that these are here */
	cfg->font = NULL;
	cfg->class = g_strdup (class);

	cfg->scrollback = gnome_config_get_int ("scrollbacklines=100");
	cfg->font    = gnome_config_get_string ("font=" DEFAULT_FONT);
	cfg->wordclass  = gnome_config_get_string ("wordclass=-A-Za-z0-9,./?%&#");
	p = gnome_config_get_string ("scrollpos=right");
	if (strcasecmp (p, "left") == 0)
		cfg->scrollbar_position = SCROLLBAR_LEFT;
	else if (strcasecmp (p, "right") == 0)
		cfg->scrollbar_position = SCROLLBAR_RIGHT;
	else
		cfg->scrollbar_position = SCROLLBAR_HIDDEN;
	p = gnome_config_get_string ("color_scheme=linux");
	if (strcasecmp (p, "linux") == 0)
		cfg->color_type = PALETTE_LINUX;
	else if (strcasecmp (p, "xterm") == 0)
		cfg->color_type = PALETTE_XTERM;
	else if (strcasecmp (p, "rxvt") == 0)
		cfg->color_type = PALETTE_RXVT;
	else if (strcasecmp (p, "custom") == 0)
		cfg->color_type = PALETTE_CUSTOM;
	else
		cfg->color_type = PALETTE_LINUX;
	cfg->bell      = gnome_config_get_bool ("bell_silenced=0");
	cfg->blink     = gnome_config_get_bool ("blinking=0");
	cfg->swap_keys = gnome_config_get_bool ("swap_del_and_backspace=0");

	cfg->login_by_default = gnome_config_get_bool ("login_by_default=0");

#ifdef ZVT_BACKGROUND_SCROLL
	cfg->scroll_background = gnome_config_get_bool ("scroll_background=0");
#endif
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
	cfg->window_title = NULL;

	cfg->termname = NULL;

	cfg->update_records = ZVT_TERM_DO_UTMP_LOG|ZVT_TERM_DO_WTMP_LOG;

	if (strcasecmp (fore_color, back_color) == 0)
		/* don't let them set identical foreground and background colors */
		cfg->color_set = 0;
	else {
		if (!gdk_color_parse (fore_color, &cfg->palette[16])
		    || !gdk_color_parse (back_color, &cfg->palette[17])){
			/* or illegal colors */
			cfg->color_set = 0;
		}
	}

	/* load the palette, if none, then 'default' to safe (linux) pallete */
	gnome_config_get_vector("palette", &colour_count, &colours);
	for (i=0;i<18;i++) {
		if (i<colour_count)
			gdk_color_parse(colours[i], &cfg->palette[i]);
		else if (i<16) {
			cfg->palette[i].red = linux_red[i];
			cfg->palette[i].green = linux_grn[i];
			cfg->palette[i].blue = linux_blu[i];
		}
	}
	g_free(colours);

	return cfg;
}

static struct terminal_config *
gather_changes (ZvtTerm *term)
{
	preferences_t *prefs = gtk_object_get_data (GTK_OBJECT (term), "prefs");
	gushort r, g, b;
	struct terminal_config *newcfg = g_malloc (sizeof (*newcfg));
	int i;

	memset (newcfg, 0, sizeof (*newcfg));

	newcfg->bell           = GTK_TOGGLE_BUTTON (prefs->bell_checkbox)->active;
	newcfg->blink          = GTK_TOGGLE_BUTTON (prefs->blink_checkbox)->active;
	newcfg->swap_keys      = GTK_TOGGLE_BUTTON (prefs->swapkeys_checkbox)->active;
	newcfg->menubar_hidden = GTK_TOGGLE_BUTTON (prefs->menubar_checkbox)->active;
	newcfg->scroll_out     = GTK_TOGGLE_BUTTON (prefs->scroll_out_checkbox)->active;
	newcfg->scroll_key     = GTK_TOGGLE_BUTTON (prefs->scroll_kbd_checkbox)->active;
	newcfg->scrollback = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (prefs->scrollback_spin));
	newcfg->login_by_default = GTK_TOGGLE_BUTTON (prefs->login_by_default_checkbox)->active;

	newcfg->transparent = GTK_TOGGLE_BUTTON (prefs->transparent_checkbox)->active;
	newcfg->shaded = GTK_TOGGLE_BUTTON (prefs->shaded_checkbox)->active;
	newcfg->background_pixmap = GTK_TOGGLE_BUTTON (prefs->pixmap_checkbox)->active;
	newcfg->pixmap_file  = g_strdup (gtk_entry_get_text (GTK_ENTRY (prefs->pixmap_entry)));
	newcfg->wordclass = g_strdup (gtk_entry_get_text (GTK_ENTRY (prefs->wordclass_entry)));
	newcfg->scrollbar_position = option_menu_get_history (GTK_OPTION_MENU (prefs->scrollbar));

#ifdef ZVT_BACKGROUND_SCROLL
	newcfg->scroll_background = GTK_TOGGLE_BUTTON (prefs->pixmap_scrollable_checkbox)->active
		&& newcfg->background_pixmap;
#endif

	g_free (newcfg->font);
	newcfg->font  = g_strdup (gtk_entry_get_text (GTK_ENTRY (prefs->font_entry)));

	if (!strcmp (gtk_entry_get_text (GTK_ENTRY (GTK_COMBO (prefs->class_box)->entry)),
			_("Default")))
		newcfg->class = g_strdup ("Config");
	else
		newcfg->class = g_strconcat ("Class-", gtk_entry_get_text (GTK_ENTRY (GTK_COMBO (prefs->class_box)->entry)), NULL);

	newcfg->color_type = option_menu_get_history (GTK_OPTION_MENU (prefs->color_scheme));
	newcfg->color_set  = option_menu_get_history (GTK_OPTION_MENU (prefs->def_fore_back));

	for (i=0;i<18;i++) {
		gnome_color_picker_get_i16 (GNOME_COLOR_PICKER (prefs->palette[i]), &r, &g,
					    &b, NULL);
		newcfg->palette[i].red   = r;
		newcfg->palette[i].green = g;
		newcfg->palette[i].blue  = b;
	}

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
	n->wordclass = g_strdup  (cfg->wordclass);
	n->termname = g_strdup  (cfg->termname);

	return n;
}

static void
terminal_config_free (struct terminal_config *cfg)
{
	g_free (cfg->class);
	g_free (cfg->font);
	g_free (cfg->wordclass);
	g_free (cfg->termname);
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

	gnome_term_set_font (term, cfg->font);
	zvt_term_set_wordclass (term, (guchar *)cfg->wordclass);
	zvt_term_set_bell(term, !cfg->bell);
	zvt_term_set_blink (term, cfg->blink);
	zvt_term_set_scroll_on_keystroke (term, cfg->scroll_key);
	zvt_term_set_scroll_on_output (term, cfg->scroll_out);
	zvt_term_set_scrollback (term, cfg->scrollback);
	zvt_term_set_del_key_swap (term, cfg->swap_keys);

	if (zvt_pixmap_support && cfg->background_pixmap) {
		int flags;
#ifdef ZVT_BACKGROUND_SCROLL
		flags = cfg->shaded?ZVT_BACKGROUND_SHADED:0;
		flags |= cfg->scroll_background?ZVT_BACKGROUND_SCROLL:0;
#else
		flags = cfg->shaded;
#endif
		zvt_term_set_background (term,
					 cfg->pixmap_file,
					 cfg->transparent, flags);
	} else if (zvt_pixmap_support && cfg->transparent)
		zvt_term_set_background (term,
					 NULL,
					 cfg->transparent, cfg->shaded);
	else
		zvt_term_set_background (term, NULL, 0, 0);

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
		gtk_widget_hide (app->menubar->parent);
	else
		gtk_widget_show (app->menubar->parent);

	return 0;
}

void save_preferences_cmd (GtkWidget *widget, ZvtTerm *term);

static void
apply_changes_cmd (GtkWidget *widget, int page, ZvtTerm *term)
{
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
	save_preferences_cmd (widget, term);
}

static void
switch_terminal_cb (GnomeMessageBox *mbox, gint button, void *term)
{
	struct terminal_config *newcfg, *loaded_cfg, *cfg;

	newcfg = gtk_object_get_data (GTK_OBJECT (term), "newcfg");
	cfg = gtk_object_get_data (GTK_OBJECT (term), "config");

	if (button == 0){
		/* yes */
         	char *prefix = g_strdup_printf("/Terminal/%s/", newcfg->class);
		gnome_config_push_prefix (prefix);
		loaded_cfg = load_config (newcfg->class);
		apply_changes (term, loaded_cfg);
		terminal_config_free (loaded_cfg);
		gnome_config_pop_prefix ();
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
	    terminal_config_free (newcfg);
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
window_destroy (GtkWidget *w, gpointer data)
{
	ZvtTerm *term = ZVT_TERM (data);
	preferences_t *prefs;
	char *tmp;

	prefs = gtk_object_get_data (GTK_OBJECT (term), "prefs");
	gtk_signal_disconnect_by_data (GTK_OBJECT (prefs->pixmap_entry), prefs);
	g_free (prefs);
	gtk_object_set_data (GTK_OBJECT (term), "prefs", NULL);

	tmp = gtk_object_get_data (GTK_OBJECT (term), "matchstr");
	if (tmp) {
		g_free(tmp);
		gtk_object_set_data(GTK_OBJECT (term), "matchstr", NULL);
	}
}

/* 
 * Set the sensitivity of the Image tab of the property  
 * dialog based on the values of the checkbuttons
 */

static void
check_image_options_sensitivity (preferences_t *prefs)
{
	gboolean has_background = GTK_TOGGLE_BUTTON (prefs->pixmap_checkbox)->active;
	gboolean has_transparency = GTK_TOGGLE_BUTTON (prefs->transparent_checkbox)->active;

	gtk_widget_set_sensitive (prefs->pixmap_label, has_background);
	gtk_widget_set_sensitive (prefs->pixmap_file_entry, has_background);
	gtk_widget_set_sensitive (prefs->shaded_checkbox,
				  has_background || has_transparency);
#ifdef ZVT_BACKGROUND_SCROLL
	gtk_widget_set_sensitive (prefs->pixmap_scrollable_checkbox,
				  has_background);
#endif
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

	check_image_options_sensitivity (prefs);
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
	if (GNOME_IS_FONT_PICKER(w)){
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
	GnomePropertyBox *box;
	preferences_t	 *prefs;
} lambda_t;

static void
check_color_sensitivity (preferences_t *prefs)
{
	int idxc = option_menu_get_history (GTK_OPTION_MENU (prefs->def_fore_back));
	gboolean sensc = (idxc == COLORS_CUSTOM);
	int idx = option_menu_get_history (GTK_OPTION_MENU (prefs->color_scheme));
	gboolean sens = (idx == PALETTE_CUSTOM);

	int i;

	gtk_widget_set_sensitive (GTK_WIDGET (prefs->fore_label), sensc);
	gtk_widget_set_sensitive (GTK_WIDGET (prefs->back_label), sensc);
	gtk_widget_set_sensitive (GTK_WIDGET (prefs->palette_label), sens);
	for (i=0;i<18;i++)
		gtk_widget_set_sensitive (GTK_WIDGET (prefs->palette[i]), i<16?sens:sensc);
}

static void
set_active (GtkWidget *widget, lambda_t *t)
{
	check_color_sensitivity (t->prefs);
	t->prefs->changed = 1;
	gnome_property_box_changed (GNOME_PROPERTY_BOX (t->box));
}

static GtkWidget *
create_option_menu_data (GnomePropertyBox *box, preferences_t *prefs, char **menu_list, int item,
			 GtkSignalFunc func)
{
	GtkWidget *omenu;
	GtkWidget *menu;
	lambda_t *t;
	int i = 0;
       
	omenu = gtk_option_menu_new ();
	menu = gtk_menu_new ();
	while (*menu_list){
		GtkWidget *entry;

		entry = gtk_menu_item_new_with_label (_(*menu_list));
		gtk_widget_show (entry);
		gtk_menu_append (GTK_MENU (menu), entry);
		menu_list++;
		i++;
	}
	gtk_option_menu_set_menu (GTK_OPTION_MENU (omenu), menu);
	gtk_option_menu_set_history (GTK_OPTION_MENU (omenu), item);
	gtk_widget_show (omenu);

	t = g_new (lambda_t, 1);
	t->box   = box;
	t->prefs = prefs;

	gtk_signal_connect_full (GTK_OBJECT (menu), 
				 "deactivate",
				 GTK_SIGNAL_FUNC (set_active), NULL,
				 t, (GtkDestroyNotify)g_free,
				 FALSE, FALSE);

	return omenu;
}

static GtkWidget *
create_option_menu (GnomePropertyBox *box, preferences_t *prefs, char **menu_list, int item, GtkSignalFunc func)
{
	return create_option_menu_data (box, prefs, menu_list, item, func);
}

char *color_scheme [] = {
	N_("Linux console"),
	N_("Color Xterm"),
	N_("rxvt"),
	N_("Custom"),
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
	LOGIN_ROW       = 7,
	WORDCLASS_ROW	= 8,
	BACKGROUND_ROW	= 1,
	PIXMAP_FILE_ROW	= 2,
	SHADED_ROW      = 4,
	SCROLL_ROW      = 1,
	SCROLLBACK_ROW  = 2,
	KBDSCROLL_ROW   = 3,
	OUTSCROLL_ROW   = 4
};

/* called back to free the ColorSelector */
static void
free_cs (GtkWidget *widget, GnomeColorPicker *cp)
{
}
	 
static void
save_preferences (GtkWidget *widget, ZvtTerm *term, 
		  struct terminal_config *cfg)
{
	char *colours[18];
	int i;
	char *scheme[4] = { "linux", "xterm", "rxvt", "custom" };

	if (cfg->font)
		gnome_config_set_string ("font", cfg->font);
	if (cfg->wordclass)
		gnome_config_set_string ("wordclass", cfg->wordclass);
	gnome_config_set_string ("scrollpos",
				 cfg->scrollbar_position == SCROLLBAR_LEFT ? "left" :
				 cfg->scrollbar_position == SCROLLBAR_RIGHT ? "right" : "hidden");
	gnome_config_set_bool   ("bell_silenced", cfg->bell);
	gnome_config_set_bool   ("blinking", cfg->blink);
	gnome_config_set_bool   ("swap_del_and_backspace", cfg->swap_keys);
	gnome_config_set_bool   ("login_by_default", cfg->login_by_default);
	gnome_config_set_int    ("scrollbacklines", cfg->scrollback);
	gnome_config_set_int    ("color_set", cfg->color_set);
	if (cfg->color_type>=4)
		cfg->color_type=4;
	gnome_config_set_string ("color_scheme", scheme[cfg->color_type]);
	gnome_config_set_bool   ("menubar", !cfg->menubar_hidden);
	gnome_config_set_bool   ("scrollonkey", cfg->scroll_key);
	gnome_config_set_bool   ("scrollonoutput", cfg->scroll_out);
	gnome_config_set_bool   ("transparent", cfg->transparent);
	gnome_config_set_bool   ("shaded", cfg->shaded);
#ifdef ZVT_BACKGROUND_SCROLL
	gnome_config_set_bool   ("scroll_background", cfg->scroll_background);
#endif
	gnome_config_set_bool   ("background_pixmap", cfg->background_pixmap);
	gnome_config_set_string ("pixmap_file", cfg->pixmap_file);

	for (i=0;i<18;i++)
		colours[i] = g_strdup_printf ("rgb:%04x/%04x/%04x", cfg->palette[i].red,
					      cfg->palette[i].green, cfg->palette[i].blue);
	gnome_config_set_vector ("palette", 18, (const char **)colours);
	for (i=0;i<18;i++)
		g_free(colours[i]);

	gnome_config_sync ();
}

void
save_preferences_cmd (GtkWidget *widget, ZvtTerm *term)
{
	struct terminal_config *cfg = 
		gtk_object_get_data (GTK_OBJECT (term), "config");
	char *prefix = g_strdup_printf ("/Terminal/%s/", cfg->class);

	gnome_config_push_prefix (prefix);
	save_preferences (widget, term, cfg);
	gnome_config_pop_prefix ();
}

void
preferences_cmd (GtkWidget *widget, ZvtTerm *term)
{
	static GnomeHelpMenuEntry help_entry = { NULL, "properties" };
	GtkWidget *l, *table, *picker, *label, *b1, *b2;
	GtkWidget *r, *frame, *hbox, *subtable, *paltable;
	preferences_t *prefs;
	GtkAdjustment *adj;
	GList *class_list = NULL;
	void *iter;
	char *some_class;
	struct terminal_config *cfg;
	int i;

	/* Is a property window for this terminal already running? */
	if (gtk_object_get_data (GTK_OBJECT (term), "newcfg"))
		return;
	
	prefs = gtk_object_get_data (GTK_OBJECT (term), "prefs");
	if (prefs) {
		/* Raise and possibly uniconify the property box */
		gdk_window_show (prefs->prop_win->window);
		return;
	}

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
	gnome_dialog_editable_enters (GNOME_DIALOG (prefs->prop_win),
				      GTK_EDITABLE (prefs->font_entry));

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
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs->blink_checkbox),
				      term->blink_enabled ? 1 : 0);
	gtk_signal_connect (GTK_OBJECT (prefs->blink_checkbox), "toggled",
			    GTK_SIGNAL_FUNC (prop_changed), prefs);
	gtk_table_attach (GTK_TABLE (table), prefs->blink_checkbox,
			  2, 3, BLINK_ROW, BLINK_ROW+1, GTK_FILL, 0, GNOME_PAD, GNOME_PAD);

	/* Show menu bar */
	prefs->menubar_checkbox = gtk_check_button_new_with_label (_("Hide menu bar"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs->menubar_checkbox),
				      cfg->menubar_hidden ? 1 : 0);
	gtk_signal_connect (GTK_OBJECT (prefs->menubar_checkbox), "toggled",
			    GTK_SIGNAL_FUNC (prop_changed), prefs);
	gtk_table_attach (GTK_TABLE (table), prefs->menubar_checkbox,
			  2, 3, MENUBAR_ROW, MENUBAR_ROW+1, GTK_FILL, 0, GNOME_PAD, GNOME_PAD);

	/* Toggle the bell */
	prefs->bell_checkbox = gtk_check_button_new_with_label (_("Silence Terminal bell"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs->bell_checkbox),
				      zvt_term_get_bell(term) ? 0 : 1);
	gtk_signal_connect (GTK_OBJECT (prefs->bell_checkbox), "toggled",
			    GTK_SIGNAL_FUNC (prop_changed), prefs);
	gtk_table_attach (GTK_TABLE (table), prefs->bell_checkbox,
			  2, 3, BELL_ROW, BELL_ROW+1, GTK_FILL, 0, GNOME_PAD, GNOME_PAD);

	/* Swap keys */
	prefs->swapkeys_checkbox = gtk_check_button_new_with_label (_("Swap DEL/Backspace"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs->swapkeys_checkbox),
				      cfg->swap_keys ? 1 : 0);
	gtk_signal_connect (GTK_OBJECT (prefs->swapkeys_checkbox), "toggled",
			    GTK_SIGNAL_FUNC (prop_changed), prefs);
	gtk_table_attach (GTK_TABLE (table), prefs->swapkeys_checkbox,
			  2, 3, SWAPKEYS_ROW, SWAPKEYS_ROW+1, GTK_FILL, 0, GNOME_PAD, GNOME_PAD);
	
	/* --login by default */
	prefs->login_by_default_checkbox = gtk_check_button_new_with_label (_("Use --login by default"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs->login_by_default_checkbox),
				      cfg->login_by_default ? 1 : 0);
	gtk_signal_connect (GTK_OBJECT (prefs->login_by_default_checkbox), "toggled",
			    GTK_SIGNAL_FUNC (prop_changed), prefs);
	gtk_table_attach (GTK_TABLE (table), prefs->login_by_default_checkbox,
			  2, 3, LOGIN_ROW, LOGIN_ROW+1, GTK_FILL, 0, GNOME_PAD, GNOME_PAD);

	/* Word selection class */
	l = aligned_label (_("Select-by-word characters"));
	gtk_table_attach (GTK_TABLE (table), l,
                          1, 2, WORDCLASS_ROW, WORDCLASS_ROW+1, GTK_FILL, 0, GNOME_PAD, GNOME_PAD);
	prefs->wordclass_entry = gtk_entry_new();
	gtk_entry_set_text (GTK_ENTRY (prefs->wordclass_entry),
			    cfg->wordclass);
	gtk_signal_connect (GTK_OBJECT (prefs->wordclass_entry), "changed",
			    GTK_SIGNAL_FUNC (prop_changed), prefs);
	gtk_table_attach (GTK_TABLE (table), prefs->wordclass_entry,
			  2, 3, WORDCLASS_ROW, WORDCLASS_ROW+1, GTK_FILL, 0, GNOME_PAD, GNOME_PAD);
	gnome_dialog_editable_enters (GNOME_DIALOG (prefs->prop_win),
				      GTK_EDITABLE (prefs->wordclass_entry));


	/* Image page */
	/* if pixmap support isn't in zvt, we still create the widgets for
	   the page, so that we can query them later, but they won't be shown
	   so the user can't change them */

	table = gtk_table_new (4, 4, FALSE);
	if(zvt_pixmap_support)
		gnome_property_box_append_page (GNOME_PROPERTY_BOX (prefs->prop_win), table, gtk_label_new (_("Image")));

	frame = gtk_frame_new (_("Background type"));
	gtk_table_attach (GTK_TABLE (table), frame,
			  1, 3, BACKGROUND_ROW, BACKGROUND_ROW+1, 
			  GTK_FILL | GTK_EXPAND, 0, GNOME_PAD, GNOME_PAD);

	subtable = gtk_table_new (4, 2, FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (subtable), GNOME_PAD_SMALL);
	gtk_table_set_row_spacings (GTK_TABLE (subtable), GNOME_PAD);
	gtk_container_add (GTK_CONTAINER (frame), subtable);

	r = gtk_radio_button_new_with_label (NULL, _("None"));
	gtk_table_attach (GTK_TABLE (subtable), r,
			  0, 1, 0, 1,
			  GTK_FILL, 0, 0, 0);
	
	/* Background Pixmap checkbox */
	r = prefs->pixmap_checkbox =
		gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (r),
							     _("Background pixmap"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs->pixmap_checkbox),
				      term->pixmap_filename ? 1 : 0);
	gtk_signal_connect (GTK_OBJECT (prefs->pixmap_checkbox), "toggled",
			    GTK_SIGNAL_FUNC (prop_changed), prefs);
	gtk_table_attach (GTK_TABLE (subtable), r,
			  0, 1, 1, 2,
			  GTK_FILL, 0, 0, 0);
	gtk_table_set_row_spacing (GTK_TABLE (subtable), 1, GNOME_PAD_SMALL);
	
	/* Background pixmap filename */
	hbox = gtk_hbox_new (FALSE, GNOME_PAD_SMALL);
	gtk_table_attach (GTK_TABLE (subtable), hbox,
			  0, 2, 2, 3,
			  GTK_FILL | GTK_EXPAND, 0, 0, 0);

	prefs->pixmap_label = gtk_label_new (_("Pixmap file:"));
	gtk_box_pack_start (GTK_BOX (hbox), prefs->pixmap_label, FALSE, FALSE, 0);

	prefs->pixmap_file_entry = gnome_file_entry_new ("pixmap",_("Browse"));
	prefs->pixmap_entry =
		gnome_file_entry_gtk_entry(GNOME_FILE_ENTRY(prefs->pixmap_file_entry));
	gtk_entry_set_text (GTK_ENTRY (prefs->pixmap_entry),
			    cfg->pixmap_file?cfg->pixmap_file:"");
	gtk_entry_set_position (GTK_ENTRY (prefs->pixmap_entry), 0);
	gtk_signal_connect (GTK_OBJECT (prefs->pixmap_entry), "changed",
			    GTK_SIGNAL_FUNC (prop_changed), prefs);
	gtk_box_pack_start (GTK_BOX (hbox), prefs->pixmap_file_entry, TRUE, TRUE, 0);

	/* Scrollable pixmap */
#ifdef ZVT_BACKGROUND_SCROLL
	prefs->pixmap_scrollable_checkbox = gtk_check_button_new_with_label (_("Background pixmap scrolls"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs->pixmap_scrollable_checkbox),
				      cfg->scroll_background ? 1 : 0);
	gtk_signal_connect (GTK_OBJECT (prefs->pixmap_scrollable_checkbox), "toggled",
			    GTK_SIGNAL_FUNC (prop_changed), prefs);
	gtk_table_attach (GTK_TABLE (subtable), prefs->pixmap_scrollable_checkbox,
			  0, 1, 3, 4,
			  GTK_FILL, 0, 0, 0);
#endif

	/* Transparency */
	r = prefs->transparent_checkbox =
		gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (r),
							     _("Transparent"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs->transparent_checkbox),
				      term->transparent ? 1 : 0);
	gtk_signal_connect (GTK_OBJECT (prefs->transparent_checkbox), "toggled",
			    GTK_SIGNAL_FUNC (prop_changed), prefs);
	gtk_table_attach (GTK_TABLE (subtable), r,
#ifdef ZVT_BACKGROUND_SCROLL
			  0, 1, 4, 5,
#else
			  0, 1, 3, 4,
#endif
			  GTK_FILL, 0, 0, 0);

	/* We pack this box into the the table so the second column
	 * is actually set to EXPAND - col spanning cells like the
	 * third row don't count in the table's algorithm :-(
	 */
	hbox = gtk_hbox_new (FALSE, GNOME_PAD_SMALL);
	gtk_table_attach (GTK_TABLE (subtable), hbox,
#ifdef ZVT_BACKGROUND_SCROLL
			  1, 2, 5, 6,
#else
			  1, 2, 4, 5,
#endif
			  GTK_FILL | GTK_EXPAND, 0, 0, 0);

	/* Shaded */
	prefs->shaded_checkbox = gtk_check_button_new_with_label (_("Background should be shaded"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs->shaded_checkbox),
				      term->shaded ? 1 : 0);
	gtk_signal_connect (GTK_OBJECT (prefs->shaded_checkbox), "toggled",
			    GTK_SIGNAL_FUNC (prop_changed), prefs);
	gtk_table_attach (GTK_TABLE (table), prefs->shaded_checkbox,
			  1, 2, SHADED_ROW, SHADED_ROW+1, GTK_FILL, 0, GNOME_PAD, GNOME_PAD_SMALL);

	check_image_options_sensitivity (prefs);


	/* Color page */
	table = gtk_table_new (4, 4, FALSE);
	gnome_property_box_append_page (GNOME_PROPERTY_BOX (prefs->prop_win), table, gtk_label_new (_("Colors")));
	
	/* Color palette */
	l = aligned_label (_("Color scheme:"));
	gtk_table_attach (GTK_TABLE (table), l,
			  1, 2, COLORPAL_ROW, COLORPAL_ROW+1, GTK_FILL, 0, GNOME_PAD, GNOME_PAD);
	prefs->color_scheme = create_option_menu (GNOME_PROPERTY_BOX (prefs->prop_win), prefs,
						  color_scheme, cfg->color_type, GTK_SIGNAL_FUNC (set_active));
	gtk_object_set_user_data (GTK_OBJECT (prefs->color_scheme), GINT_TO_POINTER (cfg->color_type));
	gtk_table_attach (GTK_TABLE (table), prefs->color_scheme,
			  2, 6, COLORPAL_ROW, COLORPAL_ROW+1, GTK_FILL, 0, GNOME_PAD, GNOME_PAD);

	/* Foreground, background buttons */
	prefs->fore_label = aligned_label (_("Foreground color:"));
	gtk_table_attach (GTK_TABLE (table), prefs->fore_label,
			  1, 2, FORECOLOR_ROW, FORECOLOR_ROW+1, GTK_FILL, 0, GNOME_PAD, GNOME_PAD);
	prefs->palette[16] = gnome_color_picker_new ();
	gtk_table_attach (GTK_TABLE (table), b1 = prefs->palette[16],
			  2, 3, FORECOLOR_ROW, FORECOLOR_ROW+1, 0, 0, GNOME_PAD, GNOME_PAD);
	gtk_signal_connect (GTK_OBJECT (b1), "destroy", GTK_SIGNAL_FUNC (free_cs), prefs->palette[16]);

	prefs->back_label = aligned_label (_("Background color:"));
	gtk_table_attach (GTK_TABLE (table), prefs->back_label,
			  1, 2, BACKCOLOR_ROW, BACKCOLOR_ROW+1, GTK_FILL, 0, GNOME_PAD, GNOME_PAD);
	prefs->palette[17] = gnome_color_picker_new ();
	gtk_table_attach (GTK_TABLE (table), b2 = prefs->palette[17],
			  2, 3, BACKCOLOR_ROW, BACKCOLOR_ROW+1, 0, 0, GNOME_PAD, GNOME_PAD);
	gtk_signal_connect (GTK_OBJECT (b2), "destroy", GTK_SIGNAL_FUNC (free_cs), prefs->palette[17]);
	l = aligned_label (_("Color palette:"));
	gtk_table_attach (GTK_TABLE (table), l,
			  1, 2, BACKCOLOR_ROW+1, BACKCOLOR_ROW+2, GTK_FILL, 0, GNOME_PAD, GNOME_PAD);
	prefs->palette_label = l;

	paltable = gtk_table_new(8,2,FALSE);
	gtk_table_attach (GTK_TABLE (table), paltable,
			  2, 3, BACKCOLOR_ROW+1, BACKCOLOR_ROW+2, GTK_FILL, 0,
			  GNOME_PAD, GNOME_PAD);
	for (i=0;i<18;i++) {
		if (i<16) {
			prefs->palette[i] = gnome_color_picker_new();
			gtk_table_attach(GTK_TABLE(paltable), prefs->palette[i], i&7, (i&7)+1,
					 i/8, i/8+1, 0, 0, 0, 0);
		}
		gnome_color_picker_set_i16(GNOME_COLOR_PICKER(prefs->palette[i]), cfg->palette[i].red,
					   cfg->palette[i].green, cfg->palette[i].blue, 0);
		gtk_signal_connect (GTK_OBJECT (prefs->palette[i]), "color_set",
				    GTK_SIGNAL_FUNC (color_changed), prefs);
	}

	/* default foreground/backgorund selector */
	l = aligned_label (_("Fore/Background Colour:"));
	gtk_table_attach (GTK_TABLE (table), l,
			  1, 2, FOREBACK_ROW, FOREBACK_ROW+1, GTK_FILL, 0, GNOME_PAD, GNOME_PAD);
	prefs->def_fore_back = create_option_menu_data (GNOME_PROPERTY_BOX (prefs->prop_win), prefs,
							fore_back_table, cfg->color_set, GTK_SIGNAL_FUNC (set_active));
	gtk_table_attach (GTK_TABLE (table), prefs->def_fore_back,
			  2, 6, FOREBACK_ROW, FOREBACK_ROW+1, GTK_FILL, 0, GNOME_PAD, GNOME_PAD);

	check_color_sensitivity (prefs);

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
						    100000.0, 1.0, 5.0, 0.0);
	prefs->scrollback_spin = gtk_spin_button_new (adj, 0, 0);
	gtk_signal_connect (GTK_OBJECT (prefs->scrollback_spin), "changed",
			    GTK_SIGNAL_FUNC (prop_changed), prefs);
	gtk_table_attach (GTK_TABLE (table), prefs->scrollback_spin, 2, 3, SCROLLBACK_ROW, 
			  SCROLLBACK_ROW+1, GTK_FILL, 0, GNOME_PAD, GNOME_PAD);
	gnome_dialog_editable_enters (GNOME_DIALOG (prefs->prop_win),
				      GTK_EDITABLE (prefs->scrollback_spin));

	/* Scroll on keystroke checkbox */
	prefs->scroll_kbd_checkbox = gtk_check_button_new_with_label (_("Scroll on keystroke"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs->scroll_kbd_checkbox),
				      cfg->scroll_key);
	gtk_signal_connect (GTK_OBJECT (prefs->scroll_kbd_checkbox), "toggled",
			    GTK_SIGNAL_FUNC (prop_changed), prefs);
	gtk_table_attach (GTK_TABLE (table), prefs->scroll_kbd_checkbox, 2, 3, 
			  KBDSCROLL_ROW, KBDSCROLL_ROW+1, GTK_FILL, 0, GNOME_PAD, GNOME_PAD);

	/* Scroll on output checkbox */
	prefs->scroll_out_checkbox = gtk_check_button_new_with_label (_("Scroll on output"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs->scroll_out_checkbox),
				      cfg->scroll_out);
	gtk_signal_connect (GTK_OBJECT (prefs->scroll_out_checkbox), "toggled",
			    GTK_SIGNAL_FUNC (prop_changed), prefs);
	gtk_table_attach (GTK_TABLE (table), prefs->scroll_out_checkbox, 2, 3, 
			  OUTSCROLL_ROW, OUTSCROLL_ROW+1, GTK_FILL, 0, GNOME_PAD, GNOME_PAD);

	/* connect the property box signals */
	gtk_signal_connect (GTK_OBJECT (prefs->prop_win), "apply",
			    GTK_SIGNAL_FUNC (apply_changes_cmd), term);
	gtk_signal_connect (GTK_OBJECT (prefs->prop_win), "destroy",
			    GTK_SIGNAL_FUNC (window_destroy), term);

	help_entry.name = "gnome-terminal";
	gtk_signal_connect (GTK_OBJECT (prefs->prop_win), "help",
			    GTK_SIGNAL_FUNC (gnome_help_pbox_display),
			    &help_entry);

	gtk_widget_show_all (prefs->prop_win);
}

#define NEED_UNUSED_FUNCTIONS
#ifdef NEED_UNUSED_FUNCTIONS
static void
color_ok (GtkWidget *w)
{
	gtk_widget_destroy (gtk_widget_get_toplevel (w));
}

void
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
#endif

void
toggle_menubar_cmd (GtkWidget *widget, ZvtTerm *term)
{
	GnomeApp *app;
	struct terminal_config *cfg;

	cfg = gtk_object_get_data (GTK_OBJECT (term), "config");

	cfg->menubar_hidden = ! cfg->menubar_hidden;

	app = GNOME_APP (gtk_widget_get_toplevel (GTK_WIDGET (term)));

	if (cfg->menubar_hidden)
		gtk_widget_hide (app->menubar->parent);
	else
		gtk_widget_show (app->menubar->parent);

	save_preferences_cmd (widget, term);
}

#ifdef HAVE_ZVT_TERM_RESET
static void
reset_terminal_soft_cmd (GtkWidget *widget, ZvtTerm *term)
{
	zvt_term_reset(term, 0);
}

/* could also possible clear the buffer? */
static void
reset_terminal_hard_cmd (GtkWidget *widget, ZvtTerm *term)
{
	zvt_term_reset(term, 1);
}
#endif

void
paste_cmd (GtkWidget *widget, ZvtTerm *term)
{
	GdkAtom string_atom;
	GdkEvent *event;
	gint32 time;
	
	string_atom = gdk_atom_intern ("STRING", FALSE);
	if (string_atom == GDK_NONE)
		return;

	event = gtk_get_current_event ();
	switch (event->type){
	case GDK_BUTTON_PRESS:
	case GDK_2BUTTON_PRESS:
	case GDK_BUTTON_RELEASE:
		time = event->button.time;
		break;

	case GDK_KEY_PRESS:
	case GDK_KEY_RELEASE:
		time = event->key.time;
		break;

	default:
		time = GDK_CURRENT_TIME;
	}
	
	gtk_selection_convert (GTK_WIDGET (term), GDK_SELECTION_PRIMARY, string_atom,
			       time);
}

void
toggle_secure_keyboard_cmd (GtkWidget *w, ZvtTerm *term)
{
	struct terminal_config *cfg;

	cfg = gtk_object_get_data (GTK_OBJECT (term), "config");

	cfg->keyboard_secured = ! cfg->keyboard_secured;

	if (cfg->keyboard_secured)
		gdk_keyboard_grab (term->term_window, TRUE, GDK_CURRENT_TIME);
	else
		gdk_keyboard_ungrab (GDK_CURRENT_TIME);
}

#ifdef ZVT_TERM_MATCH_SUPPORT
static void
load_url_cmd (GtkWidget *widget, ZvtTerm *term)
{
	char *url;

	url = gtk_object_get_data (GTK_OBJECT (term), "matchstr");
	if (url) {
		gnome_url_show(url);
	}

}
#endif

static GnomeUIInfo gnome_terminal_terminal_menu [] = {
        GNOMEUIINFO_MENU_NEW_ITEM (N_("_New terminal"), N_("Creates a new terminal window"), new_terminal, NULL),
	GNOMEUIINFO_SEPARATOR,
	GNOMEUIINFO_ITEM_NONE (N_("_Hide menubar"), NULL, toggle_menubar_cmd),
	GNOMEUIINFO_SEPARATOR,
	GNOMEUIINFO_ITEM_STOCK (N_("_Close terminal"), NULL, close_terminal_cmd,
													GNOME_STOCK_MENU_EXIT),
	GNOMEUIINFO_END
};

/*
 * Warning:
 * 
 *   If you change the layout of the popup menus, you must update the
 *   value of the POPUP_MENU_TOGGLE_INDEX_* macros to reflect the new
 *   menu item indices.
 */
static GnomeUIInfo gnome_terminal_popup_menu [] = {
        GNOMEUIINFO_MENU_NEW_ITEM (N_("_New terminal"), N_("Creates a new terminal window"), new_terminal, NULL),
        GNOMEUIINFO_MENU_PREFERENCES_ITEM(preferences_cmd, NULL),
	GNOMEUIINFO_TOGGLEITEM (N_("_Show menubar"), N_("Toggles whether or not the menubar is displayed."),
				toggle_menubar_cmd, NULL),
	GNOMEUIINFO_TOGGLEITEM (N_("_Secure keyboard"),
				N_("Toggles whether or not the keyboard is grabbed by the terminal."),
				toggle_secure_keyboard_cmd, NULL),
#ifdef HAVE_ZVT_TERM_RESET
	GNOMEUIINFO_ITEM_NONE (N_("_Reset Terminal"), NULL, reset_terminal_soft_cmd),
	GNOMEUIINFO_ITEM_NONE (N_("Reset and _Clear"), NULL, reset_terminal_hard_cmd),
#endif
#ifdef ZVT_TERM_MATCH_SUPPORT
	GNOMEUIINFO_END,	/* used as a free slot for dymanic menu item */
#endif
	GNOMEUIINFO_END
};

#ifdef ZVT_TERM_MATCH_SUPPORT
static GnomeUIInfo gnome_terminal_popup_menu_url [] = {
	GNOMEUIINFO_ITEM_NONE (N_("_Open in browser"), NULL, load_url_cmd),
};
#endif

static GnomeUIInfo gnome_terminal_help_menu [] = {
	GNOMEUIINFO_HELP ("gnome-terminal"),
	GNOMEUIINFO_MENU_ABOUT_ITEM(about_terminal_cmd, NULL),
	GNOMEUIINFO_END
};

static GnomeUIInfo gnome_terminal_edit [] = {
	GNOMEUIINFO_MENU_PASTE_ITEM(paste_cmd, NULL),
	GNOMEUIINFO_END
};

static GnomeUIInfo gnome_terminal_settings_menu [] = {
        GNOMEUIINFO_MENU_PREFERENCES_ITEM(preferences_cmd, NULL),
#ifdef HAVE_ZVT_TERM_RESET
	GNOMEUIINFO_ITEM_NONE (N_("_Reset Terminal"), NULL, reset_terminal_soft_cmd),
	GNOMEUIINFO_ITEM_NONE (N_("Reset and _Clear"), NULL, reset_terminal_hard_cmd),
#endif
	GNOMEUIINFO_ITEM_NONE (N_("C_olor selector..."), NULL, color_cmd),
	GNOMEUIINFO_END
};

static GnomeUIInfo gnome_terminal_menu[] = {
   {
	GNOME_APP_UI_SUBTREE_STOCK, N_("File"), NULL,
	gnome_terminal_terminal_menu, NULL, NULL, (GnomeUIPixmapType) 0,
	NULL, 0, (GdkModifierType) 0, NULL
   },
   {
	GNOME_APP_UI_SUBTREE_STOCK, N_("Edit"), NULL,
	gnome_terminal_edit, NULL, NULL, (GnomeUIPixmapType) 0, NULL, 0,
	(GdkModifierType) 0, NULL
   },
   {
	GNOME_APP_UI_SUBTREE_STOCK, N_("Settings"), NULL,
        gnome_terminal_settings_menu, NULL, NULL,
	(GnomeUIPixmapType) 0, NULL, 0, (GdkModifierType) 0, NULL
   },
   {
	GNOME_APP_UI_SUBTREE_STOCK, N_("Help"), NULL,
	gnome_terminal_help_menu, NULL, NULL, (GnomeUIPixmapType) 0,
	NULL, 0, (GdkModifierType) 0, NULL
   },
   GNOMEUIINFO_END
};

/*
 * Puts in *shell a pointer to the full shell pathname
 * Puts in *name the invocation name for the shell
 * *shell is allocated on the heap.
 */
static void
get_shell_name (char **shell, char **name, int isLogin)
{
	char *only_name;
	int len;

	*shell = gnome_util_user_shell ();
	g_assert (*shell != NULL);

	only_name = strrchr (*shell, '/');
	only_name++;
	
	if (isLogin){
		len = strlen (only_name);
		
		/* memory leak! */
		*name  = g_malloc (len + 2);
		**name = '-';
		strcpy ((*name)+1, only_name); 
	} else {
		*name = only_name;
	}
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
	XTextProperty text_prop;
	Atom aprop;
	char *pchEndPropName;

	switch(type){
	case VTTITLE_WINDOW:
	case VTTITLE_WINDOWICON:
		gtk_window_set_title((GtkWindow *)window, newtitle);
		break;
	/* this is a enumeration, so we can't #ifdef check it */
	/* case VTTITLE_XPROPERTY: */
	case 3:
		pchEndPropName = strchr(newtitle,'=');
		if (pchEndPropName)
			*pchEndPropName = '\0';
		aprop = XInternAtom(GDK_DISPLAY(), newtitle, False);
		if (pchEndPropName == NULL) {
			/* no "=value" given, so delete the property */
			XDeleteProperty(GDK_DISPLAY(),
					GDK_WINDOW_XWINDOW(GTK_WIDGET(window)->window),
					aprop);
		} else {
			text_prop.value = pchEndPropName+1;
			text_prop.encoding = XA_STRING;
			text_prop.format = 8;
			text_prop.nitems = strlen(text_prop.value);
			XSetTextProperty(GDK_DISPLAY(),
					 GDK_WINDOW_XWINDOW(GTK_WIDGET(window)->window),
					 &text_prop,aprop);
		}
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
	int col, row, the_char;
	struct terminal_config *cfg;

	cfg = gtk_object_get_data (GTK_OBJECT (term), "config");

	switch (info) {
	case TARGET_STRING:
	{
		char *copy = g_malloc (selection_data->length+1);
		GList *uris, *l;

		strncpy (copy, selection_data->data, selection_data->length);
		copy [selection_data->length] = 0;
		
		uris = gnome_uri_list_extract_uris (copy);

		for (l = uris; l; l = l->next){
			char *data = l->data;

			if (strncmp (data, "file:", 5) == 0)
				data += 5;
			
			vt_writechild (&term->vx->vt, data, strlen (data));
			vt_writechild (&term->vx->vt, " ", 1);
		}
		g_free (copy);
		gnome_uri_list_free_strings (uris);
		break;
	}
	case TARGET_COLOR:
	{
		guint16 *data = (guint16 *)selection_data->data;
		preferences_t *prefs;
		int i;

		if (selection_data->length != 8)
			return;

		col = x / term->charwidth;
		row = y / term->charheight;

		/* Switch to custom colors and */
		cfg->color_set = COLORS_CUSTOM;
		
		the_char = vt_get_attr_at (term->vx, col, row) & 0xff;
		if (the_char == ' ' || the_char == 0)
			i=17;
		else
			i=16;

		cfg->palette[i].red = data[0];
		cfg->palette[i].green = data[1];
		cfg->palette[i].blue = data[2];

		set_color_scheme (term, cfg);
		save_preferences_cmd (widget, term);

		prefs  = gtk_object_get_data (GTK_OBJECT (term), "prefs");
		if (prefs) {
			for (i=16;i<18;i++)
				gnome_color_picker_set_i16 (GNOME_COLOR_PICKER (prefs->palette[i]),
							    cfg->palette[i].red,
							    cfg->palette[i].green,
							    cfg->palette[i].blue, 0);
			gtk_option_menu_set_history (GTK_OPTION_MENU (prefs->def_fore_back),
						     cfg->color_set);
			check_color_sensitivity (prefs);
		}
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

static int
button_press (GtkWidget *widget, GdkEventButton *event, ZvtTerm *term)
{
	GtkWidget *menu;
	GnomeUIInfo *uiinfo;
	struct terminal_config *cfg;
	GtkCheckMenuItem *toggle_item;
#ifdef ZVT_TERM_MATCH_SUPPORT
	char *match;
	int x,y;
	GdkModifierType mask;
#endif

#ifdef ZVT_TERM_MATCH_SUPPORT
	gdk_window_get_pointer(widget->window, &x, &y, &mask);
	match = zvt_term_match_check(term, x/term->charwidth, y/term->charheight, 0);

	if (event->button == 1
	    && (event->state & GDK_CONTROL_MASK)
	    && match) {
		gtk_signal_emit_stop_by_name (GTK_OBJECT (widget), "button_press_event");
		gnome_url_show (match);
		return TRUE;
	}
#endif /* ZVT_TERM_MATCH_SUPPORT */

	if (event->button != 3
	    || (!(event->state & GDK_CONTROL_MASK) && term->vx->selected)
	    || (term->vx->vt.mode & VTMODE_SEND_MOUSE))
		return FALSE;

	gtk_signal_emit_stop_by_name (GTK_OBJECT (widget), "button_press_event");

#ifdef ZVT_TERM_MATCH_SUPPORT
	/* construct the popup menu properly */
	if (match) {
		char *tmp;

		memcpy(&gnome_terminal_popup_menu[POPUP_MENU_DYNAMIC_INDEX],
		       &gnome_terminal_popup_menu_url[0],
		       sizeof(gnome_terminal_popup_menu_url));

		tmp = gtk_object_get_data(GTK_OBJECT (term), "matchstr");
		if (tmp)
			g_free(tmp);
		gtk_object_set_data (GTK_OBJECT (term), "matchstr", g_strdup(match));
	} else {
		/* make sure the optional menu isn't there */
		memcpy(&gnome_terminal_popup_menu[POPUP_MENU_DYNAMIC_INDEX],
		       &gnome_terminal_popup_menu[POPUP_MENU_LAST_INDEX],
		       sizeof(gnome_terminal_popup_menu[0]));
	}
#endif
	uiinfo = gnome_terminal_popup_menu;
	menu = gnome_popup_menu_new (gnome_terminal_popup_menu);

	cfg = gtk_object_get_data (GTK_OBJECT (term), "config");

	/*
	 * Set the toggle state for the "show menubar"
	 * menu item.
	 */
	toggle_item = GTK_CHECK_MENU_ITEM (uiinfo [POPUP_MENU_TOGGLE_INDEX_MENUBAR].widget);
	toggle_item->active = ! cfg->menubar_hidden;

	/*
	 * Set the toggle state for the secure keyboard
	 * menu item.
	 */
	toggle_item = GTK_CHECK_MENU_ITEM (uiinfo [POPUP_MENU_TOGGLE_INDEX_SECURE].widget);
	toggle_item->active = cfg->keyboard_secured;

	gnome_popup_menu_do_popup_modal (menu, NULL, NULL, event, term);
	gtk_widget_destroy (menu);

	/*
	 * The popup menu ungrabs the keyboard if it was grabbed,
	 * so we grab it again here.
	 */
	if (cfg->keyboard_secured)
		gdk_keyboard_grab (term->term_window, TRUE, GDK_CURRENT_TIME);

	return TRUE;
}

static void
set_hints (GtkWidget *widget)
{
        ZvtTerm *term;
	GdkGeometry hints;
	GtkWidget *app;

	g_assert (widget != NULL);
	term = ZVT_TERM (widget);

	app = gtk_widget_get_toplevel(widget);
	g_assert (app != NULL);

#define PADDING 2
	hints.base_width = (GTK_WIDGET (term)->style->klass->xthickness * 2) + PADDING;
	hints.base_height =  (GTK_WIDGET (term)->style->klass->ythickness * 2);

	hints.width_inc = term->charwidth;
	hints.height_inc = term->charheight;
	hints.min_width = hints.base_width + hints.width_inc;
	hints.min_height = hints.base_height + hints.height_inc;

	gtk_window_set_geometry_hints(GTK_WINDOW(app),
				      GTK_WIDGET(term),
				      &hints,
				      GDK_HINT_RESIZE_INC|GDK_HINT_MIN_SIZE|GDK_HINT_BASE_SIZE);
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

/* do this because there have been only 5 bug reports about this */
static void
show_pty_error_dialog (int errcode)
{
	char *tmpmsg, *errmsg;
	GtkWidget *dialog;

	perror ("Error: unable to fork");

	tmpmsg = errcode ? 
		g_strdup_printf(_("The error was: %s"), g_strerror(errcode)) : 
		_("If you are using Linux 2.2.x with glibc 2.1.x, this\n"
		  "is probably due to incorrectly setup Unix98 ptys.\n\n"
		  "Please read linux/Documentation/Changes for how to\n"
		  "set them up correctly.");
	
	errmsg =  g_strdup_printf(_("There has been an error while "
				    "trying to log in.\n\n%s"), tmpmsg);
	
	
	dialog = gnome_message_box_new (errmsg,
					GNOME_MESSAGE_BOX_ERROR,
					GNOME_STOCK_BUTTON_OK, NULL);
							
	gnome_dialog_run_and_close (GNOME_DIALOG (dialog));

	if (errcode) g_free(tmpmsg);
	g_free(errmsg);
}

GtkWidget *
new_terminal_cmd (char **cmd, struct terminal_config *cfg_in, gchar *geometry)
{
	GtkWidget *app, *hbox, *scrollbar;
	ZvtTerm   *term;
	static char **env_copy;
	static int winid_pos;
	char buffer [40];
	char winclass [64];
	char *shell, *name;
	int i = 0;
	struct terminal_config *cfg;
	int width, height;
	int xpos, ypos;
	gboolean term_found = FALSE;
		
	/* FIXME: is seems like a lot of this stuff should be done by apply_changes instead */

	cfg = terminal_config_dup (cfg_in);

	/* Setup the environment for the gnome-terminals:
	 *
	 * TERM is set to xterm (which is what zvt emulates)
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
			if (strncmp (*p, "TERM=", 5) == 0) {
				if (cfg->termname && cfg->termname [0])
					env_copy [i++] = cfg->termname;
				else {
					/*
					 * DO NOT change this, its here for a reason 
					 * this emulator doesn't emulate xterm-color, or xterm-debian, or
					 * xterm-old, it emulates xterm.  
					 */
					env_copy [i++] =  "TERM=xterm";
				}
				term_found = TRUE;
			} else if ((strncmp (*p, "COLUMNS=", 8) == 0)
				   || (strncmp (*p, "LINES=", 6) == 0)
				   || (strncmp(*p, "WINDOWID=", 9) == 0)) {
				/* nothing: do not copy those */
			} else
				env_copy [i++] = *p;
		}
		if (!term_found){
			if (cfg->termname && cfg->termname [0])
				env_copy [i++] = cfg->termname;
			else
				env_copy [i++] = "TERM=xterm";
		}
		
		env_copy [i++] = "COLORTERM=gnome-terminal";
		winid_pos = i++;
		env_copy [winid_pos] = "TEST";
		env_copy [i] = NULL;
	}

	app = gnome_app_new ("Terminal", "Terminal");
	/* override the title if it was in the config */
	if (cfg->window_title) {
	  gtk_window_set_title(GTK_WINDOW(app), cfg->window_title);
 	}
	sprintf(winclass, "GnomeTerminal:%p", app);
	gtk_window_set_wmclass (GTK_WINDOW (app), "GnomeTerminal", winclass);
	gtk_window_set_policy(GTK_WINDOW (app), TRUE, TRUE, TRUE);

	if (cmd != NULL)
		initial_term = app;

	gtk_widget_realize (app);
	terminals = g_list_append (terminals, app);

	/* Setup the Zvt widget */

	/*
	 * Handle geometry specification; height and width are in
	 * terminal rows and columns
	 */

	width=80;
	height=24;
	xpos=-1;
	ypos=-1;
	if (geometry) {
		gnome_parse_geometry (geometry, &xpos, &ypos, &width, &height);
		if (width == -1 || height == -1) {
			width=80;
			height=24;
		}
		/* Only the first window gets --geometry treatment for now */
		geometry = NULL;
	}

	term = (ZvtTerm *)zvt_term_new_with_size(width, height);
	if (xpos != -1 && ypos != -1)
		gtk_widget_set_uposition (GTK_WIDGET (app), xpos, ypos);

	if ((zvt_term_get_capabilities (term) & ZVT_TERM_PIXMAP_SUPPORT) != 0){
		zvt_pixmap_support = TRUE;
	}
	gtk_object_set_data(GTK_OBJECT(app), "term", term);
	gtk_widget_grab_focus(GTK_WIDGET(term));

	gtk_widget_show (GTK_WIDGET (term));

	gtk_signal_connect_object(GTK_OBJECT(app),"configure_event",
				  GTK_SIGNAL_FUNC(term_change_pos),
				  GTK_OBJECT(term));

	zvt_term_set_scrollback (term, cfg->scrollback);
	gnome_term_set_font (term, cfg->font);
	zvt_term_set_wordclass  (term, (guchar *)cfg->wordclass);
	zvt_term_set_bell  (term, !cfg->bell);
	zvt_term_set_blink (term, cfg->blink);
	zvt_term_set_scroll_on_keystroke (term, cfg->scroll_key);
	zvt_term_set_scroll_on_output (term, cfg->scroll_out);
	zvt_term_set_del_key_swap (term, cfg->swap_keys);
		
	gtk_signal_connect (GTK_OBJECT (term), "child_died",
			    GTK_SIGNAL_FUNC (terminal_kill), term);

	gtk_signal_connect (GTK_OBJECT (term), "title_changed",
			    (GtkSignalFunc) title_changed, term);

	gtk_signal_connect (GTK_OBJECT (app), "delete_event",
			   GTK_SIGNAL_FUNC (close_app), term);

	gtk_signal_connect (GTK_OBJECT (term), "button_press_event",
			    GTK_SIGNAL_FUNC (button_press), term);

	gtk_signal_connect_after (GTK_OBJECT (term), "realize",
				  GTK_SIGNAL_FUNC (set_hints), term);

#ifdef ZVT_TERM_MATCH_SUPPORT
	zvt_term_match_add( ZVT_TERM(term), "(((news|telnet|nttp|file|http|ftp|https)://)|(www|ftp))[-A-Za-z0-9\\.]+(:[0-9]*)?", VTATTR_UNDERLINE, "host only url");
	zvt_term_match_add( ZVT_TERM(term), "(((news|telnet|nttp|file|http|ftp|https)://)|(www|ftp))[-A-Za-z0-9\\.]+(:[0-9]*)?/[-A-Za-z0-9_\\$\\.\\+\\!\\*\\(\\),;:@&=\\?/~\\#]*[^]'\\.}>\\) ,\\\"]", VTATTR_UNDERLINE, "full url");
#endif

	if (!cfg->menubar_hidden)
		gnome_app_create_menus_with_data (GNOME_APP (app), gnome_terminal_menu, term);
	
	/* Decorations */
	hbox = gtk_hbox_new (0, 0);

/*	gtk_container_set_border_width (GTK_CONTAINER (hbox), 2); */
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

	configure_term_dnd (term);

	if (zvt_pixmap_support && cfg->background_pixmap) {
		int flags;
#ifdef ZVT_BACKGROUND_SCROLL
		flags = cfg->shaded?ZVT_BACKGROUND_SHADED:0;
		flags |= cfg->scroll_background?ZVT_BACKGROUND_SCROLL:0;
#else
		flags = cfg->shaded;
#endif
		zvt_term_set_background (term,
					 cfg->pixmap_file,
					 cfg->transparent, flags);
	} else if (zvt_pixmap_support && cfg->transparent)
		zvt_term_set_background (term, NULL,
					 cfg->transparent, cfg->shaded);
	else
		zvt_term_set_background (term, NULL, 0, 0);

	gtk_widget_show (app);

	/*
	 * We need to hide this here, because the gnome_app_show
	 * method will force a show on the menu
	 */
	if (cfg->menubar_hidden) {
		gnome_app_create_menus_with_data (GNOME_APP (app), gnome_terminal_menu, term);
		gtk_widget_hide (GNOME_APP (app)->menubar->parent);
	}

	set_color_scheme (term, cfg);

	XSync(GDK_DISPLAY(), False);

	errno = 0;
	switch (zvt_term_forkpty (term, cfg->update_records)){
	case -1:
		show_pty_error_dialog(errno);
		/* should we exit maybe? */
		return NULL;
		
	case 0: {
		int open_max = sysconf (_SC_OPEN_MAX);

		for (i = 3; i < open_max; i++)
			fcntl (i, F_SETFD, 1);
		
		sprintf (buffer, "WINDOWID=%d",(int) GDK_WINDOW_XWINDOW(GTK_WIDGET(term)->window));
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

        /* IS THIS BEING DOUBLE-FREED??? --JMP g_free (shell); */

	return app;
}

GtkWidget *
new_terminal (GtkWidget *widget, ZvtTerm *term)
{
	struct terminal_config *cfg;

	cfg = gtk_object_get_data (GTK_OBJECT (term), "config");

	return new_terminal_cmd (NULL, cfg, NULL);
}

static gboolean
load_session ()
{
	int num_terms, i;
	gboolean def;
	gchar *file;

	file = gnome_client_get_config_prefix (gnome_master_client());

	gnome_config_push_prefix (file);
	num_terms = gnome_config_get_int_with_default ("dummy/num_terms", 
						       &def);
	gnome_config_pop_prefix ();

	if (def || ! num_terms)
		return FALSE;

	for (i = 0; i < num_terms; ++i){
		char *geom, **argv;
		struct terminal_config *cfg;
		char *class, *class_path;
		int argc;
		char *prefix = g_strdup_printf ("%s%d/", file, i);
	  
		gnome_config_push_prefix (prefix);

		/* NAUGHTY: The ICCCM requires that the WM stores
		   all session data on geometry! */
		geom = gnome_config_get_string ("geometry");
		gnome_config_get_vector ("command", &argc, &argv);

		class_path = g_strconcat ("class=", _("Default"), NULL);
		class = gnome_config_get_string(class_path);
		g_free (class_path);

		cfg = load_config (class);

		/* do this now, since it is only done in a session */
		cfg->invoke_as_login_shell = gnome_config_get_bool("invoke_as_login_shell");
		{
			char *ctmp;
			ctmp = gnome_config_get_string("termname=xterm");
			if(ctmp && *ctmp)
				cfg->termname = g_strconcat("TERM=", ctmp, NULL);
			else
				cfg->termname = g_strdup("TERM=xterm");
			g_free(ctmp);
		}
		cfg->window_title = gnome_config_get_string("window_title");
		if (gnome_config_get_bool ("do_utmp=true"))
			cfg->update_records |= ZVT_TERM_DO_UTMP_LOG;
		if (gnome_config_get_bool ("do_wtmp=true"))
			cfg->update_records |= ZVT_TERM_DO_WTMP_LOG;

		g_free(class);
		gnome_config_pop_prefix();

		new_terminal_cmd (argv, cfg, geom);

		g_free(cfg);
		g_free (geom);
		if (i==0) {
			/* need to save the initial argv, because of the weird
			   way it is used elsewhere, with the initial term */
			initial_command = argv;
		} else {
			g_strfreev (argv);
		}
	}
	return TRUE;
}

/* Save terminal in this session.  FIXME: should save all terminal
   windows, but currently does not.  */
static gint
save_session (GnomeClient *client, gint phase, GnomeSaveStyle save_style,
	      gint is_shutdown, GnomeInteractStyle interact_style,
	      gint is_fast, gpointer client_data)
{
	char *file = gnome_client_get_config_prefix (client);
	char *args[8];
	int i;
	GList *list;
	
	i = 0;
	for (list = terminals; list != NULL; list = list->next){
	
	        gint width, height, x, y;
		GString *geom;
		struct terminal_config *cfg;			
		ZvtTerm *term;
		GtkWidget *top;
		char *prefix;
		char *ctmp;
	  
		top = gtk_widget_get_toplevel (GTK_WIDGET (list->data));
		prefix = g_strdup_printf ("%s%d/", file, i);
		term = ZVT_TERM (gtk_object_get_data (GTK_OBJECT(list->data), 
						      "term"));
		cfg = gtk_object_get_data (GTK_OBJECT (term), "config");

		gnome_config_push_prefix (prefix);

		/*
		 * NAUGHTY: The ICCCM requires that the WM stores
		 * all session data on geometry!
		 */

		/*
		 * we can't use gnome_geometry_string because we need
		 * to calculate the terminal, not window size
		 */
		gdk_window_get_origin (top->window, &x, &y);

		width = 
		  (GTK_WIDGET (term)->allocation.width - 
		   (GTK_WIDGET (term)->style->klass->xthickness * 2)) /
		  term->charwidth;

		height = 
		  (GTK_WIDGET (term)->allocation.height - 
		   (GTK_WIDGET (term)->style->klass->ythickness * 2)) /
		  term->charheight;

		geom = g_string_new ("");
		g_string_sprintf (geom, "%dx%d%+d%+d", width, height, x, y);
		gnome_config_set_string ("geometry", geom->str);
		g_string_free (geom, TRUE);
		
		gnome_config_set_string("class", cfg->class);
		
		if (top == initial_term){
			int n;
			for (n = 0; initial_command[n]; ++n);
			gnome_config_set_vector ("command", n,
						 (const char * const*) initial_command);
		}

		save_preferences(list->data, term, cfg);
		
		/* do this now, since it is only done in a session */
		gnome_config_set_bool("invoke_as_login_shell", cfg->invoke_as_login_shell);

		ctmp = NULL;
		if(cfg->termname) {
			ctmp = strchr(cfg->termname, '=');
			if(ctmp) ctmp++;
		}
		if(!ctmp)
			ctmp = "xterm";
		gnome_config_set_string("termname", ctmp);

		gnome_config_set_string("window_title", cfg->window_title?cfg->window_title:"Terminal");
		gnome_config_set_bool("do_utmp", (cfg->update_records & ZVT_TERM_DO_UTMP_LOG) != 0);
		gnome_config_set_bool("do_wtmp", (cfg->update_records & ZVT_TERM_DO_WTMP_LOG) != 0);

		gnome_config_pop_prefix ();
		g_free (prefix);
		
		++i;
	}
	gnome_config_push_prefix (file);
	gnome_config_set_int ("dummy/num_terms", i);
	gnome_config_pop_prefix ();

#if 0
	/* What was the cfg variable ? */
	args[0] = gnome_master_client()->restart_command[0];
	args[1] = "--font";
	args[2] = cfg.font;
	args[3] = cfg.invoke_as_login_shell ? "--login" : "--nologin";
	args[4] = "--foreground";
	args[5] = g_strdup (get_color_string (cfg.user_fore));
	args[6] = "--background";
	args[7] = g_strdup (get_color_string (cfg.user_back));
	args[8] = NULL;

	g_free (args[5]);
	g_free (args[7]);
	gnome_client_set_restart_command (client, 8, args);
#endif
	gnome_config_sync ();

	args[0] = "rm";
	args[1] = gnome_config_get_real_path (file);
	args[2] = NULL;
	gnome_client_set_discard_command (client, 2, args);

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
	CLASS_KEY    = -8,
	DOUTMP_KEY   = -9,
	DONOUTMP_KEY = -10,
	DOWTMP_KEY   = -11,
	DONOWTMP_KEY = -12,
        TITLE_KEY    = -13,
	TERM_KEY     = -14
};

static struct poptOption cb_options [] = {
	{ NULL, '\0', POPT_ARG_CALLBACK, (gpointer)parse_an_arg, 0},

	{ "tclass", '\0', POPT_ARG_STRING, NULL, CLASS_KEY,
	  N_("Terminal class name"), N_("TCLASS")},

	{ "font", '\0', POPT_ARG_STRING, NULL, FONT_KEY,
	  N_("Specifies font name"), N_("FONT")},

	{ "nologin", '\0', POPT_ARG_NONE, NULL, NOLOGIN_KEY,
	  N_("Do not start up shells as login shells"), NULL},

	{ "login", '\0', POPT_ARG_NONE, NULL, LOGIN_KEY,
	  N_("Start up shells as login shells"), NULL},

	{ "geometry", '\0', POPT_ARG_STRING, NULL, GEOMETRY_KEY,
	  N_("Specifies the geometry for the main window"), N_("GEOMETRY")},

	{ "command", 'e', POPT_ARG_STRING, NULL, COMMAND_KEY,
	  N_("Execute this program instead of a shell"), N_("COMMAND")},

	{ "execute", 'x', POPT_ARG_STRING, NULL, COMMAND_KEY,
	  N_("Execute this program the same way as xterm does"), N_("COMMAND")},

	{ "foreground", '\0', POPT_ARG_STRING, NULL, FORE_KEY,
	  N_("Foreground color"), N_("COLOR")},

	{ "background", '\0', POPT_ARG_STRING, NULL, BACK_KEY,
	  N_("Background color"), N_("COLOR")},

	{ "utmp", '\0', POPT_ARG_NONE, NULL, DOUTMP_KEY,
	  N_("Update utmp entry"), N_("UTMP") },

	{ "noutmp", '\0', POPT_ARG_NONE, NULL, DONOUTMP_KEY,
	  N_("Do not update utmp entry"), N_("NOUTMP") },

	{ "wtmp", '\0', POPT_ARG_NONE, NULL, DOWTMP_KEY,
	  N_("Update wtmp entry"), N_("WTMP") },

	{ "nowtmp", '\0', POPT_ARG_NONE, NULL, DONOWTMP_KEY,
	  N_("Do not update wtmp entry"), N_("NOWTMP") },
	
	{ "title", 't', POPT_ARG_STRING, NULL, TITLE_KEY,
          N_("Set the window title"), N_("TITLE") },

	{ "termname", '\0', POPT_ARG_STRING, NULL, TERM_KEY,
          N_("Set the TERM variable"), N_("TERMNAME") },

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
		cmdline_login = TRUE;
		break;
	case NOLOGIN_KEY:
	        cfg->invoke_as_login_shell = 0;
		cmdline_login = TRUE;
	        break;
	case GEOMETRY_KEY:
		initial_global_geometry = (char *)arg;
		break;
	case COMMAND_KEY:
		  {
			  char **foo;
			  int x;
			  
			  poptParseArgvString((char *)arg,&x,&foo);
			  initial_command=malloc((x+1)*sizeof(char *));
			  initial_command[x]=NULL;
			  while (x>0) {
				  x--;
				  initial_command[x]=foo[x];
			  }
		break;
		  }
	case FORE_KEY:
		cfg->user_fore_str = arg;
		cfg->have_user_colors = 1;
		break;
	case BACK_KEY:
		cfg->user_back_str = arg;
		cfg->have_user_colors = 1;
		break;
	case DOUTMP_KEY:
		cfg->update_records_and &= ~ZVT_TERM_DO_UTMP_LOG;
		cfg->update_records_xor |= ZVT_TERM_DO_UTMP_LOG;
		break;
	case DONOUTMP_KEY:
		cfg->update_records_and &= ~ZVT_TERM_DO_UTMP_LOG;
		cfg->update_records_xor &= ~ZVT_TERM_DO_UTMP_LOG;
		break;
	case DOWTMP_KEY:
		cfg->update_records_and &= ~ZVT_TERM_DO_WTMP_LOG;
		cfg->update_records_xor |= ZVT_TERM_DO_WTMP_LOG;
		break;
	case DONOWTMP_KEY:
		cfg->update_records_and &= ~ZVT_TERM_DO_WTMP_LOG;
		cfg->update_records_xor &= ~ZVT_TERM_DO_WTMP_LOG;
		break;
	case TITLE_KEY:
	        cfg->window_title = g_strdup(arg);
                break;
	case TERM_KEY:
		cfg->termname = g_strdup_printf("TERM=%s", arg);
		break;
	default:
		break;
	}
}

static gint
session_die (gpointer client_data)
{
        gtk_main_quit ();
	return TRUE;
}

static int
main_terminal_program (int argc, char *argv [], char **environ)
{
	GnomeClient *client;
	char *program_name;
	char *class;
	struct terminal_config *default_config, *cmdline_config;
	int i, j;

	env = environ;
	
	bindtextdomain (PACKAGE, GNOMELOCALEDIR);
	textdomain (PACKAGE);

	cmdline_login = FALSE;

	cmdline_config = g_new0 (struct terminal_config, 1);
	cmdline_config->update_records_and = ~0;
	
	cb_options[0].descrip = (char *)cmdline_config;

	/* pre-scan for -x and --execute options */
	for (i=1;i<argc;i++) {
		if (!strcmp(argv[i], "-x") || !(strcmp(argv[i], "--execute"))) {
			int last=i;
			i++;
			if (i==argc) {
				/* no arg!? let popt whinge about usage */
				break;
			}
			initial_command=malloc((argc-i+1)*sizeof(char *));
			j = 0;
			while (i<argc) {
				initial_command[j] = argv[i];
				i++; j++;
			}
			initial_command[j]=NULL;
			/* 'fool' popt into thinking we have less args */
			argc=last;
			break;
		}
	}
	
	gnome_init_with_popt_table("Terminal", VERSION, argc, argv,
				   cb_options, 0, NULL);

	if(cmdline_config->user_back_str)
		gdk_color_parse(cmdline_config->user_back_str,
				&cmdline_config->palette[17]);
	
	if(cmdline_config->user_fore_str)
		gdk_color_parse(cmdline_config->user_fore_str,
				&cmdline_config->palette[16]);

	if (cmdline_config->class){
		class = g_strdup (cmdline_config->class);
	}
	else
	{
#if 0 /* program_invoation_short_name is broken on non-glibc machines at the moment */	
		program = program_invocation_short_name;
#else
		program_name = strrchr (argv[0], '/');
		if (!program_name) 
			program_name = argv[0];
		else
			program_name++;
#endif
		if (getenv ("GNOME_TERMINAL_CLASS"))
			class = g_strconcat ("Class-", getenv ("GNOME_TERMINAL_CLASS"), 
					     NULL);
		else if (strcmp (program_name, "gnome-terminal"))
			class = g_strconcat ("Class-", program_name, NULL);
		else
			class = g_strdup ("Config");
	}
	
	client = gnome_master_client ();
	gtk_signal_connect (GTK_OBJECT (client), "save_yourself",
			    GTK_SIGNAL_FUNC (save_session), NULL);
	gtk_signal_connect (GTK_OBJECT (client), "die",
			    GTK_SIGNAL_FUNC (session_die), NULL);
	
	{
		char *prefix = g_strdup_printf ("Terminal/%s/", class);
		gnome_config_push_prefix (prefix);
		default_config = load_config (class);
		gnome_config_pop_prefix();
		g_free (class);
		g_free (prefix);
	}
	
	/* now to override the defaults */
	if (cmdline_config->font){
		free (default_config->font);
		default_config->font = g_strdup (cmdline_config->font);
	}

	/* override the title*/
	if (cmdline_config->window_title) {
	  default_config->window_title = cmdline_config->window_title;
	}
	
	if (cmdline_config->have_user_colors){
		default_config->color_set = cmdline_config->color_set;
		default_config->palette[16] = cmdline_config->palette[16];
		default_config->palette[17] = cmdline_config->palette[17];
		default_config->color_set = COLORS_CUSTOM;
	}


	/* if the default is different from the commandline, use the commandline */
	default_config->invoke_as_login_shell =
		cmdline_login ? cmdline_config->invoke_as_login_shell : 
		default_config->login_by_default;
	default_config->termname = g_strdup(cmdline_config->termname);

	default_config->update_records &= cmdline_config->update_records_and;
	default_config->update_records ^= cmdline_config->update_records_xor;

	terminal_config_free (cmdline_config);
	
	if (!load_session ())
		new_terminal_cmd (initial_command, default_config, initial_global_geometry);

	terminal_config_free (default_config);

	gtk_main ();
	return 0;
}

extern char **environ;

int
main (int argc, char *argv [])
{
	return main_terminal_program (argc, argv, environ);
}

/*
 * Local variables:
 * c-basic-offset: 8
 * End:
 */
