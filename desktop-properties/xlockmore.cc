/*
 * Screen Saver property configurator
 * Written by Radek Doulik, 1997 <doulik@karlin.mff.cuni.cz>
 */
#pragma implementation "xlockmore.h"

#include <gdk/gdkprivate.h>
#include "xlockmore.h"
#include "gnome-desktop.h"
#include <stdarg.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>

#ifdef HAVE_LIBINTL
#include <libintl.h>
#define _(String) gettext(String)
#else
#define _(String) (String)
#endif

#include <sys/wait.h>


extern FILE *yyin, *yyout;
extern int yylex ();

XLockMore* XLockMore::xlockmore;

XLockMore::XLockMore ()
     : ScreenSaver ("xlockmore", "xlock more screensaver")
{
	args = NULL;
	argn = 0;

	widget = NULL;
	pPID = sPID = -1;

	mapSignal = unmapSignal = destroySignal = -1;

	addArg ("-resources");

	gint pid = forkAndExec ();

	xlockmore = this;
	yyout = fdopen (open ("/dev/null", O_WRONLY), "w");
	yyin = fdopen (pfd [0], "r");
	yylex ();
	kill (&pid);

	setupWin = NULL;
}

XLockMore::~XLockMore ()
{
	if (args)
		g_list_free (args);
}

void
XLockMore::resetArg ()
{
	if (args)
		g_list_free (args);   /* ???? */
	args = NULL;
	argn = 0;
}

void
XLockMore::addArg (char *s)
{
	args = g_list_append (args, g_strdup (s));
	argn++;
}

int
XLockMore::kill (gint *pid, gint sig, gint w)
{
	if (*pid>=0) {
		gint status;
		
		::kill (*pid, sig);
		if (w) {
			::waitpid (*pid, &status, 0);
			*pid = -1;
		}
	}
}

int
XLockMore::forkAndExec () {
	pipe (pfd);
	gint pid = fork ();
	
	if (pid) {
		close (pfd [1]);
		
		return pid;
	}
	
	char **argv = new (char *) [argn+2];
	GList *cur = args;
	int i = 1;

	argv [0] = "xlock";
	do {
		if (cur->data) {
			argv [i] = (char *)cur->data;
			// printf (" %s\n", argv [i]);
			i++;
		}
		cur = cur->next;
	} while (cur);

	argv [i] = NULL;

	close (1);
	close (0);
	close (pfd [0]);

	dup2 (pfd [1], 1);

	execvp ("xlock", argv);

	exit (1);
}

static gint
deleteSetupWin (GtkWidget *w, GdkEvent *ev, XLockMore *xm)
{
  xm->kill (&xm->sPID);
  gtk_widget_destroy (xm->setupWin);
  xm->setupWin = NULL;
  xm->mapSignal = xm->unmapSignal = -1;

  return TRUE;
}

static gint
destroySetupWin (GtkWidget *w, XLockMore *xm)
{
	deleteSetupWin (w, NULL, xm);
}

extern "C" GtkWidget *main_window;

