#include <glib.h>

#include "parseUrl.h"
#include "docobj.h"
#include "window.h"
#include "cache.h"
#include "mime.h"
#include "transport.h"
#include "visit.h"

static void visitDocument( HelpWindow win, docObj obj );
static void displayHTML( HelpWindow win, docObj obj );
static void _visitURL( HelpWindow win, gchar *ref, gboolean save );

void
visitURL( HelpWindow win, gchar *ref )
{
	_visitURL(win, ref, TRUE);
}

void visitURL_nohistory(HelpWindow win, gchar *ref )
{
	_visitURL(win, ref, FALSE);
}

static void
visitDocument(HelpWindow win, docObj obj )
{
	docObjResolveURL(obj, helpWindowCurrentRef(win));
	transport(obj, helpWindowGetCache(win));
	resolveMIME(obj);
	convertMIME(obj);
	displayHTML(win, obj);
}


/* most people will call this - it allocates a docObj type and loads   */
/* the page. Currently it frees the docObj afterwards, no history kept */
static void
_visitURL( HelpWindow win, gchar *ref, gboolean save )
{
	docObj obj;

	/* !!! This is the entire lifespan of all the docObjs */
	
	obj = docObjNew(ref);

	visitDocument(win, obj);

	/* obj was 'cleaned up' by visitDocuemnt()/resolveURL() */
	if (save) {
	        helpWindowQueueAdd(win, docObjGetAbsoluteRef(obj));
		helpWindowHistoryAdd(win, docObjGetAbsoluteRef(obj));
	}
	
	docObjFree(obj);

	/* !!! This is the entire lifespan of all the docObjs */
}

static void
displayHTML( HelpWindow win, docObj obj )
{
        DecomposedUrl decomp;
    
	/* Load the page */
	if (docObjGetConvData(obj)) {
		helpWindowHTMLSource(win, docObjGetConvData(obj),
				     docObjGetAbsoluteRef(obj));
	}

	/* Set position */
	decomp = docObjGetDecomposedUrl(obj);
	if (*(decomp->anchor))
		helpWindowJumpToAnchor(win, decomp->anchor);
	else
		helpWindowJumpToLine(win, 1);
}
