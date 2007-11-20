/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2000-2003 Ogochan & JMA (Japan Medical Association).
 * Copyright (C) 2004-2007 Ogochan.
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
extern	Bool	EnQueue(Queue *que, void *data);
extern	void	*DeQueue(Queue *que);
extern	void	*DeQueueTime(Queue *que, int ms);
extern	void	*PeekQueue(Queue *que);
extern	Bool	IsQueue(Queue *que);
extern	void	FreeQueue(Queue *que);
extern	void	OpenQueue(Queue *que);
extern	void	*GetElement(Queue *que);
extern	void	CloseQueue(Queue *que);
extern	void	RewindQueue(Queue *que);
extern	void	*WithdrawQueue(Queue *que);
extern	void	WaitQueue(Queue *que);
extern	void	WaitQueueTime(Queue *que, int ms);

#define	LockQueue(que)		pthread_mutex_lock(&(que)->qlock)
#define	UnLockQueue(que)	pthread_mutex_unlock(&(que)->qlock)
#define	ReleaseQueue(que)	pthread_cond_signal(&(que)->isdata)

#endif
