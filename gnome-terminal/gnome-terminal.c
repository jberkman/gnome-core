/*
 * The GNOME terminal, using Michael Zucchi's zvt widget.
 * (C) 1998, 1999 The Free Software Foundation
 * Copyright 2000 Helix Code, Inc.
 *
 * Authors: Miguel de Icaza (GNOME terminal)
 *          Erik Troan      (various configuration enhancements)
 *          Michael Zucchi  (zvt widget, various updates and enhancements)
 *
 * Other contributors: George Lebl, Jeff Garzik, Jay Painter,
 * Christopher Blizzard, Jens Lautenbacher, Tom Tromey, Tristan Tarant,
 * Jonathan Blandford, Cody Russell, Nat Friedman, Jacob Berkman,
 * John Harper, and Hans-Andreas Engel
 */
#include <config.h>

#include "gnome-terminal.h"

#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>

#include <popt.h>

#include <gdk/gdk.h>
#include <gdk/gdkprivate.h>
#include <gdk/gdkx.h>

#define HAVE_GNOME_PROPERTY_BOX

#include <gtk/gtkcolorseldialog.h>
#include <gtk/gtkcombo.h>
#include <gtk/gtkclipboard.h>
#include <gtk/gtkdnd.h>
#include <gtk/gtkentry.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkmain.h>
#include <gtk/gtkmenu.h>
#include <gtk/gtkmenuitem.h>
#include <gtk/gtkmessagedialog.h>
#include <gtk/gtkoptionmenu.h>
#include <gtk/gtkselection.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkspinbutton.h>
#include <gtk/gtktogglebutton.h>
#include <gtk/gtkvscrollbar.h>

#include <libgnome/gnome-i18n.h>
#include <libgnome/gnome-url.h>
#include <libgnome/gnome-util.h>

#include <libgnome/gnome-config.h>
#define HAVE_GNOME_CONFIG 1

#include <libgnomeui/gnome-about.h>
#include <libgnomeui/gnome-client.h>
#include <libgnomeui/gnome-color-picker.h>
#include <libgnomeui/gnome-dialog.h>
#include <libgnomeui/gnome-ui-init.h>
#include <libgnomeui/gnome-file-entry.h>
#include <libgnomeui/gnome-propertybox.h>
#include <libgnomeui/gnome-window-icon.h>

#include <libzvt/libzvt.h>

#include <bonobo/bonobo-main.h>
#include <bonobo/bonobo-ui-main.h>
#include <bonobo/bonobo-ui-util.h>
#include <bonobo/bonobo-window.h>

#include <glade/glade.h>

#include <X11/Xatom.h>

/* In case old gnome-libs */
#ifndef ZVT_TERM_DO_LASTLOG
#   define ZVT_TERM_DO_LASTLOG 4
#endif

/* The program environment */
extern char **environ;		

/* Initial geometry */
static char *initial_global_geometry = NULL;

/* Window icon */
static char *window_icon = GNOME_ICONDIR"/gnome-terminal.png";

char **env;

#define DEFAULT_FONT "fixed"
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
	TARGET_COLOR,
	TARGET_BGIMAGE
};

/* nautilus uses this UTTER HACK to reset backgrounds, ugly ugly ugly,
 * broken, ugly ugly, but whatever */
#define RESET_IMAGE_NAME "/nautilus/backgrounds/reset.png"


