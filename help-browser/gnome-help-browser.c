/* simple test program for the gnomehelpwin widget (from gtthelp)
 * michael fulbright
 */

#include <gnome.h>
#include "gnome-helpwin.h"

void quit_cb (GtkWidget *widget, void *data);

static void
xmhtml_activate(GtkWidget *w, XmHTMLAnchorCallbackStruct *cbs);


int
main(int argc, char *argv[]) {
    GtkWidget *help;
    GtkWidget *window;
    GtkWidget *button;
    
    gnome_init("gnome_help_browser", &argc, &argv);

    /* make the top level, container window */
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title (GTK_WINDOW (window), "Help Window");
    gtk_container_border_width (GTK_CONTAINER (window), 0);
    gtk_widget_set_usize(GTK_WIDGET(window), 400, 300);
    gtk_window_set_policy(GTK_WINDOW(window), 1, 1, 0);
    gtk_signal_connect(GTK_OBJECT(window), "destroy",
		       GTK_SIGNAL_FUNC(quit_cb), NULL);

    /* make the help window */
    help = gnome_helpwin_new();

    /* trap clicks on tags so we can stick requested link in browser */
    gtk_signal_connect(GTK_OBJECT(help), "activate",
		       GTK_SIGNAL_FUNC(xmhtml_activate), GTK_OBJECT(help));

    gtk_widget_show(help);
    gtk_container_add(GTK_CONTAINER(window), help);
    gtk_widget_show(window);

    /* load initial page into the browser */
    if (argc > 1)
	gnome_helpwin_goto(GNOME_HELPWIN(help), argv[1]);

    
    
    /* sit in main loop */
    gtk_main();

    return 0;
}


void
quit_cb (GtkWidget *widget, void *data)
{
  gtk_main_quit ();
  return;
}

static void
xmhtml_activate(GtkWidget *w, XmHTMLAnchorCallbackStruct *cbs)
{
    printf("In activate with ref = |%s|\n",cbs->href);
    fflush(stdout);
    gnome_helpwin_goto(GNOME_HELPWIN(w), cbs->href);
}

