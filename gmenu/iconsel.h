/*###################################################################*/
/*##                     gnome icon selection widget               ##*/
/*###################################################################*/

/* An example to use this widget:
 *
 * Call this:
 *
 * icon_selection_dialog( gchar *path , gchar *filename, Boolean fullpath, (function) return_func );
 *
 * path        - the directory containing the icons
 * filename    - the initial icon to select (highlighted), if NULL or file does not exist,
 *               the first available icon is selected.
 * fullpath    - if TRUE, returns the complete path to the selected icon (/usr/share/pixmaps/icon.xmp)
 *               if FALSE, only returns the filename (icon.xpm)
 * return_func - the function to call and send the icon data to when an Icon is selected.
 *
 * Example:
 *  calling function:
 *   icon_selection_dialog( "/usr/share/pixmaps" , "gnome.xpm", FALSE, icon_cb );
 *
 *  return function:
 *   void icon_cb(void *data)
 *   {
 *      gchar *icon = data;
 *      g_print("icon = %s\n",icon);
 *      g_free(icon);
 *   }
 *
 * (Don't forget to free the returned data when you are done with it)
 */

GtkWidget * icon_selection_dialog( gchar *path, gchar *file, gint fullpath, void (*func)(void *));