struct terminal_config {
	int keyboard_secured :1;		/* Does this terminal have the keyboard secured? */
        int bell             :1;                /* Do we want the bell? */
	int blink            :1; 		/* Do we want blinking cursor? */
	int scroll_key       :1;       		/* Scroll on input? */
	int scroll_out       :1;       		/* Scroll on output? */
	int swap_keys        :1;       		/* Swap Delete/Backspace? */
	int login_by_default :1;                /* do --login as default */
	int scroll_background:1; 		/* background will scroll */
	int use_bold         :1;		/* Use bold on bright colours */
	int del_is_del       :1; 		/* Generates Delete DEL/^H? */
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
	int transparent;
	int have_user_background;		/* Only used for command line parsing */
	int have_user_scroll_bg; 		/* Only used for command line parsing */
	int shaded;
	int have_user_shaded;			/* Only used for command line parsing */
	int background_pixmap;
	int terminal_id;			/* terminal id for this terminal */
	char *pixmap_file;
        char *window_title;                     /* the window title */
        char *window_icon;                      /* the window icon */
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


int use_terminal_factory   = FALSE;
int start_terminal_factory = FALSE;

/* current new terminal window id */
static int terminal_id = 0;

typedef struct {
	GtkWidget *prop_win;
	GtkWidget *blink_checkbox;
	GtkWidget *scroll_kbd_checkbox;
	GtkWidget *scroll_out_checkbox;
	GtkWidget *swapkeys_checkbox;
	GtkWidget *del_is_del_checkbox;
	GtkWidget *login_by_default_checkbox;
	GtkWidget *use_bold_checkbox;
	GtkWidget *wordclass_entry;
	GtkWidget *pixmap_checkbox;
	GtkWidget *pixmap_file_entry;
	GtkWidget *pixmap_label;
	GtkWidget *pixmap_entry;
	GtkWidget *transparent_checkbox;
	GtkWidget *shaded_checkbox;
	GtkWidget *pixmap_scrollable_checkbox;
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

#if 0 /* this is a bunch of crack rock */
/*
 * Exported interfaces, for Gtk modules that hook
 * into the gnome terminal
 */
void close_terminal_cmd         (BonoboUIComponent *uic, gpointer data, const char *cname);
void save_preferences_cmd       (BonoboUIComponent *uic, gpointer data, const char *cname);
void color_cmd                  (BonoboUIComponent *uic, gpointer data, const char *cname);
void toggle_menubar_cmd         (BonoboUIComponent *uic, gpointer data, const char *cname);
void paste_cmd                  (BonoboUIComponent *uic, gpointer data, const char *cname);
void preferences_cmd            (BonoboUIComponent *uic, gpointer data, const char *cname);
void toggle_secure_keyboard_cmd (BonoboUIComponent *uic, gpointer data, const char *cname);

GtkWidget *new_terminal_cmd (char **cmd, struct terminal_config *cfg_in, const gchar *geometry, int termid);
GtkWidget *new_terminal     (GtkWidget *widget, ZvtTerm *term);
#endif

static void parse_an_arg (poptContext state,
			  enum poptCallbackReason reason,
			  const struct poptOption *opt,
			  const char *arg, void *data);

static void set_hints (GtkWidget *widget);
static void set_size (GtkWidget *widget);

static void
about_terminal_cmd (BonoboUIComponent *uic, gpointer data, const char *cname)
{
        static GtkWidget *about = NULL;

        const gchar *authors[] = {
		"Michael Zucchi (zucchi@zedzone.mmc.com.au)",
		"Miguel de Icaza (miguel@kernel.org)",
		"Erik Troan (ewt@redhat.com)",
		"Jacob Berkman (jacob@ximian.com)",
		NULL
	};

	if (about != NULL) {
		gtk_window_present (GTK_WINDOW (about));
		return;
	}

        about = gnome_about_new (_("GNOME Terminal"), VERSION,
				 "(C) 1998, 1999 the Free Software Foundation\n"
				 "Copyright 2000, 2001 Ximian, Inc.",
				 _("The GNOME terminal emulation program."),
				 authors,
				 NULL,
				 NULL,
				 NULL);

	gtk_signal_connect (GTK_OBJECT (about), "response",
			    GTK_SIGNAL_FUNC (gtk_widget_destroy), NULL);
	gtk_signal_connect (GTK_OBJECT (about), "destroy",
			    GTK_SIGNAL_FUNC (gtk_widget_destroyed), &about);
        gtk_widget_show (about);
}

static void
close_app (GtkWidget *app)
{
	preferences_t *prefs;

        terminals = g_list_remove (terminals, app);

	/* FIXME: this should free the config info for the window */

	if (app == initial_term)
		initial_term = NULL;

	prefs = gtk_object_get_data (GTK_OBJECT (gtk_object_get_data(GTK_OBJECT(app), "term")), "prefs");
	if (prefs)
		gtk_object_destroy(GTK_OBJECT(prefs->prop_win));

	gtk_widget_destroy (app);

	if (terminals == NULL)
		bonobo_main_quit ();
}


static void
close_terminal_cmd (BonoboUIComponent *uic, gpointer data, const char *cname)
{
	GtkWidget *top = gtk_widget_get_toplevel (GTK_WIDGET (data));

	close_app(top);
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
gnome_term_set_font (ZvtTerm *term, char *font_name, const int use_bold)
{
	char *s;
	GdkFont *font;

	font = gdk_font_load (font_name);
	if (font) {
		if (use_bold)
			zvt_term_set_fonts  (term, font, 0);
		else
			zvt_term_set_fonts  (term, font, font);
	}

	if (GTK_WIDGET_REALIZED (term)) {
		set_hints (GTK_WIDGET (term));
		set_size (GTK_WIDGET (term));
	}
	
	s = gtk_object_get_user_data (GTK_OBJECT (term));
	if (s)
		g_free (s);
	gtk_object_set_user_data (GTK_OBJECT (term), g_strdup (font_name));
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
}

static struct terminal_config * 
load_config (char *class)
{
	char *p;
	struct terminal_config *cfg = g_malloc (sizeof (*cfg));
	int colour_count;
	char **colours;
	int i;

	/* It's very odd that these are here */
	cfg->font = NULL;
	cfg->class = g_strdup (class);

#ifdef HAVE_GNOME_CONFIG
	cfg->scrollback = gnome_config_get_int ("scrollbacklines=100");
	cfg->font    = gnome_config_get_string ("font=" DEFAULT_FONT);
	cfg->wordclass  = gnome_config_get_string ("wordclass=-A-Za-z0-9,./?%&#");
	p = gnome_config_get_string ("scrollpos=right");
	if (g_strcasecmp (p, "left") == 0)
		cfg->scrollbar_position = SCROLLBAR_LEFT;
	else if (g_strcasecmp (p, "right") == 0)
		cfg->scrollbar_position = SCROLLBAR_RIGHT;
	else
		cfg->scrollbar_position = SCROLLBAR_HIDDEN;
	p = gnome_config_get_string ("color_scheme=linux");
	if (g_strcasecmp (p, "linux") == 0)
		cfg->color_type = PALETTE_LINUX;
	else if (g_strcasecmp (p, "xterm") == 0)
		cfg->color_type = PALETTE_XTERM;
	else if (g_strcasecmp (p, "rxvt") == 0)
		cfg->color_type = PALETTE_RXVT;
	else if (g_strcasecmp (p, "custom") == 0)
		cfg->color_type = PALETTE_CUSTOM;
	else
		cfg->color_type = PALETTE_LINUX;
	
	cfg->keyboard_secured  = FALSE;
	
	cfg->bell              = gnome_config_get_bool ("bell_silenced=0");
	cfg->blink             = gnome_config_get_bool ("blinking=0");
	cfg->swap_keys         = gnome_config_get_bool ("swap_del_and_backspace=0");
	cfg->del_is_del        = gnome_config_get_bool ("del_is_del=0");

	cfg->login_by_default  = gnome_config_get_bool ("login_by_default=0");
	cfg->use_bold          = gnome_config_get_bool ("use_bold=true");

	cfg->scroll_background = gnome_config_get_bool ("scroll_background=0");
	/* Default colors in the case the color set is the custom one */
	cfg->color_set = gnome_config_get_int ("color_set=0");

	cfg->menubar_hidden = !gnome_config_get_bool ("menubar=true");
	cfg->scroll_key = gnome_config_get_bool ("scrollonkey=true");
	cfg->scroll_out = gnome_config_get_bool ("scrollonoutput=false");

	cfg->transparent = gnome_config_get_bool ("transparent=false");
	cfg->shaded = gnome_config_get_bool ("shaded=false");
	cfg->background_pixmap = gnome_config_get_bool ("background_pixmap=false");
	cfg->pixmap_file = gnome_config_get_string ("pixmap_file");

#else /* HAVE_GNOME_CONFIG */

	cfg->scrollback = 4096;
	cfg->font    = g_strdup (DEFAULT_FONT);
	cfg->wordclass  = g_strdup ("wordclass=-A-Za-z0-9,./?%&#");
	p = "right";
	if (g_strcasecmp (p, "left") == 0)
		cfg->scrollbar_position = SCROLLBAR_LEFT;
	else if (g_strcasecmp (p, "right") == 0)
		cfg->scrollbar_position = SCROLLBAR_RIGHT;
	else
		cfg->scrollbar_position = SCROLLBAR_HIDDEN;
	p = "linux";
	if (g_strcasecmp (p, "linux") == 0)
		cfg->color_type = PALETTE_LINUX;
	else if (g_strcasecmp (p, "xterm") == 0)
		cfg->color_type = PALETTE_XTERM;
	else if (g_strcasecmp (p, "rxvt") == 0)
		cfg->color_type = PALETTE_RXVT;
	else if (g_strcasecmp (p, "custom") == 0)
		cfg->color_type = PALETTE_CUSTOM;
	else
		cfg->color_type = PALETTE_LINUX;
	
	cfg->keyboard_secured  = FALSE;
	
	cfg->bell              = FALSE;
	cfg->blink             = TRUE;
	cfg->swap_keys         = TRUE;
	cfg->del_is_del        = FALSE;

	cfg->login_by_default  = FALSE;
	cfg->use_bold          = FALSE;

	cfg->scroll_background = FALSE;
	/* Default colors in the case the color set is the custom one */
	cfg->color_set = 0;

	cfg->menubar_hidden = TRUE;
	cfg->scroll_key = TRUE;
	cfg->scroll_out = FALSE;

	cfg->transparent = FALSE;
	cfg->shaded = FALSE;
	cfg->background_pixmap = FALSE;
	cfg->pixmap_file = NULL;

#endif /* HAVE_GNOME_CONFIG */

	cfg->window_title = NULL;
	cfg->window_icon = NULL;

	cfg->termname = NULL;
	cfg->terminal_id = 0;

	cfg->update_records = ZVT_TERM_DO_UTMP_LOG | ZVT_TERM_DO_WTMP_LOG | ZVT_TERM_DO_LASTLOG;

	/* load the palette, if none, then 'default' to safe (linux) pallete */
#ifdef HAVE_GNOME_CONFIG
	gnome_config_get_vector("palette", &colour_count, &colours);
#else
	colour_count = 0;
	colours = NULL;
#endif
	for (i=0;i<18;i++) {
		if (i>=colour_count || !gdk_color_parse(colours[i], &cfg->palette[i])) {
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
	newcfg->del_is_del     = GTK_TOGGLE_BUTTON (prefs->del_is_del_checkbox)->active;
	newcfg->menubar_hidden = GTK_TOGGLE_BUTTON (prefs->menubar_checkbox)->active;
	newcfg->scroll_out     = GTK_TOGGLE_BUTTON (prefs->scroll_out_checkbox)->active;
	newcfg->scroll_key     = GTK_TOGGLE_BUTTON (prefs->scroll_kbd_checkbox)->active;
	newcfg->scrollback = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (prefs->scrollback_spin));
	newcfg->login_by_default = GTK_TOGGLE_BUTTON (prefs->login_by_default_checkbox)->active;
	newcfg->use_bold = GTK_TOGGLE_BUTTON(prefs->use_bold_checkbox)->active;

	newcfg->transparent = GTK_TOGGLE_BUTTON (prefs->transparent_checkbox)->active;
	newcfg->shaded = GTK_TOGGLE_BUTTON (prefs->shaded_checkbox)->active;
	newcfg->background_pixmap = GTK_TOGGLE_BUTTON (prefs->pixmap_checkbox)->active;
	newcfg->pixmap_file  = g_strdup (gtk_entry_get_text (GTK_ENTRY (prefs->pixmap_entry)));
	newcfg->wordclass = g_strdup (gtk_entry_get_text (GTK_ENTRY (prefs->wordclass_entry)));
	newcfg->scrollbar_position = gtk_option_menu_get_history (GTK_OPTION_MENU (prefs->scrollbar));

	newcfg->scroll_background = GTK_TOGGLE_BUTTON (prefs->pixmap_scrollable_checkbox)->active
		&& newcfg->background_pixmap;

	g_free (newcfg->font);
	newcfg->font  = g_strdup (gtk_entry_get_text (GTK_ENTRY (prefs->font_entry)));

	if (!strcmp (gtk_entry_get_text (GTK_ENTRY (GTK_COMBO (prefs->class_box)->entry)),
			_("Default")))
		newcfg->class = g_strdup ("Config");
	else
		newcfg->class = g_strconcat ("Class-", gtk_entry_get_text (GTK_ENTRY (GTK_COMBO (prefs->class_box)->entry)), NULL);

	newcfg->color_type = gtk_option_menu_get_history (GTK_OPTION_MENU (prefs->color_scheme));
	newcfg->color_set  = gtk_option_menu_get_history (GTK_OPTION_MENU (prefs->def_fore_back));

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
	struct terminal_config *cfg = gtk_object_get_data (GTK_OBJECT (term), "config");

	terminal_config_free (cfg);
	cfg = terminal_config_dup (newcfg);
	gtk_object_set_data (GTK_OBJECT (term), "config", cfg);

	gnome_term_set_font (term, cfg->font, cfg->use_bold);
	zvt_term_set_wordclass (term, (guchar *)cfg->wordclass);
	zvt_term_set_bell(term, !cfg->bell);
	zvt_term_set_blink (term, cfg->blink);
	zvt_term_set_scroll_on_keystroke (term, cfg->scroll_key);
	zvt_term_set_scroll_on_output (term, cfg->scroll_out);
	zvt_term_set_scrollback (term, cfg->scrollback);
	zvt_term_set_del_key_swap (term, cfg->swap_keys);
	zvt_term_set_del_is_del (term, cfg->del_is_del);

	if (zvt_pixmap_support && cfg->background_pixmap) {
		int flags;
		flags = cfg->shaded?ZVT_BACKGROUND_SHADED:0;
		flags |= cfg->scroll_background?ZVT_BACKGROUND_SCROLL:0;
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
	
	{
		BonoboUIComponent *uic;
		
		uic = gtk_object_get_data (GTK_OBJECT (term), "uic");
		bonobo_ui_component_set_prop (uic, "/menu", "hidden",
					      cfg->menubar_hidden
					      ? "1" : "0", NULL);
	}

	return 0;
}

static void save_preferences_cmd (GtkWidget *widget, ZvtTerm *term);

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
switch_terminal_cb (GtkDialog *mbox, gint button, void *term)
{
	struct terminal_config *newcfg, *loaded_cfg, *cfg;

	newcfg = gtk_object_get_data (GTK_OBJECT (term), "newcfg");
	cfg = gtk_object_get_data (GTK_OBJECT (term), "config");

	if (button == 0){
		/* yes */
         	char *prefix = g_strdup_printf("/Terminal/%s/", newcfg->class);
#ifdef HAVE_GNOME_CONFIG
		gnome_config_push_prefix (prefix);
#endif
		loaded_cfg = load_config (newcfg->class);
		apply_changes (term, loaded_cfg);
		terminal_config_free (loaded_cfg);
#ifdef HAVE_GNOME_CONFIG
		gnome_config_pop_prefix ();
#endif
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

	mbox = gtk_message_dialog_new (GTK_WINDOW (prefs->prop_win),
				       GTK_DIALOG_NO_SEPARATOR,
				       GTK_MESSAGE_QUESTION,
				       GTK_BUTTONS_YES_NO,
				       _("You have switched the class of this window. Do you\n "
					 "want to reconfigure this window to match the default\n"
					 "configuration of the new class?"));
	
	gtk_signal_connect (GTK_OBJECT (mbox), "response", GTK_SIGNAL_FUNC (switch_terminal_cb),
			   term);
	gtk_signal_connect (GTK_OBJECT (mbox), "destroy", GTK_SIGNAL_FUNC (switch_terminal_closed),
			   term);

	gtk_widget_show (mbox);
}

/* this is the PREFS window destroy event ... */
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

	gtk_object_set_data(GTK_OBJECT (term), "matchstr", NULL);
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
	gtk_widget_set_sensitive (prefs->pixmap_scrollable_checkbox,
				  has_background);
}

/*
 * Called when something has changed on the properybox
 */
static void
prop_changed (GtkWidget *w, preferences_t *prefs)
{
	if (w != GTK_WIDGET (GTK_COMBO (prefs->class_box)->entry)) 
		prefs->changed = 1;
#ifdef HAVE_GNOME_PROPERTY_BOX
	gnome_property_box_changed (GNOME_PROPERTY_BOX (prefs->prop_win));
#endif
	check_image_options_sensitivity (prefs);
}

static void
color_changed (GtkWidget *w, int r, int g, int b, int a, preferences_t *prefs)
{
	prop_changed (w, prefs);
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
	gpointer /* GnomePropertyBox */ box;
	preferences_t	 *prefs;
} lambda_t;

static void
check_color_sensitivity (preferences_t *prefs)
{
	int idxc = gtk_option_menu_get_history (GTK_OPTION_MENU (prefs->def_fore_back));
	gboolean sensc = (idxc == COLORS_CUSTOM);
	int idx = gtk_option_menu_get_history (GTK_OPTION_MENU (prefs->color_scheme));
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
#ifdef HAVE_GNOME_PROPERTY_BOX
	gnome_property_box_changed (GNOME_PROPERTY_BOX (t->box));
#endif
}

static void
create_option_menu (GtkWidget *omenu,
		    GnomePropertyBox *box, preferences_t *prefs, char **menu_list, int item)
{
	GtkWidget *menu;
	lambda_t *t;
	int i = 0;
       
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

	gtk_signal_connect_full (GTK_OBJECT (omenu), 
				 "changed",
				 GTK_SIGNAL_FUNC (set_active), NULL,
				 t, (GtkDestroyNotify)g_free,
				 FALSE, FALSE);
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

#ifdef HAVE_GNOME_CONFIG
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
	gnome_config_set_bool   ("del_is_del", cfg->del_is_del);
	gnome_config_set_bool   ("login_by_default", cfg->login_by_default);
	gnome_config_set_bool   ("use_bold", cfg->use_bold);
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
	gnome_config_set_bool   ("scroll_background", cfg->scroll_background);
	gnome_config_set_bool   ("background_pixmap", cfg->background_pixmap);
	gnome_config_set_string ("pixmap_file", cfg->pixmap_file);

	for (i=0;i<18;i++)
		colours[i] = g_strdup_printf ("#%04x%04x%04x", cfg->palette[i].red,
					      cfg->palette[i].green, cfg->palette[i].blue);
	gnome_config_set_vector ("palette", 18, (const char **)colours);
	for (i=0;i<18;i++)
		g_free(colours[i]);

	gnome_config_sync ();
#endif
}

static void
save_preferences_cmd (GtkWidget *widget, ZvtTerm *term)
{
	struct terminal_config *cfg = 
		gtk_object_get_data (GTK_OBJECT (term), "config");
	char *prefix = g_strdup_printf ("/Terminal/%s/", cfg->class);

#ifdef HAVE_GNOME_CONFIG
	gnome_config_push_prefix (prefix);
#endif
	g_free (prefix);
	save_preferences (widget, term, cfg);
#ifdef HAVE_GNOME_CONFIG
	gnome_config_pop_prefix ();
#endif
}

static void
phelp_cb (GtkWidget *w, gint tab, gpointer data)
{
#ifdef HAVE_GNOME_HELP
	GnomeHelpMenuEntry help_entry = { "gnome-terminal",
					  "config.html" };
	char *pages[] = { "config.html#CONFIG-GENERAL",
			  "config.html#CONFIG-IMAGE",
			  "config.html#CONFIG-COLOUR",
			  "config.html#CONFIG-SCROLLING" };
	g_assert (tab < 4);
	help_entry.path = pages[tab];
	gnome_help_display(NULL, &help_entry);
#endif
}

static void
preferences_cmd (BonoboUIComponent *uic, ZvtTerm *term, const char *cname)
{
	GtkWidget *b1, *b2, *r;
	preferences_t *prefs;
	GList *class_list = NULL;
	void *iter;
	char *some_class;
	struct terminal_config *cfg;
	int i;
	GtkWidget *transient_parent;
	GladeXML *gui;
	gchar *glade_file;
	
	/* Is a property window for this terminal already running? */
	if (gtk_object_get_data (GTK_OBJECT (term), "newcfg"))
		return;
	
        transient_parent = gtk_widget_get_toplevel (GTK_WIDGET (term));

	prefs = gtk_object_get_data (GTK_OBJECT (term), "prefs");
        
	if (prefs) {
		if (transient_parent)
			gtk_window_set_transient_for (GTK_WINDOW (prefs->prop_win),
						      GTK_WINDOW (transient_parent));
		/* Raise and possibly uniconify the property box */
		gtk_window_present (GTK_WINDOW (prefs->prop_win));
		return;
	}

	cfg = gtk_object_get_data (GTK_OBJECT (term), "config");

	prefs = g_new0 (preferences_t, 1);
	prefs->changed = 0;

	glade_file = GNOME_TERMINAL_GLADEDIR "/gnome-terminal.glade2";

	gui = glade_xml_new (glade_file, "prefs", NULL);
	if (!gui) {
		g_warning ("Error loading `%s'", glade_file);
		return;
	}

	glade_xml_signal_autoconnect (gui);

	prefs->prop_win = glade_xml_get_widget (gui, "prefs");
	gtk_object_set_data (GTK_OBJECT (term), "prefs", prefs);
	if (transient_parent)
		gtk_window_set_transient_for (GTK_WINDOW (prefs->prop_win),
					      GTK_WINDOW (transient_parent));

	prefs->font_entry = glade_xml_get_widget (gui, "font-entry");
	gtk_entry_set_text (GTK_ENTRY (prefs->font_entry),
			    gtk_object_get_user_data (GTK_OBJECT (term)));
	gtk_entry_set_position (GTK_ENTRY (prefs->font_entry), 0);
	gtk_signal_connect (GTK_OBJECT (prefs->font_entry), "changed",
			    GTK_SIGNAL_FUNC (prop_changed), prefs);
	gnome_dialog_editable_enters (GNOME_DIALOG (prefs->prop_win),
				      GTK_EDITABLE (prefs->font_entry));

	prefs->class_box = glade_xml_get_widget (gui, "class-box");

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

	/* Toggle the use of bold */
	
	prefs->use_bold_checkbox = glade_xml_get_widget (gui, "use-bold-checkbox");
	
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs->use_bold_checkbox),
	                              cfg->use_bold ? 1 : 0);

	gtk_signal_connect (GTK_OBJECT (prefs->use_bold_checkbox), "toggled",
	                    GTK_SIGNAL_FUNC (prop_changed), prefs);

	/* Blinking status */
	prefs->blink_checkbox = glade_xml_get_widget (gui, "blink-checkbox");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs->blink_checkbox),
				      term->blink_enabled ? 1 : 0);
	gtk_signal_connect (GTK_OBJECT (prefs->blink_checkbox), "toggled",
			    GTK_SIGNAL_FUNC (prop_changed), prefs);

	/* Show menu bar */
	prefs->menubar_checkbox = glade_xml_get_widget (gui, "menubar-checkbox");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs->menubar_checkbox),
				      cfg->menubar_hidden ? 1 : 0);
	gtk_signal_connect (GTK_OBJECT (prefs->menubar_checkbox), "toggled",
			    GTK_SIGNAL_FUNC (prop_changed), prefs);

	/* Toggle the bell */
	prefs->bell_checkbox = glade_xml_get_widget (gui, "bell-checkbox");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs->bell_checkbox),
				      zvt_term_get_bell(term) ? 0 : 1);
	gtk_signal_connect (GTK_OBJECT (prefs->bell_checkbox), "toggled",
			    GTK_SIGNAL_FUNC (prop_changed), prefs);

	/* Swap keys */
	prefs->swapkeys_checkbox = glade_xml_get_widget (gui, "swapkeys-checkbox");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs->swapkeys_checkbox),
				      cfg->swap_keys ? 1 : 0);
	gtk_signal_connect (GTK_OBJECT (prefs->swapkeys_checkbox), "toggled",
			    GTK_SIGNAL_FUNC (prop_changed), prefs);
	
	/* Delete key: Send DEL/^H or kdch1 (Esc[3~) */
	prefs->del_is_del_checkbox = glade_xml_get_widget (gui, "del-is-del-checkbox");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs->del_is_del_checkbox),
				      cfg->del_is_del ? 1 : 0);
	gtk_signal_connect (GTK_OBJECT (prefs->del_is_del_checkbox), "toggled",
			    GTK_SIGNAL_FUNC (prop_changed), prefs);
	
	/* --login by default */
	prefs->login_by_default_checkbox = glade_xml_get_widget (gui, "login-by-default-checkbox");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs->login_by_default_checkbox),
				      cfg->login_by_default ? 1 : 0);
	gtk_signal_connect (GTK_OBJECT (prefs->login_by_default_checkbox), "toggled",
			    GTK_SIGNAL_FUNC (prop_changed), prefs);

	/* Word selection class */
	prefs->wordclass_entry = glade_xml_get_widget (gui, "wordclass-entry");
	gtk_entry_set_text (GTK_ENTRY (prefs->wordclass_entry),
			    cfg->wordclass);
	gtk_signal_connect (GTK_OBJECT (prefs->wordclass_entry), "changed",
			    GTK_SIGNAL_FUNC (prop_changed), prefs);
	gnome_dialog_editable_enters (GNOME_DIALOG (prefs->prop_win),
				      GTK_EDITABLE (prefs->wordclass_entry));


	/* Image page */
	/* if pixmap support isn't in zvt, we still create the widgets for
	   the page, so that we can query them later, but they won't be shown
	   so the user can't change them */
	if (!zvt_pixmap_support) {
		GtkWidget *image_table = glade_xml_get_widget (gui, "image-table");
		gtk_widget_hide (image_table);
	}
	
	r = glade_xml_get_widget (gui, "background-none-checkbox");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (r),
				      !(cfg->background_pixmap || cfg->transparent));
	
	/* Background Pixmap checkbox */
	prefs->pixmap_checkbox = glade_xml_get_widget (gui, "pixmap-checkbox");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs->pixmap_checkbox),
				      cfg->background_pixmap);
	gtk_signal_connect (GTK_OBJECT (prefs->pixmap_checkbox), "toggled",
			    GTK_SIGNAL_FUNC (prop_changed), prefs);
	
	/* Background pixmap filename */
	prefs->pixmap_label = glade_xml_get_widget (gui, "pixmap-label");

	prefs->pixmap_file_entry = glade_xml_get_widget (gui, "pixmap-file-entry");
	prefs->pixmap_entry =
		gnome_file_entry_gtk_entry(GNOME_FILE_ENTRY(prefs->pixmap_file_entry));
	gtk_entry_set_text (GTK_ENTRY (prefs->pixmap_entry),
			    cfg->pixmap_file?cfg->pixmap_file:"");
	gtk_entry_set_position (GTK_ENTRY (prefs->pixmap_entry), 0);
	gtk_signal_connect (GTK_OBJECT (prefs->pixmap_entry), "changed",
			    GTK_SIGNAL_FUNC (prop_changed), prefs);

	/* Scrollable pixmap */
	prefs->pixmap_scrollable_checkbox = glade_xml_get_widget (gui, "pixmap-scrollable-checkbox");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs->pixmap_scrollable_checkbox),
				      cfg->scroll_background ? 1 : 0);
	gtk_signal_connect (GTK_OBJECT (prefs->pixmap_scrollable_checkbox), "toggled",
			    GTK_SIGNAL_FUNC (prop_changed), prefs);

	/* Transparency */
	prefs->transparent_checkbox = glade_xml_get_widget (gui, "transparent-checkbox");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs->transparent_checkbox),
				      cfg->transparent);
	gtk_signal_connect (GTK_OBJECT (prefs->transparent_checkbox), "toggled",
			    GTK_SIGNAL_FUNC (prop_changed), prefs);

	/* Shaded */
	prefs->shaded_checkbox = glade_xml_get_widget (gui, "shaded-checkbox");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs->shaded_checkbox),
				      term->shaded ? 1 : 0);
	gtk_signal_connect (GTK_OBJECT (prefs->shaded_checkbox), "toggled",
			    GTK_SIGNAL_FUNC (prop_changed), prefs);

	check_image_options_sensitivity (prefs);


	/* Color page */
	
	/* Color palette */
	prefs->color_scheme = glade_xml_get_widget (gui, "color-scheme-optionmenu");
	create_option_menu (prefs->color_scheme,
			    GNOME_PROPERTY_BOX (prefs->prop_win), prefs,
			    color_scheme, cfg->color_type);
	gtk_object_set_user_data (GTK_OBJECT (prefs->color_scheme), GINT_TO_POINTER (cfg->color_type));

	/* Foreground, background buttons */
	prefs->fore_label = glade_xml_get_widget (gui, "foreground-label");
	
	prefs->palette[16] = glade_xml_get_widget (gui, "foreground-colorpicker");
	b1 = prefs->palette[16];
	gtk_signal_connect (GTK_OBJECT (b1), "destroy", GTK_SIGNAL_FUNC (free_cs), prefs->palette[16]);

	prefs->back_label = glade_xml_get_widget (gui, "background-label");
	
	prefs->palette[17] = glade_xml_get_widget (gui, "background-colorpicker");
	b2 = prefs->palette[17];
	gtk_signal_connect (GTK_OBJECT (b2), "destroy", GTK_SIGNAL_FUNC (free_cs), prefs->palette[17]);

	prefs->palette_label = glade_xml_get_widget (gui, "palette-label");
	
	for (i=0;i<18;i++) {
		if (i<16) {
			gchar *widget_name;

			widget_name = g_strdup_printf ("palette-%d-colorpicker", i);
			prefs->palette[i] = glade_xml_get_widget (gui, widget_name);
			g_free (widget_name);
		}
		gnome_color_picker_set_i16(GNOME_COLOR_PICKER(prefs->palette[i]), cfg->palette[i].red,
					   cfg->palette[i].green, cfg->palette[i].blue, 0);
		gtk_signal_connect (GTK_OBJECT (prefs->palette[i]), "color_set",
				    GTK_SIGNAL_FUNC (color_changed), prefs);
	}

	/* default foreground/background selector */
	prefs->def_fore_back = glade_xml_get_widget (gui, "fore-background-optionmenu");
	create_option_menu (prefs->def_fore_back,
			    GNOME_PROPERTY_BOX (prefs->prop_win), prefs,
			    fore_back_table, cfg->color_set);

	check_color_sensitivity (prefs);

	
	/* Scrolling page */

	/* Scrollbar position */
	prefs->scrollbar = glade_xml_get_widget (gui, "scrollbar-optionmenu");
	create_option_menu (prefs->scrollbar,
			    GNOME_PROPERTY_BOX (prefs->prop_win), prefs,
			    scrollbar_position_list,
			    cfg->scrollbar_position);
	gtk_object_set_user_data(GTK_OBJECT(prefs->scrollbar), GINT_TO_POINTER(cfg->scrollbar_position));

	/* Scroll back */
	prefs->scrollback_spin = glade_xml_get_widget (gui, "scrollback-spin");
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (prefs->scrollback_spin), (gfloat)cfg->scrollback);
	
	gtk_signal_connect (GTK_OBJECT (prefs->scrollback_spin), "changed",
			    GTK_SIGNAL_FUNC (prop_changed), prefs);
	gnome_dialog_editable_enters (GNOME_DIALOG (prefs->prop_win),
				      GTK_EDITABLE (prefs->scrollback_spin));

	/* Scroll on keystroke checkbox */
	prefs->scroll_kbd_checkbox = glade_xml_get_widget (gui, "scroll-kbd-checkbox");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs->scroll_kbd_checkbox),
				      cfg->scroll_key);
	gtk_signal_connect (GTK_OBJECT (prefs->scroll_kbd_checkbox), "toggled",
			    GTK_SIGNAL_FUNC (prop_changed), prefs);

	/* Scroll on output checkbox */
	prefs->scroll_out_checkbox = glade_xml_get_widget (gui, "scroll-out-checkbox");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs->scroll_out_checkbox),
				      cfg->scroll_out);
	gtk_signal_connect (GTK_OBJECT (prefs->scroll_out_checkbox), "toggled",
			    GTK_SIGNAL_FUNC (prop_changed), prefs);

	/* connect the property box signals */
	gtk_signal_connect (GTK_OBJECT (prefs->prop_win), "apply",
			    GTK_SIGNAL_FUNC (apply_changes_cmd), term);
	gtk_signal_connect (GTK_OBJECT (prefs->prop_win), "destroy",
			    GTK_SIGNAL_FUNC (window_destroy), term);
	gtk_signal_connect (GTK_OBJECT (prefs->prop_win), "help",
			    GTK_SIGNAL_FUNC (phelp_cb), NULL);

	gtk_widget_show (prefs->prop_win);

	g_object_unref (G_OBJECT (gui));
}

