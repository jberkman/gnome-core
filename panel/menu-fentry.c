/*
 * GNOME panel menu module.
 * (C) 1997, 1998, 1999, 2000 The Free Software Foundation
 * Copyright 2000 Eazel, Inc.
 *
 * Authors: Miguel de Icaza
 *          Federico Mena
 */

#include <config.h>
#include <ctype.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <gnome.h>

#include "panel-include.h"

/*#define PANEL_DEBUG 1*/

static GSList *dir_list = NULL;

static GMemChunk *file_chunk = NULL;
static GMemChunk *dir_chunk = NULL;

extern char *merge_main_dir;
extern int merge_main_dir_len;
extern char *merge_merge_dir;

void
init_fr_chunks (void)
{
	file_chunk = g_mem_chunk_create (FileRec, 64, G_ALLOC_AND_FREE);
	dir_chunk  = g_mem_chunk_create (DirRec,  16, G_ALLOC_AND_FREE);
}

/*reads in the order file and makes a list*/
static GSList *
get_presorted_from(GSList *list, char *dir)
{
	char buf[PATH_MAX+1];
	char *fname = g_concat_dir_and_file(dir, ".order");
	FILE *fp = fopen(fname, "r");
	
	if(!fp) {
		g_free(fname);
		return list;
	}
	while(fgets(buf, PATH_MAX+1, fp)!=NULL) {
		char *p = strchr(buf, '\n');
		if(p)
			*p = '\0';
		if( ! string_is_in_list(list, buf))
			list = g_slist_prepend(list, g_strdup(buf));
	}
	fclose(fp);
	g_free(fname);
	return list;
}

gboolean
fr_is_subdir (const char *dir, const char *superdir, int superdir_len)
{
	if (strncmp (dir, superdir, superdir_len-1) == 0 &&
	    (dir[superdir_len-1] == '/' ||
	     dir[superdir_len-1] == '\0')) {
		return TRUE;
	} else {
		return FALSE;
	}
}

char *
fr_get_mergedir (const char *dir)
{
	char *mergedir;
	if(merge_merge_dir != NULL &&
	   fr_is_subdir(dir, merge_main_dir, merge_main_dir_len)) {
		if (dir[merge_main_dir_len-1] == '/')
			mergedir =
				g_strconcat(merge_merge_dir,
					    &dir[merge_main_dir_len], NULL);
		else
			mergedir =
				g_strconcat(merge_merge_dir,
					    &dir[merge_main_dir_len-1], NULL);
	} else {
		mergedir = NULL;
	}

	return mergedir;
}

static GSList *
read_directory (GSList *list, char *menudir)
{
	DIR *dir;
	struct dirent *dent;

	dir = opendir (menudir);
	if (dir != NULL)  {
		while((dent = readdir (dir)) != NULL) {
			/* Skip over dot files, and duplicates */
			if (dent->d_name [0] != '.' &&
			    ! string_is_in_list(list, dent->d_name))
				list = g_slist_prepend(list, g_strdup(dent->d_name));
		}

		closedir(dir);
	}

	return list;
}

GSList *
get_files_from_menudir(char *menudir)
{
	GSList *list = NULL;
	char *mergedir;

	mergedir = fr_get_mergedir (menudir);
	
	list = get_presorted_from(list, menudir);
	list = read_directory(list, menudir);

	if(mergedir != NULL) {
		list = get_presorted_from(list, mergedir);
		list = read_directory(list, mergedir);
	}

	return g_slist_reverse(list);
}

char *
get_applet_goad_id_from_dentry(GnomeDesktopEntry *ii)
{
	int i;
	int constantlen = strlen("--activate-goad-server");

	g_return_val_if_fail(ii!=NULL,NULL);

	if (!ii->exec || !ii->type)
		return NULL;
	
	if(strcmp(ii->type, "PanelApplet")==0) {
		return g_strjoinv(" ", ii->exec);
	} else {
		/*this is here as a horrible hack since that's the way it
		  used to work, but now one should make the .desktop type
		  PanelApplet*/
		for(i=1;ii->exec[i];i++) {
			if(strncmp("--activate-goad-server",
				   ii->exec[i],constantlen)==0) {
				if(strlen(ii->exec[i])>constantlen)
					return g_strdup(&ii->exec[i][constantlen+1]);
				else
					return g_strdup(ii->exec[i+1]);
			}
		}
	}
	return NULL;
}

