/* mico-glue.h - Glue connecting MICO to C.  */

#ifndef __MICO_GLUE_H__
#define __MICO_GLUE_H__

BEGIN_GNOME_DECLS	

void panel_corba_gtk_main (char *service_name);

void panel_corba_clean_up(void);

int send_applet_session_save (const char *ior, int id,
			       const char *cfgpath,
			       const char *globcfgpath);
void send_applet_change_orient (const char *ior, int id,  int orient);
void send_applet_do_callback (const char *ior, int id, char *callback_name);
void send_applet_start_new_applet (const char *ior, char *param);

END_GNOME_DECLS

#endif /* __MICO_GLUE_H__ */
