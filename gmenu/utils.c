/*
 * This file is a part of gmenu, the GNOME panel menu editor.
 *
 * File: utils.c
 *
 * This file contains some small utility routines that are used
 * throughout gmenu, and that should probably exist in some helper
 * library such as libgnome or glib.
 *
 * Authors: John Ellis <johne@bellatlantic.net>
 *          Nat Friedman <nat@nat.org>
 */

#include "gmenu.h"

/*
 * Here is how you can expect basename_n() to behave:
 *    basename_n("/a/b/c/d", 1) ==> "d"
 *    basename_n("/a/b/c/d", 2) ==> "c/d"
 *    basename_n("/a/b/c/d", 3) ==> "b/c/d"
 *    and so on.
 * */
char *
basename_n(char *path, int n)
{
  char *p;
  int i;

  p = path + strlen(path);

  for (i = 0 ; i < n ; i ++)
    {
      p--;


      while (p >= path && *p != '/')
	p--;

      if (p < path)
	break;
    }
  return g_strdup(p + 1);
}

gint isfile(char *s)
{
   struct stat st;

   if ((!s)||(!*s)) return 0;
   if (stat(s,&st)<0) return 0;
   if (S_ISREG(st.st_mode)) return 1;
   return 0;
}

gint isdir(char *s)
{
   struct stat st;
   
   if ((!s)||(!*s)) return 0;
   if (stat(s,&st)<0) return 0;
   if (S_ISDIR(st.st_mode)) return 1;
   return 0;
}

gchar *filename_from_path(char *t)
{
        gchar *p;

        p = t + strlen(t);
        while(p > &t[0] && p[0] != '/') p--;
        p++;
        return p;
}

gchar *strip_one_file_layer(char *t)
{
	gchar *ret;
	gchar *p;

	ret = strdup(t);
	p = ret + strlen(ret);
        while(p > &ret[0] && p[0] != '/') p--;
	if (strcmp(ret,p) != 0) p[0] = '\0';
        return ret;
}

gchar *check_for_dir(char *d)
{
	if (!g_file_exists(d))
		{
		g_print(_("creating user directory: %s\n"), d);
		if (mkdir( d, 0755 ) < 0)
			{
			g_print(_("unable to create user directory: %s\n"), d);
			g_free(d);
			d = NULL;
			}
		}
	return d;
}

/* this function returns the correct path to a file given multiple paths, it
   returns null if neither is correct. The returned pointer points to a string
   that is freed each time this function is called */
gchar *correct_path_to_file(gchar *path1, gchar *path2, gchar *filename)
{
	static gchar *correct_path = NULL;

	if (correct_path) g_free(correct_path);
	correct_path = NULL;

	correct_path = g_strconcat(path1, "/", filename, NULL);
	if (isfile(correct_path)) return correct_path;
	g_free(correct_path);

	correct_path = g_strconcat(path2, "/", filename, NULL);
	if (isfile(correct_path)) return correct_path;
	g_free(correct_path);

	correct_path = NULL;
	return correct_path;
}