static void
fr_free(FileRec *fr, int free_fr)
{
	if(!fr) return;
	g_free(fr->name);
	g_free(fr->fullname);
	g_free(fr->comment);
	g_free(fr->icon);
	g_free(fr->goad_id);
	if(fr->parent && free_fr)
		fr->parent->recs = g_slist_remove(fr->parent->recs,fr);
	if(fr->type == FILE_REC_DIR) {
		DirRec *dr = (DirRec *)fr;
		GSList *li;
		for(li = dr->mfl; li!=NULL; li=g_slist_next(li))
			((MenuFinfo *)li->data)->fr = NULL;
		g_slist_free(dr->mfl);
		for(li = dr->recs; li!=NULL; li=g_slist_next(li)) {
			FileRec *ffr = li->data;
			ffr->parent = NULL;
			fr_free(ffr,TRUE);
		}
		g_slist_free(dr->recs);
	}
	if(free_fr) {
		dir_list = g_slist_remove(dir_list,fr);
		if (fr->type == FILE_REC_DIR)
			g_chunk_free (fr, dir_chunk);
		else
			g_chunk_free (fr, file_chunk);
	} else  {
		if(fr->type == FILE_REC_DIR)
			memset(fr,0,sizeof(DirRec));
		else
			memset(fr,0,sizeof(FileRec));
	}
}

static void
fr_fill_dir(FileRec *fr, int sublevels)
{
	GSList *flist;
	struct stat s;
	DirRec *dr = (DirRec *)fr;
	FileRec *ffr;
	time_t curtime = time(NULL);
	char *mergedir;
	
	g_return_if_fail(dr->recs==NULL);
	g_return_if_fail(fr!=NULL);
	g_return_if_fail(fr->name!=NULL);

	ffr = g_chunk_new0 (FileRec, file_chunk);
	ffr->type = FILE_REC_EXTRA;
	ffr->name = g_concat_dir_and_file(fr->name,".order");
	ffr->parent = dr;
	if (stat (ffr->name, &s) != -1)
		ffr->mtime = s.st_mtime;
	ffr->last_stat = curtime;
	dr->recs = g_slist_prepend(dr->recs,ffr);

	mergedir = fr_get_mergedir (fr->name);

	flist = get_files_from_menudir(fr->name);
	while(flist) {
		gboolean merged;
		char *short_name = flist->data;
		char *name = g_concat_dir_and_file(fr->name, short_name);
		GSList *tmp = flist;
		flist = flist->next;
		g_slist_free_1(tmp);

		merged = FALSE;
		if (stat (name, &s) == -1) {
			g_free(name);
			if (mergedir) {
				name = g_concat_dir_and_file(mergedir, short_name);
				if (stat (name, &s) == -1) {
					g_free(name);
					g_free(short_name);
					continue;
				}
				merged = TRUE;
			} else {
				g_free(short_name);
				continue;
			}
		}
		g_free(short_name);

		if (S_ISDIR (s.st_mode)) {
			if (merged)
				ffr = fr_read_dir(NULL, name, NULL, &s, sublevels-1);
			else 
				ffr = fr_read_dir(NULL, name, &s, NULL, sublevels-1);
			g_free(name);
			if(ffr) {
				ffr->merged = merged;
				dr->recs = g_slist_prepend(dr->recs,ffr);
			}
		} else {
			GnomeDesktopEntry *dentry;
			char *p = strrchr(name,'.');
			if (!p || (strcmp(p, ".desktop") != 0 &&
				   strcmp(p, ".kdelnk") != 0)) {
				g_free(name);
				continue;
			}

			dentry = gnome_desktop_entry_load(name);
			if(dentry) {
				ffr = g_chunk_new0 (FileRec, file_chunk);
				ffr->type = FILE_REC_FILE;
				ffr->merged = merged;
				ffr->name = name;
				ffr->mtime = s.st_mtime;
				ffr->last_stat = curtime;
				ffr->parent = dr;
				ffr->icon = dentry->icon;
				dentry->icon = NULL;
				ffr->fullname = dentry->name;
				ffr->comment  = g_strdup (dentry->comment);
				dentry->name = NULL;
				ffr->goad_id =
					get_applet_goad_id_from_dentry(dentry);
				gnome_desktop_entry_free(dentry);

				dr->recs = g_slist_prepend(dr->recs,ffr);
			} else
				g_free(name);
		}
	}
	dr->recs = g_slist_reverse(dr->recs);

	g_free(mergedir);
}

