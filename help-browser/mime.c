/* handles MIME type recognition and conversion to HTML */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <signal.h>
#include <ctype.h>
#include <fcntl.h>
#include <wait.h>
#include <errno.h>
#include <string.h>

#include <glib.h>

#include "docobj.h"
#include "mime.h"
#include "misc.h"

static void convertMan(docObj obj);
static void convertHTML(docObj obj);
static void convertINFO(docObj obj);
static void convertText(docObj obj);

void
resolveMIME( docObj obj )
{

        gchar *ref;
    
	if (docObjGetMimeType(obj))
		return;

	ref = docObjGetAbsoluteRef(obj);
	
	/* do simple recognition for now based on ref */
	if (strstr(ref, "/info/")) {
	        docObjSetMimeType(obj, "application/x-info");
	} else if (strstr(ref, "/man/")) {
		docObjSetMimeType(obj, "application/x-troff-man");
	} else if (strstr(ref, "html") || strstr(ref, "#")
		   || strstr(ref, "http")) {
		docObjSetMimeType(obj, "text/html");
	} else {
		docObjSetMimeType(obj, "text/plain");
	}

	g_message("resolved mime type: %s is %s",
		  ref, docObjGetMimeType(obj));

}


void
convertMIME( docObj obj )
{
        gchar *m;

        m = docObjGetMimeType(obj);

	if (!strcmp(m, "application/x-troff-man")) {
		convertMan(obj);
	} else if (!strcmp(m, "text/html")) {
		convertHTML(obj);
	} else if (!strcmp(m, "application/x-info")) {
		convertINFO(obj);
	} else if (!strcmp(m, "text/plain")) {
		convertText(obj);
	} else {
		convertText(obj);
	}
}

static void
convertHTML( docObj obj ) 
{
	g_return_if_fail( obj != NULL );

	/* if converted data exists lets use it */
	if (docObjGetConvData(obj))
	    return;

	docObjSetConvData(obj, docObjGetRawData(obj), FALSE);
}

static void
convertText( docObj obj )
{
	gchar *s;
	gchar *raw;

	g_return_if_fail( obj != NULL );

	/* if converted data exists lets use it */
	if (docObjGetConvData(obj))
	    return;
	
	raw = docObjGetRawData(obj);
	s = g_malloc(strlen(raw) + 30);
	strcpy(s, "<BODY><PRE>\n");
	strcat(s, raw);
	strcat(s, "\n</PRE></BODY>\n");

	docObjSetConvData(obj, s, TRUE);
}

static void
convertMan( docObj obj )
{
	/* broken - we ignore obj->rawData because man2html doesnt */
	/*          work with input from stdin                     */

        gchar *s;
	char *argv[3];

	if (! (s = docObjGetRawData(obj)))
	    return;

	argv[0] = "gnome-man2html";
	argv[1] = docObjGetDecomposedUrl(obj)->path;
	argv[2] = NULL;
	    
	g_message("filter: %s %s", argv[0], argv[1]);
	
	docObjSetConvData(obj, getOutputFrom(argv, "", 0), TRUE);
}


static void
convertINFO( docObj obj )
{
	char *argv[6];
	gchar *s;
	gchar *a;
	gchar *base;
	gchar *basepath;

	argv[0] = "gnome-info2html";
	argv[1] = "-a";
	a = docObjGetDecomposedUrl(obj)->anchor;
	if (a && *a) {
		gchar *p;
		a = alloca(strlen(a)+5);
		strcpy(a,"\"");
		strcat(a, docObjGetDecomposedUrl(obj)->anchor);
		strcat(a, "\"");
	} else {
		a = "\"Top\"";
	}
	argv[2] = a;
	argv[3] = "-b";
	basepath = g_strdup(docObjGetAbsoluteRef(obj));

	base = strrchr(basepath, '/');
	base++;
	if ((s=strstr(base, ".info")))
		*s = '\0';
	if ((s=strstr(base, ".gz")))
		*s = '\0';
	for (s=base+strlen(base)-1; isdigit(*s) && s > base; s--);
	if (*s == '-')
		*s = '\0';

	argv[4] = base;
	argv[5] = NULL;

	g_message("filter: %s %s %s %s %s",
		  argv[0],argv[1],argv[2],argv[3], argv[4]);

	if (! (s = docObjGetRawData(obj)))
	    return;

	docObjSetConvData(obj, getOutputFrom(argv, s, strlen(s)), TRUE);
	g_free(basepath);
}