#define NEED_UNUSED_FUNCTIONS
#ifdef NEED_UNUSED_FUNCTIONS
static void
color_ok (GtkWidget *w)
{
	gtk_widget_destroy (gtk_widget_get_toplevel (w));
}

static void
color_cmd (BonoboUIComponent *uic, gpointer data, const char *cname)
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

static void
toggle_menubar_cmd (BonoboUIComponent *uic, 
		    const char *path,
		    Bonobo_UIComponent_EventType type,
		    const char *state,
		    gpointer data)
{
	ZvtTerm *term;
	GtkWindow *app;
	struct terminal_config *cfg;

	if (type != Bonobo_UIComponent_STATE_CHANGED)
		return;

	term = data;

	cfg = gtk_object_get_data (GTK_OBJECT (term), "config");

	cfg->menubar_hidden = atoi (state);

	bonobo_ui_component_set_prop (uic, "/menu", "hidden", state, NULL);

	app = GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (term)));
	app->need_default_size = TRUE;
	gtk_widget_queue_resize (GTK_WIDGET (app));

	set_size (GTK_WIDGET (term));
	save_preferences_cmd (NULL, term);
}

static void
reset_terminal_soft_cmd (BonoboUIComponent *uic, ZvtTerm *term, const char *cname)
{
	zvt_term_reset(term, 0);
}

