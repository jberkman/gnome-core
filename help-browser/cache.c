/* Caching functions for ghelp */

#include <stdlib.h>
#include <glib.h>
#include "cache.h"

struct _data_cache {
    guint maxDataSize;
    guint maxEntryCount;

    guint dataSize;
    guint entryCount;

    GHashTable *hashTable;
    GList *queue;

    GCacheDestroyFunc destroyFunc;
};

struct _data_cache_entry {
    gchar *key;
    gpointer value;
    guint size;
};

static void freeEntry(struct _data_cache_entry *entry, DataCache cache);
static void removeElement(DataCache cache);

DataCache newDataCache(guint maxDataSize, guint maxEntryCount,
		       GCacheDestroyFunc destroyFunc)
{
    DataCache res;

    res = (DataCache)malloc(sizeof *res);
    res->maxDataSize = maxDataSize;
    res->maxEntryCount = maxEntryCount;
    res->destroyFunc = destroyFunc;

    res->dataSize = 0;
    res->entryCount = 0;

    res->hashTable = g_hash_table_new(g_str_hash, g_str_equal);
    res->queue = NULL;

    return res;
}

static void freeEntry(struct _data_cache_entry *entry, DataCache cache)
{
    if (cache->destroyFunc) {
	(cache->destroyFunc)(entry->value);
    }
    g_free(entry->key);
    g_free(entry);
}

void destroyDataCache(DataCache cache)
{
    g_hash_table_destroy(cache->hashTable);
    g_list_foreach(cache->queue, (GFunc)freeEntry, (gpointer)cache);
    g_list_free(cache->queue);
    free(cache);
}

gpointer lookupInDataCache(DataCache cache, gchar *key)
{
    struct _data_cache_entry *hit;
    
    if (! (hit = g_hash_table_lookup(cache->hashTable, key))) {
	return NULL;
    }

    /* Let's move this element to the end of the list */
    /* so it won't get tossed soon.                   */
    cache->queue = g_list_remove(cache->queue, hit);
    cache->queue = g_list_append(cache->queue, hit);

    return hit->value;
}

void addToDataCache(DataCache cache, gchar *key, gpointer value, guint size)
{
    struct _data_cache_entry *hit;

    hit = g_new(struct _data_cache_entry, 1);
    hit->key = g_strdup(key);
    hit->value = value;
    hit->size = size;

    cache->queue = g_list_append(cache->queue, hit);
    g_hash_table_insert(cache->hashTable, hit->key, hit);
    cache->dataSize += size;
    cache->entryCount++;

    /* If we have too much stuff in the cache, clean up a bit */
    if (cache->maxEntryCount) {
	if (cache->entryCount > cache->maxEntryCount) {
	    removeElement(cache);
	}
    }

    if (cache->maxDataSize) {
	while (cache->maxDataSize < cache->dataSize) {
	    removeElement(cache);
	}
    }
}

static void removeElement(DataCache cache)
{
    GList *top;
    struct _data_cache_entry *topItem;

    /* Remove from queue */
    top = cache->queue;
    cache->queue = g_list_remove_link(cache->queue, top);

    /* Remove from hash table */
    topItem = top->data;
    g_hash_table_remove(cache->hashTable, topItem->key);
    
    cache->dataSize -= topItem->size;
    cache->entryCount--;

    /* Free all associated memory */
    g_list_free(top);
    freeEntry(topItem, cache);
}
