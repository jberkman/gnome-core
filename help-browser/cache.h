#ifndef _GNOME_HELP_CACHE_H_
#define _GNOME_HELP_CACHE_H_

#include <glib.h>

typedef struct _data_cache *DataCache;

DataCache newDataCache(guint maxMemSize, guint maxDiskSize,
		       GCacheDestroyFunc destroyFunc, gchar *file);

void destroyDataCache(DataCache cache);

gpointer lookupInDataCache(DataCache cache, gchar *key);
gpointer lookupInDataCacheWithLen(DataCache cache, gchar *key, gint *len);

/* addToDataCache() will strdup() the key, so you should free */
/* it if you need to.  The value is *not* copied, but it *is* */
/* passed to destroyFunc() when it falls off the stack.       */
void addToDataCache(DataCache cache, gchar *key, gpointer value, guint size);

void saveCache(DataCache cache);

#endif
