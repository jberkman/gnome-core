/* transport functions */

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include <glib.h>

#include "transport.h"
#include "docobj.h"
#include "parseUrl.h"
#include "misc.h"
#include "cache.h"

void
transport( docObj obj, DataCache cache )
{
    DecomposedUrl url;
    gchar key[BUFSIZ];
    gchar *p;
    
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
	    return;
	}
    }
    
    (docObjGetTransportFunc(obj))(obj);

    if (cache) {
	p = docObjGetRawData(obj);
	addToDataCache(cache, key, g_strdup(p), strlen(p));
    }
}

void
transportUnknown( docObj obj )
{
	gchar    s[513];

	g_snprintf(s, sizeof(s), "<BODY>Error: unable to resolve transport "
		   "method for requested URL:<br><b>%s</b></BODY>",
		   docObjGetRef(obj));
	docObjSetRawData(obj, g_strdup(s), TRUE);
}

void
transportFile( docObj obj )
{
	gchar   *s=NULL, *out;
	gchar   buf[8193];
	gint    fd;
	gint    bytes;
	gint    total;
	gchar   *pipecmd=NULL;
	gchar   file[1024];

	out = NULL;

	strcpy(file, docObjGetDecomposedUrl(obj)->path);

		/* test for existance */
	printf("accessing ->%s<-\n",file);		
	if (access(file, R_OK)) {
		g_snprintf(buf, sizeof(buf), 
			   "<BODY>Error: unable to open "
			   "file %s</BODY>",file);
		docObjSetRawData(obj, g_strdup(buf), TRUE);
		return;
	}
			
	/* is the file compressed? */
	if (!strcmp(file+strlen(file)-2, ".Z"))
		pipecmd = "/bin/zcat";
	else if (!strcmp(file+strlen(file)-3, ".gz"))
		pipecmd = "/bin/zcat";
		
	if (!pipecmd) {
		fd = open(file, O_RDONLY);
		if (fd < 0) {
			g_snprintf(buf, sizeof(buf), 
				   "<BODY>Error: unable to open "
				   "file %s</BODY>",file);
			docObjSetRawData(obj, g_strdup(buf), TRUE);
			return;
		}
		total = 0;
		while ((bytes=read(fd, buf, 8192))) {
			if (s) {
				s = g_realloc(s, total+bytes);
				total += bytes;
			} else {
				s = g_malloc(bytes+1);
				*s = '\0';
				total = bytes+1;
			}
				
			buf[bytes] = '\0';
			strcat(s, buf);
		}
			
		close(fd);
	} else {
		char *argv[3];
			
		argv[0] = pipecmd;
		argv[1] = file;
		argv[2] = NULL;
			
		s = getOutputFrom(argv, NULL, 0);
	}
		
	if (out) {
		out = g_realloc(out, strlen(out)+strlen(s)+1);
		strcat(out, s);
		g_free(s);
		s = NULL;
	} else {
		out = g_strdup(s);
		g_free(s);
		s = NULL;
	}

	docObjSetRawData(obj, out, TRUE);
	return;
}

void
transportHTTP( docObj obj )
{
	char *argv[4];

	argv[0] = "lynx";
	argv[1] = "-source";
	argv[2] = docObjGetAbsoluteRef(obj);
	argv[3] = NULL;

	docObjSetRawData(obj, getOutputFrom(argv, NULL, 0), TRUE);
}

