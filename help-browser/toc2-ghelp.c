#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>

#include <glib.h>

#include "toc2.h"
#include "toc2-ghelp.h"

static gint compareItems(struct _big_table_entry *a,
			 struct _big_table_entry *b);

GList *newGhelpTable(struct _toc_config *conf)
{
    char filename[BUFSIZ];
    DIR *d;
    struct dirent *dirp;
    GList *list = NULL;
    struct _big_table_entry *entry;
    char *lang;

    while (conf->path) {
	if (conf->type != TOC_GHELP_TYPE) {
	    conf++;
	    continue;
	}

	d = opendir(conf->path);
	if (d) {
	    while ((dirp = readdir(d))) {
	        if (! (strcmp("..", dirp->d_name) &&
		       strcmp(".", dirp->d_name))) {
		    continue;
		}
			
		lang = getenv("LANGUAGE");
		if (!lang)
		    lang = getenv("LANG");
		if (!lang)
		    lang = "C";
		sprintf(filename, "%s/%s/%s/index.html",
			conf->path, dirp->d_name, lang);
		
		/* XXX need to traverse LANGUAGE to find right
		   topic.dat. In fact, this LANG stuff doesn't really
		   belong here at all */

		entry = g_malloc(sizeof(*entry));
		entry->type = TOC_GHELP_TYPE;
		entry->name = g_strdup(dirp->d_name);
		entry->section = NULL;
		entry->expanded = 1;
		entry->ext = 0;
		entry->filename = g_strdup(filename);

		list = g_list_insert_sorted(list, entry,
					    (GCompareFunc)compareItems);
	    }
	    closedir(d);
	}
	
	conf++;
    }

    return list;
}

static gint compareItems(struct _big_table_entry *a,
			 struct _big_table_entry *b)
{

    return strcmp(a->name, b->name);
}
