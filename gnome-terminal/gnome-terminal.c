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
#include <pwd.h>
#include <gdk/gdkprivate.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

char **env;

#define DEFAULT_FONT "-misc-fixed-medium-r-normal--20-200-75-75-c-100-iso8859-1"
#define EXTRA 2

/* Font used by the terminals */
char *font = NULL;

/* Number of scrollbacklines */
int scrollback;

/* Initial geometry */
char *geometry = 0;

/* The color mode */
int color_type;

/* Scrollbar position */
enum {
	SCROLLBAR_LEFT, SCROLLBAR_RIGHT, SCROLLBAR_HIDDEN
} scrollbar_position;

/* How to invoke the shell */
int invoke_as_login_shell = 0;

/* Do we want blinking cursor? */
int blink;

/* A list of all the open terminals */
GList *terminals = 0;

typedef struct {
	GtkWidget *prop_win;
	GtkWidget *blink_checkbox;
	GtkWidget *font_entry;
	GtkWidget *color_scheme;
	GtkWidget *scrollbar;
} preferences_t;

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
close_terminal_cmd (void *unused, void *data)
{
	terminals = g_list_remove (terminals, data);
	gtk_widget_destroy (gtk_widget_get_toplevel (GTK_WIDGET (data)));
	if (terminals == NULL)
		gtk_main_quit ();
}

static void
close_all_cmd (void)
{
	while (terminals)
		close_terminal_cmd (0, terminals->data);
}

/*
 * Keep a copy of the current font name
 */
void
gnome_term_set_font (ZvtTerm *term, char *font)
{
	char *s;

	zvt_term_set_font_name  (term, font);
	s = gtk_object_get_user_data (GTK_OBJECT (term));
	if (s)
		g_free (s);
	gtk_object_set_user_data (GTK_OBJECT (term), g_strdup (font));
}

static GtkWidget *
aligned_label (char *str)
{
	GtkWidget *l;

	l = gtk_label_new (str);
	gtk_misc_set_alignment (GTK_MISC (l), 1.0, 0.5);
	return l;
}

gushort linux_red[] = {0x0000,0xaaaa,0x0000,0xaaaa,0x0000,0xaaaa,0x0000,0xaaaa,
			0x5555,0xffff,0x5555,0xffff,0x5555,0xffff,0x5555,0xffff};
gushort linux_grn[] = {0x0000,0x0000,0xaaaa,0x5555,0x0000,0x0000,0xaaaa,0xaaaa,
			0x5555,0x5555,0xffff,0xffff,0x5555,0x5555,0xffff,0xffff};
gushort linux_blu[] = {0x0000,0x0000,0x0000,0x0000,0xaaaa,0xaaaa,0xaaaa,0xaaaa,
			0x5555,0x5555,0x5555,0x5555,0xffff,0xffff,0xffff,0xffff};

gushort xterm_grn[] = {0x0000,0xaaaa,0x0000,0xaaaa,0x0000,0xaaaa,0x0000,0xaaaa,
		       0x5555,0xffff,0x5555,0xffff,0x5555,0xffff,0x5555,0xffff};
gushort xterm_blu[] = {0x0000,0x0000,0xaaaa,0x5555,0x0000,0x0000,0xaaaa,0xaaaa,
		       0x5555,0x5555,0xffff,0xffff,0x5555,0x5555,0xffff,0xffff};
gushort xterm_red[] = {0x0000,0x0000,0x0000,0x0000,0xaaaa,0xaaaa,0xaaaa,0xaaaa,
		       0x5555,0x5555,0x5555,0x5555,0xffff,0xffff,0xffff,0xffff};

gushort rxvt_red[] = {0x0000,0xaaaa,0x0000,0xaaaa,0x0000,0xaaaa,0x0000,0xaaaa,
		      0x5555,0xffff,0x5555,0xffff,0x5555,0xffff,0x5555,0xffff};
