#include <glib.h>

typedef struct _data_cache *DataCache;

DataCache newDataCache(guint maxDataSize, guint maxEntryCount,
		       GCacheDestroyFunc destroyFunc);

void destroyDataCache(DataCache cache);

gpointer lookupInDataCache(DataCache cache, gchar *key);

/* addToDataCache() will strdup() the key, so you should free */
/* it if you need to.  The value is *not* copied, but it *is* */
/* passed to destroyFunc() when it falls off the stack.       */
void addToDataCache(DataCache cache, gchar *key, gpointer value, guint size);
