#ifndef __XLOCKMORE_H__
#define __XLOCKMORE_H__

#pragma interface
#include "screensaver.h"
#include <signal.h>
#include <string.h>

enum {
	XLMP_NO_ARG,
	XLMP_BOOL_ARG,
	XLMP_INT_ARG,
	XLMP_FLOAT_ARG,
	XLMP_STRING_ARG,
	XLMP_MODES_ARG,
};

struct XLockMore : public ScreenSaver {

	GList *args;
	gint argn;
	gint pPID, sPID;
	gint pfd [2];

	GtkWidget *widget;
	gint px, py, pw, ph;

	// instance for lex parser
	static XLockMore* xlockmore;

	void resetArg ();
	void addArg (char *);

	int forkAndExec ();
	int kill (gint *, gint=SIGTERM, gint=1);

	XLockMore ();
	virtual ~XLockMore ();

	gint mapSignal;
	gint unmapSignal;
	gint destroySignal;

	GtkWidget *setupWin;
	GtkWidget *modePage;
	GtkWidget *setupNotebook;
	GtkWidget *setupFrame;
	GtkWidget *setupOptions;
	GtkWidget *setupName;
	GtkWidget *setupComment;
	void prepareSetupWindow ();
};

struct XLockModePar {
	gchar *option;
	gchar *comment;
	int xtype;

	gchar *def;
	gchar *val;

	gint getBoolVal () {
		gchar v = (val) ? *val : *def;
		return (v == 'T' || v == 't');
	}

	gchar *getVal () {
		return (val) ? val : def;
	}

	void setExecArgs (XLockMore *xm) {
		gchar *rv;
		gchar *v = getVal ();

		switch (xtype) {
		case XLMP_BOOL_ARG:
			rv = new gchar [strlen (option) + 2];
			*rv = (strcasecmp (v, "true")) ? '+' : '-';
			strcpy (rv+1, option);

			xm->addArg (rv);

			delete [] rv;

			break;
		default:
			rv = new gchar [strlen (option) + 2];
			*rv = '-';
			strcpy (rv+1, option);

			xm->addArg (rv);
			xm->addArg (v);

			delete [] rv;
		}
	}

	XLockModePar (gchar *option, gchar *comment, int type, gchar *d);
	~XLockModePar ();
};

struct XLockMode : public ScreenSaverMode {
	GList *pars;
	gint parn;
	XLockMore *screenSaver;

	void addPar (XLockModePar *par);

	virtual void run (gint type, ...);
	virtual void stop (gint type, ...);
	virtual void setup ();

	XLockMode (XLockMore *ss, gchar *name, gchar *comment);
	virtual ~XLockMode ();
};

#endif
