#include <gnome.h>
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

static gint compareItems(const void *a, const void *b);

GList *newGhelpTable(struct _toc_config *conf)
{
    char filename[BUFSIZ];
    DIR *d;
    struct dirent *dirp;
    GList *list = NULL;
    struct _big_table_entry *entry;
    char *lang;
    GList *lang_list = NULL;
    GList *temp = NULL;
    struct stat buf;
    int tmp_array_size = 256, tmp_array_elems = 0;
    struct _big_table_entry **tmp_array = g_new(struct _big_table_entry *,
                                                tmp_array_size);

    lang_list = gnome_i18n_get_language_list ("LC_MESSAGE");

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

		temp= lang_list;
		while (temp)
		  {
		    lang= (gchar*) temp->data;
		    snprintf (filename, sizeof(filename),
			      "%s/%s/%s/index.html",
			      conf->path, dirp->d_name, lang);

		    if (stat (filename, &buf) == 0)
		      break;

		    temp= temp->next;
		  }

		if (temp)
		  {
		    entry = g_malloc(sizeof(*entry));
		    entry->type = TOC_GHELP_TYPE;
		    entry->name = g_strdup(dirp->d_name);
		    entry->section = NULL;
		    entry->expanded = 1;
		    entry->ext = 0;
		    entry->filename = g_strdup(filename);

                    if (tmp_array_elems >= tmp_array_size) {
                        tmp_array_size *= 2;
                        tmp_array = g_realloc(tmp_array,
                                              (sizeof(*tmp_array)
                                               * tmp_array_size));
                    }
                    tmp_array[tmp_array_elems++] = entry;
		  }
	    }
	    closedir(d);
	}

	conf++;
    }

    /* FIXME: If would be much cooler if glib had a function to sort
       lists.  */
    qsort(tmp_array, (size_t) tmp_array_elems, sizeof(*tmp_array),
          compareItems);

    {
        int i;

        for (i = tmp_array_elems - 1; i >= 0; i--)
            list = g_list_prepend(list, tmp_array[i]);

        g_free(tmp_array);
    }

    return list;
}

static gint compareItems(const void *a, const void *b)
{
    struct _big_table_entry *e1 = *(struct _big_table_entry **)a;
    struct _big_table_entry *e2 = *(struct _big_table_entry **)b;

    return strcmp(e1->name, e2->name);
}