/* could also possible clear the buffer? */
static void
reset_terminal_hard_cmd (BonoboUIComponent *uic, ZvtTerm *term, const char *cname)
{
	zvt_term_reset(term, 1);
}

static void
copy_cmd (BonoboUIComponent *uic, ZvtTerm *term, const char *cname)
{
	zvt_term_copy_clipboard (term);
}

static void
paste_cmd (BonoboUIComponent *uic, ZvtTerm *term, const char *cname)
{
	zvt_term_paste_clipboard (term);
}

static void
toggle_secure_keyboard_cmd (BonoboUIComponent *uic, 
			    const char *path,
			    Bonobo_UIComponent_EventType type,
			    const char *state,
			    gpointer data)
{
	ZvtTerm *term;
	struct terminal_config *cfg;

	if (type != Bonobo_UIComponent_STATE_CHANGED)
		return;

	term = data;

	cfg = gtk_object_get_data (GTK_OBJECT (term), "config");

	cfg->keyboard_secured = atoi (state);

	if (cfg->keyboard_secured)
		gdk_keyboard_grab (term->term_window, TRUE, GDK_CURRENT_TIME);
	else
		gdk_keyboard_ungrab (GDK_CURRENT_TIME);
}

static void
copy_url_cmd (BonoboUIComponent *uic, ZvtTerm *term, const char *cname)
{
	char *url;

	url = gtk_object_get_data (GTK_OBJECT (term), "matchstr");
	if (url) {
		gtk_clipboard_set_text (
			gtk_clipboard_get (GDK_SELECTION_PRIMARY), 
			url, -1);
	}
}

static void
load_url_cmd (BonoboUIComponent *uic, ZvtTerm *term, const char *cname)
{
	char *url;

	url = gtk_object_get_data (GTK_OBJECT (term), "matchstr");

#ifdef FIXME
	if (url && strlen (url)) {
		gnome_url_show(url, NULL);
	}
#endif /* FIXME */
}

static void
help_cmd (BonoboUIComponent *uic, gpointer data, const char *cname)
{
}

static void new_terminal (BonoboUIComponent *uic, ZvtTerm *term, const char *cname);