gushort rxvt_blu[] = {0x0000,0x0000,0xaaaa,0x5555,0x0000,0x0000,0xaaaa,0xaaaa,
		      0x5555,0x5555,0xffff,0xffff,0x5555,0x5555,0xffff,0xffff};
gushort rxvt_grn[] = {0x0000,0x0000,0x0000,0x0000,0xaaaa,0xaaaa,0xaaaa,0xaaaa,
		      0x5555,0x5555,0x5555,0x5555,0xffff,0xffff,0xffff,0xffff};

static void
set_color_scheme (ZvtTerm *term, int color_type)
{
	switch (color_type){
	case 0:
		zvt_term_set_color_scheme (term, linux_red, linux_grn, linux_blu);
		break;
	case 1:
		zvt_term_set_color_scheme (term, xterm_red, xterm_grn, xterm_blu);
		break;
	case 2:
		zvt_term_set_color_scheme (term, rxvt_red, rxvt_grn, rxvt_blu);
		break;
	}
}

static void
apply_changes (GtkWidget *widget, int page, ZvtTerm *term)
{
	int scrollpos;
	preferences_t *prefs = gtk_object_get_data (GTK_OBJECT (term), "prefs");
	GtkWidget *scrollbar = gtk_object_get_data (GTK_OBJECT (term), "scrollbar");
	GtkWidget *box       = scrollbar->parent;
	
	zvt_term_set_font_name (term, gtk_entry_get_text (GTK_ENTRY (prefs->font_entry)));
	zvt_term_set_blink (term, GTK_TOGGLE_BUTTON (prefs->blink_checkbox)->active);
	color_type = (int) gtk_object_get_user_data (GTK_OBJECT (prefs->color_scheme));
	scrollpos  = (int) gtk_object_get_user_data (GTK_OBJECT (prefs->scrollbar));

	/* sve the global variables */
	blink = GTK_TOGGLE_BUTTON (prefs->blink_checkbox)->active;
	if (font)
		g_free (font);
	font  = g_strdup (gtk_entry_get_text (GTK_ENTRY (prefs->font_entry)));
	scrollbar_position = scrollpos;

	set_color_scheme (term, color_type);
	if (scrollpos == SCROLLBAR_HIDDEN)
		gtk_widget_hide (scrollbar);
	else {
		gtk_box_reorder_child (GTK_BOX (box), scrollbar,
				       scrollpos == SCROLLBAR_LEFT ? 0 : 1);
		gtk_widget_show (scrollbar);
	}
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
	int              idx;
} lambda_t;

static void
set_active (GtkWidget *widget, lambda_t *t)
{
	gtk_object_set_user_data (GTK_OBJECT (t->menu), (void *) t->idx);
	gnome_property_box_changed (t->box);
}

static void
free_lambda (GtkWidget *w, void *l)
{
	g_free (l);
}
		    
static GtkWidget *
create_option_menu (GnomePropertyBox *box, char **menu_list, int item)
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
		t->idx  = i;
		t->menu = omenu;
		t->box  = box;
		entry = gtk_menu_item_new_with_label (_(*menu_list));
		gtk_signal_connect (GTK_OBJECT (entry), "activate",
				    GTK_SIGNAL_FUNC (set_active), t);
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

char *color_scheme [] = {
	N_("Linux console"),
	N_("Color Xterm"),
	N_("rxvt"),
	NULL
};

char *scrollbar_position_list [] = {
	N_("Left"),
	N_("Right"),
	N_("Hidden"),
	NULL
};
       
