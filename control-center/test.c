#include "control-center-widget.h"

void
main(gint argc, gchar **argv)
{
  GtkWidget *cctest;
  control_center_init ("test cp widget",
			      NULL,
			      argc,
			      argv,
			      0,
			      NULL);
  
  //  cctest = control_center_widget_new ();
  //  gtk_widget_show (cctest);
  control_center_gtk_main();
}
