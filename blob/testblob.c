/*	PANDA -- a simple transaction monitor

Copyright (C) 2000-2003 Ogochan & JMA (Japan Medical Association).

This module is part of PANDA.

	PANDA is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY.  No author or distributor accepts responsibility
to anyone for the consequences of using it or for whether it serves
any particular purpose or works at all, unless he says so in writing.
Refer to the GNU General Public License for full details. 

	Everyone is granted permission to copy, modify and redistribute
PANDA, but only under the conditions described in the GNU General
Public License.  A copy of this license is supposed to have been given
to you along with PANDA so you can know your rights and
responsibilities.  It should be in a file named COPYING.  Among other
things, the copyright notice and this notice must be preserved on all
copies. 
*/

/*
#define	DEBUG
#define	TRACE
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<string.h>
#include    <sys/types.h>
#include	<sys/time.h>
#include	<glib.h>
#include	<pthread.h>

#include	"types.h"

#include	"libmondai.h"
#include	"struct.h"
#include	"blob.h"
#include	"message.h"
#include	"debug.h"

#define	BLOB_FILE	"."
#define	OB_NUMBER	10

extern	int
main(
	int		argc,
	char	**argv)
{
	BLOB_Space	*blob;
	BLOB_State	*state;
	MonObjectType	obj[OB_NUMBER];
	int		i;
	char	buff[100];

	blob = InitBLOB(BLOB_FILE);
	sleep(1);
	FinishBLOB(blob);
	sleep(1);
	blob = InitBLOB(BLOB_FILE);
	state = ConnectBLOB(blob);
	for	( i = 0 ; i < OB_NUMBER ; i ++ ) {
		obj[i] = NewBLOB(state,BLOB_OPEN_WRITE);
	}

	for	( i = 0 ; i < 10 ; i ++ ) {
		sprintf(buff,"%d\n%d\n",i,i);
		WriteBLOB(state,obj[i],buff,strlen(buff));
	}

	for	( i = 0 ; i < 10 ; i ++ ) {
		CloseBLOB(state,obj[i]);
	}

	for	( i = 0 ; i < 10 ; i ++ ) {
		OpenBLOB(state,obj[i],BLOB_OPEN_WRITE|BLOB_OPEN_APPEND);
	}

	for	( i = 0 ; i < 10 ; i += 2 ) {
		sprintf(buff,"add %d\n",i);
		WriteBLOB(state,obj[i],buff,strlen(buff));
	}

	for	( i = 0 ; i < 10 ; i ++ ) {
		CloseBLOB(state,obj[i]);
	}
	DisConnectBLOB(state);
	FinishBLOB(blob);
	return	(0);
}