FileRec *
fr_read_dir(DirRec *dr, const char *mdir, struct stat *dstat, struct stat *merge_dstat, int sublevels)
{
	char *fname;
	struct stat s;
	FileRec *fr;
	time_t curtime = time(NULL);
	char *mergedir;
	
	g_return_val_if_fail(mdir!=NULL, NULL);

	mergedir = fr_get_mergedir (mdir);

	/*this will zero all fields*/
	if(dr == NULL) {
		dr = g_chunk_new0 (DirRec, dir_chunk);
		/* this must be set otherwise we may messup on
		   fr_free */
		dr->frec.type = FILE_REC_DIR;
	}
	fr = (FileRec *)dr;

	if(fr->last_stat < curtime-1) {
		if(dstat == NULL) {
			if (stat (mdir, &s) == -1) {
				fr_free(fr, TRUE);
				g_free(mergedir);
				return NULL;
			}

			fr->mtime = s.st_mtime;
		} else
			fr->mtime = dstat->st_mtime;

		if(mergedir) {
			if(merge_dstat == NULL) {
				if (stat (mergedir, &s) == -1) {
					dr->merge_mtime = 0;
				} else {
					dr->merge_mtime = s.st_mtime;
				}
			} else
				dr->merge_mtime = merge_dstat->st_mtime;
		}

		fr->last_stat = curtime;
	}

	fr->type = FILE_REC_DIR;
	g_free(fr->name);
	fr->name = g_strdup(mdir);

	s.st_mtime = 0;
	fname = g_concat_dir_and_file(mdir,".directory");
	if (dr->dentrylast_stat >= curtime-1 ||
	    stat (fname, &s) != -1) {
		GnomeDesktopEntry *dentry;
		dentry = gnome_desktop_entry_load(fname);
		if(dentry) {
			g_free(fr->icon);
			fr->icon = dentry->icon;
			dentry->icon = NULL;
			g_free(fr->fullname);
			fr->fullname = dentry->name;
			g_free(fr->comment);
			fr->comment = g_strdup (dentry->comment);
			dentry->name = NULL;
			gnome_desktop_entry_free(dentry);
		} else {
			g_free(fr->icon);
			fr->icon = NULL;
			g_free(fr->fullname);
			fr->fullname = NULL;
			g_free(fr->comment);
			fr->comment = NULL;
		}
		/*if we statted*/
		if(s.st_mtime)
			dr->dentrylast_stat = curtime;
		dr->dentrymtime = s.st_mtime;
	}
	g_free(fname);
	
	dir_list = g_slist_prepend(dir_list, fr);
	
	/*if this is a fake structure, so we don't actually look into
	  the directory*/
	if(sublevels > 0)
		fr_fill_dir(fr, sublevels);

	return fr;
}


FileRec *
fr_replace(FileRec *fr)
{
	char *tmp = fr->name;
	DirRec *par = fr->parent;
	
	g_assert(fr->type == FILE_REC_DIR);

	fr->parent = NULL;
	fr->name = NULL;
	fr_free(fr,FALSE);
	fr = fr_read_dir((DirRec *)fr, tmp, NULL, NULL, 1);
	if(fr)
		fr->parent = par;
	return fr;
}


