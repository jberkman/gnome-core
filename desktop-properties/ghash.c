#include "ghash.h"
#include <string.h>

guint g_hash_function_gcharp (const gchar *x)
{
  guint h = 0;
  gint g;

  while (*x != 0) {
    h = (h << 4) + *x++;
    if ((g = h & 0xf0000000) != 0)
      h = (h ^ (g >> 24)) ^ g;
  }

  return h;
}

gint g_hash_compare_gcharp (const gchar *a, const gchar *b)
{
     return !strcmp (a, b);
}



