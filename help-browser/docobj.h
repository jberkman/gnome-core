#ifndef _GNOME_HELP_DOCOBJ_H_
#define _GNOME_HELP_DOCOBJ_H_

#include <glib.h>

#include "parseUrl.h"

typedef struct _docObj *docObj;
typedef void (*TransportFunc) (docObj obj);

docObj docObjNew(gchar *ref);
void docObjFree(docObj obj);
void docObjResolveURL(docObj obj, gchar *currentRef);

void   docObjSetRef(docObj obj, gchar *ref);
gchar *docObjGetRef(docObj obj);
gchar *docObjGetAbsoluteRef(docObj obj);
gchar *docObjGetMimeType(docObj obj);
gchar *docObjGetRawData(docObj obj);
gchar *docObjGetConvData(docObj obj);
DecomposedUrl docObjGetDecomposedUrl(docObj obj);
TransportFunc docObjGetTransportFunc(docObj obj);

void docObjSetMimeType(docObj obj, gchar *s);
void docObjSetRawData(docObj obj, gchar *s, gboolean freeit);
void docObjSetConvData(docObj obj, gchar *s, gboolean freeit);

#endif





