

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <glib.h>

#include "magic.h"
#include "docobj.h"
#include "parseUrl.h"
#include "misc.h"

/* move this somewhere with info code below */
struct _pair {
	gchar *name;
	gint  val;
};

typedef struct _pair *Pair;



void
resolveMagicURL( docObj obj )
{
	gchar   *ref=docObjGetRef(obj);

	if (!strncmp(ref, "info:", 5)) {
		gchar *p, *pp, indexfile[1024], *c;
		guchar  *data;
		DIR     *d;
		struct  dirent *dirp;
		DecomposedUrl   u=NULL;
		gint val;
		gchar buf[1024];
		gchar buf2[1024];
		gchar *convanchor=NULL;

		gchar *s;
		GList *indirect=NULL, *listptr;
	
		
		u = decomposeUrl( ref );

		p = g_strdup(u->path+1);
		pp = g_malloc(strlen(p)+2);
		strcpy(pp, p);
		strcat(pp, ".");

		d = opendir("/usr/info");
		c = NULL;
		while (d && (dirp = readdir(d))) {
			if (!strncmp(dirp->d_name, pp, strlen(pp)) ||
			    !strcmp(dirp->d_name, p)) {
				if (!c || (strlen(dirp->d_name) < strlen(c))) {
					if (c)
						g_free(c);
					c = g_strdup(dirp->d_name);
				}
			}
		}

		closedir(d);
		g_free(p);
		g_free(pp);
		if (!c) {
			freeDecomposedUrl( u );
			return; /* no info file exists */
		} else {
			snprintf(indexfile, sizeof(indexfile), 
				 "/usr/info/%s",c);
			g_free(c);
		}

		data = loadFileToBuf( indexfile );
		if (!data) {
			freeDecomposedUrl( u );
			return; /* no info file exists */
		}

		if (!(c=strstr(data,"\nIndirect:\n"))) {
			gchar buf[1024];

			snprintf(buf, sizeof(buf),
				 "file:%s#%s", indexfile,
				 (*(u->anchor)) ? u->anchor : "Top");
			docObjSetRef(obj, buf);
			docObjSetMimeType(obj, "application/x-info");
			g_free(data);
			freeDecomposedUrl( u );
			return; /* no info file exists */
		}

		/* handle those fun indirects, first skip header */
		c += 12;
		while (*c != '') {
			Pair  e;
			gchar *s;
			gchar *eoln;

			e = g_malloc(sizeof(*e));
			s = strchr(c, ':');
			if (!s) {
				g_error("Ran out of indirect entries!");
				break;
			}
			*s = '\0';
			e->name = c;
			c = s+1;
			eoln = strchr(c, '\n');
			if (!eoln) {
				g_error("Ran out of indirect entries!");
				break;
			}
			*eoln = '\0';
			e->val = strtol(c, NULL, 10);
			c = eoln+1;

                        indirect = g_list_append(indirect, e);
 		}

		/* assume tag table is AFTER indirect table */
		if (!(c=strstr(c,"\nTag Table:\n(Indirect)\n"))) {

			g_error("Where is the TAG TABLE!");

			freeDecomposedUrl( u );
			return;
		}

		/* handle those fun indirects, first skip header */
		/* also, defang the anchor of '_'                */
		c += 24;
		val = -1;
		convanchor = g_strdup((*(u->anchor)) ? u->anchor : "Top");

		while (*c != '') {
			gchar *s;
			gchar anc[257];
			gchar *eoln;

			c += 6;
			s = strchr(c, 0x7f);
			if (!s) {
				g_error("Invalid table entry");
				break;
			}
			*s = '\0';

			strncpy(anc, c, sizeof(anc));
			map_spaces_to_underscores(anc);

			if (strcmp(anc, convanchor)) {
				c = strchr(s+1, '\n')+1;
				continue;
			}
			c = s+1;
			eoln = strchr(c, '\n');
			if (!eoln) {
				g_error("Ran out of indirect entries!");
				break;
			}
			*eoln = '\0';
			val = strtol(c, NULL, 10);

			break;
 		}

		if (val < 0) {
			/* free the rest too! */
			freeDecomposedUrl(u);
			g_error("Help coulnt find requested tag!");
			return;
		}

		listptr=indirect;
		while (listptr) {
			if (((Pair)listptr->data)->val > val) {
				listptr = listptr->prev;
				break;
			}
			listptr = listptr->next;
		}

		if (!listptr) {
			/* free the rest too! */
			freeDecomposedUrl(u);
			g_error("Help coulnt find requested tag!");
			return;
		}

		/* guess the filename, will need to clean where '/usr/info' */
		/* comes from                                               */
		*buf2 = '\0';
		snprintf(buf, sizeof(buf), "/usr/info/%s",
			 ((Pair)listptr->data)->name);
		if (access(buf, R_OK)) {
			/* try a .gz on the end */
			snprintf(buf, sizeof(buf), "/usr/info/%s.gz",
				 ((Pair)listptr->data)->name);
			if (!access(buf, R_OK)) {
				snprintf(buf2, sizeof(buf2),
					 "file:%s#%s", buf,
					 (*(u->anchor)) ? u->anchor : "Top");
			}
		} else {
			snprintf(buf2, sizeof(buf2),
				 "file:%s#%s", buf,
				 (*(u->anchor)) ? u->anchor : "Top");
		}
		
		if (*buf2) {
		        g_message("url magic: %s -> %s", ref, buf2);
			docObjSetRef(obj, buf2);
			docObjSetMimeType(obj, "application/x-info");
		}

		g_list_foreach(indirect, (GFunc)g_free, NULL);
		g_free(data);
		g_list_free(indirect);

		if (u)
			freeDecomposedUrl(u);

	} else {
		/* blow if nothing interesting */
		return;
	}

}
