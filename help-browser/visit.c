#include <glib.h>
#include <libgnome/gnome-help.h>

#include "parseUrl.h"
#include "docobj.h"
#include "window.h"
#include "cache.h"
#include "magic.h"
#include "mime.h"
#include "transport.h"
#include "visit.h"
#include "toc2.h"
#include "gtk-xmhtml/gtk-xmhtml.h"

static gint visitDocument( HelpWindow win, docObj obj );
static void displayHTML( HelpWindow win, docObj obj );

gint
visitURL( HelpWindow win, gchar *ref, 
	  gboolean useCache, gboolean addToQueue, gboolean addToHistory)
{
	docObj obj;

	/* !!! This is the entire lifespan of all the docObjs */

	/* trap 'toc:', 'whatis:' urls here for now */
	/* paranoid exists because of unknown memory problems */
	if (!strncmp(ref, "whatis:", 4)) {
		gchar *p=ref+7;
		gchar *paranoid = g_strdup(ref);
		GString *s;

		g_message("WHATIS requested for substr %s",p);

		helpWindowQueueMark(win);

		s = findMatchesBySubstr(helpWindowGetToc(win), p);
		if (s) {
			helpWindowHTMLSource(win, s->str, strlen(s->str),
					     paranoid, paranoid);
			helpWindowJumpToLine(win, 1);
			g_string_free(s, TRUE);
		}
		
		if (addToQueue) 
			helpWindowQueueAdd(win, paranoid);
		if (addToHistory)
			helpWindowHistoryAdd(win, paranoid);

		g_free(paranoid);
	} else if (!strncmp(ref, "toc:", 4)) {
		gchar *p=ref+4;
		gchar *paranoid = g_strdup(ref);
		GString *s;

		g_message("TOC requested for section %s",p);

		helpWindowQueueMark(win);

		s = NULL;
		if (!strncmp(p,"man",3)) {
			s = genManTocHTML(helpWindowGetToc(win));
		} else if (!strncmp(p,"info",4)) {
			s = genInfoTocHTML(helpWindowGetToc(win));
		} else if (!strncmp(p,"ghelp",5)) {
			s = genGhelpTocHTML(helpWindowGetToc(win));
		} else if (!*p) {
			gchar *hp, *q;
			hp = gnome_help_file_path("help-browser", 
						  "default-page.html");
			q = g_malloc(strlen(hp)+10);
			strcpy(q, "file:");
			strcat(q, hp);
			obj = docObjNew(q, useCache);
			docObjSetHumanRef(obj, "toc:");
			g_free(q);
			g_free(hp);
			if (visitDocument(win, obj)) {
				docObjFree(obj);
				return -1;
			}
			docObjFree(obj);
		} else {
			s = g_string_new("<BODY>Unknown TOC argument</BODY>");
		}

		if (s) {
			helpWindowHTMLSource(win, s->str, strlen(s->str),
					     paranoid, paranoid);
			helpWindowJumpToLine(win, 1);
			g_string_free(s, TRUE);
		}
		
		if (addToQueue) 
			helpWindowQueueAdd(win, paranoid);
		if (addToHistory)
			helpWindowHistoryAdd(win, paranoid);

		g_free(paranoid);
	} else {

		obj = docObjNew(ref, useCache);
		
		helpWindowQueueMark(win);
		
		if (visitDocument(win, obj)) {
			docObjFree(obj);
			return -1;
		}

		/* obj was 'cleaned up' by visitDocuemnt()/resolveURL() */
		if (addToQueue)
			helpWindowQueueAdd(win, docObjGetHumanRef(obj));
		if (addToHistory)
			helpWindowHistoryAdd(win, docObjGetHumanRef(obj));
		
		docObjFree(obj);
	}

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
