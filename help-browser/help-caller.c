#include <stdio.h>
#include <ctype.h>
#include <libgnorba/gnorba.h>

void Exception( CORBA_Environment* ev )
{
  switch( ev->_major )
    {
    case CORBA_SYSTEM_EXCEPTION:
      fprintf( stderr, "CORBA system exception %s.\n",
	       CORBA_exception_id(ev));
      exit ( 1 );
    case CORBA_USER_EXCEPTION:
      fprintf( stderr, "CORBA user exception: %s.\n",
	       CORBA_exception_id( ev ) );
      exit ( 1 );
    default:
      break;
    }
}

main(int argc, char* argv[])
{
  CORBA_ORB orb;
  CORBA_Environment ev;
  char* dummy_argv[2];
  gint  dummy_argc;
  CORBA_Object browser;
  gchar* ior;
  
  dummy_argv[0] = "help-caller";
  dummy_argv[1] = 0;
  dummy_argc = 1;

  CORBA_exception_init(&ev);
  
  orb = gnome_CORBA_init("help-caller", NULL, &dummy_argc, dummy_argv, 0, NULL, &ev);
  Exception(&ev);

  browser = goad_server_activate_with_repo_id(0, "help-browser", GOAD_ACTIVATE_REMOTE);

  if (browser == CORBA_OBJECT_NIL)
    {
      fprintf(stderr,"Cannot activate browser\n");
      exit(1);
    }
  fprintf(stderr,"gnome-help-caller gets browser at IOR='%s'\n", CORBA_ORB_object_to_string(orb, browser, &ev));
  
  help_browser_simple_browser_fetch_url(browser, "toc:", &ev);
}

					      
  
