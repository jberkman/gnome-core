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


gchar
*getOutputFrom(gchar *argv[], gchar *writePtr, gint writeBytesLeft)
{
	gint progPID;
	gint progDead;
	gint toProg[2];
	gint fromProg[2];
	gint status;
	void *oldhandler;
	gint bytes;
	guchar buf[8193];
	guchar *outbuf;
	gint   outbuflen;

	oldhandler = signal(SIGPIPE, SIG_IGN);
	
	pipe(toProg);
	pipe(fromProg);
		
	if (!(progPID = fork())) {
		close(toProg[1]);
		close(fromProg[0]);
		dup2(toProg[0], 0);   /* Make stdin the in pipe */
		dup2(fromProg[1], 1); /* Make stdout the out pipe */
			
		close(toProg[0]);
		close(fromProg[1]);
			
		execvp(argv[0], argv);
		fprintf(stderr, "Couldn't exec %s\n", argv[0]);
		_exit(1);
	}
	if (progPID < 0) {
		fprintf(stderr, "Couldn't fork %s\n", argv[0]);
		return NULL;
	}
		
	close(toProg[0]);
	close(fromProg[1]);
		
	/* Do not block reading or writing from/to prog. */
	fcntl(fromProg[0], F_SETFL, O_NONBLOCK);
	fcntl(toProg[1], F_SETFL, O_NONBLOCK);
		
	outbuf    = g_malloc(1);
	outbuf[0] = '\0';
	outbuflen = 1;

	progDead = 0;
	do {
		if (waitpid(progPID, &status, WNOHANG)) {
			progDead = 1;
		}
			
		/* write some data to the prog */
		if (writeBytesLeft) {
			gint n, r;

			n = (writeBytesLeft > 1024) ? 1024 : writeBytesLeft;
			if ((r=write(toProg[1], writePtr, n)) < 0) {
				if (errno != EAGAIN) {
					perror("getOutputFrom()");
					exit(1);
				}
				r = 0;
			}
			writeBytesLeft -= r;
			writePtr += r;
		} else {
			close(toProg[1]);
		}

		/* Read any data from prog */
		bytes = read(fromProg[0], buf, sizeof(buf)-1);
		while (bytes > 0) {
			buf[bytes] = '\0';
			if (outbuflen+bytes > outbuflen) {
				outbuf = g_realloc(outbuf,
						   outbuflen+bytes);
				outbuflen += bytes;
			}
			strcat(outbuf, buf);
			fprintf(stderr,"%8d bytes read\r",outbuflen);
			bytes = read(fromProg[0], buf, sizeof(buf)-1);
		}
			
		/* terminate when prog dies */
	} while (!progDead);
		
	close(toProg[1]);
	close(fromProg[0]);
	signal(SIGPIPE, oldhandler);

	if (writeBytesLeft) {
		fprintf(stderr, "failed to write all data to %s", argv[0]);
		g_free(outbuf);
		return NULL;
	}


/* ignore return code for now */
#if 0
	waitpid(progPID, &status, 0);
	if (!WIFEXITED(status) || WEXITSTATUS(status)) {
		fprintf(stderr, "%s failed\n", argv[0]);
		g_free(outbuf);
		return NULL;
	}
#endif

	return outbuf;
}



gint
getOutputFromBin(gchar *argv[], gchar *writePtr, gint writeBytesLeft,
	         guchar **outbuf, gint *outbuflen)
{
	gint progPID;
	gint progDead;
	gint toProg[2];
	gint fromProg[2];
	gint status;
	void *oldhandler;
	gint bytes;
	guchar buf[8193];
	guchar *tmpoutbuf;
	gint    outpos;

	oldhandler = signal(SIGPIPE, SIG_IGN);
	
	pipe(toProg);
	pipe(fromProg);
		
	if (!(progPID = fork())) {
		close(toProg[1]);
		close(fromProg[0]);
		dup2(toProg[0], 0);   /* Make stdin the in pipe */
		dup2(fromProg[1], 1); /* Make stdout the out pipe */
			
		close(toProg[0]);
		close(fromProg[1]);
			
		execvp(argv[0], argv);
		fprintf(stderr, "Couldn't exec %s\n", argv[0]);
		_exit(1);
	}
	if (progPID < 0) {
		fprintf(stderr, "Couldn't fork %s\n", argv[0]);
		return 0;
	}
		
	close(toProg[0]);
	close(fromProg[1]);
		
	/* Do not block reading or writing from/to prog. */
	fcntl(fromProg[0], F_SETFL, O_NONBLOCK);
	fcntl(toProg[1], F_SETFL, O_NONBLOCK);
		
	outpos = 0;
	tmpoutbuf = NULL;

	progDead = 0;
	do {
		if (waitpid(progPID, &status, WNOHANG)) {
			progDead = 1;
		}
			
		/* write some data to the prog */
		if (writeBytesLeft) {
			gint n, r;

			n = (writeBytesLeft > 1024) ? 1024 : writeBytesLeft;
			if ((r=write(toProg[1], writePtr, n)) < 0) {
				if (errno != EAGAIN) {
					perror("getOutputFrom()");
					exit(1);
				}
				r = 0;
			}
			writeBytesLeft -= r;
			writePtr += r;
		} else {
			close(toProg[1]);
		}

		/* Read any data from prog */
		bytes = read(fromProg[0], buf, sizeof(buf)-1);
		while (bytes > 0) {
			if (tmpoutbuf)
				tmpoutbuf=g_realloc(tmpoutbuf,outpos+bytes);
			else
				tmpoutbuf=g_malloc(bytes);

			memcpy(tmpoutbuf+outpos, buf, bytes);
			outpos += bytes;
			fprintf(stderr,"%8d bytes read\r",outpos);
			bytes = read(fromProg[0], buf, sizeof(buf)-1);
		}
			
		/* terminate when prog dies */
	} while (!progDead);
		
	close(toProg[1]);
	close(fromProg[0]);
	signal(SIGPIPE, oldhandler);

	if (writeBytesLeft) {
		fprintf(stderr, "failed to write all data to %s", argv[0]);
		g_free(outbuf);
		return 0;
	}


/* ignore return code for now */
#if 0
	waitpid(progPID, &status, 0);
	if (!WIFEXITED(status) || WEXITSTATUS(status)) {
		fprintf(stderr, "%s failed\n", argv[0]);
		g_free(outbuf);
		return 0;
	}
#endif
	*outbuf    = tmpoutbuf;
	*outbuflen = outpos;
	return outpos;
}
