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
	if (strstr(ref, "info:")) {
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

	if (! (s = docObjGetRawData(obj)))
	    return;
	
	s = execfilter("gnome-man2html {}",
		       docObjGetDecomposedUrl(obj)->path, NULL);
	docObjSetConvData(obj, s, TRUE);
}


static void
convertINFO( docObj obj )
{
	char *argv[2];
	gchar *s;

	argv[0] = "gnome-info2html";
	argv[1] = NULL;

	if (! (s = docObjGetRawData(obj)))
	    return;
	
	docObjSetConvData(obj, getOutputFrom(argv, s, strlen(s)), TRUE);
	
}


/* routines to execute filters */
gchar
*execfilter(gchar *execpath, gchar *docpath, gchar *inbuf)
{
	gchar  *cmd;
	gchar  *t;
	gchar  *argv[3];

	/* see if they need the document filename as a cmdline arg */
	cmd = alloca(strlen(execpath)+10);
	strcpy(cmd,execpath);
	if ((t=strstr(cmd,"{}"))) {
		*t = '\0';
		argv[1] = docpath;
	} else {
		argv[1] = NULL;
	}

	for (t=cmd+strlen(cmd); (!(*t) || isspace(*t)) && t != cmd; t--);
	*(t+1) = '\0';


	argv[0] = cmd;
	argv[2] = NULL;
	
	g_message("execfilter: %s %s %s",
		  ((argv[0]) ? argv[0] : ""),
		  ((argv[1]) ? argv[1] : ""),
		  ((argv[2]) ? argv[2] : ""));


	return getOutputFrom(argv, inbuf, (inbuf) ? strlen(inbuf): 0);
}
