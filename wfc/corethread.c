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
#include    <sys/types.h>
#include    <sys/socket.h>
#include	<sys/time.h>
#include	<sys/wait.h>
#include	<glib.h>
#include	<pthread.h>

#include	"types.h"

#include	"libmondai.h"
#include	"queue.h"
#include	"wfcdata.h"
#include	"wfc.h"
#include	"mqthread.h"
#include	"debug.h"

static	Queue	*CoreQueue;
extern	void
CoreEnqueue(
	SessionData	*data)
{
	EnQueue(CoreQueue,data);
}

static	void
CoreThread(
	void	*para)
{
	SessionData	*data;
	MQ_Node		*mq;

dbgmsg(">CoreThread");
	CoreQueue = NewQueue();
	do {
		data = (SessionData *)DeQueue(CoreQueue);
		dbgmsg("de queue");
		if		(  ( mq = g_hash_table_lookup(MQ_Hash,data->ld->info->name) )
				   !=  NULL  ) {
			MessageEnqueue(mq,data);
		}
	}	while	(  mq  !=  NULL  );
	pthread_exit(NULL);
dbgmsg("<CoreThread");
}

extern	void
StartCoreThread(void)
{
	static	pthread_t	core;
dbgmsg(">StartCoreThread");
	pthread_create(&core,NULL,(void *(*)(void *))CoreThread,NULL);
dbgmsg("<StartCoreThread");
}

