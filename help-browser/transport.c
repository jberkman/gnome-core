/* transport functions */

#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include <glib.h>

#include "transport.h"
#include "docobj.h"
#include "parseUrl.h"
#include "misc.h"

void
transport( docObj obj )
{
    /* XXX caching happens here.  I think. */
    (docObjGetTransportFunc(obj))(obj);
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
	gint    filenum;
	gchar   file[1024];

	gchar   prefix[1024];
	gchar   suffix[1024];
	gboolean doInfo;

	DIR     *d;
	struct  dirent *dirp;

	/* we have to handle info files specially */
	if (!strncmp(docObjGetRef(obj), "info:", 5)) {
		gchar *p, *pp, *r, *s;

		doInfo = TRUE;
		r = g_strdup(docObjGetDecomposedUrl(obj)->path);
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

		g_message("Looking for info files matching ->%s<- or ->%s<-",
			  p, pp);
		d = opendir(r);
		while (d && (dirp = readdir(d))) {
/*			g_message("transportFile(): %s %s %d",
			          dirp->d_name, p,
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
			strcpy(file, docObjGetDecomposedUrl(obj)->path);
		}

		/* test for existance */
		if (access(file, R_OK))
			/* if first file, fatal */
			if  (!filenum) {
				g_snprintf(buf, sizeof(buf), 
					   "<BODY>Error: unable to open "
					   "file %s</BODY>",file);
				docObjSetRawData(obj, g_strdup(buf), TRUE);
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

		filenum++;
	} while (doInfo);

	docObjSetRawData(obj, out, TRUE);
	return;
}

void
transportHTTP( docObj obj )
{
	char *argv[4];

	argv[0] = "lynx";
	argv[1] = "-source";
	argv[2] = docObjGetRef(obj);
	argv[3] = NULL;

	docObjSetRawData(obj, getOutputFrom(argv, NULL, 0), TRUE);
}