static void
preferences_cmd (GtkWidget *widget, ZvtTerm *term)
{
	GtkWidget *l, *table, *o, *m;
	preferences_t *prefs;

	/* Is a property window for this terminal already running? */
	prefs = gtk_object_get_data (GTK_OBJECT (term), "prefs");
	if (prefs)
		return;

	prefs = g_new0 (preferences_t, 1);
	prefs->prop_win = gnome_property_box_new ();
	gtk_object_set_data (GTK_OBJECT (term), "prefs", prefs);

	/* Look page */
	table = gtk_table_new (0, 0, 0);
	gnome_property_box_append_page (GNOME_PROPERTY_BOX (prefs->prop_win),
					table, gtk_label_new (_("Look")));
	l = aligned_label (_("Color scheme:"));
	gtk_table_attach (GTK_TABLE (table), l,
			  1, 2, 1, 2, GTK_FILL, 0, GNOME_PAD, GNOME_PAD);
	prefs->color_scheme = create_option_menu (GNOME_PROPERTY_BOX (prefs->prop_win),
						  color_scheme, 0);
	gtk_table_attach (GTK_TABLE (table), prefs->color_scheme,
			  2, 3, 1, 2, GTK_FILL, 0, GNOME_PAD, GNOME_PAD);
	
	/* Font */
	l = aligned_label (_("Font:"));
	gtk_table_attach (GTK_TABLE (table), l,
			  1, 2, 3, 4, GTK_FILL, 0, GNOME_PAD, GNOME_PAD);
	prefs->font_entry = gtk_entry_new ();
	gtk_entry_set_text (GTK_ENTRY (prefs->font_entry),
			    gtk_object_get_user_data (GTK_OBJECT (term)));
	gtk_entry_set_position (GTK_ENTRY (prefs->font_entry), 0);
	gtk_signal_connect (GTK_OBJECT (prefs->font_entry), "changed",
			    GTK_SIGNAL_FUNC (prop_changed), prefs);
	gtk_table_attach (GTK_TABLE (table), prefs->font_entry,
			  2, 3, 3, 4, GTK_FILL, 0, GNOME_PAD, GNOME_PAD);
	o = gtk_option_menu_new ();
	m = create_font_menu (term, GTK_SIGNAL_FUNC (prop_changed_zvt));
	gtk_option_menu_set_menu (GTK_OPTION_MENU (o), m);
	gtk_table_attach (GTK_TABLE (table), o,
			  3, 4, 3, 4, 0, 0, 0, 0);
	
	/* Scrollbar position */
	l = aligned_label (_("Scrollbar position"));
	gtk_table_attach (GTK_TABLE (table), l,
			  1, 2, 2, 3, GTK_FILL, 0, GNOME_PAD, GNOME_PAD);
	prefs->scrollbar = create_option_menu (GNOME_PROPERTY_BOX (prefs->prop_win),
					       scrollbar_position_list,
					       scrollbar_position);
	gtk_table_attach (GTK_TABLE (table), prefs->scrollbar,
			  2, 3, 2, 3, GTK_FILL, 0, GNOME_PAD, GNOME_PAD);

	/* Blinking status */
	prefs->blink_checkbox = gtk_check_button_new_with_label (_("Blinking cursor"));
	gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (prefs->blink_checkbox),
				     term->blink_enabled ? 1 : 0);
	gtk_signal_connect (GTK_OBJECT (prefs->blink_checkbox), "toggled",
			    GTK_SIGNAL_FUNC (prop_changed), prefs);
	gtk_table_attach (GTK_TABLE (table), prefs->blink_checkbox,
			  2, 3, 5, 6, GTK_FILL, 0, GNOME_PAD, GNOME_PAD);

	/* Connect the property box signals */
	gtk_signal_connect (GTK_OBJECT (prefs->prop_win), "apply",
			    GTK_SIGNAL_FUNC (apply_changes), term);
	gtk_signal_connect (GTK_OBJECT (prefs->prop_win), "delete_event",
			    GTK_SIGNAL_FUNC (window_closed_event), term);
	gtk_signal_connect (GTK_OBJECT (prefs->prop_win), "destroy",
			    GTK_SIGNAL_FUNC (window_closed), term);
	gtk_widget_show_all (prefs->prop_win);
}