void
XLockMore::prepareSetupWindow ()
{
	if (!setupWin) {
		setupWin = gtk_window_new (GTK_WINDOW_TOPLEVEL);
		gtk_window_set_title (GTK_WINDOW (setupWin), _("XLockMore Properties"));
		gtk_window_set_policy (GTK_WINDOW (setupWin), FALSE, FALSE, TRUE);
		gtk_signal_connect (GTK_OBJECT (setupWin), "delete_event",
				    GTK_SIGNAL_FUNC (deleteSetupWin), this);
		gtk_signal_connect (GTK_OBJECT (main_window), "delete_event",
				    GTK_SIGNAL_FUNC (deleteSetupWin), this);
		gtk_signal_connect (GTK_OBJECT (main_window), "destroy",
				    GTK_SIGNAL_FUNC (destroySetupWin), this);

		setupNotebook = gtk_notebook_new ();

		// mode setup page
		modePage = gtk_hbox_new (FALSE, 10);
		GtkWidget *vb2 = gtk_vbox_new (FALSE, GNOME_PAD),
			*vb3 = gtk_vbox_new (FALSE, 0),
			*vbox = gtk_vbox_new (FALSE, 0),
			*hbox = gtk_hbox_new (FALSE, GNOME_PAD);
		setupOptions = gtk_vbox_new (FALSE, GNOME_PAD);
		GtkWidget *bok = gtk_button_new_with_label (_("  OK  ")),
			*bapl = gtk_button_new_with_label (_(" Apply ")),
			*bcl = gtk_button_new_with_label (_(" Cancel ")),
			*bdf = gtk_button_new_with_label (_(" Defaults ")),
		        *bhelp = gtk_button_new_with_label (_("Help"));

		gtk_signal_connect (GTK_OBJECT (bcl), "clicked",
				    GTK_SIGNAL_FUNC (deleteSetupWin), this);

		GtkWidget *l1 = gtk_label_new (_(" Mode ")),
			*l2 = gtk_label_new (_(" XLockMore "));
		setupName = gtk_label_new ("name");
		// setupComment = gtk_label_new ("comment");
		// setupComment = gtk_text_new (NULL, NULL);
		gtk_misc_set_alignment (GTK_MISC (setupName), 0, 0.5);
		// gtk_misc_set_alignment (GTK_MISC (setupComment), 0, 0.5);
		setupFrame = gtk_frame_new (NULL);
		gtk_frame_set_shadow_type (GTK_FRAME (setupFrame), GTK_SHADOW_IN);
		gtk_widget_set_usize (setupFrame, 200, 150);
		GtkWidget *f1 = gtk_frame_new (_(" Options ")),
			*f2 = gtk_frame_new (_(" Preview ")),
			*f3 = gtk_frame_new (_(" Mode ")),
			*fb = gtk_frame_new (NULL);
		gtk_frame_set_shadow_type (GTK_FRAME (fb), GTK_SHADOW_OUT);
		gtk_widget_set_usize (f1, 220, -1);
		GtkWidget *sw = gtk_scrolled_window_new (NULL, NULL);
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
						GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		gtk_viewport_set_shadow_type (GTK_VIEWPORT (GTK_SCROLLED_WINDOW (sw)->viewport),
					      GTK_SHADOW_NONE);

		// xlockmore setup page
		GtkWidget *hb2 = gtk_hbox_new (FALSE, 10);

		gtk_container_add (GTK_CONTAINER (sw), setupOptions);
		gtk_container_add (GTK_CONTAINER (f1), sw);
		gtk_container_add (GTK_CONTAINER (f2), setupFrame);
		gtk_container_add (GTK_CONTAINER (f3), vb3);
		gtk_box_pack_start (GTK_BOX (vb3), setupName, FALSE, FALSE, 0);
		// gtk_box_pack_start (GTK_BOX (vb3), setupComment, FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (vb2), f2, FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (vb2), f3, FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (modePage), f1, FALSE, TRUE, 0);
		gtk_box_pack_end (GTK_BOX (modePage), vb2, FALSE, FALSE, 0);

		gtk_notebook_append_page (GTK_NOTEBOOK (setupNotebook), modePage, l1);
		gtk_notebook_append_page (GTK_NOTEBOOK (setupNotebook), hb2, l2);

		gtk_container_border_width (GTK_CONTAINER (setupFrame), GNOME_PAD);
		gtk_container_border_width (GTK_CONTAINER (sw), GNOME_PAD);
		gtk_container_border_width (GTK_CONTAINER (vb3), GNOME_PAD);
		gtk_container_border_width (GTK_CONTAINER (modePage), GNOME_PAD);
		gtk_box_pack_start (GTK_BOX (vbox), setupNotebook, FALSE, FALSE, 0);

		gtk_container_border_width (GTK_CONTAINER (hbox), GNOME_PAD);
		gtk_box_pack_end (GTK_BOX (hbox), bhelp, FALSE, FALSE, 0);
		gtk_box_pack_end (GTK_BOX (hbox), bcl, FALSE, FALSE, 0);
		gtk_box_pack_end (GTK_BOX (hbox), bapl, FALSE, FALSE, 0);
		gtk_box_pack_end (GTK_BOX (hbox), bok, FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (hbox), bdf, FALSE, FALSE, 0);
		gtk_container_add (GTK_CONTAINER (fb), hbox);
		gtk_box_pack_start (GTK_BOX (vbox), fb, TRUE, FALSE, 0);
		gtk_container_add (GTK_CONTAINER (setupWin), vbox);

		gtk_widget_show_all (setupWin);
	}
}


