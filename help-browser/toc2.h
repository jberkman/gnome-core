#ifndef _GNOME_HELP_TOC_H_
#define _GNOME_HELP_TOC_H_

#include <gtk/gtk.h>
#include <glib.h>

#define TOC_MAN_TYPE   0
#define TOC_INFO_TYPE  1
#define TOC_GHELP_TYPE 2

struct _toc_config {
    char *path;
    int type;
};

struct _big_table_entry {
    gchar *name;      /* g_malloc() */
    gint type;
    gint expanded;    /* Only used for info */
    gint indirect;    /* Only used for info */
    gchar *section;   /* Only used for info */
    gchar ext;        /* Only used for man */
    gchar *filename;
};

typedef struct _toc *Toc;

typedef void (*TocCB) (gchar *ref);

Toc newToc(gchar *manPath, gchar *infoPath, gchar *ghelpPath,
	   TocCB callback);
void showToc(Toc toc);
void hideToc(Toc toc);

gchar *tocLookupInfo(Toc toc, gchar *name, gchar *anchor);
gchar *tocLookupMan(Toc toc, gchar *name, gchar ext);
gchar *tocLookupGhelp(Toc toc, gchar *name);

#endif
