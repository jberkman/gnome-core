#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <zlib.h>
#include <glib.h>

#include "misc.h"

gchar *
getOutputFrom(gchar *argv[], gchar *writePtr, gint writeBytesLeft)
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
		g_error("couldn't exec %s", argv[0]);
		_exit(1);
	}
	if (progPID < 0) {
		g_error("couldn't fork %s", argv[0]);
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
			bytes = read(fromProg[0], buf, sizeof(buf)-1);
		}
			
		/* terminate when prog dies */
	} while (!progDead);
		
	close(toProg[1]);
	close(fromProg[0]);
	signal(SIGPIPE, oldhandler);

	if (writeBytesLeft) {
		g_error("failed to write all data to %s", argv[0]);
		g_free(outbuf);
		return NULL;
	}


/* ignore return code for now */
#if 0
	waitpid(progPID, &status, 0);
	if (!WIFEXITED(status) || WEXITSTATUS(status)) {
		g_error("getOutputFrom(): %s failed", argv[0]);
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
		g_error("couldn't exec %s", argv[0]);
		_exit(1);
	}
	if (progPID < 0) {
	        g_error("couldn't fork %s", argv[0]);
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
			bytes = read(fromProg[0], buf, sizeof(buf)-1);
		}
			
		/* terminate when prog dies */
	} while (!progDead);
		
	close(toProg[1]);
	close(fromProg[0]);
	signal(SIGPIPE, oldhandler);

	if (writeBytesLeft) {
		g_error("failed to write all data to %s", argv[0]);
		g_free(outbuf);
		return 0;
	}


/* ignore return code for now */
#if 0
	waitpid(progPID, &status, 0);
	if (!WIFEXITED(status) || WEXITSTATUS(status)) {
		g_error("getOutputFrom(): %s failed", argv[0]);
		g_free(outbuf);
		return 0;
	}
#endif
	*outbuf    = tmpoutbuf;
	*outbuflen = outpos;
	return outpos;
}


guchar
*loadFileToBuf( gchar *file )
{
	guchar buf[8193];
	guchar *out=NULL;
	gzFile *f;
	gint bytes, len=0;

	if ((f=gzopen(file, "r"))==NULL)
		return NULL;

	bytes=gzread(f, buf, 8192);
	while (bytes > 0) {
		if (!out)
			out = g_malloc(bytes);
		else 
			out = g_realloc(out, len+bytes);

		memcpy(out+len, buf, bytes);
		len += bytes;
		bytes=gzread(f, buf, 8192);
	}
	gzclose(f);
	return out;
}




void map_spaces_to_underscores( gchar *str )
{
  gchar *p;

  g_return_if_fail(str != NULL );
  for (p=str; *p; p++)
    switch (*p)
      {
      case ' ':
      case '\n':
      case '\t':
      case '`':
      case '\'':
      case '/':
      case '\\':
      case '"':
      case '.':
      case '!':
        *p = '_';
        break;
      }
}