///////////////////////////////////////////////////////////////////
//
//  XlockMode
//

XLockMode::XLockMode (XLockMore *ss, gchar *n, gchar *c) 
     : ScreenSaverMode (ss, n, c)
{
	pars = NULL;
	parn = 0;
}


XLockMode::~XLockMode () 
{
	g_list_free (pars);
}

void
XLockMode::addPar (XLockModePar *par)
{
	pars = g_list_append (pars, par);
	parn++;
}

static void
cmdline_add (gchar *s, GString *gs)
{
	// printf ("%s\n", s);
	g_string_append (gs, " ");
	g_string_append (gs, s);
}

void
XLockMode::run (gint type, ...)
{
	va_list ap;
	
	va_start (ap, type);

	// fprintf (stderr, "Hola mundo!\n");
	XLockMore *xm = (XLockMore *)parent;
	xm->resetArg ();

	GList *cur = pars;
	while (cur) {
		((XLockModePar *)cur->data)->setExecArgs (xm);
		cur = cur->next;
	}
	
	if (type == SS_PREVIEW || type == SS_TEST || type == SS_CMDLINE) {
		gchar nice [32];
		sprintf (nice, "%d", va_arg (ap, gint));
		xm->addArg ("-nice");
		xm->addArg (nice);
	}

	if (type == SS_TEST || type == SS_CMDLINE)
		if (!va_arg (ap, gint))
			xm->addArg ("-nolock");

	if (type == SS_PREVIEW || type == SS_SETUP) {

		GtkWidget *widget = va_arg (ap, GtkWidget *);
		gint px;
		gint py;
		gint pw;
		gint ph;
		gchar geo [128];
		gchar wid [32];

		if (type == SS_PREVIEW) {
			px = va_arg (ap, gint);
			py = va_arg (ap, gint);
			pw = va_arg (ap, gint);
			ph = va_arg (ap, gint);
		} else {
#define AL widget->allocation
			px = py = 12;
			pw = AL.width-24;
			ph = AL.height-24;
		}

		if (type != SS_PREVIEW)
			sprintf (geo, "%dx%d+%d+%d", pw, ph, AL.x+px, AL.y+py);
		else
			sprintf (geo, "%dx%d+%d+%d", pw, ph, px, py);
		sprintf (wid, "%d", ((GdkWindowPrivate *)widget->window)->xwindow);
#undef AL

		xm->addArg ("-inwindow");

		xm->addArg ("-geometry");
		xm->addArg (geo);

		xm->addArg ("-display");
		xm->addArg (DisplayString
			    (((GdkWindowPrivate *)widget->window)->xdisplay));

		xm->addArg ("-parent");
		xm->addArg (wid);
	}

	xm->addArg ("-mode");
	xm->addArg (name);

	switch (type) {
	case SS_CMDLINE:
		{
			gchar **cmdLine = va_arg (ap, gchar **);
			GString *cl = g_string_new (NULL);

			g_string_append (cl, "xlock");
			g_list_foreach (xm->args, (GFunc) cmdline_add, cl);

			*cmdLine = g_strdup (cl->str);
			g_string_free (cl, TRUE);
		}		
		break;
	case SS_PREVIEW:
		xm->kill (&xm->pPID);
		xm->pPID = xm->forkAndExec ();
		break;
	case SS_SETUP:
		xm->kill (&xm->sPID);
		xm->sPID = xm->forkAndExec ();
		break;
	case SS_TEST:
		xm->kill (&xm->pPID, SIGSTOP, 0);
		xm->kill (&xm->sPID, SIGSTOP, 0);
		gint pid = xm->forkAndExec ();
		gint status;
		::waitpid (pid, &status, 0);
		xm->kill (&xm->pPID, SIGCONT, 0);
		xm->kill (&xm->sPID, SIGCONT, 0);
		break;
	}

	va_end (ap);
}

void
XLockMode::stop (gint type, ...)
{
	XLockMore *xm = (XLockMore *)parent;
	
	switch (type) {
	case SS_PREVIEW:
		xm->kill (&xm->pPID);
	}
}

static void
remove_widget (GtkWidget *w, GtkWidget *c)
{
	gtk_container_remove (GTK_CONTAINER (c), w);
	// gtk_widget_destroy (w);
}

