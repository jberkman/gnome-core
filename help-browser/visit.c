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

gint
visitURL_nohistory(HelpWindow win, gchar *ref )
{
	return _visitURL(win, ref, FALSE);
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
	        helpWindowQueueAdd(win, docObjGetHumanRef(obj));
		helpWindowHistoryAdd(win, docObjGetHumanRef(obj));
	}
	
	docObjFree(obj);

	/* !!! This is the entire lifespan of all the docObjs */
	return 0;
}

static gint
visitDocument(HelpWindow win, docObj obj )
{
        if (resolveMagicURL( obj, helpWindowGetToc(win) ))
	        return -1;
	docObjResolveURL(obj, helpWindowCurrentRef(win));
	if (transport(obj, helpWindowGetCache(win)))
		return -1;
	resolveMIME(obj);
	convertMIME(obj);
	displayHTML(win, obj);
	return 0;
}

static void
displayHTML( HelpWindow win, docObj obj )
{
        guchar *buf;
	gint buflen;
        DecomposedUrl decomp;
    
	/* Load the page */
	docObjGetConvData(obj, &buf, &buflen);
	helpWindowHTMLSource(win, buf, buflen,
			     docObjGetAbsoluteRef(obj),
			     docObjGetHumanRef(obj));

	/* Set position */
	decomp = docObjGetDecomposedUrl(obj);
	if (*(decomp->anchor))
		helpWindowJumpToAnchor(win, decomp->anchor);
	else
		helpWindowJumpToLine(win, 1);
}
