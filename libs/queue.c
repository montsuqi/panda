/*	PANDA -- a simple transaction monitor

Copyright (C) 2000-2003  Ogochan.

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
#include	<sys/types.h>
#include	<pthread.h>
#include	<stdio.h>
#include	"libmondai.h"
#include	"queue.h"
#include	"debug.h"

extern	Queue	*
NewQueue(void)
{
	Queue	*que;

	que = New(Queue);
	que->head = NULL;
	que->tail = NULL;
	que->curr = NULL;
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
	LockQueue(que);
	el->prev = que->tail;
	el->next = NULL;
	if		(  que->tail  ==  NULL  ) {
		que->head = el;
	} else {
		que->tail->next = el;
	}
	que->tail = el;
	UnLockQueue(que);
	ReleaseQueue(que);
dbgmsg("<EnQueue");
}

extern	void
WaitQueue(
	Queue	*que)
{
	while	(  que->head  ==  NULL  ) {
		dbgmsg("wait");
		pthread_cond_wait(&que->isdata,&que->qlock);
	}
}

extern	void	*
DeQueue(
	Queue	*que)
{
	QueueElement	*el;
	void			*ret;

dbgmsg(">DeQueue");
	LockQueue(que);
	WaitQueue(que);
	el = que->head;
	if		(  el->next  ==  NULL  ) {
		que->tail = NULL;
	} else {
		el->next->prev = NULL;
	}
	que->head = el->next;
	ret = el->data;
	UnLockQueue(que);
	xfree(el);
dbgmsg("<DeQueue");
	return	(ret);
}

extern	void
OpenQueue(
	Queue	*que)
{
	LockQueue(que);
	que->curr = NULL;
}

extern	void
RewindQueue(
	Queue	*que)
{
	que->curr = NULL;
}

extern	void	*
GetElement(
	Queue	*que)
{
	void	*data;

	if		(  que->curr  ==  NULL  ) {
		que->curr = que->head;
	} else {
		que->curr = que->curr->next;
	}
	if		(  que->curr  !=  NULL  ) {
		data = que->curr->data;
	} else {
		data = NULL;
	}
	return	(data);
}

extern	void
CloseQueue(
	Queue	*que)
{
	que->curr = NULL;
	UnLockQueue(que);
}

extern	void	*
WithdrawQueue(
	Queue	*que)
{
	QueueElement	*el;
	void			*ret;

dbgmsg(">WithdrawQueue");
	el = que->curr;
	if		(  el->next  !=  NULL  ) {
		el->next->prev = el->prev;
	}
	if		(  el->prev  !=  NULL  ) {
		el->prev->next = el->next;
	}
	if		(  el  ==  que->tail  ) {
		que->tail = el->prev;
	}
	if		(  el  ==  que->head  ) {
		que->head = el->next;
	}

	ret = el->data;
	xfree(el);
dbgmsg("<WithdrawQueue");
	return	(ret);
}

extern	void	*
PeekQueue(
	Queue	*que)
{
	QueueElement	*el;
	void			*ret;

dbgmsg(">PeekQueue");
	pthread_mutex_lock(&que->qlock);
	if		(  ( el = que->head )  ==  NULL  ) {
		ret = NULL;
	} else {
		ret = el->data;
	}
	pthread_mutex_unlock(&que->qlock);
dbgmsg("<PeekQueue");
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
