#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <glib.h>

#include "magic.h"
#include "docobj.h"
#include "parseUrl.h"
#include "misc.h"
#include "toc2.h"

gint
resolveMagicURL( docObj obj, Toc toc )
{
    DecomposedUrl u = NULL;
    gchar *ref;
    gchar *anchor;
    gchar *file;
    gchar buf[BUFSIZ];

    ref = docObjGetRef(obj);

    if (!strncmp(ref, "info:", 5)) {
	/* Break into component parts */

	u = decomposeUrl(ref);
	anchor = *(u->anchor) ? u->anchor : "Top";

	/* Call toc code to find the file */
	if (!(file = tocLookupInfo(toc, u->path + 1, anchor))) {
	    freeDecomposedUrl(u);
	    return -1;
	}

	/* Construct the final URL */
	g_snprintf(buf, sizeof(buf), "file:%s#%s", file, anchor);
	
	g_message("magic url: %s -> %s", ref, buf);

	docObjSetRef(obj, buf);
	docObjSetMimeType(obj, "application/x-info");

	/* Clean up */
	freeDecomposedUrl(u);

	return 0;
    } else if (!strncmp(ref, "man:", 4)) {
	return 0;
    } else {
	/* blow if nothing interesting */
	return 0;
    }
}