static void
save_preferences (GtkWidget *widget, ZvtTerm *term)
{
	if (font)
		gnome_config_set_string ("/Terminal/Config/font", font);
	gnome_config_set_string ("/Terminal/Config/scrollpos",
				 scrollbar_position == SCROLLBAR_LEFT ? "left" :
				 scrollbar_position == SCROLLBAR_RIGHT ? "right" : "hidden");
	gnome_config_set_bool   ("/Terminal/Config/blinking", blink);
	gnome_config_set_int    ("/Terminal/Config/scrollbacklines", scrollback);
	gnome_config_set_string ("/Terminal/Config/color_scheme",
				 color_type == 0 ? "linux" : (color_type == 1 ? "xterm" : "rxvt"));
	gnome_config_sync ();
}

static GnomeUIInfo gnome_terminal_terminal_menu [] = {
	{ GNOME_APP_UI_ITEM, N_("New terminal"),    NULL, new_terminal },
	{ GNOME_APP_UI_ITEM, N_("Save preferences"),NULL, save_preferences },
	{ GNOME_APP_UI_ITEM, N_("Close terminal"),  NULL, close_terminal_cmd },
	{ GNOME_APP_UI_ITEM, N_("Properties..."),   NULL, preferences_cmd, 0, 0,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_PROP },
	{ GNOME_APP_UI_SEPARATOR },
	{ GNOME_APP_UI_ITEM, N_("Close all terminals"),  NULL, close_all_cmd },
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

/*
 * Puts in *shell a pointer to the full shell pathname
 * Puts in *name the invocation name for the shell
 */
static void
get_shell_name (char **shell, char **name)
{
	struct passwd *pw;
	char *only_name;
	int len;

	pw = getpwuid(getuid());
	if (pw) {
		*shell = pw->pw_shell;
		only_name = strrchr (pw->pw_shell, '/');
		
		if (invoke_as_login_shell){
			len = strlen (only_name);
		
			*name  = g_malloc (len + 2);
			**name = '-';
			strcpy ((*name)+1, only_name); 
		} else
			*name = only_name;
	} else {
		*shell = "/bin/bash";
		if (invoke_as_login_shell)
			*name  = "-bash";
		else
			*name  = "bash";
	}
}

void
terminal_kill (GtkWidget *widget, void *data)
{
	GnomeApp *app = GNOME_APP (gtk_widget_get_toplevel (GTK_WIDGET (data)));

	close_terminal_cmd (widget, app);
}

void
new_terminal (void)
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
	 */
	if (!env_copy){
		char **p;
		
		for (p = env; *p; p++)
			;
		i = p - env;
		env_copy = (char **) g_malloc (sizeof (char *) * (i + 1 + EXTRA));
		for (i = 0, p = env; *p; p++){
			if (strncmp (*p, "TERM", 4) == 0)
				env_copy [i++] = "TERM=xterm-color";
			else
				env_copy [i++] = *p;
		}
		env_copy [i++] = "COLORTERM=gnome-terminal";
		winid_pos = i++;
		env_copy [winid_pos] = "TEST";
		env_copy [i] = NULL;
	}

	app = gnome_app_new ("Terminal", "Terminal");
	gtk_window_set_wmclass (GTK_WINDOW (app), "GnomeTerminal", "GnomeTerminal");
	gtk_widget_realize (app);
	terminals = g_list_prepend (terminals, app);

	/* Setup the Zvt widget */
	term = ZVT_TERM (zvt_term_new ());
	gtk_widget_show (GTK_WIDGET (term));
	zvt_term_set_scrollback (term, scrollback);
	gnome_term_set_font (term, font);
	zvt_term_set_blink (term, blink);
	gtk_signal_connect (GTK_OBJECT (term), "child_died",
			    GTK_SIGNAL_FUNC (terminal_kill), term);
	
	gnome_app_create_menus_with_data (GNOME_APP (app), gnome_terminal_menu, term);
	
	/* Decorations */
	hbox = gtk_hbox_new (0, 0);
	gtk_widget_show (hbox);
	get_shell_name (&shell, &name);

	scrollbar = gtk_vscrollbar_new (GTK_ADJUSTMENT (term->adjustment));
	gtk_object_set_data (GTK_OBJECT (term), "scrollbar", scrollbar);
	GTK_WIDGET_UNSET_FLAGS (scrollbar, GTK_CAN_FOCUS);
	
	if (scrollbar_position == SCROLLBAR_LEFT)
		gtk_box_pack_start (GTK_BOX (hbox), scrollbar, 0, 1, 0);
	else
		gtk_box_pack_end (GTK_BOX (hbox), scrollbar, 0, 1, 0);
	if (scrollbar_position != SCROLLBAR_HIDDEN)
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
	gtk_widget_show (app);
	set_color_scheme (term, color_type);

	switch (zvt_term_forkpty (term)){
	case -1:
		perror ("Error: unable to fork");
		return;
		
	case 0: 
		sprintf (buffer, "WINDOWID=%d",(int) ((GdkWindowPrivate *)app->window)->xwindow);
		env_copy [winid_pos] = buffer;
		execle (shell, name, NULL, env_copy);
		perror ("Could not exec\n");
		_exit (127);
	}
}

