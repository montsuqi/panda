/*	OSEKI -- Object Store Engine Kernel Infrastructure

Copyright (C) 2004 Ogochan.

This module is part of OSEKI.

	OSEKI is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY.  No author or distributor accepts responsibility
to anyone for the consequences of using it or for whether it serves
any particular purpose or works at all, unless he says so in writing.
Refer to the GNU General Public License for full details. 

	Everyone is granted permission to copy, modify and redistribute
OSEKI, but only under the conditions described in the GNU General
Public License.  A copy of this license is supposed to have been given
to you along with OSEKI so you can know your rights and
responsibilities.  It should be in a file named COPYING.  Among other
things, the copyright notice and this notice must be preserved on all
copies. 
*/

#ifndef	_OSEKI_LOCK_H
#define	_OSEKI_LOCK_H

#define	InitLock(obj)	\
	{					\
		pthread_mutex_init(&(obj)->mutex,NULL);	\
		pthread_cond_init(&(obj)->cond,NULL);	\
	}
#define	Lock(obj)		pthread_mutex_lock(&(obj)->mutex)
#define	UnLock(obj)		\
	{					\
		pthread_mutex_unlock(&(obj)->mutex);	\
		pthread_cond_signal(&(obj)->cond);		\
	}
#define	DestroyLock(obj)	\
	{					\
		pthread_mutex_destroy(&(obj)->mutex);	\
		pthread_cond_destroy(&(obj)->cond);		\
	}

#define	WaitLock(obj)	pthread_cond_wait(&(obj)->cond,&(obj)->mutex)

#endif
