/* handles url type references and retrieving the sorresponding doc data */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <glib.h>

#include "docobj.h"
#include "transport.h"
#include "mime.h"

#include "parseUrl.h"

void transportUnknown( docObj *obj );
void transportFile( docObj *obj );
void transportHTTP( docObj *obj );

/* parse a URL into component pieces */
void
resolveURL( docObj *obj )
{
	gchar *s;

	g_return_if_fail( obj != NULL );
	g_return_if_fail( obj->ref != NULL );

	if (obj->url.u)
		return;

	if (isRelative(obj->ref)) {
		obj->url.u = decomposeUrlRelative(obj->ref, CurrentRef, &s);
		g_free(obj->ref);
		obj->ref = s;
        } else {
		obj->url.u = decomposeUrl(obj->ref);
	}

	printf("%s %s\n",obj->ref, obj->url.u->access);
	/* stupid test for transport types with currently understand */
	if (!strncmp(obj->url.u->access, "file", 4)) {
		obj->url.method = TRANS_FILE;
		obj->url.func   = transportFile;
	} else if (!strncmp(obj->url.u->access, "http", 4)) {
		obj->url.method = TRANS_HTTP;
		obj->url.func   = transportHTTP;
	} else {
		obj->url.method = TRANS_UNKNOWN;
		obj->url.func   = transportUnknown;
	}
}

/* after calling resolveURL, we can now transport the doc */
void
transport( docObj *obj )
{
	(obj->url.func)(obj);
}

void
transportUnknown( docObj *obj )
{
	gchar    s[513];

	g_snprintf(s, sizeof(s), "<BODY>Error: unable to resolve transport "
		   "method for requested URL:<br><b>%s</b></BODY>",obj->ref);
	obj->rawData = g_strdup(s);
	obj->freeraw = TRUE;
}

void
transportFile( docObj *obj )
{
	gchar   *s=NULL;
	gchar   buf[8193];
	gchar   *file;
	gint    fd;
	gint    bytes;
	gint    total;
	gchar   *pipecmd=NULL;

	/* is the file compressed? */
	file = obj->url.u->path;
	if (!strcmp(file+strlen(file)-2, ".Z"))
		pipecmd = "/bin/zcat";
	else if (!strcmp(file+strlen(file)-2, ".gz"))
		pipecmd = "/bin/gzip -d -c";


	if (!pipecmd) {
		fd = open(file, O_RDONLY);
		if (fd < 0) {
			g_snprintf(buf, sizeof(buf), 
				   "<BODY>Error: unable to open "
				   "file %s</BODY>",file);
			obj->rawData = g_strdup(buf);
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
		obj->rawData = s;
		obj->freeraw = TRUE;
	} else {
		char *argv[3];

		argv[0] = pipecmd;
		argv[1] = file;
		argv[2] = NULL;

		obj->rawData = getOutputFrom(argv, NULL, 0);
		obj->freeraw = TRUE;
	}

	return;
}


void
transportHTTP( docObj *obj )
{
	char *argv[4];

	argv[0] = "lynx";
	argv[1] = "-source";
	argv[2] = obj->ref;
	argv[3] = NULL;

	obj->rawData = getOutputFrom(argv, NULL, 0);
	obj->freeraw = TRUE;
}

