/*
 * The GNOME terminal, using Michael Zucchi's zvt widget.
 * (C) 1998 The Free Software Foundation
 *
 * Authors: Miguel de Icaza (GNOME terminal)
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

extern char **environ;		/* The program environment */

char **env;

#define DEFAULT_FONT "-misc-fixed-medium-r-normal--20-200-75-75-c-100-iso8859-1"
#define EXTRA 2

/* Name of section to discard if --discard given.  */
static char *discard_section = NULL;

/* Initial geometry */
char *geometry = 0;


/* The color set */
enum color_set_enum { COLORS_WHITE_ON_BLACK,
	COLORS_BLACK_ON_WHITE,
	COLORS_GREEN_ON_BLACK,
	COLORS_BLACK_ON_LIGHT_YELLOW,
	COLORS_CUSTOM
};

enum scrollbar_position_enum { SCROLLBAR_LEFT = 0, SCROLLBAR_RIGHT = 1, SCROLLBAR_HIDDEN = 2 };

struct terminal_config {
	int color_type; 			/* The color mode */
	enum color_set_enum color_set;
	char *font; 				/* Font used by the terminals */
	int scrollback; 			/* Number of scrollbacklines */
	char * terminal_class;
	enum scrollbar_position_enum scrollbar_position;
	int invoke_as_login_shell; 		/* How to invoke the shell */
	int blink; 				/* Do we want blinking cursor? */
	GdkColor user_fore, user_back; 		/* The custom colors */
	int menubar_hidden; 			/* Whether to show the menubar */
} ;

struct terminal_config cfg;

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
	GtkWidget *menubar_checkbox;
	GtkWidget *font_entry;
	GtkWidget *color_scheme;
	GtkWidget *def_fore_back;
	GtkWidget *scrollbar;
	GtkWidget *scrollback_spin;
	GtkWidget *class_box;
	GnomeColorSelector *fore_cs;
	GnomeColorSelector *back_cs;
	int changed;
} preferences_t;

static char * user_fore_color, * user_back_color;	/* as specified on command line */
static char * user_font;

void new_terminal (void);

