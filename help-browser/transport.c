/* transport functions */

#include <stdio.h>
#include <string.h>

#include <glib.h>

#include "transport.h"
#include "docobj.h"
#include "parseUrl.h"
#include "misc.h"
#include "cache.h"

gint
transport( docObj obj, DataCache cache )
{
    DecomposedUrl url;
    gchar key[BUFSIZ];
    gchar *p;
    gint  rc;

    if (cache) {
	url = docObjGetDecomposedUrl(obj);
	sprintf(key, "%s://%s%s", url->access, url->host, url->path);
	p = lookupInDataCache(cache, key);
	if (p) {
	    g_message("cache hit: %s", key);
	    /* XXX potential problem here.  The cache can free up */
	    /* space if it hits a limit, which would invalidate   */
	    /* this pointer.  It's not right, but for now, the    */
	    /* short lifespan of obj will probably save us.       */
	    docObjSetRawData(obj, p, FALSE);
	    return 0;
	}
    }
    
    rc = (docObjGetTransportFunc(obj))(obj);
    
    if (rc)
	    return rc;

    if (cache) {
	p = docObjGetRawData(obj);
	addToDataCache(cache, key, g_strdup(p), strlen(p));
    }
    
    return 0;
}

gint
transportUnknown( docObj obj )
{
#if 0
	gchar    s[513];
	g_snprintf(s, sizeof(s), "<BODY>Error: unable to resolve transport "
		   "method for requested URL:<br><b>%s</b></BODY>",
		   docObjGetRef(obj));
	docObjSetRawData(obj, g_strdup(s), TRUE);
#endif
	return -1;
}

gint
transportFile( docObj obj )
{
	guchar *buf;
	
	if (loadFileToBuf(docObjGetDecomposedUrl(obj)->path, &buf)) 
		return -1;
	
	docObjSetRawData(obj, buf, TRUE);
	return 0;
}

gint
transportHTTP( docObj obj )
{
	char *argv[4];

	argv[0] = "lynx";
	argv[1] = "-source";
	argv[2] = docObjGetAbsoluteRef(obj);
	argv[3] = NULL;

	docObjSetRawData(obj, getOutputFrom(argv, NULL, 0), TRUE);
	return 0;
}

