/* handles the loading and conversion of help documents into HTML */

#include <string.h>
#include <stdio.h>
#include <glib.h>

#include "docobj.h"
#include "parseUrl.h"
#include "transport.h"

struct _docObj {
    /* URL information */
    gchar *ref;
    gchar *absoluteRef;
    gchar *mimeType;
    DecomposedUrl decomposedUrl;
    
    /* The data */
    gchar *rawData;
    gchar *convData;
    gboolean  freeraw;
    gboolean  freeconv;

    /* Transport info */
    TransportMethod   transportMethod;
    TransportFunc     transportFunc;
};

docObj
docObjNew(gchar *ref)
{
	docObj p;

	p = g_malloc(sizeof(*p));
	p->ref = g_strdup(ref);
	p->absoluteRef = NULL;
	p->mimeType = NULL;
	p->decomposedUrl = NULL;

	p->rawData  = NULL;
	p->convData = NULL;
	p->freeraw  = FALSE;
	p->freeconv = FALSE;
	
	p->transportMethod = TRANS_UNRESOLVED;
	p->transportFunc = transportUnknown;

	return p;
}

void
docObjFree(docObj obj)
{
	g_return_if_fail( obj != NULL );

	g_free(obj->ref);
	g_free(obj->absoluteRef);
	g_free(obj->mimeType);
	
	if (obj->freeraw && obj->rawData)
		g_free(obj->rawData);
	if (obj->freeconv && obj->convData)
		g_free(obj->convData);

	if (obj->decomposedUrl)
		freeDecomposedUrl(obj->decomposedUrl);

	g_free(obj);
}

/* parse a URL into component pieces */
void
docObjResolveURL(docObj obj, gchar *currentRef)
{
	DecomposedUrl decomp = NULL;

	g_return_if_fail( obj != NULL );
	g_return_if_fail( obj->ref != NULL );

	if (obj->decomposedUrl)
	    return;

	/* HACKHACKHACK for info support */
	if (isRelative(obj->ref)) {
	    g_message("got relative reference: %s", obj->ref);
	    decomp = decomposeUrlRelative(obj->ref, currentRef,
					  &(obj->absoluteRef));
        } else {
	    g_message("got absolute reference: %s", obj->ref);
	    decomp = decomposeUrl(obj->ref);
	    obj->absoluteRef = g_strdup(obj->ref);
	}

	g_message("decomposed to: %s %s %s %s", decomp->access, 
		  decomp->host, decomp->path, decomp->anchor);

	/* stupid test for transport types we currently understand */
	if (!strncmp(decomp->access, "file", 4)) {
	    obj->transportMethod = TRANS_FILE;
	    obj->transportFunc   = transportFile;
	} else if (!strncmp(decomp->access, "http", 4)) {
	    obj->transportMethod = TRANS_HTTP;
	    obj->transportFunc   = transportHTTP;
	} else {
	    obj->transportMethod = TRANS_UNKNOWN;
	    obj->transportFunc   = transportUnknown;
	}

	obj->decomposedUrl = decomp;
}

void
docObjSetRef(docObj obj, gchar *ref)
{
	if (obj->ref)
		g_free(obj->ref);
	obj->ref = g_strdup(ref);
}

gchar *docObjGetRef(docObj obj)
{
    return obj->ref;
}

gchar *docObjGetAbsoluteRef(docObj obj)
{
    return obj->absoluteRef;
}

gchar *docObjGetMimeType(docObj obj)
{
    return obj->mimeType;
}

gchar *docObjGetRawData(docObj obj)
{
    return obj->rawData;
}

gchar *docObjGetConvData(docObj obj)
{
    return obj->convData;
}

DecomposedUrl docObjGetDecomposedUrl(docObj obj)
{
    return obj->decomposedUrl;
}

TransportFunc docObjGetTransportFunc(docObj obj)
{
    return obj->transportFunc;
}
    
void docObjSetMimeType(docObj obj, gchar *s)
{
    if (obj->mimeType)
	g_free(obj->mimeType);

    obj->mimeType = g_strdup(s);
}

void docObjSetRawData(docObj obj, gchar *s, gboolean freeit)
{
    obj->rawData = s;
    obj->freeraw = freeit;
}

void docObjSetConvData(docObj obj, gchar *s, gboolean freeit)
{
    obj->convData = s;
    obj->freeconv = freeit;
}
