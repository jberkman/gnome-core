#ifndef MENU_FENTRY_H
#define MENU_FENTRY_H

#include <sys/stat.h>
#include <unistd.h>

BEGIN_GNOME_DECLS

typedef struct _MenuFinfo MenuFinfo;
typedef struct _FileRec FileRec;
typedef struct _DirRec DirRec;

struct _MenuFinfo {
	char *menudir;
	gboolean applets;
	char *dir_name;
	char *pixmap_name;	
	gboolean fake_menu;
	gboolean title;
	FileRec *fr;
};

enum {
	FILE_REC_FILE, /*.desktop file record*/
	FILE_REC_DIR,  /*directory*/
	FILE_REC_EXTRA /*just check the mtime*/
};

struct _FileRec {
	int type;
	gboolean merged;
	char *name;
	char *comment;
	char *fullname;
	char *icon;
	char *goad_id;
	DirRec *parent;
	time_t mtime;
	time_t last_stat;
};

struct _DirRec {
	FileRec frec;
	time_t dentrymtime;
	time_t dentrylast_stat;
	time_t merge_mtime;
	GSList *recs; /*records for directories*/
	GSList *mfl;  /*records of menus using this record*/
};

char * get_applet_goad_id_from_dentry(GnomeDesktopEntry *ii);

GSList * get_files_from_menudir(char *menudir);

FileRec * fr_read_dir(DirRec *dr, const char *mdir, struct stat *dstat, struct stat *merge_dstat, int sublevels);
FileRec * fr_replace(FileRec *fr);
FileRec * fr_check_and_reread(FileRec *fr);
FileRec * fr_get_dir(const char *mdir);
gboolean fr_is_subdir (const char *dir, const char *superdir, int superdir_len);
char * fr_get_mergedir (const char *dir);

void init_fr_chunks (void);
END_GNOME_DECLS

#endif
