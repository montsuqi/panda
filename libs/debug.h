/*	PANDA -- a simple transaction monitor

Copyright (C) 1989-2003 Ogochan.

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
*	debug macros
*/

#ifndef	_INC_DEBUG_H
#define	_INC_DEBUG_H
#include	<stdio.h>

#include	"message.h"

#ifdef	TRACE
#define	dbgmsg(s)			MessageDebug(__FILE__,__LINE__,(s))
#define	PASS(s)				MessageDebug(__FILE__,__LINE__,(s))
#else
#define	dbgmsg(s)			/*	*/
#define	PASS(s)				/*	*/
#endif

#define	Error(s)			_Message(MESSAGE_ERROR,__FILE__,__LINE__,(s))
#define	Warning(s)			_Message(MESSAGE_WARN,__FILE__,__LINE__,(s))
#define	Message(l,s)		_Message((l),__FILE__,__LINE__,(s))

#endif