FileRec *
fr_check_and_reread(FileRec *fr)
{
	DirRec *dr = (DirRec *)fr;
	FileRec *ret = fr;
	time_t curtime = time(NULL);

	g_return_val_if_fail(fr != NULL, fr);
	g_return_val_if_fail(fr->type == FILE_REC_DIR, fr);

	if(dr->recs == NULL) {
		fr_fill_dir(fr,1);
	} else {
		int reread = FALSE;
		int any_change = FALSE;
		struct stat ds;
		GSList *li;
		if (fr->last_stat < curtime-1) {
			if(stat(fr->name, &ds)==-1) {
				fr_free(fr,TRUE);
				return NULL;
			}
			if(ds.st_mtime != fr->mtime)
				reread = TRUE;

			if(dr->merge_mtime > 0) {
				char *mergedir = fr_get_mergedir (fr->name);
				if(mergedir != NULL) {
					if(stat(mergedir, &ds) >= 0 &&
					   ds.st_mtime != dr->merge_mtime)
						reread = TRUE;
					g_free(mergedir);
				}
			}
		}
		for(li = dr->recs; !reread && li!=NULL; li=g_slist_next(li)) {
			FileRec *ffr = li->data;
			DirRec *ddr;
			int r;
			char *p;
			struct stat s;

			switch(ffr->type) {
			case FILE_REC_DIR:
				ddr = (DirRec *)ffr;
				p = g_concat_dir_and_file(ffr->name,
							  ".directory");
				if (ddr->dentrylast_stat >= curtime-1) {
					g_free (p);
					break;
				}
				if(stat(p,&s)==-1) {
					if(dr->dentrymtime) {
						g_free(ffr->icon);
						ffr->icon = NULL;
						g_free(ffr->fullname);
						ffr->fullname = NULL;
						g_free(ffr->comment);
						ffr->comment = NULL;
						ddr->dentrymtime = 0;
						any_change = TRUE;
					}
					dr->dentrylast_stat = 0;
					g_free(p);
					break;
				}
				if(ddr->dentrymtime != s.st_mtime) {
					GnomeDesktopEntry *dentry;
					dentry = gnome_desktop_entry_load(p);
					if(dentry) {
						g_free(ffr->icon);
						ffr->icon = dentry->icon;
						dentry->icon = NULL;
						g_free(ffr->fullname);
						ffr->fullname = dentry->name;
						g_free(ffr->comment);
						ffr->comment = g_strdup (dentry->comment);
						dentry->name = NULL;
						gnome_desktop_entry_free(dentry);
					} else {
						g_free(ffr->icon);
						ffr->icon = NULL;
						g_free(ffr->fullname);
						ffr->fullname = NULL;
						g_free(ffr->comment);
						ffr->comment = NULL;
					}
					ddr->dentrymtime = s.st_mtime;
					dr->dentrylast_stat = curtime;
					any_change = TRUE;
				}
				g_free(p);
				break;
			case FILE_REC_FILE:
				if (ffr->last_stat >= curtime-1)
					break;
				if(stat(ffr->name,&s)==-1) {
					reread = TRUE;
					break;
				}
				if(ffr->mtime != s.st_mtime) {
					GnomeDesktopEntry *dentry;
					dentry = gnome_desktop_entry_load(ffr->name);
					if(dentry) {
						g_free(ffr->icon);
						ffr->icon = dentry->icon;
						dentry->icon = NULL;
						g_free(ffr->fullname);
						ffr->fullname = dentry->name;
						g_free(ffr->comment);
						ffr->comment = g_strdup (dentry->comment);
						dentry->name = NULL;
						gnome_desktop_entry_free(dentry);
					} else {
						reread = TRUE;
						break;
					}
					ffr->mtime = s.st_mtime;
					ffr->last_stat = curtime;
					any_change = TRUE;
				}
				break;
			case FILE_REC_EXTRA:
				if (ffr->last_stat >= curtime-1)
					break;
				r = stat(ffr->name,&s);
				if((r==-1 && ffr->mtime) ||
				   (r!=-1 && ffr->mtime != s.st_mtime))
					reread = TRUE;
				break;
			}
		}
		if(reread) {
			ret = fr_replace(fr);
		} else if(any_change) {
			GSList *li;
			for(li = dr->mfl; li!=NULL; li=g_slist_next(li))
				((MenuFinfo *)li->data)->fr = NULL;
			g_slist_free(dr->mfl);
			dr->mfl = NULL;
		}
	}
	return ret;
}

FileRec *
fr_get_dir(const char *mdir)
{
	GSList *li;
	g_return_val_if_fail(mdir!=NULL, NULL);
	for(li=dir_list; li!=NULL; li=g_slist_next(li)) {
		FileRec *fr = li->data;
		g_assert(fr!=NULL);
		g_assert(fr->name!=NULL);
		if(strcmp(fr->name, mdir)==0)
			return fr_check_and_reread(fr);
	}
	return fr_read_dir(NULL, mdir, NULL, NULL, 1);
}
