/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */

/* Copyright (C) 1998 Redhat Software Inc. 
 * Authors: Jonathan Blandford <jrb@redhat.com>
 */
#ifndef __CAPLET_MANAGER_H__
#define __CAPLET_MANAGER_H__

#include "control-center.h"
#include "tree.h"

#ifdef __cplusplus
extern "C" {
#pragma }
#endif /* __cplusplus */

void request_new_id (CORBA_Object *cco, CORBA_unsigned_long *xid, CORBA_short *id);
void launch_caplet (node_data *data);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