static BonoboUIVerb terminal_verbs[] = {
	BONOBO_UI_UNSAFE_VERB ("NewTerminal", new_terminal),
	BONOBO_UI_UNSAFE_VERB ("CloseTerminal", close_terminal_cmd),
	BONOBO_UI_UNSAFE_VERB ("Copy", copy_cmd),
	BONOBO_UI_UNSAFE_VERB ("Paste", paste_cmd),
	BONOBO_UI_UNSAFE_VERB ("Preferences", preferences_cmd),
	BONOBO_UI_UNSAFE_VERB ("Reset", reset_terminal_soft_cmd),
	BONOBO_UI_UNSAFE_VERB ("Clear", reset_terminal_hard_cmd),
	BONOBO_UI_UNSAFE_VERB ("ColorSelector", color_cmd),
	BONOBO_UI_UNSAFE_VERB ("UsersGuide", help_cmd),
	BONOBO_UI_UNSAFE_VERB ("About", about_terminal_cmd),
	BONOBO_UI_UNSAFE_VERB ("CopyURL", copy_url_cmd),
	BONOBO_UI_UNSAFE_VERB ("OpenURL", load_url_cmd),
	BONOBO_UI_VERB_END
};

/*
 * Puts in *shell a pointer to the full shell pathname
 * Puts in *name the invocation name for the shell
 * *shell is newly allocated 
 * *name is newly allocated
 */
static void
get_shell_name (char **shell, char **name, gboolean isLogin)
{
	char *only_name;
	int len;

	*shell = gnome_util_user_shell ();
	g_assert (*shell != NULL);

	only_name = strrchr (*shell, '/');
	if (only_name != NULL)
		only_name++;
	else
		only_name = *shell;
	
	if (isLogin){
		len = strlen (only_name);
		
		*name  = g_malloc (len + 2);
		**name = '-';
		strcpy ((*name)+1, only_name); 
	} else {
		*name = g_strdup (only_name);
	}
}

static void
terminal_kill (GtkWidget *widget, void *data)
{
	GtkWidget *app = gtk_widget_get_toplevel (GTK_WIDGET (data));

	close_app (GTK_WIDGET (app));
}

/* called for "title_changed" event.  Use it to change the window title */
static void
title_changed(ZvtTerm *term, VTTITLE_TYPE type, char *newtitle)
{
	BonoboWindow *window = BONOBO_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (term)));
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

static gchar *
extract_filename (const gchar* uri)
{
	/* file uri with a hostname */
	if (strncmp (uri, "file://", strlen ("file://")) == 0) {
		char *hostname = g_strdup (&uri[strlen("file://")]);
		char *p = strchr (hostname, '/');
		char *path;
		char localhostname[1024];
		/* if we can't find the '/' this uri is bad */
		if(p == NULL) {
			g_free (hostname);
			return NULL;
		}
		/* if no hostname */
		if(p == hostname)
			return hostname;

		path = g_strdup (p);
		*p = '\0';

		/* if really local */
		if (g_strcasecmp (hostname, "localhost") == 0 ||
		    g_strcasecmp (hostname, "localhost.localdomain") == 0) {
			g_free (hostname);
			return path;
		}

		/* ok get the hostname */
		if (gethostname (localhostname,
				 sizeof (localhostname)) < 0) {
			strcpy (localhostname, "");
		}

		/* if really local */
		if (localhostname[0] &&
		    g_strcasecmp (hostname, localhostname) == 0) {
			g_free (hostname);
			return path;
		}
		
		g_free (hostname);
		g_free (path);
		return NULL;

	/* if the file doesn't have the //, we take it containing 
	   a local path */
	} else if (strncmp(uri, "file:", strlen("file:"))==0) {
		const char *path = &uri[strlen("file:")];
		/* if empty bad */
		if(!*path) return NULL;
		return g_strdup(path);
	}
	return NULL;
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
#ifdef HAVE_GNOME_URI_LIST
		GList *uris, *l;
#endif		

		strncpy (copy, selection_data->data, selection_data->length);
		copy [selection_data->length] = 0;
#ifdef HAVE_GNOME_URI_LIST		
		uris = gnome_uri_list_extract_uris (copy);

		for (l = uris; l; l = l->next){
			char *data = l->data;

			if (strncmp (data, "file:", 5) == 0)
				data += 5;
			
			vt_writechild (&term->vx->vt, data, strlen (data));
			vt_writechild (&term->vx->vt, " ", 1);
		}
		gnome_uri_list_free_strings (uris);
#endif
		g_free (copy);
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
		break;
	}
	case TARGET_BGIMAGE:
	{
		char *filename = (char *)selection_data->data;
		preferences_t *prefs;
		gboolean back_set;
		gboolean back_reset;

		if (filename != NULL &&
		    strstr (filename, RESET_IMAGE_NAME) != NULL) {
			zvt_term_set_background (term, NULL, 0, 0);

			cfg->background_pixmap = FALSE;

			/* Switch to white on black colors */
			cfg->color_set = COLORS_WHITE_ON_BLACK;
		
			set_color_scheme (term, cfg);
			save_preferences_cmd (widget, term);

			back_set = FALSE;
			back_reset = TRUE;
		} else if (zvt_pixmap_support &&
			   filename != NULL) {
			int flags;

			cfg->background_pixmap = TRUE;
			cfg->transparent = FALSE;
			g_free (cfg->pixmap_file);
			cfg->pixmap_file = extract_filename (filename);

			flags = cfg->shaded?ZVT_BACKGROUND_SHADED:0;
			flags |= cfg->scroll_background?ZVT_BACKGROUND_SCROLL:0;

			zvt_term_set_background (term,
						 cfg->pixmap_file,
						 cfg->transparent, flags);
			back_set = TRUE;
			back_reset = FALSE;
		} else {
			zvt_term_set_background (term, NULL, 0, 0);
			cfg->background_pixmap = FALSE;

			back_set = FALSE;
			back_reset = FALSE;
		}

		prefs  = gtk_object_get_data (GTK_OBJECT (term), "prefs");
		if (prefs) {
			if (back_reset) {
				gtk_option_menu_set_history (GTK_OPTION_MENU (prefs->def_fore_back),
							     cfg->color_set);
				check_color_sensitivity (prefs);
			} else if (back_set) {
				gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs->pixmap_checkbox), TRUE);
				gtk_entry_set_text (GTK_ENTRY (prefs->pixmap_entry),
						    cfg->pixmap_file ? cfg->pixmap_file : "");
			} else {
				gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs->pixmap_checkbox), FALSE);
			}
		}

		gtk_widget_queue_draw (GTK_WIDGET (term));
		break;
	}
	}
}

static void
configure_term_dnd (ZvtTerm *term)
{
	static GtkTargetEntry target_table[] = {
		{ "STRING",     0, TARGET_STRING },
		{ "text/plain", 0, TARGET_STRING },
		{ "application/x-color", 0, TARGET_COLOR },
		{ "property/bgimage",    0, TARGET_BGIMAGE }
	};

	gtk_signal_connect (GTK_OBJECT (term), "drag_data_received",
			    GTK_SIGNAL_FUNC(drag_data_received), NULL);

	gtk_drag_dest_set (GTK_WIDGET (term),
			   GTK_DEST_DEFAULT_MOTION |
			   GTK_DEST_DEFAULT_HIGHLIGHT |
			   GTK_DEST_DEFAULT_DROP,
			   target_table, 4,
			   GDK_ACTION_COPY);
}

static int
button_press (GtkWidget *widget, GdkEventButton *event, ZvtTerm *term)
{
	BonoboUIComponent *uic;
	GtkWidget *menu;
	struct terminal_config *cfg;
	char *match;
	int x,y;
	GdkModifierType mask;

	gdk_window_get_pointer(widget->window, &x, &y, &mask);
	match = zvt_term_match_check(term, x/term->charwidth, y/term->charheight, 0);

	if (event->button == 1
	    && (event->state & GDK_CONTROL_MASK)
	    && match) {
#ifdef FIXME
		gnome_url_show (match, NULL);
#endif /* FIXME */
	}

	if (event->button != 3
	    || (!(event->state & GDK_CONTROL_MASK) && term->vx->selected)
	    || (term->vx->vt.mode & VTMODE_SEND_MOUSE))
		return FALSE;

	uic = gtk_object_get_data (GTK_OBJECT (term), "uic");

	/* construct the popup menu properly */
	if (match) {
		char *tmp;
		gtk_object_set_data_full (GTK_OBJECT (term), "matchstr",
					  g_strdup(match), (GDestroyNotify)g_free);
	}

	menu = gtk_menu_new ();

	bonobo_ui_component_set_prop (uic, "/commands/CopyURL",
				      "hidden", match ? "0" : "1", NULL);

	bonobo_ui_component_set_prop (uic, "/commands/OpenURL",
				      "hidden", match ? "0" : "1", NULL);

	bonobo_window_add_popup (
		BONOBO_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (term))),
		GTK_MENU (menu), "/popups/TerminalPopup");

	cfg = gtk_object_get_data (GTK_OBJECT (term), "config");

#if 0
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
#endif
	
#if 0
	gnome_popup_menu_do_popup_modal (menu, NULL, NULL, event, term);
	gtk_widget_destroy (menu);
#else
	gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, event->button, event->time);
#endif

#if 0
	/*
	 * The popup menu ungrabs the keyboard if it was grabbed,
	 * so we grab it again here.
	 */
	if (cfg->keyboard_secured)
		gdk_keyboard_grab (term->term_window, TRUE, GDK_CURRENT_TIME);
#endif

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
	hints.base_width = (widget->style->xthickness * 2) + PADDING;
	hints.base_height =  (widget->style->ythickness * 2);

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
set_size (GtkWidget *widget)
{
        ZvtTerm *term;
	GtkWidget *app;
	GtkRequisition toplevel_request;
	GtkRequisition widget_request;
	gint w, h;

	g_assert (widget != NULL);
	term = ZVT_TERM (widget);

	app = gtk_widget_get_toplevel(widget);
	g_assert (app != NULL);

	gtk_widget_size_request (app, &toplevel_request);
	gtk_widget_size_request (widget, &widget_request);

	w = toplevel_request.width - widget_request.width;
	h = toplevel_request.height - widget_request.height;

	w += widget->style->xthickness * 2 + PADDING;
	h += widget->style->ythickness * 2;

	w += term->charwidth * term->grid_width;
	h += term->charheight * term->grid_height;

	gtk_window_resize (GTK_WINDOW (app), w, h);
}

static gint
term_change_pos(GtkWidget *widget)
{
	static int x=-999;
	static int y=-999;
	int nx,ny;
	
	if(!widget->window ||
	   !ZVT_TERM(widget)->transparent)
		return FALSE;
	
	gdk_window_get_position(widget->window,&nx,&ny);
	
	if(nx!=x || ny!=y)
		gtk_widget_queue_draw(widget);
	
	return FALSE;
}

