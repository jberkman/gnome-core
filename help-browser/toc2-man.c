#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <glib.h>
#include <libgnome/gnome-defs.h>
#include <libgnome/gnome-i18n.h>

#include "toc2.h"
#include "toc2-man.h"

static gint compareItems(struct _big_table_entry *a,
			 struct _big_table_entry *b);

static struct _man_sections {
    gchar ch;
    gchar *name;
    gint flag;
} man_sections[] = {
    { '1', N_("User Commands"), 0 },
    { '2', N_("System Calls"), 0 },
    { '3', N_("Library Functions"), 0 },
    { '4', N_("Special Files"), 0 },
    { '5', N_("File Formats"), 0 },
    { '6', N_("Games"), 0 },
    { '7', N_("Miscellaneous"), 0 },
    { '8', N_("Administration"), 0 },
    { '9', N_("man9"), 0 },
    { 'n', N_("mann"), 0 },
    { 'x', N_("manx"), 0 },
    { 0, NULL, 0 }
};

gchar *getManSection(gchar ch)
{
    struct _man_sections *p = man_sections;

    while (p->ch) {
	if (p->ch == ch) {
	    return _(p->name);
	}
	p++;
    }

    return (gchar *) "Unknown Section";
}

GList *newManTable(struct _toc_config *conf)
{
    struct _man_sections *p;
    gchar dirname[BUFSIZ];
    gchar filename[BUFSIZ];
    DIR *d;
    struct dirent *dirp;
    GList *list = NULL;
    struct _big_table_entry *entry;
    gchar *s;
    
    while (conf->path) {
	if (conf->type != TOC_MAN_TYPE) {
	    conf++;
	    continue;
	}
	p = man_sections;
	while (p->ch) {
	    snprintf(dirname, sizeof(dirname), "%s/man%c", conf->path, p->ch);
	    d = opendir(dirname);
	    if (d) {
	        while (d && (dirp = readdir(d))) {
		    if (! (strcmp("..", dirp->d_name) &&
			   strcmp(".", dirp->d_name))) {
		        continue;
		    }
		    /* Add to table */
		    entry = g_malloc(sizeof(*entry));
		    snprintf(filename, sizeof(filename),
			     "%s/%s", dirname, dirp->d_name);
		    entry->filename = g_strdup(filename);
		    
		    entry->name = g_strdup(dirp->d_name);
		    if ((s = strrchr(entry->name, '.'))) {
			*s = '\0';
		    }
		    
		    entry->type = TOC_MAN_TYPE;
		    entry->ext = p->ch;
		    entry->expanded = 1;
		    entry->section = NULL;

		    list = g_list_insert_sorted(list, entry,
						(GCompareFunc)compareItems);
		}
		if (d && dirp) {
		    p->flag = 1;
		}
		closedir(d);
	    }
	    
	    p++;
	}
	conf++;
    }


    return list;
}

/* Sort according to section/name */
static gint compareItems(struct _big_table_entry *a,
			 struct _big_table_entry *b)
{
    gint res;

    res = a->ext - b->ext;
    
    /* If they are the same, sort based on length of filename */
    if (!res) {
	res = strcmp(a->name, b->name);
    }
    
    return res;
}
