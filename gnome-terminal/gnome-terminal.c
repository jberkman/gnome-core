/*
 * The GNOME terminal, using Michael Zucchi's zvt widget.
 */
#include <config.h>
#include <unistd.h>
#include <gnome.h>
#include <zvt/zvtterm.h>
#include <pwd.h>
#include <gdk/gdkprivate.h>
#include <string.h>

char **env;

#define DEFAULT_FONT "-misc-fixed-medium-r-normal--20-200-75-75-c-100-iso8859-1"
#define EXTRA 2

/* Font used by the terminals */
char *font;

/* Number of scrollbacklines */
int scrollback;

/* A list of all the open terminals */
GList *terminals = 0;

void new_terminal (char *fontname, int scrollback);

static void
new_terminal_cmd (GtkWidget *w)
{
	new_terminal (font, scrollback);
}

static void
about_terminal_cmd (GtkWidget *widget, void *data)
{
}

static void
close_terminal_cmd (GtkWidget *widget, void *data)
{
	GnomeApp *data;

	terminals = g_list_remove (terminals, data);
	gtk_widget_destroy (GTK_WIDGET (data));
}

static void
close_all_cmd (GtkWidget *widget)
{
	while (terminals)
		close_terminal_cmd (widget, terminals->data);
	gtk_main_quit ();
}

static GnomeUIInfo gnome_terminal_terminal_menu [] = {
	{ GNOME_APP_UI_ITEM, N_("New terminal"),  NULL, new_terminal_cmd },
	{ GNOME_APP_UI_ITEM, N_("Close terminal"),  NULL, close_terminal_cmd },
	{ GNOME_APP_UI_SEPARATOR },
	{ GNOME_APP_UI_ITEM, N_("Close all terminals"),  NULL, close_all_cmd },
	GNOMEUIINFO_END
};

static GnomeUIInfo gnome_terminal_options_menu [] = {
	{ GNOME_APP_UI_ITEM, N_("Hide menubar"),        NULL, about_terminal_cmd },
	{ GNOME_APP_UI_ITEM, N_("Secure keyboard"),     NULL, about_terminal_cmd },
	{ GNOME_APP_UI_ITEM, N_("Select font"),         NULL, about_terminal_cmd },
	{ GNOME_APP_UI_ITEM, N_("Color set"),           NULL, about_terminal_cmd },
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
	{ GNOME_APP_UI_SUBTREE, N_("Options"), NULL,  &gnome_terminal_options_menu },
	{ GNOME_APP_UI_SUBTREE, N_("Help"), NULL,     &gnome_terminal_about_menu },
	GNOMEUIINFO_END
};

void
new_terminal (char *fontname, int scrollbacklines)
{
	GtkWidget *app, *hbox, *scrollbar;
	ZvtTerm   *term;
	static char **env_copy;
	struct passwd *pw;
	static int winid_pos;
	char buffer [40];
	char *shell, *name;
	
	if (!env_copy){
		int i = 0;
		char **p;
		
		for (p = env; *p; p++)
			;
		i = env - p;
		env_copy = (char **) g_malloc (sizeof (char **) * (i + 1 + EXTRA));
		for (i = 0, p = env; *p; p++)
			env_copy [i++] = *p;
		env_copy [i++] = "COLORTERM=gnome-terminal";
		winid_pos = i++;
		env_copy [i] = NULL;
	}
	app = gnome_app_new ("GnomeTerminal", "Terminal");
	gnome_app_create_menus_with_data (GNOME_APP (app), gnome_terminal_menu, app);
	gtk_window_set_wmclass (GTK_WINDOW (app), "GnomeTerminal", "GnomeTerminal");
	gtk_widget_realize (app);
	terminals = g_list_prepend (terminals, app);
	
	hbox = gtk_hbox_new (0, 0);
	term = ZVT_TERM (zvt_term_new ());

	zvt_term_set_scrollback (term, scrollbacklines);
	zvt_term_set_font_name  (term, fontname);

	scrollbar = gtk_vscrollbar_new (GTK_ADJUSTMENT (term->adjustment));
	GTK_WIDGET_UNSET_FLAGS (scrollbar, GTK_CAN_FOCUS);
	gtk_box_pack_start (GTK_BOX (hbox), scrollbar, 0, 1, 0);
	gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (term), 1, 1, 0);
	gnome_app_set_contents (GNOME_APP (app), hbox);
	gtk_widget_show_all (app);

	switch (zvt_term_forkpty (term)){
	case -1:
		perror ("Error: unable to fork");
		return;
		
	case 0:
		pw = getpwuid(getpid());
		sprintf (buffer, "WINDOWID=%d",(int) ((GdkWindowPrivate *)app->window)->xwindow);
		env_copy [winid_pos] = buffer;
		if (pw) {
			shell = pw->pw_shell;
			name  = strrchr (pw->pw_shell, '/');
		} else {
			shell = "/bin/bash";
			name  = "bash";
		}
		execle (shell, name, NULL, env_copy);
		_exit (127);
	}
}

int
main (int argc, char *argv [], char **environ)
{
	argp_program_version = VERSION;

	env = environ;
	
	bindtextdomain (PACKAGE, GNOMELOCALEDIR);
	textdomain (PACKAGE);
	gnome_init ("Terminal", NULL, argc, argv, 0, NULL);

	scrollback = gnome_config_get_int    ("/Terminal/Config/scrollbacklines=100");
	font       = gnome_config_get_string ("/Terminal/Config/font=" DEFAULT_FONT);

	new_terminal (font, scrollback);
	
	gtk_main ();
	return 0;
}
