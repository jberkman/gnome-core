/* handles url type references and retrieving the sorresponding doc data */

#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <glib.h>

#include "transport.h"
#include "docobj.h"
#include "misc.h"
#include "url.h"

#include "parseUrl.h"

static void transportUnknown( docObj *obj );
static void transportFile( docObj *obj );
static void transportHTTP( docObj *obj );

/* parse a URL into component pieces */
void
resolveURL( docObj *obj )
{
	gchar *s;

	g_return_if_fail( obj != NULL );
	g_return_if_fail( obj->ref != NULL );

	if (obj->url.u)
		return;

	/* HACKHACKHACK for info support */
	if (!strncmp(obj->ref, "info:", 5)) {
		gchar *r, *s;

		obj->url.u = g_malloc(sizeof(*(obj->url.u)));
		r = g_strdup(obj->ref+5);
		s = r + strlen(r) - 1;
		while (s > r && *s != '#')
			s--;
		if (*s == '#') {
			obj->url.u->anchor = g_strdup(s+1);
			*s = '\0';
		} else {
			obj->url.u->anchor = g_strdup("");
		}
		obj->url.u->access = g_strdup("file");
		obj->url.u->host   = g_strdup("");
		obj->url.u->path   = g_malloc(strlen(r) + 16);
		strcpy(obj->url.u->path, "/usr/info/");
		strcat(obj->url.u->path, r);
		
		g_free(r);
	} else if (isRelative(obj->ref)) {
	    printf("RELATIVE: %s\n", obj->ref);
		obj->url.u = decomposeUrlRelative(obj->ref, CurrentRef, &s);
		g_free(obj->ref);
		obj->ref = s;
        } else {
		obj->url.u = decomposeUrl(obj->ref);
	}

	printf("%s %s %s %s\n",obj->ref, obj->url.u->access, 
	       obj->url.u->path, obj->url.u->anchor);
	/* stupid test for transport types with currently understand */
	if (!strncmp(obj->url.u->access, "file", 4)) {
		obj->url.method = TRANS_FILE;
		obj->url.func   = (transportFunc)transportFile;
	} else if (!strncmp(obj->url.u->access, "http", 4)) {
		obj->url.method = TRANS_HTTP;
		obj->url.func   = (transportFunc)transportHTTP;
	} else {
		obj->url.method = TRANS_UNKNOWN;
		obj->url.func   = (transportFunc)transportUnknown;
	}
}

/* after calling resolveURL, we can now transport the doc */
void
transport( docObj *obj )
{
	(obj->url.func)(obj);
}

static void
transportUnknown( docObj *obj )
{
	gchar    s[513];

	g_snprintf(s, sizeof(s), "<BODY>Error: unable to resolve transport "
		   "method for requested URL:<br><b>%s</b></BODY>",obj->ref);
	obj->rawData = g_strdup(s);
	obj->freeraw = TRUE;
}

static void
transportFile( docObj *obj )
{
	gchar   *s=NULL, *out;
	gchar   buf[8193];
	gint    fd;
	gint    bytes;
	gint    total;
	gchar   *pipecmd=NULL;
	gint    filenum;
	gchar   file[1024];

	gchar   prefix[1024];
	gchar   suffix[1024];
	gboolean doInfo;

	DIR     *d;
	struct  dirent *dirp;

	/* we have to handle info files specially */
	if (!strncmp(obj->ref, "info:", 5)) {
		gchar *p, *pp, *r, *s;

		doInfo = TRUE;
		r = g_strdup(obj->url.u->path);
		s = r+strlen(r)-1;
		while (s > r && *s != '/')
			s--;
		if (*s == '/')
			*s++ = '\0';
		else
			return; /* VERY BAD - no file specified */

		p = g_strdup(s);
		pp = g_malloc(strlen(p)+2);
		strcpy(pp, p);
		strcat(pp, ".");

		printf("Looking for info files matching ->%s<- or ->%s<-\n",
		       p, pp);
		d = opendir(r);
		while (d && (dirp = readdir(d))) {
/*			printf("%s %s %d\n", dirp->d_name, p,
			       strncmp(dirp->d_name, p, strlen(p)));
*/
			if (!strncmp(dirp->d_name, pp, strlen(pp)) ||
			    !strcmp(dirp->d_name, p))
				break;
		}

		if (!d || !dirp) 
			return; /* no info file exists */

		/* setup prefix, suffix for info files          */
		/* various naming schemes used, so this is ugly */
		/* some like -> 'wget.info'                     */
		/* or        -> 'emacs.gz' 'emacs-1.gz', etc    */
		/* or even   -> 'libc.info.gz' 'libc.info-1.gz' */
                /* or        -> 'wget' 'wget-1'                 */

		/* see if we have a '.gz' on the end            */
		strcpy(prefix, r);
		strcat(prefix, "/");
		strcat(prefix, dirp->d_name);
		if (!strcmp(prefix+strlen(prefix)-3, ".gz")) {
			strcpy(suffix,".gz");
			*(prefix+strlen(prefix)-3) = '\0';
		} else {
			*suffix = '\0';
		}
		closedir(d);
		g_free(p);
		g_free(pp);
		g_free(r);
	} else {
		doInfo = FALSE;
		*prefix = '\0';
		*suffix = '\0';
	}

	filenum = 0;
	out = NULL;
	do {
		if (doInfo) {
			if (!filenum) 
				snprintf(file, sizeof(file),
					 "%s%s", prefix, suffix);
			else
				snprintf(file, sizeof(file),
					 "%s-%d%s", prefix, filenum, suffix);
		} else {
			strcpy(file, obj->url.u->path);
		}

		/* test for existance */
		if (access(file, R_OK))
			/* if first file, fatal */
			if  (!filenum) {
				g_snprintf(buf, sizeof(buf), 
					   "<BODY>Error: unable to open "
					   "file %s</BODY>",file);
				obj->rawData = g_strdup(buf);
				obj->freeraw = TRUE;
				return;
			} else {
				/* one of many, we just leave loop */
				break;
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
				obj->rawData = g_strdup(buf);
				obj->freeraw = TRUE;
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
		
		obj->freeraw = TRUE;
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

		filenum++;
	} while (doInfo);

	obj->rawData = out;
	return;
}


static void
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