static void
runSetupXLock (GtkWidget *w, XLockMode *m)
{
	// printf ("run\n");
	m->run (SS_SETUP, w);
}

static void
killSetupXLock (GtkWidget *, XLockMode *m)
{
	// printf ("kill\n");
	((XLockMore *)m->parent)->kill (&((XLockMore *)m->parent)->sPID);
}

static gint
runFirstSetup (XLockMode *m)
{
	m->run (SS_SETUP, ((XLockMore *)m->parent)->setupFrame);

	return FALSE;
}

void
XLockMode::setup ()
{
	XLockMore *xm = (XLockMore *)parent;

	if (xm->mapSignal>=0)
		gtk_signal_disconnect (GTK_OBJECT (xm->setupFrame), xm->mapSignal);
	if (xm->unmapSignal>=0)
		gtk_signal_disconnect (GTK_OBJECT (xm->setupFrame), xm->unmapSignal);

	xm->kill (&xm->sPID);
	xm->prepareSetupWindow ();
	gtk_notebook_set_page (GTK_NOTEBOOK (xm->setupNotebook), 0);

	gtk_label_set (GTK_LABEL (xm->setupName), name);
	// gtk_label_set (GTK_LABEL (xm->setupComment), comment);
	//gtk_text_insert (GTK_TEXT (xm->setupComment), NULL,
	//	      &xm->setupComment->style->white, NULL, comment, -1);

	/* gtk_widget_set_events (xm->setupFrame, GDK_ALL_EVENTS_MASK);
	   gtk_signal_connect (GTK_OBJECT (xm->setupFrame),
	   "expose_event",
	   (GtkSignalFunc) runSetupXlock,
	   (gpointer)this); */
	
	//gtk_widget_hide (xm->setupOptions);
	//gtk_container_disable_resize (GTK_CONTAINER (xm->setupOptions));
	
	// remove old parameters
	gtk_container_foreach (GTK_CONTAINER (xm->setupOptions),
			       (GtkCallback) remove_widget,
			       xm->setupOptions);

	// insert new ones
	GList *cur = pars;
	do {
		if (cur->data) {
	       
			XLockModePar *par = (XLockModePar *)cur->data;

			GtkWidget *w;
			GtkWidget *l;
			GtkWidget *e;

			switch (par->xtype) {
			case XLMP_STRING_ARG:
				w = gtk_hbox_new (FALSE, 10);
				l = gtk_label_new (par->option);
				e = gtk_entry_new ();
				gtk_widget_set_usize (e, 80, -1);
				gtk_entry_set_text (GTK_ENTRY (e), par->getVal ());

				gtk_box_pack_start (GTK_BOX (w), l, FALSE, FALSE, 0);
				gtk_box_pack_end (GTK_BOX (w), e, FALSE, FALSE, 0);

				gtk_widget_show (e);
				gtk_widget_show (l);
				break;
			case XLMP_BOOL_ARG:
				w = gtk_check_button_new_with_label (par->option);
				gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (w),
							     par->getBoolVal ());

				break;
			}

			gtk_box_pack_start (GTK_BOX (xm->setupOptions), w, FALSE, FALSE, 0);
			gtk_widget_show (w);
		}
		cur = cur->next;
	} while (cur);

	//gtk_container_enable_resize (GTK_CONTAINER (xm->setupOptions));
	//gtk_widget_show (xm->setupOptions);

	xm->mapSignal = gtk_signal_connect (GTK_OBJECT (xm->setupFrame), "map",
					    (GtkSignalFunc) runSetupXLock, this);
	xm->unmapSignal = gtk_signal_connect (GTK_OBJECT (xm->setupFrame), "unmap",
					      (GtkSignalFunc) killSetupXLock, this);

	//run (SS_SETUP, xm->setupFrame);
	gtk_idle_add ((GtkFunction) runFirstSetup, this);
}


///////////////////////////////////////////////////////////////////////////
//
//  XLockModePar
//

XLockModePar::XLockModePar (gchar *o, gchar *c, int t, gchar *d)
{
	option = g_strdup (o);
	comment = g_strdup (c);
	xtype = t;
	def = g_strdup (d);
	val = NULL;
}

XLockModePar::~XLockModePar ()
{
	g_free (option);
	g_free (comment);
}