static void
about_terminal_cmd (void)
{
        GtkWidget *about;

        gchar *authors[] = {
		"Zvt terminal widget: Michael Zucchi (zucchi@zedzone.box.net.au)",
		"GNOME terminal: Miguel de Icaza (miguel@kernel.org)",
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
close_app (GtkWidget *app) {
        terminals = g_list_remove(terminals, app);

	if (app == initial_term)
		initial_term = NULL;

	gtk_widget_destroy(app);

	if(terminals == NULL)
	       gtk_main_quit();
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
void
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
set_color_scheme (ZvtTerm *term, int color_type)
{
	switch (color_type){
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
	switch (cfg.color_set){
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
		red   [16] = cfg.user_fore.red;
		green [16] = cfg.user_fore.green;
		blue  [16] = cfg.user_fore.blue;
		red   [17] = cfg.user_back.red;
		green [17] = cfg.user_back.green;
		blue  [17] = cfg.user_back.blue;
		break;
	}
	zvt_term_set_color_scheme (term, red, green, blue);
	gtk_widget_queue_draw (GTK_WIDGET (term));
}

static void
load_config (struct terminal_config * cfg, char * class)
{
	char *p;
	char * fore_color = NULL;
	char * back_color = NULL;
 	char * prefix = alloca(strlen(class) + 20);

	/* FIXME: memory leak */
	memset(cfg, 0, sizeof(*cfg));

	/* It's very odd that these are here */
	cfg->font = NULL;
	cfg->invoke_as_login_shell = 0;
	cfg->menubar_hidden = 0;
	cfg->terminal_class = g_strdup(class);

	sprintf(prefix, "/Terminal/%s/", class);
	gnome_config_push_prefix(prefix);

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
	cfg->blink = gnome_config_get_bool ("blinking-0");

	/* Default colors in the case the color set is the custom one */
	fore_color = gnome_config_get_string ("foreground=gray");
	back_color = gnome_config_get_string ("background=black");
	cfg->color_set = gnome_config_get_int ("color_set=0");

	cfg->menubar_hidden = !gnome_config_get_bool ("menubar=true");
	
	if (strcasecmp (fore_color, back_color) == 0)
		/* don't let them set identical foreground and background colors */
		cfg->color_set = 0;
	else {
		if (!gdk_color_parse (fore_color, &cfg->user_fore) || !gdk_color_parse (back_color, &cfg->user_back)){
			/* or illegal colors */
			cfg->color_set = 0;
		}
	}

	gnome_config_pop_prefix();
}

static struct terminal_config * 
gather_changes (ZvtTerm *term) {
	preferences_t *prefs = gtk_object_get_data (GTK_OBJECT (term), "prefs");
	int r, g, b;
	struct terminal_config * newcfg = g_malloc(sizeof(*newcfg));

	memset(newcfg, 0, sizeof(*newcfg));

	newcfg->blink = GTK_TOGGLE_BUTTON (prefs->blink_checkbox)->active;
	newcfg->menubar_hidden = GTK_TOGGLE_BUTTON (prefs->menubar_checkbox)->active;
	newcfg->scrollback = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (prefs->scrollback_spin)); 
	(int) newcfg->scrollbar_position = gtk_object_get_user_data (GTK_OBJECT (prefs->scrollbar));

	if (newcfg->font)
		g_free (newcfg->font);
	newcfg->font  = g_strdup (gtk_entry_get_text (GTK_ENTRY (prefs->font_entry)));

	free(newcfg->terminal_class);
	if (!strcmp(gtk_entry_get_text(GTK_ENTRY (GTK_COMBO(prefs->class_box)->entry)),
			_("Default")))
		newcfg->terminal_class = g_strdup(_("Config"));
	else
		newcfg->terminal_class = g_strconcat("Class-", gtk_entry_get_text(GTK_ENTRY (GTK_COMBO(prefs->class_box)->entry)), NULL);

	newcfg->color_type = (int) gtk_object_get_user_data (GTK_OBJECT (prefs->color_scheme));
	newcfg->color_set  = (int) gtk_object_get_user_data (GTK_OBJECT (prefs->def_fore_back));

	gnome_color_selector_get_color_int (prefs->fore_cs, &r, &g, &b, 65535);
	newcfg->user_fore.red   = r;
	newcfg->user_fore.green = g;
	newcfg->user_fore.blue  = b;
	gnome_color_selector_get_color_int (prefs->back_cs, &r, &g, &b, 65535);
	newcfg->user_back.red   = r;
	newcfg->user_back.green = g;
	newcfg->user_back.blue  = b;

	return newcfg;
}

/* prototype */
static void switch_terminal_class (ZvtTerm *term, struct terminal_config * newcfg);

static int
apply_changes (ZvtTerm *term, struct terminal_config * newcfg)
{
	GtkWidget *scrollbar = gtk_object_get_data (GTK_OBJECT (term), "scrollbar");
	GtkWidget *box       = scrollbar->parent;
	GnomeApp *app = GNOME_APP (gtk_widget_get_toplevel (GTK_WIDGET (term)));

	if (strcmp(cfg.terminal_class, newcfg->terminal_class)) {
		switch_terminal_class(term, newcfg);
		return 1;
	}

	free(cfg.font);
	free(cfg.terminal_class);
	cfg = *newcfg;

	zvt_term_set_font_name (term, newcfg->font);
	zvt_term_set_blink (term, newcfg->blink);
	zvt_term_set_scrollback (term, cfg.scrollback);
	if (cfg.scrollbar_position == SCROLLBAR_HIDDEN)
		gtk_widget_hide (scrollbar);
	else {
		gtk_box_set_child_packing (GTK_BOX (box), scrollbar,
					   FALSE, TRUE, 0,
					   (cfg.scrollbar_position == SCROLLBAR_LEFT) ?
					       GTK_PACK_START : GTK_PACK_END);
		gtk_widget_show (scrollbar);
	}

	set_color_scheme (term, cfg.color_type);

	if (cfg.menubar_hidden)
		gtk_widget_hide (app->menubar);
	else
		gtk_widget_show (app->menubar);

	return 0;
}

static void
apply_changes_cmd (GtkWidget *widget, int page, ZvtTerm *term) {
	struct terminal_config * newcfg;

	if (page != -1) return;

	newcfg = gather_changes (term);
	if (!apply_changes(term, newcfg))
		free(newcfg);
}

static void
switch_terminal_cb (GnomeMessageBox * mbox, gint button, void * term)
{
	struct terminal_config * newcfg, loaded_cfg;

	newcfg = gtk_object_get_data (GTK_OBJECT (term), "newcfg");

	if (button == 0) {
		/* yes */
		load_config(&loaded_cfg, newcfg->terminal_class);
		free(cfg.terminal_class);
		cfg.terminal_class = g_strdup(loaded_cfg.terminal_class);
		apply_changes(term, &loaded_cfg);
	} else if (button == 1) {
		/* no */
		free(cfg.terminal_class);
		cfg.terminal_class = g_strdup(newcfg->terminal_class);
		apply_changes(term, newcfg);
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
	g_free (newcfg);
	gtk_object_set_data (GTK_OBJECT (term), "newcfg", NULL);
}

static void
switch_terminal_class (ZvtTerm *term, struct terminal_config * newcfg) 
{
	GtkWidget * mbox;
	preferences_t *prefs = gtk_object_get_data (GTK_OBJECT (term), "prefs");

	gtk_object_set_data(GTK_OBJECT(term), "newcfg", newcfg);

	if (!prefs->changed) {
	    /* just do it! */
	    switch_terminal_cb (NULL, 0, term);
	    free(newcfg);
	    gtk_object_set_data (GTK_OBJECT (term), "newcfg", NULL);
	    return;
	}

	mbox = gnome_message_box_new(_("You have switched the class of this window. Do you\n "
				    "want to reconfigure this window to match the default\n"
				    "configuration of the new class?"),
				 GNOME_MESSAGE_BOX_QUESTION,
                                 GNOME_STOCK_BUTTON_YES,
                                 GNOME_STOCK_BUTTON_NO, GNOME_STOCK_BUTTON_CANCEL, NULL);

	gtk_signal_connect(GTK_OBJECT(mbox), "clicked", GTK_SIGNAL_FUNC(switch_terminal_cb),
			   term);
	gtk_signal_connect(GTK_OBJECT(mbox), "destroy", GTK_SIGNAL_FUNC(switch_terminal_closed),
			   term);

	gtk_widget_show(mbox);

}

static void
window_closed (GtkWidget *w, void *data)
{
	ZvtTerm *term = ZVT_TERM (data);
	preferences_t *prefs;

	prefs = gtk_object_get_data (GTK_OBJECT (term), "prefs");
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
	if (w != GTK_WIDGET (GTK_COMBO(prefs->class_box)->entry)) 
		prefs->changed = 1;
	gnome_property_box_changed (GNOME_PROPERTY_BOX (prefs->prop_win));
}

static void
prop_changed_zvt (void *data, char *font_name)
{
	ZvtTerm *term = ZVT_TERM (data);
	preferences_t *prefs;
	
	prefs = gtk_object_get_data (GTK_OBJECT (term), "prefs");
	gtk_entry_set_text (GTK_ENTRY (prefs->font_entry), font_name);
	gtk_entry_set_position (GTK_ENTRY (prefs->font_entry), 0);
}

typedef struct {
	GtkWidget        *menu;
	GnomePropertyBox *box;
	preferences_t	 *prefs;
	int              idx;
	void             *data1, *data2;
} lambda_t;

static void
set_active_data(GtkWidget * omenu, int idx, GtkWidget * sens1, GtkWidget * sens2)
{
	gtk_object_set_user_data (GTK_OBJECT (omenu), (void *) idx);

	if (sens1) gtk_widget_set_sensitive (GTK_WIDGET (sens1), idx == COLORS_CUSTOM);
	if (sens2) gtk_widget_set_sensitive (GTK_WIDGET (sens2), idx == COLORS_CUSTOM);
}


static void
set_active (GtkWidget *widget, lambda_t *t)
{
	set_active_data(t->menu, t->idx, t->data1, t->data2);
	t->prefs->changed = 1;
	gnome_property_box_changed (GNOME_PROPERTY_BOX (t->box));
}

static void
free_lambda (GtkWidget *w, void *l)
{
	g_free (l);
}
		    
static GtkWidget *
create_option_menu_data (GnomePropertyBox *box, preferences_t * prefs, char **menu_list, int item, GtkSignalFunc func, void *data1, void *data2)
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
create_option_menu (GnomePropertyBox *box, preferences_t * prefs, char **menu_list, int item, GtkSignalFunc func)
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
	COLORPAL_ROW  = 1,
	FOREBACK_ROW  = 2,
	FORECOLOR_ROW = 3,
	BACKCOLOR_ROW = 4,
	CLASS_ROW    = 1,
	SCROLL_ROW    = 2,
	SCROLLBACK_ROW = 3,
	FONT_ROW      = 4,
	BLINK_ROW     = 5,
	MENUBAR_ROW    = 6
};

/* called back to free the ColorSelector */
static void
free_cs (GtkWidget *widget, GnomeColorSelector *cs)
{
	gnome_color_selector_destroy (cs);
}
	 
static void
preferences_cmd (GtkWidget *widget, ZvtTerm *term)
{
	GtkWidget *l, *table, *o, *m, *b1, *b2;
	preferences_t *prefs;
	GtkAdjustment *adj;
	GList * class_list = NULL;
	void * iter;
	char * some_class;

	/* Is a property window for this terminal already running? */
	if (gtk_object_get_data (GTK_OBJECT (term), "prefs") ||
	    gtk_object_get_data (GTK_OBJECT (term), "newcfg"))
		return;

	prefs = g_new0 (preferences_t, 1);
	prefs->changed = 0;

	prefs->prop_win = gnome_property_box_new ();
	gtk_object_set_data (GTK_OBJECT (term), "prefs", prefs);

	/* general page */
	table = gtk_table_new (3, 5, FALSE);
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
			    GTK_SIGNAL_FUNC (prop_changed), prefs);
	gtk_table_attach (GTK_TABLE (table), prefs->font_entry,
			  2, 3, FONT_ROW, FONT_ROW+1, GTK_FILL, 0, GNOME_PAD, GNOME_PAD);
	o = gtk_option_menu_new ();
	m = create_font_menu (term, GTK_SIGNAL_FUNC (prop_changed_zvt));
	gtk_option_menu_set_menu (GTK_OPTION_MENU (o), m);
	gtk_table_attach (GTK_TABLE (table), o,
			  3, 5, FONT_ROW, FONT_ROW+1, 0, 0, 0, 0);

	/* Terminal class */
	l = aligned_label (_("Terminal Class"));
	gtk_table_attach (GTK_TABLE (table), l,
			  1, 2, CLASS_ROW, CLASS_ROW+1, GTK_FILL, 0, GNOME_PAD, GNOME_PAD);
	prefs->class_box = gtk_combo_new ();
	iter = gnome_config_init_iterator_sections("/Terminal");
	while (gnome_config_iterator_next(iter, &some_class, NULL)) {
		if (!strcmp(some_class, "Config") || !strncmp(some_class, "Class-", 6)) {
		    if (!strcmp(some_class, "Config")) 
			    some_class = _("Default");
		    else
			    some_class += 6;

		    class_list = g_list_append(class_list, some_class);
		}
	}
	gtk_combo_set_popdown_strings (GTK_COMBO (prefs->class_box), class_list);
	if (!strcmp(cfg.terminal_class, "Config")) 
		some_class = _("Default");
	else
		some_class = cfg.terminal_class + 6;
	gtk_entry_set_text (GTK_ENTRY (GTK_COMBO(prefs->class_box)->entry), some_class);
	gtk_signal_connect (GTK_OBJECT (GTK_COMBO(prefs->class_box)->entry), "changed",
			    GTK_SIGNAL_FUNC (prop_changed), prefs);
	gtk_table_attach (GTK_TABLE (table), prefs->class_box,
			  2, 3, CLASS_ROW, CLASS_ROW+1, GTK_FILL, 0, GNOME_PAD, GNOME_PAD);
	
	/* Scrollbar position */
	l = aligned_label (_("Scrollbar position"));
	gtk_table_attach (GTK_TABLE (table), l,
			  1, 2, SCROLL_ROW, SCROLL_ROW+1, GTK_FILL, 0, GNOME_PAD, GNOME_PAD);
	prefs->scrollbar = create_option_menu (GNOME_PROPERTY_BOX (prefs->prop_win), prefs,
					       scrollbar_position_list,
					       cfg.scrollbar_position, GTK_SIGNAL_FUNC (set_active));
	gtk_table_attach (GTK_TABLE (table), prefs->scrollbar,
			  2, 3, SCROLL_ROW, SCROLL_ROW+1, GTK_FILL, 0, GNOME_PAD, GNOME_PAD);

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
				     cfg.menubar_hidden ? 1 : 0);
	gtk_signal_connect (GTK_OBJECT (prefs->menubar_checkbox), "toggled",
			    GTK_SIGNAL_FUNC (prop_changed), prefs);
	gtk_table_attach (GTK_TABLE (table), prefs->menubar_checkbox,
			  2, 3, MENUBAR_ROW, MENUBAR_ROW+1, GTK_FILL, 0, GNOME_PAD, GNOME_PAD);

	/* Scroll back */
	l = aligned_label (_("Scrollback lines"));
        gtk_table_attach (GTK_TABLE (table), l, 1, 2, SCROLLBACK_ROW, SCROLLBACK_ROW+1, GTK_FILL, 0, GNOME_PAD, GNOME_PAD);
	adj = (GtkAdjustment *) gtk_adjustment_new ((gfloat)cfg.scrollback, 1.0, 1000.0, 1.0, 5.0, 0.0);
	prefs->scrollback_spin = gtk_spin_button_new (adj, 0, 0);
	gtk_signal_connect (GTK_OBJECT (prefs->scrollback_spin), "changed",
			    GTK_SIGNAL_FUNC (prop_changed), prefs);
	gtk_table_attach (GTK_TABLE (table), prefs->scrollback_spin,
			  2, 3, SCROLLBACK_ROW, SCROLLBACK_ROW+1, GTK_FILL, 0, GNOME_PAD, GNOME_PAD);
	/* Color page */
	table = gtk_table_new (4, 4, FALSE);
	gnome_property_box_append_page (GNOME_PROPERTY_BOX (prefs->prop_win), table, gtk_label_new (_("Colors")));
	
	/* Color palette */
	l = aligned_label (_("Color palette:"));
	gtk_table_attach (GTK_TABLE (table), l,
			  1, 2, COLORPAL_ROW, COLORPAL_ROW+1, GTK_FILL, 0, GNOME_PAD, GNOME_PAD);
	prefs->color_scheme = create_option_menu (GNOME_PROPERTY_BOX (prefs->prop_win), prefs,
						  color_scheme, cfg.color_type, GTK_SIGNAL_FUNC (set_active));
	gtk_table_attach (GTK_TABLE (table), prefs->color_scheme,
			  2, 6, COLORPAL_ROW, COLORPAL_ROW+1, GTK_FILL, 0, GNOME_PAD, GNOME_PAD);

	/* Foreground, background buttons */
	l = aligned_label (_("Foreground color:"));
	gtk_table_attach (GTK_TABLE (table), l,
			  1, 2, FORECOLOR_ROW, FORECOLOR_ROW+1, GTK_FILL, 0, GNOME_PAD, GNOME_PAD);
	prefs->fore_cs = gnome_color_selector_new ((SetColorFunc) (prop_changed), prefs);
	gtk_table_attach (GTK_TABLE (table), b1 = gnome_color_selector_get_button (prefs->fore_cs),
			  2, 3, FORECOLOR_ROW, FORECOLOR_ROW+1, 0, 0, GNOME_PAD, GNOME_PAD);
	gtk_signal_connect (GTK_OBJECT (b1), "destroy", GTK_SIGNAL_FUNC (free_cs), prefs->fore_cs);
	gnome_color_selector_set_color_int (prefs->fore_cs, cfg.user_fore.red, cfg.user_fore.green, cfg.user_fore.blue, 65355);
	
	l = aligned_label (_("Background color:"));
	gtk_table_attach (GTK_TABLE (table), l,
			  1, 2, BACKCOLOR_ROW, BACKCOLOR_ROW+1, GTK_FILL, 0, GNOME_PAD, GNOME_PAD);
	prefs->back_cs = gnome_color_selector_new ((SetColorFunc) (prop_changed), prefs);
	gtk_table_attach (GTK_TABLE (table), b2 = gnome_color_selector_get_button (prefs->back_cs),
			  2, 3, BACKCOLOR_ROW, BACKCOLOR_ROW+1, 0, 0, GNOME_PAD, GNOME_PAD);
	gtk_signal_connect (GTK_OBJECT (b2), "destroy", GTK_SIGNAL_FUNC (free_cs), prefs->back_cs);
	gnome_color_selector_set_color_int (prefs->back_cs, cfg.user_back.red, cfg.user_back.green, cfg.user_back.blue, 65355);

	/* default foreground/backgorund selector */
	l = aligned_label (_("Colors:"));
	gtk_table_attach (GTK_TABLE (table), l,
			  1, 2, FOREBACK_ROW, FOREBACK_ROW+1, GTK_FILL, 0, GNOME_PAD, GNOME_PAD);
	prefs->def_fore_back = create_option_menu_data (GNOME_PROPERTY_BOX (prefs->prop_win), prefs,
							fore_back_table, cfg.color_set, GTK_SIGNAL_FUNC (set_active),
							b1, b2);
	set_active_data(prefs->def_fore_back, cfg.color_set, b2, b2);
	gtk_table_attach (GTK_TABLE (table), prefs->def_fore_back,
			  2, 6, FOREBACK_ROW, FOREBACK_ROW+1, GTK_FILL, 0, GNOME_PAD, GNOME_PAD);

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
			    "clicked", GTK_SIGNAL_FUNC(color_ok), c);
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
save_preferences (GtkWidget *widget, ZvtTerm *term, struct terminal_config * cfg)
{
 	char * prefix = alloca(strlen(cfg->terminal_class) + 20);

	sprintf(prefix, "/Terminal/%s/", cfg->terminal_class);
	gnome_config_push_prefix(prefix);

	if (cfg->font)
		gnome_config_set_string ("font", cfg->font);
	gnome_config_set_string ("scrollpos",
				 cfg->scrollbar_position == SCROLLBAR_LEFT ? "left" :
				 cfg->scrollbar_position == SCROLLBAR_RIGHT ? "right" : "hidden");
	gnome_config_set_bool   ("blinking", cfg->blink);
	gnome_config_set_int    ("scrollbacklines", cfg->scrollback);
	gnome_config_set_int    ("color_set", cfg->color_set);
	gnome_config_set_string ("color_scheme",
		     cfg->color_type == 0 ? "linux" : (cfg->color_type == 1 ? "xterm" : "rxvt"));
	gnome_config_set_string ("foreground", get_color_string (cfg->user_fore));
	gnome_config_set_string ("background", get_color_string (cfg->user_back));
	gnome_config_set_bool   ("menubar", !cfg->menubar_hidden);
	gnome_config_sync ();

	gnome_config_pop_prefix();
}

static void
save_preferences_cmd (GtkWidget *widget, ZvtTerm *term) {
	save_preferences(widget, term, &cfg);
}

static void
show_menu_cmd (GtkWidget *widget, ZvtTerm *term)
{
	GnomeApp *app = GNOME_APP (gtk_widget_get_toplevel (GTK_WIDGET (term)));

	cfg.menubar_hidden = 0;
	gtk_widget_show (app->menubar);
}

static void
hide_menu_cmd (GtkWidget *widget, ZvtTerm *term)
{
	GnomeApp *app = GNOME_APP (gtk_widget_get_toplevel (GTK_WIDGET (term)));

	cfg.menubar_hidden = 1;
	gtk_widget_hide (app->menubar);
}

static GnomeUIInfo gnome_terminal_terminal_menu [] = {
	{ GNOME_APP_UI_ITEM, N_("New terminal"),    NULL, new_terminal },
	{ GNOME_APP_UI_ITEM, N_("Save preferences"),NULL, save_preferences_cmd },
	{ GNOME_APP_UI_ITEM, N_("Close terminal"),  NULL, close_terminal_cmd },
	{ GNOME_APP_UI_SEPARATOR },
	{ GNOME_APP_UI_ITEM, N_("Hide menubar"), NULL, hide_menu_cmd },
	{ GNOME_APP_UI_ITEM, N_("Properties..."),   NULL, preferences_cmd, 0, 0,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_PROP },
	{ GNOME_APP_UI_ITEM, N_("Color selector..."),   NULL, color_cmd },
	{ GNOME_APP_UI_SEPARATOR },
	GNOMEUIINFO_END
};

static GnomeUIInfo gnome_terminal_about_menu [] = {
	{ GNOME_APP_UI_ITEM, N_("About..."), NULL, about_terminal_cmd, NULL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_ABOUT },
	GNOMEUIINFO_SEPARATOR,
	GNOMEUIINFO_HELP ("Terminal"),
	GNOMEUIINFO_END
};

static GnomeUIInfo gnome_terminal_menu [] = {
	{ GNOME_APP_UI_SUBTREE, N_("Terminal"), NULL, &gnome_terminal_terminal_menu },
	{ GNOME_APP_UI_SUBTREE, N_("Help"), NULL,     &gnome_terminal_about_menu },
	GNOMEUIINFO_END
};

static void
set_shell_to (char *the_shell, char **shell, char **name)
{
	char *only_name;
	int len;

	only_name = strrchr (the_shell, '/');
	only_name++;
	
	if (cfg.invoke_as_login_shell){
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
get_shell_name (char **shell, char **name)
{
	*shell = gnome_util_user_shell ();
	set_shell_to (*shell, shell, name);
}

void
terminal_kill (GtkWidget *widget, void *data)
{
	GnomeApp *app = GNOME_APP (gtk_widget_get_toplevel (GTK_WIDGET (data)));

	close_terminal_cmd (widget, app);
}

static void
drop_data_available (void *widget, GdkEventDropDataAvailable *event, gpointer data)
{
	ZvtTerm *term = ZVT_TERM (widget);
	char *p = event->data;
	int count = event->data_numbytes;
	int len, col, row, the_char;

	if (strcmp (event->data_type, "text/plain") == 0 ||
	    strcmp (event->data_type, "url:ALL") == 0){
		do {
			len = 1 + strlen (p);
			count -= len;
			
			vt_writechild (&term->vx->vt, p, len - 1);
			vt_writechild (&term->vx->vt, " ", 1);
			p += len;
		} while (count > 0);

		return;
	}

	/* Color dropped */
	if (strcmp (event->data_type, "application/x-color") == 0){
		gdouble *data = event->data;
		int x, y;
		
		/* Get drop site, and make the coordinates local to our window */
		gdk_window_get_origin (GTK_WIDGET (term)->window, &x, &y);
		x = event->coords.x - x;
		y = event->coords.y - y;

		col = x / term->charwidth;
		row = y / term->charheight;

		/* Switch to custom colors and */
		cfg.color_set = COLORS_CUSTOM;
		
		the_char = vt_get_attr_at (term->vx, col, row) & 0xff;
		if (the_char == ' ' || the_char == 0){
			/* copy the current foreground color */
			cfg.user_fore.red   = red [16];
			cfg.user_fore.green = green [16];
			cfg.user_fore.blue  = blue [16];

			/* Accept the dropped colors */
			cfg.user_back.red   = data [1] * 65535;
			cfg.user_back.green = data [2] * 65535;
			cfg.user_back.blue  = data [3] * 65535;
		} else {
			/* copy the current background color */
			cfg.user_back.red   = red [17];
			cfg.user_back.green = green [17];
			cfg.user_back.blue  = blue [17];
			
			/* Accept the dropped colors */
			cfg.user_fore.red   = data [1] * 65535;
			cfg.user_fore.green = data [2] * 65535;
			cfg.user_fore.blue  = data [3] * 65535;
		}
		set_color_scheme (term, cfg.color_type);
	}
}

static void
configure_term_dnd (ZvtTerm *term)
{
	char *drop_types []= { "url:ALL", "text/plain", "application/x-color" };
	
	gtk_widget_dnd_drop_set (GTK_WIDGET (term), TRUE, drop_types, 3, FALSE);
	gtk_signal_connect (GTK_OBJECT (term), "drop_data_available_event",
			    GTK_SIGNAL_FUNC (drop_data_available), term);
}

static int
button_press (GtkWidget *widget, GdkEventButton *event, ZvtTerm *term)
{
	GtkWidget * menu;
	GtkWidget * menu_item;
	GnomeUIInfo * item;

	/* FIXME: this should popup a menu instead */
	if (event->state & GDK_CONTROL_MASK){
		menu = gtk_menu_new (); 

		for (item = gnome_terminal_terminal_menu; item->type != GNOME_APP_UI_ENDOFINFO;
		     item++) {
			if (item->type != GNOME_APP_UI_ITEM) continue;

			if (!strcmp(item->label, "Hide menubar") && cfg.menubar_hidden) {
				menu_item = gtk_menu_item_new_with_label("Show menubar");
				gtk_signal_connect (GTK_OBJECT (menu_item), "activate", 
						    show_menu_cmd, term);
			} else {
				menu_item = gtk_menu_item_new_with_label(item->label);
				gtk_signal_connect (GTK_OBJECT (menu_item), "activate", 
						    item->moreinfo, term);
			}

			gtk_menu_append(GTK_MENU(menu), menu_item);
			gtk_widget_show(menu_item);
		}

		gtk_menu_popup (GTK_MENU (menu), NULL, NULL, 0, NULL, 3, event->time);

		return 1;
	}

	return 0;
}

void
new_terminal_cmd (char **cmd)
{
	GtkWidget *app, *hbox, *scrollbar;
	ZvtTerm   *term;
	static char **env_copy;
	static int winid_pos;
	char buffer [40];
	char *shell, *name;
	int i = 0;

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
				 || (strncmp (*p, "LINES=", 6) == 0)) {
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
	terminals = g_list_prepend (terminals, app);

	/* Setup the Zvt widget */
	term = ZVT_TERM (zvt_term_new ());
	gtk_widget_show (GTK_WIDGET (term));
#if ZVT_USES_MINIMAL_ALLOC
	gtk_widget_set_usize (GTK_WIDGET (term),
			      80 * term->charwidth,
			      25 * term->charheight);
#endif
	zvt_term_set_scrollback (term, cfg.scrollback);
	gnome_term_set_font (term, cfg.font);
	zvt_term_set_blink (term, cfg.blink);
	gtk_signal_connect (GTK_OBJECT (term), "child_died",
			    GTK_SIGNAL_FUNC (terminal_kill), term);

	gtk_signal_connect(GTK_OBJECT(app), "delete_event",
			   GTK_SIGNAL_FUNC(close_app), term);

	gtk_signal_connect (GTK_OBJECT (term), "button_press_event",
			    GTK_SIGNAL_FUNC (button_press), term);
	
	gnome_app_create_menus_with_data (GNOME_APP (app), gnome_terminal_menu, term);
	if (cfg.menubar_hidden)
		gtk_widget_hide (GNOME_APP (app)->menubar);
	
	/* Decorations */
	hbox = gtk_hbox_new (0, 0);
	gtk_widget_show (hbox);
	get_shell_name (&shell, &name);

	scrollbar = gtk_vscrollbar_new (GTK_ADJUSTMENT (term->adjustment));
	gtk_object_set_data (GTK_OBJECT (term), "scrollbar", scrollbar);
	GTK_WIDGET_UNSET_FLAGS (scrollbar, GTK_CAN_FOCUS);
	
	if (cfg.scrollbar_position == SCROLLBAR_LEFT)
		gtk_box_pack_start (GTK_BOX (hbox), scrollbar, 0, 1, 0);
	else
		gtk_box_pack_end (GTK_BOX (hbox), scrollbar, 0, 1, 0);
	if (cfg.scrollbar_position != SCROLLBAR_HIDDEN)
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
	gtk_widget_show (app);
	set_color_scheme (term, cfg.color_type);

	switch (zvt_term_forkpty (term)){
	case -1:
		perror ("Error: unable to fork");
		return;
		
	case 0: {
		sprintf (buffer, "WINDOWID=%d",(int) ((GdkWindowPrivate *)app->window)->xwindow);
		env_copy [winid_pos] = buffer;
		if (cmd) {
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

void
new_terminal ()
{
	new_terminal_cmd (NULL);
}

static gboolean
load_session (GnomeClient *client)
{
	int num_terms, i;
	gboolean def;

	gnome_config_push_prefix (gnome_client_get_config_prefix (client));

	num_terms = gnome_config_get_int_with_default ("dummy/num_terms", &def);
	if (def || ! num_terms)
		return FALSE;

	for (i = 0; i < num_terms; ++i) {
		char buffer[50], *geom, **argv;
		int argc;

		sprintf (buffer, "%d/geometry", i);
		geom = gnome_config_get_string (buffer);

		sprintf (buffer, "%d/command", i);
		gnome_config_get_vector (buffer, &argc, &argv);

		geometry = geom;
		new_terminal_cmd (argv);

		g_free (geom);
		gnome_string_array_free (argv);
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
	if (gnome_client_get_id (client)) {
		char *section = gnome_client_get_config_prefix (client);
		char *args[11];
		int argc = 0, d1, d2, i;
		GList *list;

		gnome_config_clean_file (section);
		gnome_config_push_prefix (section);

		i = 0;
		for (list = terminals; list != NULL; list = list->next) {
			/* FIXME: we really ought to be able to set
			   the font and other information on a
			   per-terminal basis.  But that means a lot
			   of rewriting of this whole file, which I
			   don't feel like doing right now.  Global
			   variables are evil.  */
			char buffer[50], *geom;
			GtkWidget *top = gtk_widget_get_toplevel (GTK_WIDGET (list->data));

			sprintf (buffer, "%d/geometry", i);
			geom = gnome_geometry_string (top->window);
			gnome_config_set_string (buffer, geom);
			g_free (geom);

			if (top == initial_term) {
				int n;
				for (n = 0; initial_command[n]; ++n)
					;
				sprintf (buffer, "%d/command", i);
				gnome_config_set_vector (buffer, n,
							 initial_command);
			}

			++i;
		}
		gnome_config_set_int ("dummy/num_terms", i);

		gnome_config_pop_prefix ();
		gnome_config_sync ();

		args[argc++] = (char *) client_data;
		args[argc++] = "--font";
		args[argc++] = cfg.font;
		args[argc++] = cfg.invoke_as_login_shell ? "--login" : "--nologin";
		args[argc++] = "--foreground";
		args[d1 = argc++] = g_strdup (get_color_string (cfg.user_fore));
		args[argc++] = "--background";
		args[d2 = argc++] = g_strdup (get_color_string (cfg.user_back));

		args[argc] = NULL;

		gnome_client_set_restart_command (client, argc, args);

		g_free (args[d1]);
		g_free (args[d2]);

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
	COMMAND_KEY  = -5,
	FORE_KEY     = -6,
	BACK_KEY     = -7,
	DISCARD_KEY  = -8,
	CLASS_KEY    = -9
};

static struct argp_option argp_options [] = {
	{ "class",      CLASS_KEY,    N_("CLASS"), 0, N_("Tmerinal class name"), 0 },
	{ "font",       FONT_KEY,     N_("FONT"), 0, N_("Specifies font name"), 0 },
	{ "nologin",    NOLOGIN_KEY,  NULL,       0, N_("Do not start up shells as login shells"), 0 },
	{ "login",      LOGIN_KEY,    NULL,       0, N_("Start up shells as login shells"), 0 },
	{ "geometry",   GEOMETRY_KEY, N_("GEOMETRY"),0,N_("Specifies the geometry for the main window"), 0 },
	{ "command",    COMMAND_KEY,  N_("COMMAND"),0,N_("Execute this program instead of a shell"), 0 },
	{ "foreground", FORE_KEY,     N_("COLOR"),0, N_("Foreground color"), 0 },
	{ "background", BACK_KEY,     N_("COLOR"),0, N_("Background color"), 0 },
	{ "discard",    DISCARD_KEY,  N_("ID"),   OPTION_HIDDEN, NULL, 0 },
	{ NULL, 0, NULL, 0, NULL, 0 },
};

static error_t
parse_an_arg (int key, char *arg, struct argp_state *state)
{
	switch (key){
	case CLASS_KEY:
		free(cfg.terminal_class);
		cfg.terminal_class = g_strconcat("Class-", arg, NULL);
		break;
	case FONT_KEY:
		user_font = arg;
		break;
	case LOGIN_KEY:
		cfg.invoke_as_login_shell = 1;
		break;
	case NOLOGIN_KEY:
	        cfg.invoke_as_login_shell = 0;
	        break;
	case GEOMETRY_KEY:
		geometry = arg;
		break;
	case COMMAND_KEY:
		initial_command = &state->argv[state->next - 1];
		state->next = state->argc;
		break;
	case FORE_KEY:
		user_fore_color = arg;
		cfg.color_set  = COLORS_CUSTOM;
		break;
	case BACK_KEY:
		user_back_color = arg;
		cfg.color_set  = COLORS_CUSTOM;
		break;
	case DISCARD_KEY:
		gnome_client_disable_master_connection ();
		discard_section = arg;
		break;
	default:
		return ARGP_ERR_UNKNOWN;
	}
	return 0;
}

static struct argp parser =
{
	argp_options, parse_an_arg, NULL, NULL, NULL, NULL, NULL
};

int
main (int argc, char *argv [], char **environ)
{
	GnomeClient *client, *clone;
	char * program_name;
	char * class;

	argp_program_version = VERSION;

	env = environ;
	
	bindtextdomain (PACKAGE, GNOMELOCALEDIR);
	textdomain (PACKAGE);

	program_name = strrchr(argv[0], '/');
	if (!program_name) 
		program_name = argv[0];
	else
		program_name++;

	if (getenv("GNOME_TERMINAL_CLASS"))
		class = g_strconcat("Class-", getenv("GNOME_TERMINAL_CLASS"), NULL);
	else if (strcmp(program_name, "gnome-terminal"))
		class = g_strconcat("Class-", program_name, NULL);
	else
		class = g_strdup("Config");
	
	gnome_init ("Terminal", &parser, argc, argv, 0, NULL);

	if (discard_section) {
		gnome_config_clean_file (discard_section);
		return 0;
	}

	client = gnome_master_client ();
	gtk_signal_connect (GTK_OBJECT (client), "save_yourself",
			    GTK_SIGNAL_FUNC (save_session), argv[0]);
	gtk_signal_connect (GTK_OBJECT (client), "die",
			    GTK_SIGNAL_FUNC (die), NULL);

	load_config (&cfg, class);
	g_free(class);

	/* now to override the defaults */
	if (user_font)
	    free(cfg.font), cfg.font = g_strdup(user_font);
	if (user_fore_color || user_back_color) {
		int failed = 0;
		
		if (user_fore_color && !gdk_color_parse (user_fore_color, &cfg.user_fore))
			failed = 1;

		if (user_back_color && !gdk_color_parse (user_back_color, &cfg.user_back))
			failed = 1;

		if (failed) 
			cfg.color_set = 0;
		else
			cfg.color_set = COLORS_CUSTOM;
	}

	clone = gnome_cloned_client ();
	if (! clone || ! load_session (clone))
		new_terminal_cmd (initial_command);

	gtk_main ();
	return 0;
}
