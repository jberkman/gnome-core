#ifndef _GNOME_HELP_DOCOBJ_H_
#define _GNOME_HELP_DOCOBJ_H_

#include "gnome-helpwin.h"
#include "url.h"
#include "queue.h"

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
extern gchar CurrentRef[];
extern gchar LoadingRef[]; /* HACK */
extern Queue queue;

void visitURL( GnomeHelpWin *help, gchar *ref );
void visitURL_nohistory( GnomeHelpWin *help, gchar *ref );
void visitDocument( GnomeHelpWin *help, docObj *obj );
docObj *docObj_new( void );
void docObj_free( docObj *obj );
void displayHTML( GnomeHelpWin *help, docObj *obj );
#endif





