/*
 * The GNOME terminal, using Michael Zucchi's zvt widget.
 * (C) 1998 Miguel de Icaza, Michael Zucchi.
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

/* The property window  and the font window */
GtkWidget *prop_win = 0, *font_button;

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

static GtkWidget *
aligned_label (char *str)
{
	GtkWidget *l;

	l = gtk_label_new (str);
	gtk_misc_set_alignment (GTK_MISC (l), 1.0, 0.5);
	return l;
}

static void
apply_changes (GtkWidget *widget, int page, ZvtTerm *term)
{
	zvt_term_set_font_name (term, GTK_LABEL (GTK_BUTTON (font_button)->child)->label);
}

static int
window_closed (void)
{
	prop_win = 0;
	return FALSE;
}

static void
choose_font (void)
{
	char *font;
	GtkWidget *l;
	
	font = gnome_font_select ();
	if (strcmp (font, GTK_LABEL(GTK_BUTTON (font_button)->child)->label) == 0)
		return;
	
	gtk_container_remove (GTK_CONTAINER (font_button), GTK_BUTTON (font_button)->child);
	l = gtk_label_new (font);
	gtk_widget_show (l);
	gtk_container_add (GTK_CONTAINER (font_button), l);
	gnome_property_box_changed (GNOME_PROPERTY_BOX (prop_win));
}

static void
preferences_cmd (GtkWidget *widget, ZvtTerm *term)
{
	GtkWidget *l, *e, *table, *c;
	
	if (prop_win)
		return;

	prop_win = gnome_property_box_new ();

	/* Look page */
	table = gtk_table_new (0, 0, 0);
	gnome_property_box_append_page (GNOME_PROPERTY_BOX (prop_win),
					table, gtk_label_new (_("Look")));
	l = aligned_label (_("Color scheme:"));
	gtk_table_attach (GTK_TABLE (table), l,
			  1, 2, 1, 2, GTK_FILL, 0, GNOME_PAD, GNOME_PAD);
	l = aligned_label (_("Font:"));
	gtk_table_attach (GTK_TABLE (table), l,
			  1, 2, 2, 3, GTK_FILL, 0, GNOME_PAD, GNOME_PAD);
	font_button = gtk_button_new_with_label (font);
	gtk_signal_connect (GTK_OBJECT (font_button), "clicked", GTK_SIGNAL_FUNC(choose_font), NULL);
	gtk_table_attach (GTK_TABLE (table), font_button,
			  2, 3, 2, 3, GTK_FILL, 0, GNOME_PAD, GNOME_PAD);
	l = aligned_label (_("Scrollbar position"));
	gtk_table_attach (GTK_TABLE (table), l,
			  1, 2, 3, 4, GTK_FILL, 0, GNOME_PAD, GNOME_PAD);

	c = gtk_check_button_new_with_label (_("Blinking cursor"));
	gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (c),
				     blink ? 1 : 0);
	gtk_table_attach (GTK_TABLE (table), c,
			  2, 3, 5, 6, GTK_FILL, 0, GNOME_PAD, GNOME_PAD);

	/* Connect the property box signals */
	gtk_signal_connect (GTK_OBJECT (prop_win), "apply",
			    GTK_SIGNAL_FUNC (apply_changes), term);
	gtk_signal_connect (GTK_OBJECT (prop_win), "delete_event",
			    GTK_SIGNAL_FUNC (window_closed), term);
	gtk_signal_connect (GTK_OBJECT (prop_win), "destroy",
			    GTK_SIGNAL_FUNC (window_closed), term);
	gtk_widget_show_all (prop_win);
}

static GnomeUIInfo gnome_terminal_terminal_menu [] = {
	{ GNOME_APP_UI_ITEM, N_("New terminal"),    NULL, new_terminal },
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

	/* Setup the environment for the gnome-terminals:
	 *
	 * TERM is set to xterm-color (which is what zvt emulates)
	 * COLORTERM is set for slang-based applications to auto-detect color
	 * WINDOWID spot is reserved for the xterm compatible variable.
	 */
	if (!env_copy){
		int i = 0;
		char **p;
		
		for (p = env; *p; p++)
			;
		i = env - p;
		env_copy = (char **) g_malloc (sizeof (char *) * (i + 1 + EXTRA));
		for (i = 0, p = env; *p; p++){
			if (strncmp (*p, "TERM", 4) == 0)
				env_copy [i++] = "TERM=xterm-color";
			else
				env_copy [i++] = *p;
		}
		env_copy [i++] = "COLORTERM=gnome-terminal";
		winid_pos = i++;
		env_copy [i] = NULL;
	}

	app = gnome_app_new ("GnomeTerminal", "Terminal");
	gtk_window_set_wmclass (GTK_WINDOW (app), "GnomeTerminal", "GnomeTerminal");
	gtk_widget_realize (app);
	terminals = g_list_prepend (terminals, app);

	/* Setup the Zvt widget */
	term = ZVT_TERM (zvt_term_new ());
	zvt_term_set_scrollback (term, scrollback);
	zvt_term_set_font_name  (term, font);
	zvt_term_set_blink (term, blink);
	gtk_signal_connect (GTK_OBJECT (term), "child_died",
			    GTK_SIGNAL_FUNC (terminal_kill), term);
	
	gnome_app_create_menus_with_data (GNOME_APP (app), gnome_terminal_menu, term);
	
	/* Decorations */
	hbox = gtk_hbox_new (0, 0);
	get_shell_name (&shell, &name);

	if (scrollbar_position != SCROLLBAR_HIDDEN){
		scrollbar = gtk_vscrollbar_new (GTK_ADJUSTMENT (term->adjustment));
		GTK_WIDGET_UNSET_FLAGS (scrollbar, GTK_CAN_FOCUS);

		if (scrollbar_position == SCROLLBAR_LEFT)
			gtk_box_pack_start (GTK_BOX (hbox), scrollbar, 0, 1, 0);
		else
			gtk_box_pack_end (GTK_BOX (hbox), scrollbar, 0, 1, 0);
	}			
	gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (term), 1, 1, 0);
	gnome_app_set_contents (GNOME_APP (app), hbox);
	gtk_widget_show_all (app);

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
	blink = gnome_config_get_bool ("/Terminal/Config/blinking=0");
}

/* Keys for the ARGP parser, should be negative */
enum {
	FONT_KEY    = -1,
	NOLOGIN_KEY = -2,
	LOGIN_KEY   = -3
};

static struct argp_option argp_options [] = {
	{ "font",    FONT_KEY,    N_("FONT"), 0, N_("Specifies font name"),                    0 },
	{ "nologin", NOLOGIN_KEY, NULL,       0, N_("Do not start up shells as login shells"), 0 },
	{ "login",   LOGIN_KEY,   NULL,       0, N_("Start up shells as login shells"), 0 },
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
