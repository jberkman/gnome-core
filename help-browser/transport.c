/* transport functions */

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <glib.h>

#include "transport.h"
#include "docobj.h"
#include "parseUrl.h"
#include "misc.h"
#include "cache.h"

static int getHostByName(const char *host, struct in_addr *address);

gint
transport( docObj obj, DataCache cache )
{
    DecomposedUrl url;
    gchar key[BUFSIZ];
    guchar *p, *copy;
    gint  rc;
    gint len;

    url = docObjGetDecomposedUrl(obj);
    g_snprintf(key, sizeof key, "%s://%s%s", url->access, url->host, url->path);

    if (docObjUseCache(obj) && cache) {
	p = lookupInDataCacheWithLen(cache, key, &len);
	if (p) {
	    g_message("cache hit: %s", key);
	    /* XXX potential problem here.  The cache can free up */
	    /* space if it hits a limit, which would invalidate   */
	    /* this pointer.  It's not right, but for now, the    */
	    /* short lifespan of obj will probably save us.       */
	    docObjSetRawData(obj, p, len, FALSE);
	    return 0;
	}
    }
    
    rc = (docObjGetTransportFunc(obj))(obj);
    
    if (rc)
	    return rc;

    if (cache) {
	docObjGetRawData(obj, &p, &len);
	copy = g_malloc(len);
	memcpy(copy, p, len);
	addToDataCache(cache, key, copy, len, ! docObjUseCache(obj));
    }
    
    return 0;
}

gint
transportUnknown( docObj obj )
{
	return -1;
}

gint
transportFile( docObj obj )
{
	guchar *buf;
	gchar filename[BUFSIZ];
	guchar *s, *end;
	guchar *mime;
	gint len;

	if (loadFileToBuf(docObjGetDecomposedUrl(obj)->path, &buf, &len)) 
		return -1;

	/* Hack to handle .so in man pages */
	mime = docObjGetMimeType(obj);
	if (mime && !strcmp(mime, "application/x-troff-man")) {
	    if (!strncmp(buf, ".so ", 4)) {
		strncpy(filename, docObjGetDecomposedUrl(obj)->path,
		       sizeof(filename));
		if ((s = strrchr(filename, '/'))) {
		    *s = '\0';
		    if ((s = strrchr(filename, '/'))) {
			s++;
			*s = '\0';
			s = buf + 4;
			while (isspace(*s)) {
			    s++;
			}
			end = s;
			while (!isspace(*end)) {
			    end++;
			}
			*end = '\0';
			strcat(filename, s);
			g_free(buf);
			g_message(".so: %s -> %s",
				  docObjGetDecomposedUrl(obj)->path, filename);
			if (loadFileToBuf(filename, &buf, &len))
			    return -1;
		    }
		}
	    }
	}
    
	docObjSetRawData(obj, buf, len, TRUE);
	return 0;
}

static int
getHostByName(const char *host, struct in_addr *address)
{
    struct hostent *hostinfo;

    hostinfo = gethostbyname(host);
    if (!hostinfo)
	return 1;

    memcpy(address, hostinfo->h_addr_list[0], hostinfo->h_length);
    return 0;
}

gint
transportHTTP( docObj obj )
{
    struct sockaddr_in destPort;
    struct in_addr serverAddress;
    gchar buf[BUFSIZ];
    gchar *hostname;
    gchar *outbuf;
    guchar *copy;
    gchar *s;
    gchar *mimeType, *mimeTypeEnd;
    int bytes;
    int outbuflen;
    gint copylen;
    int sock;

    hostname = docObjGetDecomposedUrl(obj)->host;
    
    if (isdigit(hostname[0])) {
	if (!inet_aton(hostname, &serverAddress)) {
	    g_warning("unable to resolve host: %s", hostname);
	    return -1;
	}
    } else {
	if (getHostByName(hostname, &serverAddress)) {
	    g_warning("unable to resolve host: %s", hostname);
	    return -1;
	}
    }
    
    destPort.sin_family = AF_INET;
    destPort.sin_port = htons(80);
    destPort.sin_addr = serverAddress;

    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (connect(sock, (struct sockaddr *) &destPort, sizeof(destPort))) {
	g_warning("unable to connect to host: %s", hostname);
	return -1;
    }

    if (sizeof (buf) == g_snprintf(buf, sizeof buf,
				   "GET %s HTTP/1.0\r\nHost: %s\r\n\r\n",
				   docObjGetDecomposedUrl(obj)->path,
				   docObjGetDecomposedUrl(obj)->host)) {
        g_warning ("buffer too small");
	return -1;
    }
    write(sock, buf, strlen(buf));

    /* This is not efficient */
    outbuf = NULL;
    outbuflen = 0;
    while ((bytes = read(sock, buf, sizeof(buf))) > 0) {
	outbuf = g_realloc(outbuf, outbuflen + bytes);
	memcpy(outbuf + outbuflen, buf, bytes);
	outbuflen += bytes;
    }
    close(sock);

    if ((s = strstr(outbuf, "\n\n"))) {
	*s = '\0';
	s += 2;
    } else {
	s = strstr(outbuf, "\r\n\r\n");
	*s = '\0';
	s += 4;
    }

    /* Mime type */
    mimeType = strstr(outbuf, "Content-Type:");
    if (mimeType) {
	mimeType += 14;
	mimeTypeEnd = mimeType;
	while (! isspace(*mimeTypeEnd)) {
	    mimeTypeEnd++;
	}
	*mimeTypeEnd = '\0';
	docObjSetMimeType(obj, mimeType);
    }
    
    copylen = outbuflen - (s - outbuf);
    copy = g_malloc(copylen);
    memcpy(copy, s, copylen);

    docObjSetRawData(obj, copy, copylen, TRUE);
    g_free(outbuf);

    return 0;
}
