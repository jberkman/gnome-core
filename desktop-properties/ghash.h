#ifndef __G_HASH_H__
#define __G_HASH_H__

#include "glib.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

guint g_hash_function_gcharp (const gchar *);

gint g_hash_compare_gcharp (const gchar *, const gchar *);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
