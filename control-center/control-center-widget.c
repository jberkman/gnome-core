/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */

error_t
control_panel_widget_init (char *app_id, struct argp *app_parser,
                           int argc, char **argv, unsigned int flags,
                           int *arg_index, char *argv0)
{
        error_t retval;

                if(!argv0)
                g_error("Invalid argv0 argument!\n");

        if(argv0[0]!='#')
                myinvoc = get_full_path(argv0);
        else
                myinvoc = g_strdup(argv0);
        if(!myinvoc)
                g_error("Invalid argv0 argument!\n");

        do_multi = (multi_applet!=FALSE);
        start_new_func = new_func;
        start_new_func_data = new_func_data;
        die_on_last = last_die;

        panel_corba_register_arguments();

        gnome_client_disable_master_connection ();
        ret = gnome_init(app_id,app_parser,argc,argv,flags,arg_index);

        if (!gnome_panel_applet_init_corba())
                g_error("Could not communicate with the panel\n");

        return retval;
}
