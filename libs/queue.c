/*	PANDA -- a simple transaction monitor

Copyright (C) 2000-2002  Ogochan.

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
#include	<pthread.h>
#include	<stdio.h>
#include	"queue.h"
#include	"misc.h"
#include	"debug.h"

extern	Queue	*
NewQueue(void)
{
	Queue	*que;

	que = New(Queue);
	que->head = NULL;
	que->tail = NULL;
	pthread_cond_init(&que->isdata,NULL);
	pthread_mutex_init(&que->qlock,NULL);
	return	(que);
}

extern	void
EnQueue(
	Queue	*que,
	void	*data)
{
	QueueElement	*el;

dbgmsg(">EnQueue");
	el = New(QueueElement);
	el->data = data;
	pthread_mutex_lock(&que->qlock);
	el->prev = que->tail;
	el->next = NULL;
	if		(  que->tail  ==  NULL  ) {
		que->head = el;
	} else {
		que->tail->next = el;
	}
	que->tail = el;
	pthread_mutex_unlock(&que->qlock);
	pthread_cond_signal(&que->isdata);
dbgmsg("<EnQueue");
}

extern	void	*
DeQueue(
	Queue	*que)
{
	QueueElement	*el;
	void			*ret;

dbgmsg(">DeQueue");
	pthread_mutex_lock(&que->qlock);
	while	(  ( el = que->head )  ==  NULL  ) {
		dbgmsg("wait");
		pthread_cond_wait(&que->isdata,&que->qlock);
	}
	el = que->head;
	if		(  el->next  ==  NULL  ) {
		que->tail = NULL;
	} else {
		el->next->prev = NULL;
	}
	que->head = el->next;
	ret = el->data;
	pthread_mutex_unlock(&que->qlock);
	xfree(el);
dbgmsg("<DeQueue");
	return	(ret);
}

extern	Bool
IsQueue(
	Queue	*que)
{
	Bool	rc;

	pthread_mutex_lock(&que->qlock);
	rc = (  que->head  ==  NULL  ) ? FALSE : TRUE;
	pthread_mutex_unlock(&que->qlock);
	return	(rc);
}

extern	void
FreeQueue(
	Queue	*que)
{
	while	(  IsQueue(que)  ) {
		DeQueue(que);
	}
	pthread_mutex_destroy(&que->qlock);
	pthread_cond_destroy(&que->isdata);
	xfree(que);
}
