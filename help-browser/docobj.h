#ifndef _GNOME_HELP_DOCOBJ_H_
#define _GNOME_HELP_DOCOBJ_H_

#include "gnome-helpwin.h"
#include "url.h"
#include "queue.h"
#include "history.h"
#include "window.h"

struct _docObj {
	gchar *ref;
	gchar *mimeType;
	gchar *rawData;
	gchar *convData;
	gboolean  freeraw;
	gboolean  freeconv;
	urlType  url;
};

typedef struct _docObj docObj;

/* URL for data in widget */
/* XXX this stuff belongs in the HelpWindow structure */
extern gchar CurrentRef[];
extern gchar LoadingRef[]; /* HACK */
extern History history;

void visitURL( HelpWindow win, gchar *ref );
void visitURL_nohistory( HelpWindow win, gchar *ref );

docObj *docObj_new( void );
void docObj_free( docObj *obj );
#endif





