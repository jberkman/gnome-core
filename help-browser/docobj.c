/* handles the loading and conversion of help documents into HTML */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "gnome-helpwin.h"

#include "docobj.h"
#include "transport.h"
#include "mime.h"

#include "parseUrl.h"
#include "queue.h"

gchar  CurrentRef[1024];
gchar  LoadingRef[1024];

Queue queue;

docObj
*docObj_new(void)
{
	docObj *p;

	p = g_malloc(sizeof(docObj));
	p->ref      = NULL;
	p->mimeType = NULL;
	p->rawData  = NULL;
	p->convData = NULL;
	p->freeraw  = FALSE;
	p->freeconv = FALSE;
	p->url.u    = NULL;

	return p;
}

void
docObj_free(docObj *obj)
{
	g_return_if_fail( obj != NULL );

	if (obj->freeraw && obj->rawData)
		g_free(obj->rawData);
	if (obj->freeconv && obj->convData)
		g_free(obj->convData);

	if (obj->url.u)
		freeDecomposedUrl(obj->url.u);

	g_free(obj);
}

void
visitDocument( GnomeHelpWin *help, docObj *obj )
{

	resolveURL(obj);
	transport(obj);
	resolveMIME(obj);
	convertMIME(obj);
	displayHTML(help, obj);

}


/* most people will call this - it allocates a docObj type and loads   */
/* the page. Currently it frees the docObj afterwards, no history kept */
static void
_visitURL( GnomeHelpWin *help, gchar *ref, gboolean save )
{
	docObj *obj;

	obj = docObj_new();
	obj->ref = g_strdup(ref);

	visitDocument(help, obj);

	/* obj->ref was 'cleaned up' by visitDocuemnt()/resolveURL() */
	if (save)
		queue_add(queue, obj->ref);
	docObj_free(obj);
}

void
visitURL( GnomeHelpWin *help, gchar *ref )
{
	_visitURL(help, ref, TRUE);
}

void visitURL_nohistory(GnomeHelpWin *help, gchar *ref )
{
	_visitURL(help, ref, FALSE);
}

void
displayHTML( GnomeHelpWin *help, docObj *obj )
{
	strncpy(CurrentRef, obj->ref, sizeof(CurrentRef));

	if (obj->convData) {
		snprintf(LoadingRef, sizeof(LoadingRef), "%s://%s%s",
			 obj->url.u->access, 
			 obj->url.u->host, 
			 obj->url.u->path);
		gtk_xmhtml_source( GTK_XMHTML(help), obj->convData );
	}

	printf("anchor is ->%s<-\n", (obj->url.u->anchor));
	if (*(obj->url.u->anchor))
		jump_to_anchor( help, obj->url.u->anchor);
	else
		jump_to_line( help, 0 );
}
