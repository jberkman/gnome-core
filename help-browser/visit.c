#include <glib.h>

#include "parseUrl.h"
#include "docobj.h"
#include "window.h"
#include "cache.h"
#include "magic.h"
#include "mime.h"
#include "transport.h"
#include "visit.h"

static gint visitDocument( HelpWindow win, docObj obj );
static void displayHTML( HelpWindow win, docObj obj );
static gint _visitURL( HelpWindow win, gchar *ref, gboolean save );

gint
visitURL( HelpWindow win, gchar *ref )
{
	return _visitURL(win, ref, TRUE);
}

gint visitURL_nohistory(HelpWindow win, gchar *ref )
{
	return _visitURL(win, ref, FALSE);
}

static gint
visitDocument(HelpWindow win, docObj obj )
{
	resolveMagicURL( obj );
	docObjResolveURL(obj, helpWindowCurrentRef(win));
	if (transport(obj, helpWindowGetCache(win)))
		return -1;
	resolveMIME(obj);
	convertMIME(obj);
	displayHTML(win, obj);
	return 0;
}


/* most people will call this - it allocates a docObj type and loads   */
/* the page. Currently it frees the docObj afterwards, no history kept */
static gint
_visitURL( HelpWindow win, gchar *ref, gboolean save )
{
	docObj obj;

	/* !!! This is the entire lifespan of all the docObjs */
	
	obj = docObjNew(ref);

	helpWindowQueueMark(win);
	
	if (visitDocument(win, obj)) {
		docObjFree(obj);
		return -1;
	}

	/* obj was 'cleaned up' by visitDocuemnt()/resolveURL() */
	if (save) {
	        helpWindowQueueAdd(win, docObjGetAbsoluteRef(obj));
		helpWindowHistoryAdd(win, docObjGetAbsoluteRef(obj));
	}
	
	docObjFree(obj);

	/* !!! This is the entire lifespan of all the docObjs */
	return 0;
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