/* do this because there have been only 5 bug reports about this */
static void
show_pty_error_dialog (int errcode)
{
	GtkWidget *dialog;

	perror ("Error: unable to fork");
	
	dialog = gtk_message_dialog_new (NULL, 
					 GTK_DIALOG_NO_SEPARATOR,
					 GTK_MESSAGE_ERROR,
					 GTK_BUTTONS_OK,
					 _("There has been an error while trying to log in:\n\n%s"),
					 g_strerror (errcode));
							
	gtk_dialog_run (GTK_DIALOG (dialog));
}

static GtkWidget *
new_terminal_cmd (char **cmd, struct terminal_config *cfg_in, const gchar *geometry, int termid)
{
	BonoboUIComponent *uic;
	Bonobo_UIContainer container;

	GtkWidget *app, *hbox, *scrollbar;
	ZvtTerm   *term;
	static char **env_copy;
	static int winid_pos;
	static int term_pos;
	char buffer [40];
	char winclass [64];
	char *shell, *name;
	int i = 0;
	struct terminal_config *cfg;
	int width, height;
	int xpos, ypos;
		
	/* FIXME: is seems like a lot of this stuff should be done by apply_changes instead */

	cfg = terminal_config_dup (cfg_in);

	cfg->terminal_id = termid;

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
		        if ((strncmp (*p, "COLUMNS=", 8) == 0)
				   || (strncmp (*p, "LINES=", 6) == 0)
				   || (strncmp(*p, "WINDOWID=", 9) == 0)
			           || (strncmp (*p, "TERM=", 5) == 0)
				   || (strncmp (*p, "GNOME_DESKTOP_ICON=", 19) == 0)) {
				/* nothing: do not copy those */
			} else
				env_copy [i++] = *p;
		}
		term_pos = i++;
		env_copy [term_pos] = "";
		env_copy [i++] = "COLORTERM=gnome-terminal";
		winid_pos = i++;
		env_copy [winid_pos] = "TEST";
		env_copy [i] = NULL;
	}

	app = bonobo_window_new ("Terminal", _("Terminal"));
	/* override the title if it was in the config */
	if (cfg->window_title) {
	  gtk_window_set_title(GTK_WINDOW(app), cfg->window_title);
 	}

	/* override the icon if it was in the config */
	if (cfg->window_icon)
	  gnome_window_icon_set_from_file (GTK_WINDOW(app), cfg->window_icon);

	g_snprintf (winclass, sizeof (winclass), "Terminal.%d", termid);
	gtk_window_set_wmclass (GTK_WINDOW (app), winclass, "Terminal");

	if (cmd != NULL)
		initial_term = app;

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
#ifdef HAVE_GNOME_PARSE_GEOMETRY
	if (geometry) {
		gnome_parse_geometry (geometry, &xpos, &ypos, &width, &height);
		if (width < 1 || height < 1) {
			width=80;
			height=24;
		}
		/* Only the first window gets --geometry treatment for now */
		geometry = NULL;
	}
#endif
	term = (ZvtTerm *)zvt_term_new_with_size(width, height);

	zvt_term_set_auto_window_hint (term, FALSE);
	
	/* This is a bad hack ... basically, the "geometry_widget" feature of GtkWindow
	 * only works properly when the widget's requisition is bigger than the surroundings,
	 * so we force a big requisition ... it won't actually matter for anything
	 */
	gtk_widget_set_usize (GTK_WIDGET (term), 2000, 2000);

	gtk_object_set_data (GTK_OBJECT (term), "config", cfg);
	if (xpos != -1 && ypos != -1)
		gtk_window_move (GTK_WINDOW (app), xpos, ypos);

	if ((zvt_term_get_capabilities (term) & ZVT_TERM_PIXMAP_SUPPORT) != 0){
		zvt_pixmap_support = TRUE;
	}
	gtk_object_set_data(GTK_OBJECT(app), "term", term);
	gtk_widget_grab_focus(GTK_WIDGET(term));

	zvt_term_set_scrollback (term, cfg->scrollback);
	gnome_term_set_font (term, cfg->font, cfg->use_bold);
	zvt_term_set_wordclass  (term, (guchar *)cfg->wordclass);
	zvt_term_set_bell  (term, !cfg->bell);
	zvt_term_set_blink (term, cfg->blink);
	zvt_term_set_scroll_on_keystroke (term, cfg->scroll_key);
	zvt_term_set_scroll_on_output (term, cfg->scroll_out);
	zvt_term_set_del_key_swap (term, cfg->swap_keys);
	zvt_term_set_del_is_del (term, cfg->del_is_del);

	gtk_signal_connect (GTK_OBJECT (term), "child_died",
			    GTK_SIGNAL_FUNC (terminal_kill), term);

	gtk_signal_connect (GTK_OBJECT (term), "title_changed",
			    (GtkSignalFunc) title_changed, term);

	gtk_signal_connect (GTK_OBJECT (app), "delete_event",
			   GTK_SIGNAL_FUNC (close_app), term);

	gtk_signal_connect (GTK_OBJECT (term), "button_press_event",
			    GTK_SIGNAL_FUNC (button_press), term);

	zvt_term_match_add( ZVT_TERM(term), "(((news|telnet|nttp|file|http|ftp|https)://)|(www|ftp)[-A-Za-z0-9]*\\.)[-A-Za-z0-9\\.]+(:[0-9]*)?", VTATTR_UNDERLINE, "host only url");
	zvt_term_match_add( ZVT_TERM(term), "(((news|telnet|nttp|file|http|ftp|https)://)|(www|ftp)[-A-Za-z0-9]*\\.)[-A-Za-z0-9\\.]+(:[0-9]*)?/[-A-Za-z0-9_\\$\\.\\+\\!\\*\\(\\),;:@&=\\?/~\\#\\%]*[^]'\\.}>\\) ,\\\"]", VTATTR_UNDERLINE, "full url");

	uic = bonobo_ui_component_new_default ();
	
	gtk_object_set_data (GTK_OBJECT (term), "uic", uic);
	
	container = bonobo_object_corba_objref (
		BONOBO_OBJECT (bonobo_window_get_ui_container (BONOBO_WINDOW (app))));
	bonobo_ui_component_set_container (uic, container, NULL);
	
	bonobo_ui_component_add_verb_list_with_data (uic, terminal_verbs, term);
	bonobo_ui_util_set_ui (uic, GNOME_TERMINAL_DATADIR,
			       "gnome-terminal.xml", "gnome-terminal", NULL);
	
	bonobo_ui_component_add_listener (uic, "ToggleMenuBar", 
					  toggle_menubar_cmd, term);
	
	bonobo_ui_component_add_listener (uic, "SecureKeyboard",
					  toggle_secure_keyboard_cmd, term);
						   
	/* Decorations */
	hbox = gtk_hbox_new (0, 0);

