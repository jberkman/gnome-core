/* handles MIME type recognition and conversion to HTML */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <signal.h>
#include <ctype.h>
#include <fcntl.h>
#include <wait.h>
#include <errno.h>

#include <glib.h>

#include "mime.h"
#include "misc.h"

static void convertMan(docObj *obj);
static void convertHTML(docObj *obj);
static void convertINFO(docObj *obj);
static void convertText(docObj *obj);

void
resolveMIME( docObj *obj )
{

	if (obj->mimeType)
		return;

	/* do simple recognition for now based on ref */
	if (strstr(obj->ref, "info:")) {
		obj->mimeType = "application/x-info";
	} else if (strstr(obj->ref, "/man/")) {
		obj->mimeType = "application/x-troff-man";
	} else if (strstr(obj->ref, "html") || strstr(obj->ref, "#")
		   || strstr(obj->ref, "http")) {
		obj->mimeType = "text/html";
	} else {
		obj->mimeType = "text/plain";
	}

	printf("->%s<-  ->%s<-\n",obj->ref, obj->mimeType);

}


void
convertMIME( docObj *obj )
{

	if (!strcmp(obj->mimeType, "application/x-troff-man")) {
		convertMan(obj);
	} else if (!strcmp(obj->mimeType, "text/html")) {
		convertHTML(obj);
	} else if (!strcmp(obj->mimeType, "application/x-info")) {
		convertINFO(obj);
	} else if (!strcmp(obj->mimeType, "text/plain")) {
		convertText(obj);
	} else {
		convertText(obj);
	}
}

static void
convertHTML( docObj *obj ) 
{
	g_return_if_fail( obj != NULL );

	/* if converted data exists lets use it */
	if (obj->convData)
		return;

	obj->convData = obj->rawData;
	obj->freeconv = FALSE;
}

static void
convertText( docObj *obj )
{
	gchar *s;

	g_return_if_fail( obj != NULL );

	/* if converted data exists lets use it */
	if (obj->convData)
		return;
	
	s = g_malloc(strlen(obj->rawData) + 30);
	strcpy(s, "<BODY><PRE>\n");
	strcat(s, obj->rawData);
	strcat(s, "\n</PRE></BODY>\n");

	obj->convData = s;
	obj->freeconv = TRUE;
}

static void
convertMan( docObj *obj )
{
	/* broken - we ignore obj->rawData because man2html doesnt */
	/*          work with input from stdin                     */
	if (!obj->rawData)
		return;
	obj->convData = execfilter("gnome-man2html {}",obj->url.u->path, NULL);
}


static void
convertINFO( docObj *obj )
{
	char *argv[2];

	argv[0] = "gnome-info2html";
	argv[1] = NULL;

	if (!obj->rawData)
		return;
	obj->convData = getOutputFrom(argv, obj->rawData, 
				      strlen(obj->rawData));
	obj->freeconv = TRUE;
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
	
	printf("Executing command ->%s<- ->%s<- ->%s<-\n",
	       ((argv[0]) ? argv[0] : ""),
	       ((argv[1]) ? argv[1] : ""),
	       ((argv[2]) ? argv[2] : ""));


	return getOutputFrom(argv, inbuf, (inbuf) ? strlen(inbuf): 0);
}
