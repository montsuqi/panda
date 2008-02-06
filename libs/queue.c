/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2000-2003 Ogochan & JMA (Japan Medical Association).
 * Copyright (C) 2004-2008 Ogochan.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

/*
#define	DEBUG
#define	TRACE
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include	<sys/types.h>
#include	<sys/time.h>
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

extern	Bool
EnQueue(
	Queue	*que,
	void	*data)
{
	QueueElement	*el;
	Bool	rc;

ENTER_FUNC;
	if		(	(  que             !=  NULL  )
			&&	(  LockQueue(que)  ==  0     ) ) {
		el = New(QueueElement);
		el->data = data;
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
		rc = TRUE;
	} else {
		rc = FALSE;
	}
LEAVE_FUNC;
	return	(rc);
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

extern	void
WaitQueueTime(
	Queue	*que,
	int		ms)
{
	struct	timespec	wtime;
	struct	timeval	tv;

	if		(  ms  ==  0  ) {
		WaitQueue(que);
	} else {
		if	(  que->head  ==  NULL  ) {
			dbgmsg("wait");
			gettimeofday(&tv,NULL);
			wtime.tv_sec = tv.tv_sec + ms / 1000;
			wtime.tv_nsec = tv.tv_usec * 1000 + ( ms % 1000 ) * 1000000;
			pthread_cond_timedwait(&que->isdata,&que->qlock,&wtime);
		}
	}
}

extern	void	*
DeQueue(
	Queue	*que)
{
	QueueElement	*el;
	void			*ret;

ENTER_FUNC;
	if		(	(  que             !=  NULL  )
			&&	(  LockQueue(que)  ==  0     ) ) {
		WaitQueue(que);
		el = que->head;
		if		(  el->next  ==  NULL  ) {
			que->tail = NULL;
		} else {
			el->next->prev = NULL;
		}
		que->head = el->next;
		ret = el->data;
		xfree(el);
		UnLockQueue(que);
	} else {
		ret = NULL;
	}
LEAVE_FUNC;
	return	(ret);
}

extern	void	*
DeQueueTime(
	Queue	*que,
	int		ms)
{
	QueueElement	*el;
	void			*ret;

ENTER_FUNC;
	if		(	(  que             !=  NULL  )
			&&	(  LockQueue(que)  ==  0     ) ) {
		WaitQueueTime(que,ms);
		if		(  ( el = que->head )  !=  NULL  ) {
			if		(  el->next  ==  NULL  ) {
				que->tail = NULL;
			} else {
				el->next->prev = NULL;
			}
			que->head = el->next;
			ret = el->data;
			xfree(el);
		} else {
			ret = NULL;
		}
		UnLockQueue(que);
	} else {
		ret = NULL;
	}
LEAVE_FUNC;
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

	if		(  que  !=  NULL  ) {
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

ENTER_FUNC;
	if		(  que  !=  NULL  ) {
		if		(  ( el = que->curr )  !=  NULL  ) {
			que->curr = que->curr->next;
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
		} else {
			ret = NULL;
		}
	} else {
		ret = NULL;
	}
LEAVE_FUNC;
	return	(ret);
}

extern	void	*
PeekQueue(
	Queue	*que)
{
	QueueElement	*el;
	void			*ret;

ENTER_FUNC;
	if		(  que  !=  NULL  ) {
		if		(  ( el = que->head )  ==  NULL  ) {
			ret = NULL;
		} else {
			ret = el->data;
		}
	} else {
		ret = NULL;
	}
LEAVE_FUNC;
	return	(ret);
}

extern	Bool
IsQueue(
	Queue	*que)
{
	Bool	rc;

	if		(  que  !=  NULL  ) {
		rc = (  que->head  ==  NULL  ) ? FALSE : TRUE;
	} else {
		rc = FALSE;
	}
	return	(rc);
}

extern	void
FreeQueue(
	Queue	*que)
{
ENTER_FUNC;
	if		(  que  !=  NULL  ) {
		while	(  IsQueue(que)  ) {
			WithdrawQueue(que);
		}
		pthread_mutex_destroy(&que->qlock);
		pthread_cond_destroy(&que->isdata);
		xfree(que);
	}
LEAVE_FUNC;
}
