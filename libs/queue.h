/*	PANDA -- a simple transaction monitor

Copyright (C) 2001-2003 Ogochan & JMA (Japan Medical Association).

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

#ifndef	_QUEUE_H
#define	_QUEUE_H
#include	"types.h"
#include	<pthread.h>

typedef	struct _QueueElement	{
	struct	_QueueElement	*next
				,			*prev;
	void	*data;
}	QueueElement;

typedef	struct {
	pthread_mutex_t	qlock;
	pthread_cond_t	isdata;
	QueueElement	*head
	,				*tail
	,				*curr;
}	Queue;

#undef	GLOBAL
#ifdef	MAIN
#define	GLOBAL		/*	*/
#else
#define	GLOBAL		extern
#endif

extern	Queue	*NewQueue(void);
extern	void	EnQueue(Queue *que, void *data);
extern	void	*DeQueue(Queue *que);
extern	void	*PeekQueue(Queue *que);
extern	Bool	IsQueue(Queue *que);
extern	void	FreeQueue(Queue *que);
extern	void	OpenQueue(Queue *que);
extern	void	*GetElement(Queue *que);
extern	void	CloseQueue(Queue *que);
extern	void	RewindQueue(Queue *que);
extern	void	*WithdrawQueue(Queue *que);
extern	void	WaitQueue(Queue *que);

#define	LockQueue(que)		pthread_mutex_lock(&(que)->qlock)
#define	UnLockQueue(que)	pthread_mutex_unlock(&(que)->qlock)
#define	ReleaseQueue(que)	pthread_cond_signal(&(que)->isdata)

#endif