static void
terminal_load_defaults (void)
{
	char *p;

	scrollback = gnome_config_get_int    ("/Terminal/Config/scrollbacklines=100");
	if (!font)
		font       = gnome_config_get_string ("/Terminal/Config/font=" DEFAULT_FONT);
	p = gnome_config_get_string ("/Terminal/Config/scrollpos=left");
	if (strcasecmp (p, "left") == 0)
		scrollbar_position = SCROLLBAR_LEFT;
	else if (strcasecmp (p, "right") == 0)
		scrollbar_position = SCROLLBAR_RIGHT;
	else
		scrollbar_position = SCROLLBAR_HIDDEN;
	p = gnome_config_get_string ("/Terminal/Config/color_scheme=linux");
	if (strcasecmp (p, "linux") == 0)
		color_type = 0;
	else if (strcasecmp (p, "xterm") == 0)
		color_type = 1;
	else
		color_type = 2;
	blink = gnome_config_get_bool ("/Terminal/Config/blinking=0");
}

/* Keys for the ARGP parser, should be negative */
enum {
	FONT_KEY     = -1,
	NOLOGIN_KEY  = -2,
	LOGIN_KEY    = -3,
	GEOMETRY_KEY = -4
};

static struct argp_option argp_options [] = {
	{ "font",     FONT_KEY,     N_("FONT"), 0, N_("Specifies font name"),                    0 },
	{ "nologin",  NOLOGIN_KEY,  NULL,       0, N_("Do not start up shells as login shells"), 0 },
	{ "login",    LOGIN_KEY,    NULL,       0, N_("Start up shells as login shells"), 0 },
	{ "geometry", GEOMETRY_KEY, N_("GEOMETRY"),0,N_("Specifies the geometry for the main window"), 0 },
	{ NULL, 0, NULL, 0, NULL, 0 },
};

static error_t
parse_an_arg (int key, char *arg, struct argp_state *state)
{
	switch (key){
	case FONT_KEY:
		font = arg;
		break;
	case LOGIN_KEY:
		invoke_as_login_shell = 1;
		break;
	case NOLOGIN_KEY:
	        invoke_as_login_shell = 0;
	        break;
	case GEOMETRY_KEY:
		geometry = arg;
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
	argp_program_version = VERSION;

	env = environ;
	
	bindtextdomain (PACKAGE, GNOMELOCALEDIR);
	textdomain (PACKAGE);
	
	gnome_init ("Terminal", &parser, argc, argv, 0, NULL);

	terminal_load_defaults ();

	new_terminal ();
	
	gtk_main ();
	return 0;
}
