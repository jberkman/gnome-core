/*###################################################################*/
/*##                       gqmenu (GNOME menu editor) 0.2.1        ##*/
/*###################################################################*/

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <gnome.h>
#include "iconsel.h"

	/* definitions */
#define GMENU_VERSION_MAJOR 0
#define GMENU_VERSION_MINOR 2
#define GMENU_VERSION_REV 1

typedef struct _Desktop_Data Desktop_Data;
struct _Desktop_Data
{
	gchar *path;
	gchar *name;
	gchar *comment;
	gchar *tryexec;
	gchar *exec;
	gchar *icon;
	gint terminal;
	gchar *doc;
	gchar *type;
	gint multiple_args;
	GtkWidget *pixmap;
	gint isfolder;
	gint expanded;
};

typedef struct _Misc_Dialog Misc_Dialog;
struct _Misc_Dialog
{
	GtkWidget *dialog;
	GtkWidget *entry;
};

char *homedir(int uid);
int isfile(char *s);
int isdir(char *s);
int filesize(char *s);
void get_current_dir();
void set_current_dir(char *s);