/*	gtk_container_set_border_width (GTK_CONTAINER (hbox), 2); */
       	gtk_box_set_spacing (GTK_BOX (hbox), 0);

	gtk_widget_show (GTK_WIDGET (term));
	gtk_widget_show (hbox);

	get_shell_name (&shell, &name, cfg->invoke_as_login_shell);

	scrollbar = gtk_vscrollbar_new (GTK_ADJUSTMENT (term->adjustment));
	gtk_object_set_data (GTK_OBJECT (term), "scrollbar", scrollbar);
	GTK_WIDGET_UNSET_FLAGS (scrollbar, GTK_CAN_FOCUS);
	
	if (cfg->scrollbar_position == SCROLLBAR_LEFT)
		gtk_box_pack_start (GTK_BOX (hbox), scrollbar, 0, 1, 0);
	else
		gtk_box_pack_end (GTK_BOX (hbox), scrollbar, 0, 1, 0);
	if (cfg->scrollbar_position != SCROLLBAR_HIDDEN)
		gtk_widget_show (scrollbar);

	gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (term), 1, 1, 0);
	
	configure_term_dnd (term);

	if (zvt_pixmap_support && cfg->background_pixmap) {
		int flags;
		flags = cfg->shaded?ZVT_BACKGROUND_SHADED:0;
		flags |= cfg->scroll_background?ZVT_BACKGROUND_SCROLL:0;
		zvt_term_set_background (term,
					 cfg->pixmap_file,
					 cfg->transparent, flags);
	} else if (zvt_pixmap_support && cfg->transparent)
		zvt_term_set_background (term, NULL,
					 cfg->transparent, cfg->shaded);
	else
		zvt_term_set_background (term, NULL, 0, 0);

	bonobo_window_set_contents (BONOBO_WINDOW (app), hbox);

	bonobo_ui_component_set_prop (uic, "/commands/ToggleMenuBar",
				      "state",
				      cfg->menubar_hidden
				      ? "1" : "0", NULL);

	bonobo_ui_component_set_prop (uic, "/commands/SecureKeyboard",
				      "state",
				      cfg->keyboard_secured
				      ? "1" : "0", NULL);

	/* 
	 * we need to realize the term before it gets shown so that
	 * the sizing works right 
	 */

	gtk_widget_realize (GTK_WIDGET (term));
	set_hints (GTK_WIDGET (term));
	set_size (GTK_WIDGET (term));
	
	set_color_scheme (term, cfg);

	gtk_widget_show (app);

	gdk_flush ();

	errno = 0;
	switch (zvt_term_forkpty (term, cfg->update_records)){
	case -1:
		show_pty_error_dialog(errno);
		g_free (shell);
		g_free (name);

		/* should we exit maybe? */
		return NULL;
		
	case 0: {
		int open_max = sysconf (_SC_OPEN_MAX);

		for (i = 3; i < open_max; i++)
			fcntl (i, F_SETFD, 1);

		/* set delayed env variables */
		g_snprintf (buffer, sizeof (buffer),
			    "WINDOWID=%d",(int) GDK_WINDOW_XWINDOW(GTK_WIDGET(term)->window));
		env_copy [winid_pos] = buffer;

		if (cfg->termname && cfg->termname [0])
			env_copy [term_pos] = cfg->termname;
		else
			env_copy [term_pos] = "TERM=xterm";

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
	g_free (name);

	return app;
}

static void
new_terminal (BonoboUIComponent *uic, ZvtTerm *term, const char *cname)
{
	struct terminal_config *cfg;

	cfg = gtk_object_get_data (GTK_OBJECT (term), "config");

	new_terminal_cmd (NULL, cfg, NULL, terminal_id++);
}

GtkWidget *
new_terminal_for_client (const char *geom)
{
	if (terminals != 0)
	{
		ZvtTerm *term = ZVT_TERM (gtk_object_get_data
					  (GTK_OBJECT(terminals->data),
					   "term"));
		struct terminal_config *cfg
			= gtk_object_get_data (GTK_OBJECT (term), "config");
		GtkWidget *w = new_terminal_cmd (NULL, cfg, geom ? geom
						 : initial_global_geometry,
						 terminal_id++);
		return w;
	}
	else
		return 0;
}

static void
load_factory_settings ()
{
	const char *path;
	gchar *buf;

	path = gnome_client_get_config_prefix (gnome_master_client ());
#ifdef HAVE_GNOME_CONFIG
	gnome_config_push_prefix (path);

	buf = g_strdup_printf ("factory/start=%d", start_terminal_factory);
	start_terminal_factory = gnome_config_get_int (buf);
	g_free (buf);

	buf = g_strdup_printf ("factory/use=%d", use_terminal_factory);
	use_terminal_factory = gnome_config_get_int (buf);
	g_free (buf);

	gnome_config_pop_prefix ();
#endif
}

static gboolean
load_session (void)
{
#ifdef HAVE_GNOME_CONFIG
	int num_terms, i;
	gboolean def;
	const gchar *file;

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
		int termid;

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
		cfg->window_icon = gnome_config_get_string("window_icon");
		if (gnome_config_get_bool ("do_utmp=true"))
			cfg->update_records |= ZVT_TERM_DO_UTMP_LOG;
		if (gnome_config_get_bool ("do_wtmp=true"))
			cfg->update_records |= ZVT_TERM_DO_WTMP_LOG;
		if (gnome_config_get_bool ("do_lastlog=true"))
			cfg->update_records |= ZVT_TERM_DO_LASTLOG;

		start_terminal_factory = gnome_config_get_bool ("start_terminal_factory=false");
		use_terminal_factory = gnome_config_get_bool ("use_terminal_factory=false");

		termid = gnome_config_get_int("terminal_id=-1");
		if (termid!=-1)
			terminal_id = termid;

		g_free(class);
		gnome_config_pop_prefix();

		new_terminal_cmd (argv, cfg, geom, terminal_id++);

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
#else /* HAVE_GNOME_CONFIG */
	return FALSE;
#endif /* HAVE_GNOME_CONFIG */
}

/* Save terminal in this session.  FIXME: should save all terminal
   windows, but currently does not.  */
static gint
save_session (GnomeClient *client, gint phase, GnomeSaveStyle save_style,
	      gint is_shutdown, GnomeInteractStyle interact_style,
	      gint is_fast, gpointer client_data)
{
#ifdef HAVE_GNOME_CONFIG
	const char *file = gnome_client_get_config_prefix (client);
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
		   (GTK_WIDGET (term)->style->xthickness * 2)) /
		  term->charwidth;

		height = 
		  (GTK_WIDGET (term)->allocation.height - 
		   (GTK_WIDGET (term)->style->ythickness * 2)) /
		  term->charheight;

		geom = g_string_new ("");
		g_string_sprintf (geom, "%dx%d%+d%+d", width, height, x, y);
		gnome_config_set_string ("geometry", geom->str);
		g_string_free (geom, TRUE);
		
		gnome_config_set_string("class", cfg->class);
		gnome_config_set_int("terminal_id", cfg->terminal_id);
		
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

		gnome_config_set_string ("window_title",
					 cfg->window_title != NULL ?
					   cfg->window_title : _("Terminal"));
		gnome_config_set_string ("window_icon",
					 cfg->window_icon != NULL ?
					   cfg->window_icon : GNOME_ICONDIR"/gnome-terminal.png");
		gnome_config_set_bool("do_utmp", (cfg->update_records & ZVT_TERM_DO_UTMP_LOG) != 0);
		gnome_config_set_bool("do_wtmp", (cfg->update_records & ZVT_TERM_DO_WTMP_LOG) != 0);
		gnome_config_set_bool("start_terminal_factory", 
				      start_terminal_factory);
		gnome_config_set_bool("use_terminal_factory",
				      use_terminal_factory);
		gnome_config_set_bool("do_lastlog", (cfg->update_records & ZVT_TERM_DO_LASTLOG) != 0);

		gnome_config_pop_prefix ();
		g_free (prefix);
		
		++i;
	}
	gnome_config_push_prefix (file);
	gnome_config_set_int ("dummy/num_terms", i);
	gnome_config_set_int ("factory/start", start_terminal_factory);
	gnome_config_set_int ("factory/use", use_terminal_factory);
	gnome_config_pop_prefix ();

#if 0
	char *args[8];
	
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

#if 0 /* broken in gnome 2 */
	args[0] = "rm";
	args[1] = gnome_config_get_real_path (file);
	args[2] = NULL;
	gnome_client_set_discard_command (client, 2, args);
#endif

#endif /* HAVE_GNOME_CONFIG */
	return TRUE;
}

/* Keys for the ARGP parser, should be negative */
enum {
	FONT_KEY      	    = -1,
	NOLOGIN_KEY   	    = -2,
	LOGIN_KEY     	    = -3,
	GEOMETRY_KEY  	    = -4,
	COMMAND_KEY   	    = 'e',
	FORE_KEY      	    = -6,
	BACK_KEY      	    = -7,
	CLASS_KEY     	    = -8,
	DOUTMP_KEY    	    = -9,
	DONOUTMP_KEY  	    = -10,
	DOWTMP_KEY    	    = -11,
	DONOWTMP_KEY  	    = -12,
        TITLE_KEY     	    = -13,
	TERM_KEY            = -14,
	START_FACTORY_KEY   = -15,
	USE_FACTORY_KEY     = -16,
	DOLASTLOG_KEY       = -17,
	DONOLASTLOG_KEY     = -18,
	ICON_KEY            = -19,
        PIXMAP_KEY          = -20,
        SHADED_KEY          = -21,
        NOSHADED_KEY        = -22,
        TRANSPARENT_KEY     = -23,

        SOLID_KEY           = -24,
        BGSCROLL_PIXMAP_KEY   = -25,
        BGNOSCROLL_PIXMAP_KEY = -26
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

	{ "solid", '\0', POPT_ARG_NONE, NULL, SOLID_KEY,
	  N_("Solid background"), N_("SOLID") },	

	{ "pixmap", '\0', POPT_ARG_STRING, NULL, PIXMAP_KEY,
	  N_("Background pixmap"), N_("PIXMAP")},

	{ "bgscroll", '\0', POPT_ARG_NONE, NULL, BGSCROLL_PIXMAP_KEY,
	  N_("Background pixmap scrolls"), N_("BGSCROLL")},

	{ "bgnoscroll", '\0', POPT_ARG_NONE, NULL, BGNOSCROLL_PIXMAP_KEY,
	  N_("Background pixmap does not scroll"), N_("BGNOSCROLL")},

	{ "shaded", '\0', POPT_ARG_NONE, NULL, SHADED_KEY,
	  N_("Shade background"), N_("SHADED") },	

	{ "noshaded", '\0', POPT_ARG_NONE, NULL, NOSHADED_KEY,
	  N_("Do not shade background"), N_("NOSHADED") },	

	{ "transparent", '\0', POPT_ARG_NONE, NULL, TRANSPARENT_KEY,
	  N_("Transparent background"), N_("TRANSPARENT") },	

	{ "utmp", '\0', POPT_ARG_NONE, NULL, DOUTMP_KEY,
	  N_("Update utmp entry"), N_("UTMP") },

	{ "noutmp", '\0', POPT_ARG_NONE, NULL, DONOUTMP_KEY,
	  N_("Do not update utmp entry"), N_("NOUTMP") },

	{ "wtmp", '\0', POPT_ARG_NONE, NULL, DOWTMP_KEY,
	  N_("Update wtmp entry"), N_("WTMP") },

	{ "nowtmp", '\0', POPT_ARG_NONE, NULL, DONOWTMP_KEY,
	  N_("Do not update wtmp entry"), N_("NOWTMP") },

	{ "lastlog", '\0', POPT_ARG_NONE, NULL, DOLASTLOG_KEY,
	  N_("Update lastlog entry"), N_("LASTLOG") },

	{ "nolastlog", '\0', POPT_ARG_NONE, NULL, DONOLASTLOG_KEY,
	  N_("Do not update lastlog entry"), N_("NOLASTLOG") },
	
	{ "title", 't', POPT_ARG_STRING, NULL, TITLE_KEY,
          N_("Set the window title"), N_("TITLE") },

	{ "icon", '\0', POPT_ARG_STRING, NULL, ICON_KEY,
	  N_("Set the window icon"), N_("ICON") },

	{ "termname", '\0', POPT_ARG_STRING, NULL, TERM_KEY,
          N_("Set the TERM variable"), N_("TERMNAME") },

	{ "start-factory-server", '\0', POPT_ARG_NONE, NULL, START_FACTORY_KEY,
	  N_("Try to start a TerminalFactory"), NULL },

