#ifndef EXEC_QUEUE_H
#define EXEC_QUEUE_H

#include <gtk/gtk.h>

BEGIN_GNOME_DECLS

/* this applet has finished loading, if it was the one we were waiting
   on, start the next applet */
void exec_queue_done(int applet_id);

void exec_prog(int applet_id, char *path, char *param);

typedef struct _AppletChild AppletChild;
/*used in the SIGCHLD handler and as a list of started applets*/
struct _AppletChild {
	int applet_id;
	pid_t pid;
};

END_GNOME_DECLS

#endif
