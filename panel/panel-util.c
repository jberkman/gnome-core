#include <config.h>
#include <string.h>
#include <glib.h>
#include <fcntl.h>

#include "panel-util.h"

/* this function might be a slight overkill, but it should work
   perfect, hopefully it should be 100% buffer overrun safe too*/
char *
get_full_path(char *argv0)
{
	char buf[PATH_MAX+2];
	char *cmdbuf;
	int i;
#if 0
	int cmdsize=100;
	int fd[2];
#else
	FILE *fwhich;
#endif

	if(!argv0)
		return NULL;

	if(*argv0 == '/')
		return g_strdup(argv0);


	if(strchr(argv0,'/')) {
		char *curpath = getcwd(NULL,0);
		char *outbuf;

		if(!curpath)
			return NULL;

		outbuf = g_copy_strings(curpath,"/",argv0,NULL);
		free(curpath);

		realpath(outbuf,buf);
	
		return g_strdup(buf);
	}

#if 0
	if(pipe(fd) == -1)
		return NULL;

	/*dynamically reallocates cmdbuf until the command fits*/
	for(;;) {
		cmdbuf = (char *)g_malloc(cmdsize);
		if(g_snprintf(cmdbuf, cmdsize, "sh -c 'which %s > /dev/fd/%d'",
			      argv0,fd[1])>-1)
			break;
	
		g_free(cmdbuf);
		cmdsize*=2;
	}
		
	system(cmdbuf);
	g_free(cmdbuf);

	i=read(fd[0],buf,PATH_MAX+1);
	close(fd[0]);
	close(fd[1]);
	if(i <= 0)
		return NULL;

	buf[i]='\0';
	if(buf[i-1]=='\n')
		buf[i-1]='\0';

	if(buf[0]=='\0')
		return NULL;

#else
	cmdbuf = g_copy_strings("/usr/bin/which ", argv0, NULL);
	fwhich = popen(cmdbuf, "r");
	g_free(cmdbuf);
	
	if (fwhich == NULL)
	  return NULL;
	if (fgets(buf, PATH_MAX+1, fwhich) == NULL) {
	  	pclose(fwhich);
		return NULL;
	}

	pclose(fwhich);

	i = strlen(buf)-1;
	if(buf[i]=='\n')
		buf[i]='\0';

#endif
	return g_strdup(buf);
}

/*this is used to do an immediate move instead of set_uposition, which
queues one*/
void
move_window(GtkWidget *widget, int x, int y)
{
	gdk_window_set_hints(widget->window, x, y, 0, 0, 0, 0, GDK_HINT_POS);
	gdk_window_move(widget->window, x, y);
	/* FIXME: this should draw only the newly exposed area! */
	gtk_widget_draw(widget, NULL);
}



int
string_is_in_list(GList *list,char *text)
{
	for(;list!=NULL;list=g_list_next(list))
		if(strcmp(text,list->data)==0)
			return TRUE;
	return FALSE;
}