	{ "use-factory", '\0', POPT_ARG_NONE, NULL, USE_FACTORY_KEY,
	  N_("Try to create the terminal with the TerminalFactory"), NULL },

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
		start_terminal_factory = FALSE;
		use_terminal_factory = FALSE;
		break;
	case FONT_KEY:
		cfg->font = (char *)arg;
		start_terminal_factory = FALSE;
		use_terminal_factory = FALSE;
		break;
	case LOGIN_KEY:
		cfg->invoke_as_login_shell = 1;
		cmdline_login = TRUE;
		start_terminal_factory = FALSE;
		use_terminal_factory = FALSE;
		break;
	case NOLOGIN_KEY:
	        cfg->invoke_as_login_shell = 0;
		cmdline_login = TRUE;
		start_terminal_factory = FALSE;
		use_terminal_factory = FALSE;
	        break;
	case GEOMETRY_KEY:
		initial_global_geometry = (char *)arg;
		break;
	case COMMAND_KEY:
		  {
			  const char **foo;
			  int x;
			  
			  poptParseArgvString((char *)arg,&x,&foo);
			  initial_command=malloc((x+1)*sizeof(char *));
			  initial_command[x]=NULL;
			  while (x>0) {
				  x--;
				  initial_command[x]=foo[x];
			  }
		
		start_terminal_factory = FALSE;
		use_terminal_factory = FALSE;
		break;
		  }
	case FORE_KEY:
		cfg->user_fore_str = arg;
		start_terminal_factory = FALSE;
		use_terminal_factory = FALSE;
		break;
	case BACK_KEY:
		cfg->user_back_str = arg;
		start_terminal_factory = FALSE;
		use_terminal_factory = FALSE;
		break;
	case SOLID_KEY:
		cfg->background_pixmap = 0;
		cfg->transparent = 0;
		cfg->have_user_background = 1;
		start_terminal_factory = FALSE;
		use_terminal_factory = FALSE;
		break;
	case PIXMAP_KEY:
		cfg->background_pixmap = 1;
		cfg->transparent = 0;
		cfg->pixmap_file = g_strdup(arg);
		cfg->have_user_background = 1;
		start_terminal_factory = FALSE;
		use_terminal_factory = FALSE;
		break;
	case BGSCROLL_PIXMAP_KEY:
		cfg->scroll_background = 1;
		cfg->have_user_scroll_bg = 1;
		start_terminal_factory = FALSE;
		use_terminal_factory = FALSE;
		break;
	case BGNOSCROLL_PIXMAP_KEY:
		cfg->scroll_background = 0;
		cfg->have_user_scroll_bg = 1;
		start_terminal_factory = FALSE;
		use_terminal_factory = FALSE;
		break;
	case SHADED_KEY:
		cfg->shaded = 1;
		cfg->have_user_shaded = 1;
		start_terminal_factory = FALSE;
		use_terminal_factory = FALSE;
		break;
	case NOSHADED_KEY:
		cfg->shaded = 0;
		cfg->have_user_shaded = 1;
		start_terminal_factory = FALSE;
		use_terminal_factory = FALSE;
		break;
	case TRANSPARENT_KEY:
		cfg->background_pixmap = 0;
		cfg->transparent = 1;
		cfg->have_user_background = 1;
		start_terminal_factory = FALSE;
		use_terminal_factory = FALSE;
		break;
	case DOUTMP_KEY:
		cfg->update_records_and &= ~ZVT_TERM_DO_UTMP_LOG;
		cfg->update_records_xor |= ZVT_TERM_DO_UTMP_LOG;
		start_terminal_factory = FALSE;
		use_terminal_factory = FALSE;
		break;
	case DONOUTMP_KEY:
		cfg->update_records_and &= ~ZVT_TERM_DO_UTMP_LOG;
		cfg->update_records_xor &= ~ZVT_TERM_DO_UTMP_LOG;
		start_terminal_factory = FALSE;
		use_terminal_factory = FALSE;
		break;
	case DOWTMP_KEY:
		cfg->update_records_and &= ~ZVT_TERM_DO_WTMP_LOG;
		cfg->update_records_xor |= ZVT_TERM_DO_WTMP_LOG;
		start_terminal_factory = FALSE;
		use_terminal_factory = FALSE;
		break;
	case DONOWTMP_KEY:
		cfg->update_records_and &= ~ZVT_TERM_DO_WTMP_LOG;
		cfg->update_records_xor &= ~ZVT_TERM_DO_WTMP_LOG;
		start_terminal_factory = FALSE;
		use_terminal_factory = FALSE;
		break;
	case DOLASTLOG_KEY:
		cfg->update_records_and &= ~ZVT_TERM_DO_LASTLOG;
		cfg->update_records_xor |= ZVT_TERM_DO_LASTLOG;
		break;
	case DONOLASTLOG_KEY:
		cfg->update_records_and &= ~ZVT_TERM_DO_LASTLOG;
		cfg->update_records_xor &= ~ZVT_TERM_DO_LASTLOG;
		break;
	case TITLE_KEY:
	        cfg->window_title = g_strdup(arg);
		start_terminal_factory = FALSE;
		use_terminal_factory = FALSE;
                break;
	case ICON_KEY:
		cfg->window_icon = g_strdup(arg);
		start_terminal_factory = FALSE;
		use_terminal_factory = FALSE;
		break;
	case TERM_KEY:
		cfg->termname = g_strdup_printf("TERM=%s", arg);
		start_terminal_factory = FALSE;
		use_terminal_factory = FALSE;
		break;
	case START_FACTORY_KEY:
		start_terminal_factory = TRUE;
		break;
	case USE_FACTORY_KEY:
		use_terminal_factory = TRUE;
		break;
	default:
		break;
	}
}

static gint
session_die (gpointer client_data)
{
        bonobo_main_quit ();
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

	bindtextdomain (GETTEXT_PACKAGE, GNOMELOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	cmdline_login = FALSE;

	cmdline_config = g_new0 (struct terminal_config, 1);
	cmdline_config->update_records_and = ~0;
	
	cb_options[0].descrip = (char *)cmdline_config;

	/* pre-scan for -x and --execute options */
	for (i=1;i<argc;i++) {
		if (!strcmp (argv [i], "-x") || !(strcmp (argv [i], "--execute"))) {
			int last = i;
			i++;
			if (i == argc) {
				/* no arg!? let popt whinge about usage */
				break;
			}
			initial_command = malloc ((argc-i+1) * sizeof (char *));
			j = 0;
			while (i < argc) {
				initial_command [j] = argv [i];
				i++; 
				j++;
			}
			initial_command [j]=NULL;
			/* 'fool' popt into thinking we have less args */
			argc = last;
			use_terminal_factory = FALSE;
			break;
		}
	}

	if (!gnome_program_init ("Terminal", VERSION,
				 LIBGNOMEUI_MODULE,
                                 argc, argv,
				 GNOME_PARAM_POPT_TABLE, cb_options,
				 NULL))
		g_error (_("Cannot gnome_program_init ()"));

	env = environ;
	
	gnome_window_icon_set_default_from_file (GNOME_ICONDIR"/gnome-terminal.png");

	/* since -x gets stripped out of the commands, this
	   will make it override --use-factory */
	if (use_terminal_factory && initial_command)
		use_terminal_factory = FALSE;

	if(cmdline_config->user_back_str)
		gdk_color_parse (cmdline_config->user_back_str,
				 &cmdline_config->palette[17]);
	
	if(cmdline_config->user_fore_str)
		gdk_color_parse (cmdline_config->user_fore_str,
				 &cmdline_config->palette [16]);

	if (cmdline_config->class){
		class = g_strdup (cmdline_config->class);
	}
	else
	{
#if 0 
		/* program_invocation_short_name is broken on non-glibc machines at the moment */
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
		
	{
#ifdef HAVE_GNOME_CONFIG	
		char *prefix = g_strdup_printf ("Terminal/%s/", class);
		gnome_config_push_prefix (prefix);
#endif
		default_config = load_config (class);
#ifdef HAVE_GNOME_CONFIG
		gnome_config_pop_prefix();
		g_free (prefix);
#endif
		g_free (class);
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

	/* override the icon*/
	if (cmdline_config->window_icon) {
	  default_config->window_icon = cmdline_config->window_icon;
	}
	
	if (cmdline_config->user_back_str){
		default_config->palette[17] = cmdline_config->palette[17];
		default_config->color_set = COLORS_CUSTOM;
	}
	if (cmdline_config->user_fore_str){
		default_config->palette[16] = cmdline_config->palette[16];
		default_config->color_set = COLORS_CUSTOM;
	}
	/* disallow identical foreground and background colors */
	if(default_config->color_set==COLORS_CUSTOM &&
	   gdk_color_equal(&default_config->palette[16], 
			   &default_config->palette[17])) {
		default_config->color_set = 0;		
	}

	if (cmdline_config->have_user_background){
		default_config->background_pixmap = cmdline_config->background_pixmap;
		if(cmdline_config->background_pixmap) {
			default_config->pixmap_file = cmdline_config->pixmap_file;
		}
		default_config->transparent = cmdline_config->transparent;
	}

	if (cmdline_config->have_user_scroll_bg){
		default_config->scroll_background = cmdline_config->scroll_background;
	}

	if (cmdline_config->have_user_shaded){
		default_config->shaded = cmdline_config->shaded;
	}

	/* if the default is different from the commandline, use the commandline */
	default_config->invoke_as_login_shell =
		cmdline_login ? cmdline_config->invoke_as_login_shell : 
		default_config->login_by_default;
	default_config->termname = g_strdup(cmdline_config->termname);

	default_config->update_records &= cmdline_config->update_records_and;
	default_config->update_records ^= cmdline_config->update_records_xor;

	terminal_config_free (cmdline_config);

	load_factory_settings ();

#if 0
	if (start_terminal_factory) {
		corba_init_server (orb);
		if (!has_terminal_factory)
			corba_activate_server ();
	}
#endif

	if (!load_session ()) {
		gboolean has_term = FALSE;
#if 0
		if (use_terminal_factory) {
			CORBA_Environment ev;
			CORBA_Object term;

			CORBA_exception_init (&ev);
			term = create_terminal_via_factory
				(initial_global_geometry
				 ? initial_global_geometry : "80x24", &ev);
			if (ev._major != CORBA_NO_EXCEPTION)
				exit (5);
			if (term) {
				CORBA_Object_release (term, &ev);
				has_term = TRUE;
			}
			CORBA_exception_free (&ev);
		}
#endif
		if (!has_term) {
			new_terminal_cmd (initial_command, default_config,
					  initial_global_geometry,
					  terminal_id++);
		}
	}

	terminal_config_free (default_config);

	if (!terminals)
		return 0;

	client = gnome_master_client ();

	if (client) {
		gtk_signal_connect (GTK_OBJECT (client), "save_yourself",
				    GTK_SIGNAL_FUNC (save_session), NULL);
		gtk_signal_connect (GTK_OBJECT (client), "die",
				    GTK_SIGNAL_FUNC (session_die), NULL);
	}


	glade_init ();
	/* bugzilla.gnome.org 60538 */
	glade_require ("gnome");

	bonobo_main ();

	return 0;
}

extern char **environ;

static gint
print_nl (gpointer data)
{
	g_print ("\t\tsize\n");
	return TRUE;
}

int
main (int argc, char *argv [])
{
	signal (SIGHUP, SIG_IGN);

	if (getenv ("GRUNTIME_DEBUG"))
		gtk_timeout_add (1000, print_nl, NULL);

	return main_terminal_program (argc, argv, environ);
}

/*
 * Local variables:
 * c-basic-offset: 8
 * End:
 */
