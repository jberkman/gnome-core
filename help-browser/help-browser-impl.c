#include <stdio.h>

#include "help-browser.h"
#include "window.h"
/*** App-specific servant structures ***/
typedef struct {
   POA_help_browser_simple_browser servant;
   PortableServer_POA poa;
   HelpWindow         window;

} impl_POA_help_browser_simple_browser;

/*** Implementation stub prototypes ***/
static void impl_help_browser_simple_browser__destroy(impl_POA_help_browser_simple_browser * servant,
						    CORBA_Environment * ev);
void
 impl_help_browser_simple_browser_fetch_url(impl_POA_help_browser_simple_browser * servant,
					    CORBA_char * URL,
					    CORBA_Environment * ev);

void
 impl_help_browser_simple_browser_show_url(impl_POA_help_browser_simple_browser * servant,
					   CORBA_char * URL,
					   CORBA_Environment * ev);

/*** epv structures ***/
static PortableServer_ServantBase__epv impl_help_browser_simple_browser_base_epv =
{
   NULL,			/* _private data */
   (gpointer) & impl_help_browser_simple_browser__destroy,	/* finalize routine */
   NULL,			/* default_POA routine */
};
static POA_help_browser_simple_browser__epv impl_help_browser_simple_browser_epv =
{
   NULL,			/* _private */
   (gpointer) & impl_help_browser_simple_browser_fetch_url,

   (gpointer) & impl_help_browser_simple_browser_show_url,

};

/*** vepv structures ***/
static POA_help_browser_simple_browser__vepv impl_help_browser_simple_browser_vepv =
{
   &impl_help_browser_simple_browser_base_epv,
   &impl_help_browser_simple_browser_epv,
};

/*** Stub implementations ***/
help_browser_simple_browser 
impl_help_browser_simple_browser__create(PortableServer_POA poa, HelpWindow window, CORBA_Environment * ev)
{
   help_browser_simple_browser retval;
   impl_POA_help_browser_simple_browser *newservant;
   PortableServer_ObjectId *objid;

   newservant = g_new0(impl_POA_help_browser_simple_browser, 1);
   newservant->servant.vepv = &impl_help_browser_simple_browser_vepv;
   newservant->poa = poa;
   newservant->window = window;
   POA_help_browser_simple_browser__init((PortableServer_Servant) newservant, ev);
   objid = PortableServer_POA_activate_object(poa, newservant, ev);
   CORBA_free(objid);
   retval = PortableServer_POA_servant_to_reference(poa, newservant, ev);
   
   return retval;
}

/* You shouldn't call this routine directly without first deactivating the servant... */
static void
impl_help_browser_simple_browser__destroy(impl_POA_help_browser_simple_browser * servant, CORBA_Environment * ev)
{

   POA_help_browser_simple_browser__fini((PortableServer_Servant) servant, ev);
   g_free(servant);
}

void
impl_help_browser_simple_browser_fetch_url(impl_POA_help_browser_simple_browser * servant,
					   CORBA_char * URL,
					   CORBA_Environment * ev)
{
  fprintf(stderr,"gnome-help-browser: fetch_url called\n");
  helpWindowShowURL(servant->window, URL, TRUE, TRUE);
}

void
impl_help_browser_simple_browser_show_url(impl_POA_help_browser_simple_browser * servant,
					  CORBA_char * URL,
					  CORBA_Environment * ev)
{
  fprintf(stderr,"gnome-help-browser: show_url called\n");
  helpWindowShowURL(servant->window, URL, TRUE, TRUE);
}
