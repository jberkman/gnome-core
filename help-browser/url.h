#ifndef _GNOME_HELP_URL_H_
#define _GNOME_HELP_URL_H_

#include "parseUrl.h"

enum _transportMethod {TRANS_FILE, TRANS_HTTP,
		       TRANS_UNKNOWN, TRANS_UNRESOLVED};
typedef enum _transportMethod transportMethod;

typedef void (*transportFunc) (void *obj);

struct _urlType {
	transportMethod   method;
	transportFunc     func;
	DecomposedUrl     u;
};

typedef struct _urlType urlType;
#endif

