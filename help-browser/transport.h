#ifndef _GNOME_HELP_TRANSPORT_H_
#define _GNOME_HELP_TRANSPORT_H_

#include "docobj.h"

enum _TransportMethod {TRANS_FILE, TRANS_HTTP,
		       TRANS_UNKNOWN, TRANS_UNRESOLVED};

typedef enum _TransportMethod TransportMethod;

void transport(docObj obj);

void transportHTTP(docObj obj);
void transportFile(docObj obj);
void transportUnknown(docObj obj);

#endif


