/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */

/* Copyright (C) 1998 Redhat Software Inc. 
 * Authors: Jonathan Blandford <jrb@redhat.com>
 */
#ifndef __CORBA_H__
#define __CORBA_H__

#include "control-center.h"

#ifdef __cplusplus
extern "C" {
#pragma }
#endif /* __cplusplus */



void control_center_corba_gtk_main_quit(void);
void control_center_corba_gtk_main (gint *argc, char **argv);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

